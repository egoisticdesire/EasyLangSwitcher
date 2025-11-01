#include "tray.h"
#include <QApplication>
#include <QCursor>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QMouseEvent>

TrayManager::TrayManager(SettingsWindow &settingsWindow, QWidget *parent)
    : QWidget(parent), settingsWindow(settingsWindow) {
    ui.setupUi(this);
    setWindowFlags(Qt::Popup | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::StrongFocus);

    ui.info_frame->installEventFilter(this);
    qApp->installEventFilter(this);

    fadeIn = new QPropertyAnimation(this, "windowOpacity", this);
    fadeIn->setDuration(180);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);

    fadeOut = new QPropertyAnimation(this, "windowOpacity", this);
    fadeOut->setDuration(140);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::InCubic);
    connect(fadeOut, &QPropertyAnimation::finished, this, &QWidget::hide);

    setupUiBehavior();
    setupTrayIcon();

    setWindowOpacity(0.0);
    hide();
}

void TrayManager::setupTrayIcon() {
    trayIcon.setIcon(SvgHelper::loadSvgIcon(":/icons/icons/FluentFlashSparkle24FilledW.svg"));
    trayIcon.setToolTip("Easy Lang Switcher");
    trayIcon.setVisible(true);

    connect(&trayIcon, &QSystemTrayIcon::activated, this, [this](const QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::Context) {
            if (isVisible()) hideAnimated();
            else showAtCursor();
        }
    });
}

void TrayManager::setupUiBehavior() {
    connect(ui.settings_btn, &QToolButton::clicked, this, [this]() {
        settingsWindow.show();
        hideAnimated();
    });
    connect(ui.exit_btn, &QToolButton::clicked, this, [this]() {
        emit exitRequested();
    });
    connect(ui.toggle_btn, &QToolButton::clicked, this, [this]() {
        enabled = !enabled;
        animateToggleButton();
        emit keyboardToggled(enabled);
    });

    updateInfo();
    HoverEffectHelper::initializeHoverEffects(this);
}

void TrayManager::updateInfo() const {
    ui.status_value->setText(enabled ? tr("Enabled") : tr("Disabled"));
    ui.hotkey_value->setText(settingsWindow.getHotkeyName());
    ui.delay_value->setText(QString::number(settingsWindow.getSwitchDelayMs()));
    ui.toggle_btn->setText(enabled ? tr("  Disable") : tr("  Enable"));

    ui.toggle_btn->setIcon(
        enabled
            ? SvgHelper::loadSvgIcon(":/icons/icons/FluentFlashSparkle24RegularW.svg")
            : SvgHelper::loadSvgIcon(":/icons/icons/FluentFlashSparkle24FilledW.svg")
    );
    ui.settings_btn->setIcon(SvgHelper::loadSvgIcon(":/icons/icons/FluentFlashSettings24RegularW.svg"));
    ui.exit_btn->setIcon(SvgHelper::loadSvgIcon(":/icons/icons/FluentFlashOff24RegularW.svg"));
}

void TrayManager::showAtCursor() {
    updateInfo();
    resize(sizeHint());
    move(QCursor::pos() + QPoint(3, -height() - 3));
    setWindowOpacity(0.0);
    setVisible(true);
    raise();
    activateWindow();
    setFocus(Qt::ActiveWindowFocusReason);

    QTimer::singleShot(0, this, [this]() { AcrylicHelper::enableAcrylic(this); });
    fadeIn->start();
}

void TrayManager::hideAnimated() const { if (isVisible()) fadeOut->start(); }

bool TrayManager::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui.info_frame) {
        if (event->type() == QEvent::Enter) HoverEffectHelper::animateHover(ui.info_frame, true);
        else if (event->type() == QEvent::Leave) HoverEffectHelper::animateHover(ui.info_frame, false);
    }
    if (isVisible() && event->type() == QEvent::MouseButtonPress) {
        if (const auto *me = static_cast<QMouseEvent *>(event);
            !geometry().contains(me->globalPosition().toPoint()))
            hideAnimated();
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

void TrayManager::animateToggleButton() {
    auto *btn = ui.toggle_btn;
    auto *effect = new QGraphicsOpacityEffect(btn);
    btn->setGraphicsEffect(effect);

    auto *fadeOutBtn = new QPropertyAnimation(effect, "opacity");
    fadeOutBtn->setDuration(160);
    fadeOutBtn->setStartValue(1.0);
    fadeOutBtn->setEndValue(0.0);

    auto *fadeInBtn = new QPropertyAnimation(effect, "opacity");
    fadeInBtn->setDuration(200);
    fadeInBtn->setStartValue(0.0);
    fadeInBtn->setEndValue(1.0);

    connect(fadeOutBtn, &QPropertyAnimation::finished, [this, fadeInBtn]() {
        updateInfo();
        fadeInBtn->start(QAbstractAnimation::DeleteWhenStopped);
    });
    connect(fadeInBtn, &QPropertyAnimation::finished, [effect]() { effect->deleteLater(); });
    fadeOutBtn->start(QAbstractAnimation::DeleteWhenStopped);
}
