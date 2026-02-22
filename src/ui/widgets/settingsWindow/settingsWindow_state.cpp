#include "settingsWindow.h"
#include "../notifications/inAppNotification.h"
#include "../autoStartup.h"
#include "../../../core/i18n/lang.h"
#include "../../../core/config/logger.h"
#include "../../../core/config/appSettings.h"
#include "../../helpers/keySequenceHelper.h"
#include <QAction>
#include <QPushButton>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QSlider>
#include <QSpinBox>
#include <QToolButton>

void SettingsWindow::initLanguageAndStartup() {
    // Восстанавливаем состояние кнопок из конфига
    ui.btn_en_lang->setChecked(AppSettings::appLang == "en");
    ui.btn_ru_lang->setChecked(AppSettings::appLang == "ru");

    // Обработка кликов (язык)
    connect(ui.btn_en_lang, &QPushButton::clicked, this, [this]() {
        if (!ui.btn_en_lang->isChecked()) return;
        ui.btn_ru_lang->setChecked(false);
        markChanged();
    });

    connect(ui.btn_ru_lang, &QPushButton::clicked, this, [this]() {
        if (!ui.btn_ru_lang->isChecked()) return;
        ui.btn_en_lang->setChecked(false);
        markChanged();
    });

    // Автозагрузка (состояние и коннекты)
    ui.btn_enable_startup->setChecked(AutoStartupManager::isAutoStartupEnabled());
    ui.btn_disable_startup->setChecked(!AutoStartupManager::isAutoStartupEnabled());

    connect(ui.btn_enable_startup, &QPushButton::clicked, [this]() {
        AutoStartupManager::setAutoStartup(true);
        AppSettings::autoStartup = true;
        markChanged();
    });

    connect(ui.btn_disable_startup, &QPushButton::clicked, [this]() {
        AutoStartupManager::setAutoStartup(false);
        AppSettings::autoStartup = false;
        markChanged();
    });

    // Слайдеры задержки (коннекты)
    connect(ui.key_delay_slider, &QSlider::valueChanged, this, [this](const int value) {
        const int stepped = (value / 10) * 10;
        if (value != stepped) ui.key_delay_slider->setValue(stepped);
        if (AppSettings::switchDelayMs != stepped) {
            AppSettings::switchDelayMs = stepped;
            ui.key_delay_spinbox->setValue(stepped);
            markChanged();
        }
    });

    connect(ui.key_delay_spinbox, qOverload<int>(&QSpinBox::valueChanged), this, [this](const int value) {
        const int stepped = ((value + 5) / 10) * 10;
        if (value != stepped) ui.key_delay_spinbox->setValue(stepped);
        if (AppSettings::switchDelayMs != stepped) {
            AppSettings::switchDelayMs = stepped;
            ui.key_delay_slider->setValue(stepped);
            markChanged();
        }
    });
}

void SettingsWindow::initAutosaveLogic() {
    autosaveTimer.setSingleShot(true);
    autosaveTimer.setInterval(autosaveIntervalMs);
    LOG_DEBUG() << "Autosave timer configured, interval=" << autosaveIntervalMs;

    connect(&autosaveTimer, &QTimer::timeout, this, [this]() {
        // Обновляем временные переменные из UI, которые не обновляются мгновенно
        const QString pendingLang = ui.btn_en_lang->isChecked() ? "en" : "ru";

        // Временно присваиваем, чтобы проверить dirty-статус
        const QString oldLang = AppSettings::appLang;
        AppSettings::appLang = pendingLang;

        // Проверяем, есть ли реальные отличия от сохраненного файла
        if (!AppSettings::isDirty()) {
            AppSettings::appLang = oldLang;
            hasPendingChanges = false;
            LOG_DEBUG() << "Autosave skipped: no actual changes compared to disk";
            return;
        }

        // Если изменения есть — сохраняем
        AppSettings::save();
        hasPendingChanges = false;
        LOG_DEBUG() << "Autosave successfully performed";
        emit settingsSaved();
        emit settingsChanged();
    });

    connect(this, &SettingsWindow::settingsSaved, this, [this]() {
        if (updateBtnToolTip) {
            updateBtnToolTip->hideNow();
        }
        InAppNotification::showFor(this, Lang::tr("SETTINGS_ALL_CHANGES_SAVED"));
        QTimer::singleShot(50, this, [this]() { refreshTranslations(); });
    });

    // Кнопка сброса и Esc
    auto *closeAction = new QAction(this);
    closeAction->setShortcut(Qt::Key_Escape);
    connect(closeAction, &QAction::triggered, this, &SettingsWindow::close);
    addAction(closeAction);
    connect(ui.btn_close_bot_sider, &QPushButton::clicked, closeAction, &QAction::trigger);
    connect(ui.btn_restore_default, &QToolButton::clicked, this, &SettingsWindow::restoreDefaultsForCurrentPage);
}

