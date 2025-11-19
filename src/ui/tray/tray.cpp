#include "tray.h"
#include "../../core/helpers/iconHelper.h"
#include "../../core/helpers/acrylicHelper.h"
#include "../../core/helpers/hoverHelper.h"
#include "../../core/config/app_config.h"
#include <QApplication>
#include <QCursor>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QMouseEvent>
#include <QPushButton>

TrayManager::TrayManager(QWidget *parent)
    : QWidget(parent) {
    ui.setupUi(this);
    setWindowFlags(Qt::Popup | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::StrongFocus);

    // Создаём окно настроек один раз (можно создать лениво при первом клике)
    settingsWindow = new SettingsWindow(nullptr);
    settingsWindow->setAttribute(Qt::WA_DeleteOnClose, false);

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
    trayIcon.setIcon(IconHelper::loadIcon(":/icons/icons/FlashSparkleFilled2.png"));
    trayIcon.setToolTip("EasyLangSwitcher");
    trayIcon.setVisible(true);

    connect(&trayIcon, &QSystemTrayIcon::activated, this,
            [this](const QSystemTrayIcon::ActivationReason reason) {
                switch (reason) {
                    case QSystemTrayIcon::Trigger:
                        // ЛКМ — быстрый вкл/выкл
                        enabled = !enabled;
                        emit keyboardToggled(enabled);
                        updateTrayIcon();
                        break;
                    case QSystemTrayIcon::Context:
                        // ПКМ — показать меню
                        if (isVisible()) hideAnimated();
                        else showAtCursor();
                        break;
                    default:
                        break;
                }
            });
}

void TrayManager::setupUiBehavior() {
    connect(ui.settings_btn, &QPushButton::clicked, this, [this]() {
        if (!settingsWindow) {
            settingsWindow = new SettingsWindow(nullptr);
            settingsWindow->setAttribute(Qt::WA_DeleteOnClose, false);
        }
        settingsWindow->openCentered();
    });

    connect(ui.exit_btn, &QToolButton::clicked, this, [this]() {
        emit exitRequested();
    });
    connect(ui.toggle_btn, &QToolButton::clicked, this, [this]() {
        enabled = !enabled;
        emit keyboardToggled(enabled);
        animateToggleButton();
        updateTrayIcon();
    });

    updateInfo();
    HoverEffectHelper::initializeHoverEffects(this);
}

void TrayManager::updateInfo() const {
    ui.status_value->setText(enabled ? tr("Enabled") : tr("Disabled"));
    ui.hotkey_value->setText(AppConfig::hotkeyName);
    ui.delay_value->setText(QString::number(AppConfig::switchDelayMs));
    ui.toggle_btn->setText(enabled ? tr("  Disable") : tr("  Enable"));

    ui.toggle_btn->setIcon(
        enabled
            ? IconHelper::loadIcon(":/icons/icons/FlashSparkleRegular.svg")
            : IconHelper::loadIcon(":/icons/icons/FlashSparkleFilled.svg")
    );
    ui.settings_btn->setIcon(IconHelper::loadIcon(":/icons/icons/FlashSettingsRegular.svg"));
    ui.exit_btn->setIcon(IconHelper::loadIcon(":/icons/icons/FlashOffRegular.svg"));
}

void TrayManager::showAtCursor() {
    updateInfo();

    // размер окна
    resize(sizeHint());

    // получаем текущий экран под курсором
    const QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen) screen = QGuiApplication::primaryScreen();

    const QRect workArea = screen->geometry();
    const QPoint cursorPos = QCursor::pos();

    QPoint pos = cursorPos;

    // горизонтальная ориентация
    if (pos.x() + width() > workArea.right())
        pos.rx() -= width() + 3;
    else
        pos.rx() += 3;

    // вертикальная ориентация
    if (pos.y() + height() > workArea.bottom())
        pos.ry() -= height() + 3;
    else
        pos.ry() += 3;

    // применяем скорректированную позицию
    move(pos);

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

void TrayManager::updateTrayIcon() {
    trayIcon.setIcon(
        enabled
            ? IconHelper::loadIcon(":/icons/icons/FlashSparkleFilled2.png")
            : IconHelper::loadIcon(":/icons/icons/FlashSparkleRegular2.png")
    );
}
