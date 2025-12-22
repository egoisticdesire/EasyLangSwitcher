#include "settingsWindow.h"
#include "saveNotification.h"
#include "autoStartup.h"
#include "../../core/i18n/lang.h"
#include "../../core/config/logger.h"
#include "../../core/config/appSettings.h"
#include "../helpers/acrylicHelper.h"
#include "../helpers/iconHelper.h"
#include "../helpers/keySequenceHelper.h"
#include "../helpers/vkMapper.h"
#include <QGuiApplication>
#include <QTimer>
#include <QScreen>
#include <QAction>
#include <QPushButton>
#include <QKeySequenceEdit>

SettingsWindow::SettingsWindow(QWidget *parent)
    : QWidget(parent) {
    ui.setupUi(this);

    initUpdateFrequency();

    // временно скрываем кнопки
    ui.btn_exclusions_top_sider->hide();
    ui.btn_indicator_top_sider->hide();
    ui.btn_info_top_sider->hide();

    ui.app_theme_frame->hide();
    ui.app_theme_label->hide();
    //-----------------------------

    addSelectorForFrame(ui.key_select_frame);
    addSelectorForFrame(ui.app_startup_frame);
    // addSelectorForFrame(ui.app_theme_frame);
    addSelectorForFrame(ui.app_lang_frame);

    // восстановить состояние
    ui.btn_en_lang->setChecked(AppSettings::appLang == "en");
    ui.btn_ru_lang->setChecked(AppSettings::appLang == "ru");

    keyHoverWarning = new KeyHoverWarning(this);
    ui.key_select_label_img->setAttribute(Qt::WA_Hover);
    ui.key_select_label_img->installEventFilter(this);

    refreshTranslations();

    connect(ui.btn_en_lang, &QPushButton::clicked, this, [this]() {
        if (!ui.btn_en_lang->isChecked()) return;
        // AppSettings::appLang = "en";
        ui.btn_ru_lang->setChecked(false);
        markChanged();
    });

    connect(ui.btn_ru_lang, &QPushButton::clicked, this, [this]() {
        if (!ui.btn_ru_lang->isChecked()) return;
        // AppSettings::appLang = "ru";
        ui.btn_en_lang->setChecked(false);
        markChanged();
    });

    m_shadow = new QGraphicsDropShadowEffect(ui.content_container);
    m_shadow->setBlurRadius(12);
    m_shadow->setOffset(-2, 0);
    m_shadow->setColor(QColor(0, 0, 0, 140));
    ui.content_container->setGraphicsEffect(m_shadow);

    // временно красим иконку
    ui.btn_general_top_sider->setIcon(
        IconHelper::loadIcon(":/icons/icons/SettingsFilled.svg", QColor(225, 225, 225), QSize(42, 42)));

    ui.key_select_label_img->setIcon(
        IconHelper::loadIcon(":/icons/icons/InfoRegular.svg", QColor(175, 175, 175), QSize(16, 16)));

    ui.btn_upd_manually->setIcon(
        IconHelper::loadIcon(":/icons/icons/SyncFilled.svg", QColor(175, 175, 175), QSize(18, 18)));

    buildPresetMap();

    const auto KEY_PLACEHOLDER = Lang::tr("SETTINGS_KEY_SEQUENCE");
    m_keyHelper = new KeySequenceHelper(
        this,
        "btn_sequence",
        IconHelper::loadIcon(":/icons/icons/ClearFilled.svg", QColor(175, 175, 175), QSize(12, 12)),
        KEY_PLACEHOLDER,
        this
    );

    autosaveTimer.setSingleShot(true);
    autosaveTimer.setInterval(autosaveIntervalMs);

    LOG_DEBUG() << "Autosave timer configured, interval=" << autosaveIntervalMs;

    connect(&autosaveTimer, &QTimer::timeout, this, [this]() {
        if (ui.btn_en_lang->isChecked()) {
            AppSettings::appLang = "en";
        } else if (ui.btn_ru_lang->isChecked()) {
            AppSettings::appLang = "ru";
        }

        AppSettings::save();
        hasPendingChanges = false;
        LOG_DEBUG() << "Autosave successfully";
        emit settingsSaved();
    });

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::ClickFocus);

    auto *closeAction = new QAction(this);
    closeAction->setShortcut(Qt::Key_Escape);
    connect(closeAction, &QAction::triggered, this, &SettingsWindow::close);
    addAction(closeAction);

    connect(ui.btn_close_bot_sider, &QPushButton::clicked, closeAction, &QAction::trigger);

    connect(ui.btn_restore_default, &QToolButton::clicked,
            this, &SettingsWindow::restoreDefaultsForCurrentPage);

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

    dragger = new WindowDragger(this);
    dragger->addIgnoredWidget(ui.btn_close_bot_sider);

    const QList<QPushButton *> presetButtons = ui.key_select_frame->findChildren<QPushButton *>();

    bool matchedPreset = false;
    for (auto *btn: presetButtons) {
        const QString obj = btn->objectName();
        if (const int vk = presetMap.value(obj, 0); vk != 0 && vk == AppSettings::hotkeyMainVk) {
            btn->setChecked(true);
            matchedPreset = true;

            LOG_DEBUG() << QString("Matched preset button '%1' for hotkey '%2'").arg(obj, AppSettings::hotkeyName);
        } else {
            btn->setChecked(false);
        }
    }

    if (!matchedPreset) {
        if (auto *seq = findChild<QKeySequenceEdit *>("btn_sequence")) {
            if (const QString name = AppSettings::hotkeyName; !name.isEmpty()) {
                seq->setKeySequence(QKeySequence(name));
                LOG_DEBUG() << "Populated sequence edit with saved custom key: '" << name << "'";
            }
        }
    }

    if (m_keyHelper) {
        connect(m_keyHelper, &KeySequenceHelper::hotkeySelected, this,
                [this](const int mainVk, int /*mods*/, const QString &name) {
                    if (mainVk == 0) {
                        LOG_DEBUG() << "Sequence cleared by user";

                        restorePreviousPresetIfNeeded();
                        return;
                    }

                    if (presetMap.values().contains(AppSettings::hotkeyMainVk)) {
                        AppSettings::previousHotkeyMainVk = AppSettings::hotkeyMainVk;
                        AppSettings::previousHotkeyName = AppSettings::hotkeyName;

                        LOG_DEBUG() << "Saved previous preset: vk=" << AppSettings::previousHotkeyMainVk
                << "; name='" << AppSettings::previousHotkeyName << "'";
                    } else {
                        LOG_DEBUG() << "Current hotkey is custom - not saving previous preset";
                    }

                    LOG_DEBUG() << QString("Custom hotkey selected: vk=%1; name='%2'").arg(mainVk).arg(name);
                    applyHotkeyIfChanged(mainVk, name);
                });
    }

    for (auto *btn: presetButtons) {
        connect(btn, &QPushButton::clicked, this, [this, btn]() {
            if (!btn) return;
            const QString obj = btn->objectName();
            const int vk = presetMap.value(obj, 0);
            if (vk == 0) return;
            const QString name = nameFromVk(vk);

            LOG_DEBUG() << QString("Preset '%1' selected: vk=%2; name='%3'").arg(obj).arg(vk).arg(name);

            // Обновляем hotkey
            applyHotkeyIfChanged(vk, name);

            // Теперь, когда инпут очищается, сохраняем в AppSettings
            AppSettings::previousHotkeyMainVk = vk;
            AppSettings::previousHotkeyName = name;

            // Очищаем QKeySequenceEdit, чтобы плейсхолдер показывался
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

    ui.key_delay_slider->setValue(AppSettings::switchDelayMs);
    ui.key_delay_spinbox->setValue(AppSettings::switchDelayMs);
    ui.key_delay_slider->setSingleStep(10);
    ui.key_delay_slider->setPageStep(50);
    ui.key_delay_slider->setTickInterval(10);
    ui.key_delay_slider->setTracking(true);
    ui.key_delay_spinbox->setSingleStep(10);

    connect(ui.key_delay_slider, &QSlider::valueChanged,
            this, [this](const int value) {
                const int stepped = (value / 10) * 10;
                if (value != stepped)
                    ui.key_delay_slider->setValue(stepped);

                if (AppSettings::switchDelayMs != stepped) {
                    AppSettings::switchDelayMs = stepped;
                    ui.key_delay_spinbox->setValue(stepped);
                    markChanged();
                }
            });

    connect(ui.key_delay_spinbox, qOverload<int>(&QSpinBox::valueChanged),
            this, [this](const int value) {
                const int stepped = ((value + 5) / 10) * 10;

                if (value != stepped)
                    ui.key_delay_spinbox->setValue(stepped);

                if (AppSettings::switchDelayMs != stepped) {
                    AppSettings::switchDelayMs = stepped;
                    ui.key_delay_slider->setValue(stepped);
                    markChanged();
                }
            });

    connect(this, &SettingsWindow::settingsSaved, this, [this]() {
        SaveNotification::showFor(this, Lang::tr("SETTINGS_ALL_CHANGES_SAVED"));
        QTimer::singleShot(50, this, [this]() { refreshTranslations(); });
    });

    LOG_DEBUG() << "SettingsWindow initialized";
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
    if (ev->type() == QEvent::WindowActivate) {
        QTimer::singleShot(0, this, [this]() {
            AcrylicHelper::setAcrylicEnabled(this, true);
            AcrylicHelper::updateRegion(this);
        });
        LOG_DEBUG() << "Settings window is active";
    } else if (ev->type() == QEvent::WindowDeactivate) {
        QTimer::singleShot(0, this, [this]() {
            AcrylicHelper::setAcrylicEnabled(this, false);
            AcrylicHelper::updateRegion(this);
        });
        LOG_DEBUG() << "Settings window is inactive";
    }
    return QWidget::event(ev);
}

bool SettingsWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui.key_select_label_img && keyHoverWarning) {
        if (event->type() == QEvent::Enter) {
            keyHoverWarning->showNear(ui.key_select_label_img);
        } else if (event->type() == QEvent::Leave) {
            keyHoverWarning->hideNow();
        }
    } else if (watched == ui.btn_upd_manually && updateBtnToolTip) {
        if (event->type() == QEvent::Enter) {
            // Показываем тултип с нужным ключом локализации
            updateBtnToolTip->showAt(ui.btn_upd_manually, "SETTINGS_TOOLTIP_CHECK_NOW");
        } else if (event->type() == QEvent::Leave) {
            updateBtnToolTip->hideAnimated();
        }
    }

    return QWidget::eventFilter(watched, event);
}

