#include "settingsWindow.h"
#include "notifications/inAppNotification.h"
#include "autoStartup.h"
#include "../../core/i18n/lang.h"
#include "../../core/config/logger.h"
#include "../../core/config/appSettings.h"
#include "../helpers/acrylicHelper.h"
#include "../helpers/iconHelper.h"
#include "../helpers/keySequenceHelper.h"
#include "../helpers/vkMapper.h"
#include <QGuiApplication>
#include <QScreen>
#include <QAction>
#include <QPushButton>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <algorithm>

SettingsWindow::SettingsWindow(QWidget *parent) : QWidget(parent) {
    ui.setupUi(this);

    // Визуальное оформление
    initVisuals();

    // Логика обновлений
    initUpdateLogic();

    // Язык и автозагрузка
    initLanguageAndStartup();

    // Хоткеи
    buildPresetMap();
    initHotkeyLogic();

    // Автосохранение
    initAutosaveLogic();

    // Системные настройки окна
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::ClickFocus);

    refreshTranslations();
    LOG_DEBUG() << "SettingsWindow initialized";
}

void SettingsWindow::initVisuals() {
    // Временно скрываем элементы
    ui.btn_exclusions_top_sider->hide();
    ui.btn_indicator_top_sider->hide();
    ui.btn_info_top_sider->hide();
    ui.app_theme_frame->hide();
    ui.app_theme_label->hide();
    // ---------------

    // Селекторы
    addSelectorForFrame(ui.key_select_frame);
    addSelectorForFrame(ui.app_startup_frame);
    // addSelectorForFrame(ui.app_theme_frame);
    addSelectorForFrame(ui.app_lang_frame);

    // Тень
    m_shadow = new QGraphicsDropShadowEffect(ui.content_container);
    m_shadow->setBlurRadius(12);
    m_shadow->setOffset(-2, 0);
    m_shadow->setColor(QColor(0, 0, 0, 120));
    ui.content_container->setGraphicsEffect(m_shadow);

    // Иконки
    ui.btn_general_top_sider->setIcon(
        IconHelper::loadIcon(":/icons/icons/SettingsFilled.svg", QColor(225, 225, 225), QSize(42, 42)));
    ui.key_select_label_img->setIcon(
        IconHelper::loadIcon(":/icons/icons/InfoRegular.svg", QColor(175, 175, 175), QSize(16, 16)));
    ui.btn_upd_manually->setIcon(
        IconHelper::loadIcon(":/icons/icons/SyncFilled.svg", QColor(175, 175, 175), QSize(18, 18)));

    // Драггер
    dragger = new WindowDragger(this);
    dragger->addIgnoredWidget(ui.btn_close_bot_sider);

    // Предупреждение при наведении
    keyHoverWarning = new KeyHoverWarning(this);
    ui.key_select_label_img->setAttribute(Qt::WA_Hover);
    ui.key_select_label_img->installEventFilter(this);

    // Слайдер задержки
    ui.key_delay_slider->setValue(AppSettings::switchDelayMs);
    ui.key_delay_spinbox->setValue(AppSettings::switchDelayMs);
    ui.key_delay_slider->setSingleStep(10);
    ui.key_delay_slider->setPageStep(50);
    ui.key_delay_slider->setTickInterval(10);
    ui.key_delay_slider->setTracking(true);
    ui.key_delay_spinbox->setSingleStep(10);
}

