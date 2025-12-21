#include "tray.h"
#include "../widgets/soundManager.h"
#include "../../core/config/logger.h"
#include "../../core/config/appSettings.h"
#include "../../core/i18n/lang.h"
#include "../helpers/acrylicHelper.h"
#include "../helpers/hoverHelper.h"
#include "../helpers/iconHelper.h"
#include <QApplication>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QMouseEvent>

TrayManager::TrayManager(QWidget *parent)
    : QWidget(parent) {
    ui.setupUi(this);

    setWindowFlags(Qt::Popup | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::StrongFocus);

    // Создаем один раз при запуске
    settingsWindow = new SettingsWindow(nullptr);
    settingsWindow->setAttribute(Qt::WA_DeleteOnClose, false);

    // Анимации
    fadeIn = new QPropertyAnimation(this, "windowOpacity", this);
    fadeIn->setDuration(180);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);

    posAnim = new QPropertyAnimation(this, "pos", this);
    posAnim->setDuration(250);
    posAnim->setEasingCurve(QEasingCurve::OutBack);

    showGroup = new QParallelAnimationGroup(this);
    showGroup->addAnimation(fadeIn);
    showGroup->addAnimation(posAnim);

    fadeOut = new QPropertyAnimation(this, "windowOpacity", this);
    fadeOut->setDuration(140);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::InCubic);
    connect(fadeOut, &QPropertyAnimation::finished, this, &QWidget::hide);

    // Звуки
    audioEffectOn = new QSoundEffect(this);
    audioEffectOn->setSource(QUrl("qrc:/sounds/sounds/on.wav"));
    audioEffectOn->setVolume(0.5f);

    audioEffectOff = new QSoundEffect(this);
    audioEffectOff->setSource(QUrl("qrc:/sounds/sounds/off.wav"));
    audioEffectOff->setVolume(0.5f);

    soundManager::instance().registerEffect(audioEffectOn);
    soundManager::instance().registerEffect(audioEffectOff);

    // Таймер для разделения Single и Double кликов
    clickTimer = new QTimer(this);
    clickTimer->setSingleShot(true);
    connect(clickTimer, &QTimer::timeout, this, [this]() {
        enabled = !enabled;
        enabled ? audioEffectOn->play() : audioEffectOff->play();
        emit keyboardToggled(enabled);
        updateTrayIcon();
        LOG_DEBUG() << "Single click: Toggle keyboard";
    });

    ui.info_frame->installEventFilter(this);
    if (auto *app = qobject_cast<QApplication *>(QCoreApplication::instance())) {
        app->installEventFilter(this);
    }

    setupUiBehavior();
    setupTrayIcon();

    setWindowOpacity(0.0);
    hide();
    updateInfo();

    LOG_DEBUG() << "TrayManager initialized";
}

// Деструктор для очистки памяти
TrayManager::~TrayManager() {
    if (settingsWindow) {
        delete settingsWindow; // Удаляем окно из памяти
        settingsWindow = nullptr;
    }
}

void TrayManager::openSettings() const {
    if (!settingsWindow) return;

    // Сначала скрываем меню трея, так как мы уже нажали кнопку
    hideAnimated();

    if (settingsWindow->isVisible()) {
        settingsWindow->showNormal(); // На случай, если оно свернуто
        settingsWindow->raise(); // Поверх других окон
        settingsWindow->activateWindow(); // Фокус на окно
    } else {
        settingsWindow->openCentered(); // Открываем по центру
    }
    LOG_DEBUG() << "Settings window opened or brought to front";
}

void TrayManager::showAtCursor() {
    updateInfo();
    resize(sizeHint());

    const QPoint cursorPos = QCursor::pos();
    const QScreen *screen = QGuiApplication::screenAt(cursorPos);
    if (!screen) screen = QGuiApplication::primaryScreen();

    const QRect screenRect = screen->geometry();
    QPoint finalPos = cursorPos;
    constexpr int padding = 3;
    constexpr int slideDist = 12;

    bool isLeft, isTop;

    if (finalPos.x() + width() > screenRect.right()) {
        finalPos.rx() -= (width() + padding);
        isLeft = false;
    } else {
        finalPos.rx() += padding;
        isLeft = true;
    }

    if (finalPos.y() + height() > screenRect.bottom()) {
        finalPos.ry() -= (height() + padding);
        isTop = false;
    } else {
        finalPos.ry() += padding;
        isTop = true;
    }

    QPoint startPos = finalPos;
    if (cursorPos.y() > screenRect.bottom() - 100 || cursorPos.y() < screenRect.top() + 100) {
        startPos.setY(isTop ? finalPos.y() - slideDist : finalPos.y() + slideDist);
    } else {
        startPos.setX(isLeft ? finalPos.x() - slideDist : finalPos.x() + slideDist);
    }

    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    posAnim->setStartValue(startPos);
    posAnim->setEndValue(finalPos);

    move(startPos);
    setVisible(true);
    raise();
    activateWindow();
    showGroup->start();

    QTimer::singleShot(1, this, [this]() { AcrylicHelper::enableAcrylic(this); });
}

