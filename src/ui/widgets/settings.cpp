#include "settings.h"
#include "../../core/helpers/acrylicHelper.h"
#include <QGuiApplication>
#include <QTimer>
#include <QMouseEvent>
#include <QPushButton>
#include <QEvent>
#include <QDebug>

SettingsWindow::SettingsWindow(QWidget *parent)
    : QWidget(parent) {
    ui.setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    auto *closeAction = new QAction(this);
    closeAction->setShortcut(Qt::Key_Escape);
    connect(closeAction, &QAction::triggered, this, &SettingsWindow::close);
    addAction(closeAction);

    connect(ui.btn_close_bot_sider, &QPushButton::clicked, closeAction, &QAction::trigger);
}

SettingsWindow::~SettingsWindow() = default;

void SettingsWindow::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    // При показе включаем акрил (активное состояние)
    QTimer::singleShot(0, this, [this]() {
        AcrylicHelper::setAcrylicEnabled(this, true);
        AcrylicHelper::updateRegion(this); // пересчитать скругления/регион
    });
}

void SettingsWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton)
        return;

    QWidget *w = childAt(event->position().toPoint());

    if (w && (
            w->inherits("QPushButton")
            || w->inherits("QSlider")
            || w->inherits("QComboBox")
            || w->inherits("QTableWidget")
            || w->inherits("QLineEdit")
            || w->inherits("QSpinBox")
            || w->inherits("QAbstractItemView"))) {
        dragging = false;
        return;
    }

    dragging = true;
    dragStartPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
}

void SettingsWindow::mouseMoveEvent(QMouseEvent *event) {
    if (dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - dragStartPos);
    }
}

void SettingsWindow::mouseReleaseEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    dragging = false;
}

// Главное: ловим активацию/деактивацию окна через event()
bool SettingsWindow::event(QEvent *ev) {
    if (ev->type() == QEvent::WindowActivate) {
        // окно стало активным (пользователь переключился на него)
        QTimer::singleShot(0, this, [this]() {
            AcrylicHelper::setAcrylicEnabled(this, true);
            AcrylicHelper::updateRegion(this);
            // при необходимости установить полную прозрачность/цвета
        });
    } else if (ev->type() == QEvent::WindowDeactivate) {
        // окно потеряло активность — отключаем/ослабляем акрил
        QTimer::singleShot(0, this, [this]() {
            AcrylicHelper::setAcrylicEnabled(this, false);
            AcrylicHelper::updateRegion(this);
        });
    }
    return QWidget::event(ev);
}

void SettingsWindow::openCentered() {
    const QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen)
        screen = QGuiApplication::screens().first();

    QRect geom = screen->availableGeometry();

    QSize s = sizeHint();
    if (!s.isValid()) s = QSize(850, 500);

    int x = geom.center().x() - s.width() / 2;
    int y = geom.center().y() - s.height() / 2;

    resize(s);
    move(x, y);

    show();
    raise();
    activateWindow();
}
