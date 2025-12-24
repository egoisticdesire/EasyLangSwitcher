#include "tray.h"
#include "../widgets/soundManager.h"
#include "../../core/config/logger.h"
#include "../../core/config/appSettings.h"
#include "../../core/i18n/lang.h"
#include "../helpers/acrylicHelper.h"
#include "../helpers/trayHoverHelper.h"
#include "../helpers/iconHelper.h"
#include <QMouseEvent>
#include <QScreen>
#include <QGraphicsOpacityEffect>

TrayManager::TrayManager(QWidget *parent)
    : QWidget(parent) {
    ui.setupUi(this);

    setWindowFlags(Qt::Popup | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::StrongFocus);

    updateManager = new UpdateManager(this);
    settingsWindow = new SettingsWindow(nullptr);
    settingsWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    settingsWindow->setUpdateManager(updateManager);
    updateManager->start();

    // Обработка обновлений
    connect(updateManager, &UpdateManager::updateAvailable, this, [this](const QString &tag, const QString &url) {
        trayIcon.showMessage(
            Lang::tr("SETTINGS_APP_UPD_AVAILABLE_TITLE"),
            Lang::tr("SETTINGS_APP_UPD_AVAILABLE_MSG").arg(tag),
            QSystemTrayIcon::Information,
            0
        );
    });

    setupAnimations();

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
        updateInfo();
    });

    ui.info_frame->installEventFilter(this);

    setupUiBehavior();
    setupTrayIcon();

    setWindowOpacity(0.0);
    hide();
    updateInfo();

    LOG_DEBUG() << "TrayManager initialized";
}

TrayManager::~TrayManager() {
    delete settingsWindow;
}

void TrayManager::setupAnimations() {
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
    fadeOut->setDuration(160);
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::InCubic);

    connect(fadeOut, &QPropertyAnimation::finished, this, [this]() {
        this->hide();
        m_isClosing = false;
    });
}

void TrayManager::showAtCursor() {
    m_isClosing = false;
    fadeOut->stop();

    updateInfo();

    // Подготовка размеров попапа до его появления
    layout()->activate();
    adjustSize();

    const QPoint cursorPos = QCursor::pos();
    const QScreen *screen = QGuiApplication::screenAt(cursorPos);
    if (!screen) screen = QGuiApplication::primaryScreen();

    const QRect screenRect = screen->availableGeometry();
    QPoint finalPos = cursorPos;
    constexpr int padding = 3;
    constexpr int slideDist = 12;

    // Вычисление позиции
    bool isLeft = true;
    if (finalPos.x() + width() > screenRect.right()) {
        finalPos.setX(finalPos.x() - width() - padding);
        isLeft = false;
    } else {
        finalPos.setX(finalPos.x() + padding);
    }

    bool isTop = true;
    if (finalPos.y() + height() > screenRect.bottom()) {
        finalPos.setY(finalPos.y() - height() - padding);
        isTop = false;
    } else {
        finalPos.setY(finalPos.y() + padding);
    }

    // Анимация вылета
    QPoint startPos = finalPos;
    if (cursorPos.y() > screenRect.bottom() - 100 || cursorPos.y() < screenRect.top() + 100) {
        startPos.setY(isTop ? finalPos.y() - slideDist : finalPos.y() + slideDist);
    } else {
        startPos.setX(isLeft ? finalPos.x() - slideDist : finalPos.x() + slideDist);
    }

    move(startPos);
    setWindowOpacity(0.0);

    show();
    raise();
    activateWindow();

    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    posAnim->setStartValue(startPos);
    posAnim->setEndValue(finalPos);

    showGroup->start();

    QTimer::singleShot(0, this, [this]() { AcrylicHelper::enableAcrylic(this); });
}

void TrayManager::hideAnimated() const {
    if (!isVisible() || m_isClosing) return;
    m_isClosing = true;
    showGroup->stop();
    fadeOut->setStartValue(this->windowOpacity());
    fadeOut->start();
}

void TrayManager::openSettings() const {
    hideAnimated();
    if (!settingsWindow) return;

    QTimer::singleShot(0, settingsWindow, [this]() {
        settingsWindow->openCentered();
        settingsWindow->raise();
        settingsWindow->activateWindow();
    });
}

void TrayManager::setupTrayIcon() {
    updateTrayIcon();
    trayIcon.setToolTip(AppSettings::APP_NAME);
    trayIcon.setVisible(true);

    connect(&trayIcon, &QSystemTrayIcon::activated, this, [this](const QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            clickTimer->start(250);
        } else if (reason == QSystemTrayIcon::DoubleClick) {
            clickTimer->stop();
            openSettings();
        } else if (reason == QSystemTrayIcon::Context) {
            if (isVisible() && !m_isClosing) hideAnimated();
            else showAtCursor();
        }
    });
}

void TrayManager::updateTrayIcon() {
    trayIcon.setIcon(IconHelper::loadIcon(enabled
                                              ? ":/icons/icons/FlashSparkleFilled2.png"
                                              : ":/icons/icons/FlashSparkleRegular2.png"));
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

    ui.toggle_btn->setIcon(IconHelper::loadIcon(enabled
                                                    ? ":/icons/icons/FlashSparkleRegular.svg"
                                                    : ":/icons/icons/FlashSparkleFilled.svg"));
    ui.settings_btn->setIcon(IconHelper::loadIcon(":/icons/icons/FlashSettingsRegular.svg"));
    ui.exit_btn->setIcon(IconHelper::loadIcon(":/icons/icons/FlashOffRegular.svg"));
}

void TrayManager::animateToggleButton() {
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

    connect(fadeInBtn, &QPropertyAnimation::finished, this, [this]() {
        ui.toggle_btn->setGraphicsEffect(nullptr);
    });

    fadeOutBtn->start(QAbstractAnimation::DeleteWhenStopped);
}

void TrayManager::setupUiBehavior() {
    connect(ui.settings_btn, &QPushButton::clicked, this, &TrayManager::openSettings);

    if (settingsWindow) {
        connect(settingsWindow, &SettingsWindow::settingsChanged, this, &TrayManager::updateInfo);
    }

    connect(ui.exit_btn, &QToolButton::clicked, this, &TrayManager::exitRequested);

    connect(ui.toggle_btn, &QToolButton::clicked, this, [this]() {
        enabled = !enabled;
        enabled ? audioEffectOn->play() : audioEffectOff->play();
        emit keyboardToggled(enabled);
        animateToggleButton();
        updateTrayIcon();
    });

    TrayHoverHelper::initializeHover(this);
}

bool TrayManager::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui.info_frame) {
        if (event->type() == QEvent::Enter) TrayHoverHelper::animateHover(ui.info_frame, true);
        else if (event->type() == QEvent::Leave) TrayHoverHelper::animateHover(ui.info_frame, false);
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