void SettingsWindow::markChanged() {
    // Если пользователь вернул настройки в исходное состояние (до сохранения)
    // нам нужно проверить, "грязные" ли данные сейчас.
    // Но для языка мы берем значение прямо из UI, так как AppSettings::appLang
    // обновляется только в момент срабатывания таймера

    // Временный замер для точной проверки
    const QString currentUILang = ui.btn_en_lang->isChecked() ? "en" : "ru";
    const QString backupLang = AppSettings::appLang;
    AppSettings::appLang = currentUILang;

    if (!AppSettings::isDirty()) {
        if (hasPendingChanges) {
            autosaveTimer.stop();
            hasPendingChanges = false;
            LOG_DEBUG() << "Changes reverted to original, timer stopped";
        }
        AppSettings::appLang = backupLang;
        return;
    }

    AppSettings::appLang = backupLang;

    // Стандартная логика запуска таймера
    hasPendingChanges = true;
    autosaveTimer.start();
    LOG_DEBUG() << "Marked changed, autosave scheduled";
}

void SettingsWindow::refreshTranslations() const {
    ui.key_select_label_msg->setText(Lang::tr("SETTINGS_SELECT_KEY_LABEL"));
    ui.key_delay_label->setText(Lang::tr("SETTINGS_SWITCH_DELAY_LABEL"));
    ui.app_startup_label->setText(Lang::tr("SETTINGS_APP_STARTUP_LABEL"));
    ui.app_lang_label->setText(Lang::tr("SETTINGS_APP_LANG_LABEL"));
    ui.btn_restore_default->setText(Lang::tr("SETTINGS_RESTORE_DEFAULT_LABEL"));
    ui.btn_general_top_sider->setText(Lang::tr("SETTINGS_SIDER_MENU_GENERAL"));
    ui.btn_exclusions_top_sider->setText(Lang::tr("SETTINGS_SIDER_MENU_EXCLUSIONS"));
    ui.btn_info_top_sider->setText(Lang::tr("SETTINGS_SIDER_MENU_INFO"));
    ui.btn_close_bot_sider->setText(Lang::tr("SETTINGS_SIDER_MENU_CLOSE"));
    ui.btn_enable_startup->setText(Lang::tr("SETTINGS_ENABLE_STARTUP_LABEL"));
    ui.btn_disable_startup->setText(Lang::tr("SETTINGS_DISABLE_STARTUP_LABEL"));
    ui.app_upd_check_label->setText(Lang::tr("SETTINGS_APP_UPD_CHECK_LABEL"));
    ui.btn_upd_frequency->setText(AppSettings::updateFrequencyToString(AppSettings::updateFrequency));
    if (updPopup) updPopup->refreshTranslations();
    if (updateBtnToolTip) updateBtnToolTip->refreshTranslations();
    keyHoverWarning->setText(Lang::tr("SETTINGS_KEY_HOVER_WARNING_POPUP"));
    if (const auto *seq = findChild<QKeySequenceEdit *>("btn_sequence"))
        if (auto *le = seq->findChild<QLineEdit *>()) le->setPlaceholderText(Lang::tr("SETTINGS_KEY_SEQUENCE"));
    if (m_keyHelper) m_keyHelper->setPlaceholder(Lang::tr("SETTINGS_KEY_SEQUENCE"));
}
