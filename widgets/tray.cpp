#include "tray.h"
#include <QApplication>
#include <QTimer>
#include <QCursor>
#include <QMouseEvent>
#include <QGraphicsOpacityEffect>

#include <Windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

// WinAPI типы
using pSetWindowCompositionAttribute = BOOL (WINAPI *)(HWND, struct WINDOWCOMPOSITIONATTRIBDATA *);

enum WINDOWCOMPOSITIONATTRIB {
    WCA_ACCENT_POLICY = 19
};

enum ACCENT_STATE {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4
};

struct ACCENT_POLICY {
    ACCENT_STATE AccentState;
    DWORD AccentFlags;
    DWORD GradientColor;
    DWORD AnimationId;
};

struct WINDOWCOMPOSITIONATTRIBDATA {
    WINDOWCOMPOSITIONATTRIB Attribute;
    PVOID Data;
    SIZE_T SizeOfData;
};

TrayManager::TrayManager(SettingsWindow &settingsWindow, QWidget *parent)
    : QWidget(parent), settingsWindow(settingsWindow) {
    ui.setupUi(this);
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
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

QIcon TrayManager::loadSvgIcon(const QString &path, const QSize &size) {
    QSvgRenderer renderer(path);
    if (!renderer.isValid()) return QIcon();

    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    return QIcon(pixmap);
}

void TrayManager::setupTrayIcon() {
    trayIcon.setIcon(loadSvgIcon(":/icons/icons/FluentFlashSparkle24FilledW.svg"));
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
    });

    updateInfo();
}

void TrayManager::updateInfo() const {
    ui.status_value->setText(enabled ? tr("Enabled") : tr("Disabled"));
    ui.hotkey_value->setText(settingsWindow.getHotkeyName());
    ui.delay_value->setText(QString::number(settingsWindow.getSwitchDelayMs()));
    ui.toggle_btn->setText(enabled ? tr("  Disable") : tr("  Enable"));

    const QIcon ic = enabled
                         ? loadSvgIcon(":/icons/icons/FluentFlashSparkle24RegularW.svg")
                         : loadSvgIcon(":/icons/icons/FluentFlashSparkle24FilledW.svg");
    ui.toggle_btn->setIcon(ic);
    ui.settings_btn->setIcon(loadSvgIcon(":/icons/icons/FluentFlashSettings24RegularW.svg"));
    ui.exit_btn->setIcon(loadSvgIcon(":/icons/icons/FluentFlashOff24RegularW.svg"));
}

void TrayManager::enableAcrylic() const {
    const auto hwnd = reinterpret_cast<HWND>(winId());
    if (!hwnd) return;

    // 1) Убираем WS_EX_LAYERED, если он есть (чтобы DWM корректно применил blur)
    if (const LONG ex = GetWindowLongW(hwnd, GWL_EXSTYLE); ex & WS_EX_LAYERED) {
        SetWindowLongW(hwnd, GWL_EXSTYLE, ex & ~WS_EX_LAYERED);
    }

    // 2) Применяем DWM-скругления (если поддерживается) — Windows 11/10+.
    //    DWMWA_WINDOW_CORNER_PREFERENCE = 33 (значение ОС может меняться, но обычно 33)
    //    Включаем PREFERRED_ROUND (значение 2) — рекомендуемый вариант.
    constexpr DWORD DWMWA_WINDOW_CORNER_PREFERENCE = 33;
    constexpr int DWMWCP_ROUND = 2;
    const HRESULT hrCorner = DwmSetWindowAttribute(
        hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &DWMWCP_ROUND,
        sizeof(DWMWCP_ROUND)
    );
    // не фатально, но логируем при необходимости
    if (FAILED(hrCorner)) {
        // qDebug() << "DwmSetWindowAttribute corner failed:" << hrCorner;
    }

    // 3) Режим акрила через SetWindowCompositionAttribute
    const auto setWindowCompositionAttribute =
            reinterpret_cast<pSetWindowCompositionAttribute>(
                GetProcAddress(GetModuleHandleW(L"user32.dll"),
                               "SetWindowCompositionAttribute"));

    if (!setWindowCompositionAttribute) {
        qWarning() << "SetWindowCompositionAttribute not available.";
    } else {
        ACCENT_POLICY policy{};
        policy.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;

        // GradientColor: AARRGGBB: альфа в старшем байте
        // 0xA0 (~63%) — типичный уровень мутности Win11
        constexpr DWORD alpha = 0xA0;
        constexpr DWORD rgb = (0x20) | (0x20 << 8) | (0x20 << 16); // #202020
        policy.GradientColor = (alpha << 24) | rgb;
        policy.AccentFlags = 2; // 0 или 2 — без шумового слоя

        WINDOWCOMPOSITIONATTRIBDATA data{};
        data.Attribute = WCA_ACCENT_POLICY;
        data.Data = &policy;
        data.SizeOfData = sizeof(policy);

        if (const BOOL res = setWindowCompositionAttribute(hwnd, &data); !res) {
            qWarning() << "SetWindowCompositionAttribute returned false";
        }
    }

    // 4) Устанавливаем реальную форму окна — скруглённый регион.
    //    Это помогает, когда DWM не полностью учитывает QSS-радиус.
    // region должен быть пересоздан при изменении размера (см. resizeEvent)
    if (const HRGN hrgn = CreateRoundRectRgn(0, 0, width() + 1, height() + 1, WINDOW_RADIUS, WINDOW_RADIUS)) {
        // SetWindowRgn передаёт владение HRGN системе — не нужно DeleteObject после этого.
        SetWindowRgn(hwnd, hrgn, TRUE);
    }
}