bool SettingsWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result) {
    if (eventType == "windows_generic_MSG") {
        // Третий параметр - цвет фона, который будет на превью вместо прозрачности.
        if (AcrylicHelper::handleIconicMessages(this, message, QColor(32, 32, 32))) {
            *result = 0;
            return true;
        }
    }
    return QWidget::nativeEvent(eventType, message, result);
}

void SettingsWindow::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);

    if (updateBtnToolTip) {
        updateBtnToolTip->hide();
    }

    if (keyHoverWarning) {
        keyHoverWarning->hideNow();
    }

    if (updPopup) {
        updPopup->hide();
    }
}

void SettingsWindow::openCentered() {
    const QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) screen = QGuiApplication::screens().first();
    const QRect geom = screen->availableGeometry();
    QSize s = sizeHint();
    if (!s.isValid()) s = QSize(850, 500);

    const int x = geom.center().x() - s.width() / 2;
    const int y = geom.center().y() - s.height() / 2;

    LOG_DEBUG() << QString("Settings window size: (%1, %2); position: (%3, %4)")
                    .arg(s.width()).arg(s.height()).arg(x).arg(y);

    resize(s);
    move(x, y);
    show();
    raise();
    activateWindow();
}

void SettingsWindow::buildPresetMap() {
    presetMap.clear();
    for (const QList<QPushButton *>
         presetButtons = ui.key_select_frame->findChildren<QPushButton *>(); const auto *btn: presetButtons) {
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

int SettingsWindow::vkFromPresetObjectName(const QString &obj) const {
    return presetMap.value(obj, 0);
}

QString SettingsWindow::nameFromVk(const int vk) {
    return VkMapper::vkToName(vk);
}

void SettingsWindow::applyHotkeyIfChanged(const int newVk, const QString &newName) {
    if (newVk == AppSettings::hotkeyMainVk && newName == AppSettings::hotkeyName) {
        LOG_DEBUG() << "Hotkey not changed";
        return;
    }

    AppSettings::hotkeyMainVk = newVk;
    AppSettings::hotkeyModifiers = 0;
    AppSettings::hotkeyName = newName;

    for (const QList<QPushButton *> presetButtons = ui.key_select_frame->findChildren<QPushButton *>(); auto *btn:
         presetButtons) {
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
            // emit settingsChanged();
            AppSettings::previousHotkeyMainVk = 0;
            AppSettings::previousHotkeyName.clear();

            LOG_DEBUG() << "Previous preset not restored - hotkey already matches";
        }
    } else {
        // emit settingsChanged();
        LOG_DEBUG() << "No previous preset to restore";
    }
}

void SettingsWindow::restoreDefaultsForCurrentPage() {
    switch (const int page = ui.content_container->currentIndex()) {
        case 0:
            restoreDefaults_General();
            break;

        // case 1: restoreDefaults_Indicator(); break;
        // case 2: restoreDefaults_Exclusions(); break;

        default:
            LOG_DEBUG() << "Unhandled page index " << page;
            return;
    }

    markChanged();
}

void SettingsWindow::restoreDefaults_General() {
    AppSettings::reset();

    // обновление UI
    for (auto *btn: ui.key_select_frame->findChildren<QPushButton *>()) {
        const int vk = vkFromPresetObjectName(btn->objectName());
        btn->blockSignals(true);
        btn->setChecked(vk != 0 && vk == AppSettings::hotkeyMainVk);
        btn->blockSignals(false);
    }

    if (auto *seq = findChild<QKeySequenceEdit *>("btn_sequence")) {
        if (AppSettings::hotkeyMainVk == AppSettings::defaultHotkeyMainVk &&
            presetMap.values().contains(AppSettings::defaultHotkeyMainVk)) {
            seq->setKeySequence(QKeySequence());
            seq->setStyleSheet("color: rgba(255, 255, 255, 225);");
            if (auto *le = seq->findChild<QLineEdit *>())
                le->setPlaceholderText(Lang::tr("SETTINGS_KEY_SEQUENCE"));
        } else {
            seq->setKeySequence(QKeySequence(AppSettings::hotkeyName));
        }
        seq->clearFocus();
    }

    ui.key_delay_slider->setValue(AppSettings::defaultSwitchDelayMs);
    ui.key_delay_spinbox->setValue(AppSettings::defaultSwitchDelayMs);

    // запуск анимации индикатора: key_select_frame
    for (auto *sel: selectors) {
        if (!sel) continue;
        if (sel->boundFrame() == ui.key_select_frame) {
            // убедиться, что внутреннее состояние чисто
            sel->stopAndResetAnimation();
            // и запустить анимацию на следующем обороте цикла событий
            QTimer::singleShot(0, sel, &AnimatedSelector::animateToCurrentState);
            break;
        }
    }

    // AppSettings::appLang = AppSettings::defaultAppLang;
    // ui.btn_en_lang->setChecked(true);
    // ui.btn_ru_lang->setChecked(false);
    //
    // // запуск анимации индикатора: app_lang_frame
    // for (auto *sel: selectors) {
    //     if (!sel) continue;
    //     if (sel->boundFrame() == ui.app_lang_frame) {
    //         sel->stopAndResetAnimation();
    //         QTimer::singleShot(0, sel, &AnimatedSelector::animateToCurrentState);
    //         break;
    //     }
    // }

    markChanged();
}

void SettingsWindow::setUpdateManager(UpdateManager *manager) {
    if (!manager) return;
    this->updateManager = manager;

    connect(ui.btn_upd_manually, &QPushButton::clicked, updateManager, &UpdateManager::checkForUpdatesForce);

    connect(updPopup, &UpdFrequencyPopup::selected, this, [this](const AppSettings::UpdateFrequency) {
        if (updateManager) updateManager->checkForUpdatesIfDue();
    });
}

void SettingsWindow::initUpdateFrequency() {
    ui.btn_upd_frequency->setText(
        AppSettings::updateFrequencyToString(AppSettings::updateFrequency));

    updPopup = new UpdFrequencyPopup(this);

    connect(ui.btn_upd_frequency, &QPushButton::clicked, this, [this] {
        updPopup->setCurrent(AppSettings::updateFrequency);

        constexpr int padding = 4;
        updPopup->setFixedWidth(ui.btn_upd_frequency->width() + (padding * 2));

        const QPoint btnGlobal =
                ui.btn_upd_frequency->mapToGlobal(QPoint(-padding, 0));

        const int offsetY = updPopup->currentButtonYOffset();

        updPopup->move(btnGlobal.x(), btnGlobal.y() - offsetY);
        updPopup->show();
    });


    connect(updPopup, &UpdFrequencyPopup::selected, this, [this](const AppSettings::UpdateFrequency value) {
        AppSettings::updateFrequency = value;
        markChanged();

        ui.btn_upd_frequency->setText(AppSettings::updateFrequencyToString(value));
    });

    connect(this, &SettingsWindow::settingsSaved,
            updPopup, &UpdFrequencyPopup::refreshTranslations);

    updateBtnToolTip = new CustomToolTip(this);

    // Устанавливаем фильтр на кнопку ручной проверки
    ui.btn_upd_manually->installEventFilter(this);
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
    ui.btn_upd_frequency->setText(
        AppSettings::updateFrequencyToString(AppSettings::updateFrequency));
    if (updPopup) updPopup->refreshTranslations();
    if (updateBtnToolTip) updateBtnToolTip->refreshTranslations();

    keyHoverWarning->setText(Lang::tr("SETTINGS_KEY_HOVER_WARNING_POPUP"));

    if (const auto *seq = findChild<QKeySequenceEdit *>("btn_sequence"))
        if (auto *le = seq->findChild<QLineEdit *>())
            le->setPlaceholderText(Lang::tr("SETTINGS_KEY_SEQUENCE"));

    if (m_keyHelper) m_keyHelper->setPlaceholder(Lang::tr("SETTINGS_KEY_SEQUENCE"));
}