void TrayManager::animateToggleButton() {
    // Создаем эффект только на время анимации, чтобы не портить цвет текста
    auto *effect = new QGraphicsOpacityEffect(ui.toggle_btn);
    ui.toggle_btn->setGraphicsEffect(effect);

    auto *fadeOutBtn = new QPropertyAnimation(effect, "opacity");
    fadeOutBtn->setDuration(160);
    fadeOutBtn->setStartValue(1.0);
    fadeOutBtn->setEndValue(0.0);

    auto *fadeInBtn = new QPropertyAnimation(effect, "opacity");
    fadeInBtn->setDuration(200);
    fadeInBtn->setStartValue(0.0);
    fadeInBtn->setEndValue(1.0);

    connect(fadeOutBtn, &QPropertyAnimation::finished, this, [this, fadeInBtn]() {
        updateInfo();
        fadeInBtn->start(QAbstractAnimation::DeleteWhenStopped);
    });

    // Удаляем эффект полностью после завершения, чтобы вернуть родной рендеринг текста
    connect(fadeInBtn, &QPropertyAnimation::finished, this, [this]() {
        ui.toggle_btn->setGraphicsEffect(nullptr);
    });

    fadeOutBtn->start(QAbstractAnimation::DeleteWhenStopped);
}

void TrayManager::hideAnimated() const {
    if (isVisible() && fadeOut->state() != QAbstractAnimation::Running) {
        fadeOut->start();
    }
}

void TrayManager::setupTrayIcon() {
    trayIcon.setIcon(IconHelper::loadIcon(":/icons/icons/FlashSparkleFilled2.png"));
    trayIcon.setToolTip(AppSettings::APP_NAME);
    trayIcon.setVisible(true);

    connect(&trayIcon, &QSystemTrayIcon::activated, this,
            [this](const QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger) {
                    clickTimer->start(250);
                } else if (reason == QSystemTrayIcon::DoubleClick) {
                    clickTimer->stop();
                    openSettings();
                } else if (reason == QSystemTrayIcon::Context) {
                    isVisible() ? hideAnimated() : showAtCursor();
                }
            });
}

void TrayManager::setupUiBehavior() {
    // Используем метод напрямую
    connect(ui.settings_btn, &QPushButton::clicked, this, [this]() {
        openSettings();
    });

    if (settingsWindow) {
        connect(settingsWindow, &SettingsWindow::settingsChanged, this, [this]() {
            updateInfo();
        });
    }

    connect(ui.exit_btn, &QToolButton::clicked, this, [this]() {
        emit exitRequested();
    });

    connect(ui.toggle_btn, &QToolButton::clicked, this, [this]() {
        enabled = !enabled;
        enabled ? audioEffectOn->play() : audioEffectOff->play();
        emit keyboardToggled(enabled);
        animateToggleButton();
        updateTrayIcon();
    });

    HoverEffectHelper::initializeHoverEffects(this);
}

void TrayManager::updateInfo() const {
    ui.status_value->setText(enabled ? Lang::tr("TRAY_TOGGLE_ENABLED") : Lang::tr("TRAY_TOGGLE_DISABLED"));
    ui.hotkey_value->setText(AppSettings::hotkeyName);
    ui.delay_value->setText(QString::number(AppSettings::switchDelayMs));
    ui.toggle_btn->setText(enabled ? Lang::tr("TRAY_TOGGLE_PAUSE") : Lang::tr("TRAY_TOGGLE_RESUME"));

    ui.settings_btn->setText(Lang::tr("TRAY_SETTINGS"));
    ui.exit_btn->setText(Lang::tr("TRAY_EXIT"));

    ui.status_key->setText(Lang::tr("TRAY_LABEL_STATUS"));
    ui.hotkey_key->setText(Lang::tr("TRAY_LABEL_HOTKEY"));
    ui.delay_key->setText(Lang::tr("TRAY_LABEL_DELAY"));

    ui.toggle_btn->setIcon(IconHelper::loadIcon(
        enabled
            ? ":/icons/icons/FlashSparkleRegular.svg"
            : ":/icons/icons/FlashSparkleFilled.svg"));
    ui.settings_btn->setIcon(IconHelper::loadIcon(":/icons/icons/FlashSettingsRegular.svg"));
    ui.exit_btn->setIcon(IconHelper::loadIcon(":/icons/icons/FlashOffRegular.svg"));
}

bool TrayManager::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui.info_frame) {
        if (event->type() == QEvent::Enter) HoverEffectHelper::animateHover(ui.info_frame, true);
        else if (event->type() == QEvent::Leave) HoverEffectHelper::animateHover(ui.info_frame, false);
    }
    if (isVisible() && event->type() == QEvent::MouseButtonPress) {
        if (const auto *me = dynamic_cast<QMouseEvent *>(event)) {
            if (!geometry().contains(me->globalPosition().toPoint())) {
                hideAnimated();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void TrayManager::focusOutEvent(QFocusEvent *event) {
    hideAnimated();
    QWidget::focusOutEvent(event);
}

void TrayManager::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    AcrylicHelper::updateRegion(this);
}

void TrayManager::updateTrayIcon() {
    trayIcon.setIcon(IconHelper::loadIcon(
        enabled
            ? ":/icons/icons/FlashSparkleFilled2.png"
            : ":/icons/icons/FlashSparkleRegular2.png"));
}
