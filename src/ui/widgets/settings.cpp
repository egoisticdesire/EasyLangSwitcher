#include "settings.h"
#include "../../core/config/logger.h"
#include "../../core/config/app_settings.h"
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

    const auto KEY_PLACEHOLDER = QStringLiteral("Key...");

    ui.key_select_label_img->setPixmap(
        IconHelper::loadIcon(":/icons/icons/InfoRegular.svg", QColor(175, 175, 175)).pixmap(16, 16));

    buildPresetMap();

    const auto *keyHelper = new KeySequenceHelper(
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
        AppSettings::save();
        hasPendingChanges = false;
        LOG_DEBUG() << "Autosave successfully";
    });

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::ClickFocus);

    auto *closeAction = new QAction(this);
    closeAction->setShortcut(Qt::Key_Escape);
    connect(closeAction, &QAction::triggered, this, &SettingsWindow::close);
    addAction(closeAction);

    connect(ui.btn_close_bot_sider, &QPushButton::clicked, closeAction, &QAction::trigger);

    addSelectorForFrame(ui.key_select_frame);
    addSelectorForFrame(ui.app_startup_frame);
    addSelectorForFrame(ui.app_theme_frame);
    addSelectorForFrame(ui.app_lang_frame);

    dragger = new WindowDragger(this);
    dragger->addIgnoredWidget(ui.btn_close_bot_sider);

    const QList<QPushButton *> presetButtons = ui.key_select_frame->findChildren<QPushButton *>();

    bool matchedPreset = false;
    for (auto *btn: presetButtons) {
        const QString obj = btn->objectName();
        if (const int vk = presetMap.value(obj, 0); vk != 0 && vk == AppSettings::hotkeyMainVk) {
            btn->setChecked(true);
            matchedPreset = true;

            LOG_DEBUG() << "Matched preset button '" << obj << "' for hotkey '" << AppSettings::hotkeyName << "'";
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

    if (keyHelper) {
        connect(keyHelper, &KeySequenceHelper::hotkeySelected, this,
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

                    LOG_DEBUG() << "Custom hotkey selected: vk=" << mainVk << "; name='" << name << "'";
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

            LOG_DEBUG() << "Preset '" << obj << "' selected: vk=" << vk << "; name='" << name << "'";

            // Обновляем hotkey
            applyHotkeyIfChanged(vk, name);

            // Теперь, когда инпут очищается, сохраняем в AppSettings
            AppSettings::previousHotkeyMainVk = vk;
            AppSettings::previousHotkeyName = name;

            // Очищаем QKeySequenceEdit, чтобы плейсхолдер показывался
            if (auto *seqEdit = findChild<QKeySequenceEdit *>("btn_sequence")) {
                seqEdit->setKeySequence(QKeySequence());
                seqEdit->clearFocus();
                this->setFocus(Qt::OtherFocusReason);
                if (auto *le = seqEdit->findChild<QLineEdit *>()) le->setPlaceholderText(QStringLiteral("Key..."));
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


    LOG_DEBUG() << "SettingsWindow initialized";
}

SettingsWindow::~SettingsWindow() = default;

void SettingsWindow::addSelectorForFrame(QFrame *frame, const QString &extraStyle) {
    if (!frame) return;
    const auto sel = new AnimatedSelector(this);
    sel->bindToFrame(frame, extraStyle);
    selectors.append(sel);
    QTimer::singleShot(0, sel, &AnimatedSelector::initPosition);
    LOG_DEBUG() << "Selector added for frame '" << frame->objectName() << "'";
}

void SettingsWindow::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        AcrylicHelper::setAcrylicEnabled(this, true);
        AcrylicHelper::updateRegion(this);
    });
    QTimer::singleShot(0, this, [this]() {
        for (const AnimatedSelector *sel: selectors) if (sel) sel->initPosition();
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

void SettingsWindow::openCentered() {
    const QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) screen = QGuiApplication::screens().first();
    const QRect geom = screen->availableGeometry();
    QSize s = sizeHint();
    if (!s.isValid()) s = QSize(850, 500);

    const int x = geom.center().x() - s.width() / 2;
    const int y = geom.center().y() - s.height() / 2;

    LOG_DEBUG() << "Settings window size: " << QString("(%1, %2)").arg(s.width()).arg(s.height())
                    << "; position: " << QString("(%1, %2)").arg(x).arg(y);

    resize(s);
    move(x, y);
    show();
    raise();
    activateWindow();
}

void SettingsWindow::buildPresetMap() {
    presetMap.clear();
    for (const QList<QPushButton *> presetButtons = ui.key_select_frame->findChildren<QPushButton *>(); const auto *btn:
         presetButtons) {
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

            LOG_DEBUG() << "Previous preset restored: vk=" << AppSettings::previousHotkeyMainVk
                        << "; name='" << AppSettings::previousHotkeyName << "'";

            AppSettings::previousHotkeyMainVk = 0;
            AppSettings::previousHotkeyName.clear();

            markChanged();
        } else {
            emit settingsChanged();
            AppSettings::previousHotkeyMainVk = 0;
            AppSettings::previousHotkeyName.clear();

            LOG_DEBUG() << "Previous preset not restored - hotkey already matches";
        }
    } else {
        emit settingsChanged();
        LOG_DEBUG() << "No previous preset to restore";
    }
}