void SettingsWindow::initHotkeyLogic() {
    const auto KEY_PLACEHOLDER = Lang::tr("SETTINGS_KEY_SEQUENCE");
    m_keyHelper = new KeySequenceHelper(
        this, "btn_sequence",
        IconHelper::loadIcon(":/icons/icons/ClearFilled.svg", QColor(175, 175, 175), QSize(12, 12)),
        KEY_PLACEHOLDER, this);

    // Коннект для кастомного ввода
    connect(m_keyHelper, &KeySequenceHelper::hotkeySelected, this,
            [this](const int mainVk, int /*mods*/, const QString &name) {
                if (mainVk == 0) {
                    LOG_DEBUG() << "Sequence cleared by user";
                    restorePreviousPresetIfNeeded();
                    return;
                }

                if (const auto it = std::find(presetMap.constBegin(), presetMap.constEnd(), AppSettings::hotkeyMainVk);
                    it != presetMap.constEnd()) {
                    AppSettings::previousHotkeyMainVk = AppSettings::hotkeyMainVk;
                    AppSettings::previousHotkeyName = AppSettings::hotkeyName;
                    LOG_DEBUG() << "Saved previous preset: vk="
                                << AppSettings::previousHotkeyMainVk << "; name='"
                                << AppSettings::previousHotkeyName << "'";
                } else {
                    LOG_DEBUG() << "Current hotkey is custom - not saving previous preset";
                }

                LOG_DEBUG() << QString("Custom hotkey selected: vk=%1; name='%2'").arg(mainVk).arg(name);
                applyHotkeyIfChanged(mainVk, name);
            });

    // Обработка кнопок-пресетов
    const QList<QPushButton *> presetButtons = ui.key_select_frame->findChildren<QPushButton *>();
    bool matchedPreset = false;

    for (auto *btn: presetButtons) {
        if (const int vk = presetMap.value(btn->objectName(), 0); vk != 0 && vk == AppSettings::hotkeyMainVk) {
            btn->setChecked(true);
            matchedPreset = true;
            LOG_DEBUG() << QString("Matched preset button '%1' for hotkey '%2'")
                            .arg(btn->objectName(), AppSettings::hotkeyName);
        } else {
            btn->setChecked(false);
        }

        connect(btn, &QPushButton::clicked, this, [this, btn]() {
            if (!btn) return;
            const int vk_ = presetMap.value(btn->objectName(), 0);
            if (vk_ == 0) return;
            const QString name = nameFromVk(vk_);

            LOG_DEBUG() << QString("Preset '%1' selected: vk=%2; name='%3'").arg(btn->objectName()).arg(vk_).arg(name);
            applyHotkeyIfChanged(vk_, name);

            AppSettings::previousHotkeyMainVk = vk_;
            AppSettings::previousHotkeyName = name;

            if (auto *seq = findChild<QKeySequenceEdit *>("btn_sequence")) {
                seq->setKeySequence(QKeySequence());
                seq->setStyleSheet("color: rgba(255, 255, 255, 225);");
                seq->clearFocus();
                this->setFocus(Qt::OtherFocusReason);
                if (auto *le = seq->findChild<QLineEdit *>())
                    le->setPlaceholderText(Lang::tr("SETTINGS_KEY_SEQUENCE"));
            }
        });
    }

    if (!matchedPreset) {
        if (auto *seq = findChild<QKeySequenceEdit *>("btn_sequence")) {
            if (const QString name = AppSettings::hotkeyName; !name.isEmpty()) {
                seq->setKeySequence(QKeySequence(name));
                LOG_DEBUG() << "Populated sequence edit with saved custom key: '" << name << "'";
            }
        }
    }
}

