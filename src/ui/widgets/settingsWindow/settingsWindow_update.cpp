#include "../../../core/config/appSettings.h"
#include "../../../core/config/logger.h"
#include "../notifications/inAppNotification.h"
#include "settingsWindow.h"

#include <QLocale>
#include <QPushButton>

void SettingsWindow::initUpdateLogic()
{
    updPopup = new UpdFrequencyPopup(this);
    updateBtnToolTip = new CustomToolTip(this);
    ui.btn_upd_manually->installEventFilter(this);

    ui.btn_upd_frequency->setText(AppSettings::updateFrequencyToString(AppSettings::updateFrequency));

    connect(ui.btn_upd_frequency, &QPushButton::clicked, this, [this] {
        updPopup->setCurrent(AppSettings::updateFrequency);
        constexpr int padding = 4;
        updPopup->setFixedWidth(ui.btn_upd_frequency->width() + (padding * 2));
        const QPoint btnGlobal = ui.btn_upd_frequency->mapToGlobal(QPoint(-padding, -updPopup->currentButtonYOffset()));
        updPopup->move(btnGlobal);
        updPopup->show();
    });

    connect(updPopup, &UpdFrequencyPopup::selected, this, [this](const AppSettings::UpdateFrequency value) {
        AppSettings::updateFrequency = value;
        markChanged();
        ui.btn_upd_frequency->setText(AppSettings::updateFrequencyToString(value));
    });

    connect(this, &SettingsWindow::settingsSaved, updPopup, &UpdFrequencyPopup::refreshTranslations);
}

void SettingsWindow::setUpdateManager(UpdateManager* manager)
{
    if (manager == nullptr) {
        return;
    }
    if (updateManager == manager) {
        return;
    }

    if (updateManager) {
        disconnect(updateManager, nullptr, this, nullptr);
    }
    disconnect(ui.btn_upd_manually, &QPushButton::clicked, this, nullptr);

    this->updateManager = manager;

    const auto finishChecking = [this]() { stopSyncAnimation(); };

    connect(updateManager,
            &UpdateManager::updateAvailable,
            this,
            [this, finishChecking](const QString& ver, const QString& url, const bool isManual) {
                finishChecking();
                refreshUpdateButtonTooltipLive();
                Q_UNUSED(ver);
                Q_UNUSED(url);
                Q_UNUSED(isManual);
            });

    connect(updateManager,
            &UpdateManager::noUpdateAvailable,
            this,
            [this, finishChecking](const QString& ver, const bool isManual) {
                finishChecking();
                refreshUpdateButtonTooltipLive();
                Q_UNUSED(ver);
                Q_UNUSED(isManual);
            });

    connect(updateManager,
            &UpdateManager::updateError,
            this,
            [this, finishChecking](const QString& errorMsg, const bool isManual) {
                finishChecking();

                if (isManual) {
                    InAppNotification::showFor(this, errorMsg, InAppNotification::Type::Error);
                }
                LOG_ERROR() << "Update error: " << errorMsg;
            });

    connect(ui.btn_upd_manually, &QPushButton::clicked, this, [this]() {
        if (!isManualCheckActive()) {
            m_syncRotationAnim->setLoopCount(-1);
            m_syncRotationAnim->start();

            updateManager->checkForUpdatesForce();
        }
    });
}

QString SettingsWindow::lastUpdateCheckDisplay()
{
    const QLocale locale = (AppSettings::appLang == "ru") ? QLocale(QLocale::Russian, QLocale::Russia)
                                                          : QLocale(QLocale::English, QLocale::UnitedStates);
    if (AppSettings::lastUpdateCheckDateTime().isValid()) {
        return locale.toString(AppSettings::lastUpdateCheckDateTime(), QLocale::ShortFormat);
    }
    if (AppSettings::lastUpdateCheckDate().isValid()) {
        return locale.toString(AppSettings::lastUpdateCheckDate(), QLocale::ShortFormat);
    }
    return {};
}

void SettingsWindow::refreshUpdateButtonTooltipLive() const
{
    if (updateBtnToolTip == nullptr) {
        return;
    }
    if (!ui.btn_upd_manually->underMouse()) {
        return;
    }
    if (!InAppNotification::stack.isEmpty()) {
        return;
    }

    if (m_hasPendingUpdate && !m_pendingUpdateVersion.isEmpty()) {
        if (const QString lastCheck = lastUpdateCheckDisplay(); !lastCheck.isEmpty()) {
            updateBtnToolTip->showAt(ui.btn_upd_manually,
                                     "SETTINGS_TOOLTIP_UPDATE_AVAILABLE_WITH_LAST_CHECK",
                                     m_pendingUpdateVersion,
                                     lastCheck);
        }
        else {
            updateBtnToolTip->showAt(ui.btn_upd_manually, "SETTINGS_TOOLTIP_UPDATE_AVAILABLE", m_pendingUpdateVersion);
        }
    }
    else if (const QString lastCheck = lastUpdateCheckDisplay(); !lastCheck.isEmpty()) {
        updateBtnToolTip->showAt(ui.btn_upd_manually, "SETTINGS_TOOLTIP_LAST_CHECK", lastCheck);
    }
    else {
        updateBtnToolTip->showAt(ui.btn_upd_manually, "SETTINGS_TOOLTIP_CHECK_NOW");
    }
}

void SettingsWindow::setPendingUpdateHint(const bool hasPending, const QString& version)
{
    m_hasPendingUpdate = hasPending;
    m_pendingUpdateVersion = hasPending ? version : QString{};
    updateManualCheckButtonIcon();
    refreshUpdateButtonTooltipLive();
}