void TrayManager::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);

    // При изменении размера пересоздаём регион с тем же радиусом
    const auto hwnd = reinterpret_cast<HWND>(winId());
    if (!hwnd) return;

    if (const HRGN r = CreateRoundRectRgn(0, 0, width() + 1, height() + 1, WINDOW_RADIUS, WINDOW_RADIUS)) {
        SetWindowRgn(hwnd, r, TRUE);
        // SetWindowRgn владеет регионом — не вызываем DeleteObject(r)
    }
}


void TrayManager::animateToggleButton() {
    QWidget *btn = ui.toggle_btn;
    auto *effect = new QGraphicsOpacityEffect(btn);
    btn->setGraphicsEffect(effect);

    auto *fadeOutBtn = new QPropertyAnimation(effect, "opacity");
    fadeOutBtn->setDuration(110);
    fadeOutBtn->setStartValue(1.0);
    fadeOutBtn->setEndValue(0.0);

    auto *fadeInBtn = new QPropertyAnimation(effect, "opacity");
    fadeInBtn->setDuration(140);
    fadeInBtn->setStartValue(0.0);
    fadeInBtn->setEndValue(1.0);

    connect(fadeOutBtn, &QPropertyAnimation::finished, [this, fadeInBtn]() {
        updateInfo();
        fadeInBtn->start(QAbstractAnimation::DeleteWhenStopped);
    });
    connect(fadeInBtn, &QPropertyAnimation::finished, [effect]() {
        effect->deleteLater();
    });

    fadeOutBtn->start(QAbstractAnimation::DeleteWhenStopped);
}

void TrayManager::showAtCursor() {
    updateInfo();
    resize(sizeHint());

    // позиция рядом с курсором
    const QPoint cursor = QCursor::pos();
    move(cursor.x() + 3, cursor.y() - height() - 3);

    // показываем окно невидимым сначала
    setWindowOpacity(0.0);
    setVisible(true);
    raise();
    activateWindow();
    setFocus(Qt::ActiveWindowFocusReason);

    // включаем акрил на следующем цикле событий
    static bool acrylicApplied = false;
    if (!acrylicApplied) {
        acrylicApplied = true;
        QTimer::singleShot(0, this, [this]() {
            enableAcrylic();
        });
    } else {
        // для повторных открытий, чтобы акрил не пропадал
        QTimer::singleShot(0, this, [this]() { enableAcrylic(); });
    }

    // плавное появление
    fadeIn->start();
}



void TrayManager::hideAnimated() const {
    if (!isVisible()) return;
    fadeOut->start();
}

bool TrayManager::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui.info_frame) {
        if (event->type() == QEvent::Enter) startHoverBrightening(ui.info_frame, true);
        else if (event->type() == QEvent::Leave) startHoverBrightening(ui.info_frame, false);
    }

    if (isVisible() && event->type() == QEvent::MouseButtonPress) {
        if (const auto *me = static_cast<QMouseEvent *>(event); !geometry().contains(me->globalPosition().toPoint()))
            hideAnimated();
    }

    return QWidget::eventFilter(obj, event);
}

void TrayManager::focusOutEvent(QFocusEvent *event) {
    hideAnimated();
    QWidget::focusOutEvent(event);
}

void TrayManager::startHoverBrightening(const QFrame *frame, const bool enter) {
    for (const auto labels = frame->findChildren<QLabel *>(); QLabel *lbl: labels) {
        auto *eff = qobject_cast<QGraphicsOpacityEffect *>(lbl->graphicsEffect());
        if (!eff) {
            eff = new QGraphicsOpacityEffect(lbl);
            lbl->setGraphicsEffect(eff);
        }
        auto *anim = new QPropertyAnimation(eff, "opacity");
        anim->setDuration(180);
        anim->setStartValue(eff->opacity());
        anim->setEndValue(enter ? 1.0 : 0.7);
        anim->setEasingCurve(QEasingCurve::InOutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
}