void SettingsWindow::initUpdateLogic() {
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
        if (ui.btn_en_lang->isChecked()) AppSettings::appLang = "en";
        else if (ui.btn_ru_lang->isChecked()) AppSettings::appLang = "ru";

        AppSettings::save();
        hasPendingChanges = false;
        LOG_DEBUG() << "Autosave successfully";
        emit settingsSaved();
    });

    connect(this, &SettingsWindow::settingsSaved, this, [this]() {
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

void SettingsWindow::buildPresetMap() {
    presetMap.clear();
    for (const auto presetButtons = ui.key_select_frame->findChildren<QPushButton *>();
         const auto *btn: presetButtons) {
        const QString obj = btn->objectName().toLower();
        int vk = 0;
        if (obj.contains("lctrl")) vk = VK_LCONTROL;
        else if (obj.contains("rctrl")) vk = VK_RCONTROL;
        else if (obj.contains("lalt")) vk = VK_LMENU;
        else if (obj.contains("ralt")) vk = VK_RMENU;
        else if (obj.contains("lshift")) vk = VK_LSHIFT;
        else if (obj.contains("rshift")) vk = VK_RSHIFT;
        else if (obj.contains("caps")) vk = VK_CAPITAL;

        if (vk != 0) presetMap.insert(btn->objectName(), vk);
    }
    LOG_DEBUG() << "Preset map built: size=" << presetMap.size();
}

void SettingsWindow::applyHotkeyIfChanged(const int newVk, const QString &newName) {
    if (newVk == AppSettings::hotkeyMainVk && newName == AppSettings::hotkeyName) {
        LOG_DEBUG() << "Hotkey not changed";
        return;
    }
    AppSettings::hotkeyMainVk = newVk;
    AppSettings::hotkeyModifiers = 0;
    AppSettings::hotkeyName = newName;

    for (const auto presetButtons = ui.key_select_frame->findChildren<QPushButton *>(); auto *btn: presetButtons) {
        const int vk = vkFromPresetObjectName(btn->objectName());
        btn->setChecked(vk != 0 && vk == newVk);
    }
    markChanged();
}

void SettingsWindow::markChanged() {
    if (!hasPendingChanges) {
        hasPendingChanges = true;
        autosaveTimer.start();
        LOG_DEBUG() << "Marked changed, autosave scheduled in " << autosaveIntervalMs / 1000 << " sec";
        emit settingsChanged();
    } else {
        autosaveTimer.start();
        LOG_DEBUG() << "Change already pending - timer restarted";
    }
}

void SettingsWindow::setUpdateManager(UpdateManager *manager) {
    if (!manager) return;
    this->updateManager = manager;

    // 1. Если обновление найдено
    connect(updateManager, &UpdateManager::updateAvailable, this, [this](const QString &ver, const QString &url) {
        if (m_currentGlobalNotif) m_currentGlobalNotif->deleteLater();

        m_currentGlobalNotif = new GlobalNotification(GlobalNotification::UpdateAvailable, ver, url);
        // Тут добавим логику позиционирования позже
        m_currentGlobalNotif->show();
    });

    // 2. Если обновлений нет (ручная проверка)
    connect(updateManager, &UpdateManager::noUpdateAvailable, this, [this](const QString &ver) {
        // Показываем уведомление "У вас последняя версия" только если окно настроек активно
        // (чтобы не спамить при фоновой проверке)
        if (this->isActiveWindow()) {
            if (m_currentGlobalNotif) m_currentGlobalNotif->deleteLater();
            m_currentGlobalNotif = new GlobalNotification(GlobalNotification::UpToDate, ver, "");
            m_currentGlobalNotif->show();
        }
    });

    // Коннекты для кнопок
    connect(ui.btn_upd_manually, &QPushButton::clicked, updateManager, &UpdateManager::checkForUpdatesForce);
}

SettingsWindow::~SettingsWindow() = default;

void SettingsWindow::addSelectorForFrame(QFrame *frame, const QString &extraStyle) {
    if (!frame) return;
    const auto sel = new AnimatedSelector(this);
    sel->bindToFrame(frame, extraStyle);
    selectors.append(sel);
    QTimer::singleShot(0, sel, &AnimatedSelector::initPosition);
}

void SettingsWindow::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        AcrylicHelper::setAcrylicEnabled(this, true);
        AcrylicHelper::updateRegion(this);
    });
    QTimer::singleShot(0, this, [this]() {
        for (AnimatedSelector *sel: selectors) if (sel) sel->initPosition();
    });
}

bool SettingsWindow::event(QEvent *ev) {
    if (ev->type() == QEvent::WindowActivate || ev->type() == QEvent::WindowDeactivate) {
        bool active = (ev->type() == QEvent::WindowActivate);
        QTimer::singleShot(0, this, [this, active]() {
            AcrylicHelper::setAcrylicEnabled(this, active);
            AcrylicHelper::updateRegion(this);
        });
        LOG_DEBUG() << "Settings window is " << (active ? "active" : "inactive");
    }
    return QWidget::event(ev);
}

bool SettingsWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui.key_select_label_img && keyHoverWarning) {
        if (event->type() == QEvent::Enter) keyHoverWarning->showNear(ui.key_select_label_img);
        else if (event->type() == QEvent::Leave) keyHoverWarning->hideNow();
    } else if (watched == ui.btn_upd_manually && updateBtnToolTip && InAppNotification::stack.isEmpty()) {
        if (event->type() == QEvent::Enter) updateBtnToolTip->showAt(ui.btn_upd_manually, "SETTINGS_TOOLTIP_CHECK_NOW");
        else if (event->type() == QEvent::Leave) updateBtnToolTip->hideAnimated();
    }
    return QWidget::eventFilter(watched, event);
}

bool SettingsWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result) {
    if (eventType == "windows_generic_MSG") {
        if (AcrylicHelper::handleIconicMessages(this, message, QColor(32, 32, 32))) {
            *result = 0;
            return true;
        }
    }
    return QWidget::nativeEvent(eventType, message, result);
}

void SettingsWindow::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    if (updateBtnToolTip || !InAppNotification::stack.isEmpty()) updateBtnToolTip->hide();
    if (keyHoverWarning) keyHoverWarning->hideNow();
    if (updPopup) updPopup->hide();
}

