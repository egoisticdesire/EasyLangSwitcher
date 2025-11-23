#include "settings.h"
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
#include <QDebug>

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
    connect(&autosaveTimer, &QTimer::timeout, this, [this]() {
        AppSettings::save();
        hasPendingChanges = false;
        qDebug() << "[SettingsWindow] autosave successfully.";
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
            qDebug() << "[SettingsWindow] matched preset button" << obj << "for hotkey" << AppSettings::hotkeyName;
        } else {
            btn->setChecked(false);
        }
    }

    if (!matchedPreset) {
        if (auto *seq = findChild<QKeySequenceEdit *>("btn_sequence")) {
            if (const QString name = AppSettings::hotkeyName; !name.isEmpty()) {
                seq->setKeySequence(QKeySequence(name));
                qDebug() << "[SettingsWindow] populated sequence edit with saved custom key:" << name;
            }
        }
    }

    if (keyHelper) {
        connect(keyHelper, &KeySequenceHelper::hotkeySelected, this,
                [this](const int mainVk, int /*mods*/, const QString &name) {
                    if (mainVk == 0) {
                        qDebug() << "[SettingsWindow] sequence cleared by user";
                        restorePreviousPresetIfNeeded();
                        return;
                    }

                    if (presetMap.values().contains(AppSettings::hotkeyMainVk)) {
                        previousPresetVk = AppSettings::hotkeyMainVk;
                        previousPresetName = AppSettings::hotkeyName;
                        qDebug() << "[SettingsWindow] saved previousPreset vk=" << previousPresetVk << "name=" <<
                                previousPresetName;
                    } else {
                        qDebug() << "[SettingsWindow] current hotkey is custom; not saving previousPreset";
                    }

                    qDebug() << "[SettingsWindow] custom hotkey selected mainVk=" << mainVk << "name=" << name;
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
            qDebug() << "[SettingsWindow] preset selected" << obj << "vk=" << vk << "name=" << name;

            // Обновляем hotkey
            applyHotkeyIfChanged(vk, name);

            // Теперь, когда инпут очищается, previousPresetVk = текущий vk
            previousPresetVk = vk;
            previousPresetName = name;

            // Очищаем QKeySequenceEdit, чтобы плейсхолдер показывался
            if (auto *seqEdit = findChild<QKeySequenceEdit *>("btn_sequence")) {
                seqEdit->setKeySequence(QKeySequence());
                seqEdit->clearFocus();
                // move focus away to window so placeholder shows correctly
                this->setFocus(Qt::OtherFocusReason);
                // ensure placeholder is set (KeySequenceHelper will also guard)
                if (auto *le = seqEdit->findChild<QLineEdit *>()) le->setPlaceholderText(QStringLiteral("Key..."));
            }
        });
    }
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
        for (const AnimatedSelector *sel: selectors) if (sel) sel->initPosition();
    });
}

bool SettingsWindow::event(QEvent *ev) {
    if (ev->type() == QEvent::WindowActivate) {
        QTimer::singleShot(0, this, [this]() {
            AcrylicHelper::setAcrylicEnabled(this, true);
            AcrylicHelper::updateRegion(this);
        });
    } else if (ev->type() == QEvent::WindowDeactivate) {
        QTimer::singleShot(0, this, [this]() {
            AcrylicHelper::setAcrylicEnabled(this, false);
            AcrylicHelper::updateRegion(this);
        });
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
    resize(s);
    move(x, y);
    show();
    raise();
    activateWindow();
}

// ----- helpers -----

void SettingsWindow::buildPresetMap() {
    presetMap.clear();
    for (const QList<QPushButton *> presetButtons = ui.key_select_frame->findChildren<QPushButton *>(); const auto *btn:
         presetButtons) {
        const QString obj = btn->objectName().toLower();
        int vk = 0;
        if (obj.contains("lctrl") || obj.contains("btn_lctrl")) vk = VK_LCONTROL;
        else if (obj.contains("rctrl") || obj.contains("btn_rctrl")) vk = VK_RCONTROL;
        else if (obj.contains("lalt") || obj.contains("btn_lalt")) vk = VK_LMENU;
        else if (obj.contains("ralt") || obj.contains("btn_ralt")) vk = VK_RMENU;
        else if (obj.contains("lshift") || obj.contains("btn_lshift")) vk = VK_LSHIFT;
        else if (obj.contains("rshift") || obj.contains("btn_rshift")) vk = VK_RSHIFT;
        else if (obj.contains("caps") || obj.contains("btn_caps")) vk = VK_CAPITAL;

        if (vk != 0) presetMap.insert(btn->objectName(), vk);
    }
}

int SettingsWindow::vkFromPresetObjectName(const QString &obj) const {
    return presetMap.value(obj, 0);
}

QString SettingsWindow::nameFromVk(const int vk) {
    return VkMapper::vkToName(vk);
}

void SettingsWindow::applyHotkeyIfChanged(const int newVk, const QString &newName) {
    if (newVk == AppSettings::hotkeyMainVk && newName == AppSettings::hotkeyName) {
        qDebug() << "[SettingsWindow] applyHotkeyIfChanged - no change";
        return;
    }

    qDebug() << "[SettingsWindow] applyHotkeyIfChanged - applying vk=" << newVk << "name=" << newName;
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
        qDebug() << "[SettingsWindow] marked changed, autosave scheduled in 1s.";
        emit settingsChanged();
    } else {
        autosaveTimer.start();
        qDebug() << "[SettingsWindow] change already pending - timer restarted";
    }
}

void SettingsWindow::restorePreviousPresetIfNeeded() {
    if (previousPresetVk != 0) {
        if (AppSettings::hotkeyMainVk != previousPresetVk) {
            qDebug() << "[SettingsWindow] restoring previous preset vk=" << previousPresetVk << "name=" <<
                    previousPresetName;
            AppSettings::hotkeyMainVk = previousPresetVk;
            AppSettings::hotkeyModifiers = 0;
            AppSettings::hotkeyName = previousPresetName;

            for (const QList<QPushButton *> presetButtons = ui.key_select_frame->findChildren<QPushButton *>(); auto *
                 btn: presetButtons) {
                const int vk = vkFromPresetObjectName(btn->objectName());
                btn->setChecked(vk != 0 && vk == previousPresetVk);
            }

            previousPresetVk = 0;
            previousPresetName.clear();

            markChanged();
        } else {
            emit settingsChanged();
            previousPresetVk = 0;
            previousPresetName.clear();
        }
    } else {
        emit settingsChanged();
    }
}