void SettingsWindow::openCentered() {
    const QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    const QRect geom = screen->availableGeometry();

    // Принудительно заставляем Qt пересчитать размеры контента
    if (layout()) layout()->activate();
    adjustSize();

    const QSize s = size();

    // Считаем центр
    const int x = geom.left() + (geom.width() - s.width()) / 2;
    const int y = geom.top() + (geom.height() - s.height()) / 2;

    // Сначала двигаем, потом показываем
    move(x, y);

    if (isHidden()) {
        show();
    } else {
        if (isMinimized()) showNormal();
        raise();
        activateWindow();
    }

    // Фокус и акрил
    QTimer::singleShot(50, this, [this]() {
        const bool trulyActive = this->isActiveWindow() || (QApplication::activeWindow() == this);
        AcrylicHelper::setAcrylicEnabled(this, trulyActive);
        AcrylicHelper::updateRegion(this);
    });
}

int SettingsWindow::vkFromPresetObjectName(const QString &obj) const { return presetMap.value(obj, 0); }
QString SettingsWindow::nameFromVk(const int vk) { return VkMapper::vkToName(vk); }

void SettingsWindow::restorePreviousPresetIfNeeded() {
    if (AppSettings::previousHotkeyMainVk != 0) {
        if (AppSettings::hotkeyMainVk != AppSettings::previousHotkeyMainVk) {
            AppSettings::hotkeyMainVk = AppSettings::previousHotkeyMainVk;
            AppSettings::hotkeyModifiers = 0;
            AppSettings::hotkeyName = AppSettings::previousHotkeyName;
            for (auto *btn: ui.key_select_frame->findChildren<QPushButton *>()) {
                const int vk = vkFromPresetObjectName(btn->objectName());
                btn->setChecked(vk != 0 && vk == AppSettings::previousHotkeyMainVk);
            }
            LOG_DEBUG() << QString("Previous preset restored: vk=%1; name='%2'")
                            .arg(AppSettings::previousHotkeyMainVk).arg(AppSettings::previousHotkeyName);
            AppSettings::previousHotkeyMainVk = 0;
            AppSettings::previousHotkeyName.clear();
            markChanged();
        } else {
            AppSettings::previousHotkeyMainVk = 0;
            AppSettings::previousHotkeyName.clear();
        }
    }
}

void SettingsWindow::restoreDefaultsForCurrentPage() {
    if (ui.content_container->currentIndex() == 0) restoreDefaults_General();
    markChanged();
}

void SettingsWindow::restoreDefaults_General() {
    AppSettings::reset();
    for (auto *btn: ui.key_select_frame->findChildren<QPushButton *>()) {
        const int vk = vkFromPresetObjectName(btn->objectName());
        btn->blockSignals(true);
        btn->setChecked(vk != 0 && vk == AppSettings::hotkeyMainVk);
        btn->blockSignals(false);
    }

    if (auto *seq = findChild<QKeySequenceEdit *>("btn_sequence")) {
        const auto it = std::find(presetMap.constBegin(), presetMap.constEnd(), AppSettings::defaultHotkeyMainVk);

        if (const bool isDefaultInPresets = (it != presetMap.constEnd());
            AppSettings::hotkeyMainVk == AppSettings::defaultHotkeyMainVk && isDefaultInPresets) {
            seq->setKeySequence(QKeySequence());
            if (auto *le = seq->findChild<QLineEdit *>()) le->setPlaceholderText(Lang::tr("SETTINGS_KEY_SEQUENCE"));
        } else {
            seq->setKeySequence(QKeySequence(AppSettings::hotkeyName));
        }
        seq->clearFocus();
    }

    ui.key_delay_slider->setValue(AppSettings::defaultSwitchDelayMs);
    ui.key_delay_spinbox->setValue(AppSettings::defaultSwitchDelayMs);

    for (auto *sel: selectors) {
        if (sel && sel->boundFrame() == ui.key_select_frame) {
            sel->stopAndResetAnimation();
            QTimer::singleShot(0, sel, &AnimatedSelector::animateToCurrentState);
            break;
        }
    }
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
    if (m_currentGlobalNotif) m_currentGlobalNotif->refreshTranslations();
    keyHoverWarning->setText(Lang::tr("SETTINGS_KEY_HOVER_WARNING_POPUP"));
    if (const auto *seq = findChild<QKeySequenceEdit *>("btn_sequence"))
        if (auto *le = seq->findChild<QLineEdit *>()) le->setPlaceholderText(Lang::tr("SETTINGS_KEY_SEQUENCE"));
    if (m_keyHelper) m_keyHelper->setPlaceholder(Lang::tr("SETTINGS_KEY_SEQUENCE"));
}
