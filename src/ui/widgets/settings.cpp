#include "settings.h"
#include <QGuiApplication>
#include <QTimer>
#include <QScreen>
#include <QAction>
#include <QPushButton>

SettingsWindow::SettingsWindow(QWidget *parent)
    : QWidget(parent) {
    ui.setupUi(this);

    // системные флаги окна
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::ClickFocus);

    // Esc → закрытие
    auto *closeAction = new QAction(this);
    closeAction->setShortcut(Qt::Key_Escape);
    connect(closeAction, &QAction::triggered, this, &SettingsWindow::close);
    addAction(closeAction);

    connect(ui.btn_close_bot_sider, &QPushButton::clicked,
            closeAction, &QAction::trigger);

    // Добавляем анимированные индикаторы
    addSelectorForFrame(ui.key_select_frame);
    addSelectorForFrame(ui.app_startup_frame);
    addSelectorForFrame(ui.app_theme_frame);
    addSelectorForFrame(ui.app_lang_frame);

    // Drag по пустой области
    // можно указать, что кнопка закрытия — интерактивная
    dragger = new WindowDragger(this);
    dragger->addIgnoredWidget(ui.btn_close_bot_sider);
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

    // включить акрил при показе
    QTimer::singleShot(0, this, [this]() {
        AcrylicHelper::setAcrylicEnabled(this, true);
        AcrylicHelper::updateRegion(this);
    });

    // корректно разместить индикатор
    QTimer::singleShot(0, this, [this]() {
        for (const AnimatedSelector *sel: selectors) {
            if (sel) sel->initPosition();
        }
    });
}


bool SettingsWindow::event(QEvent *ev) {
    // акрил при активации/деактивации окна
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
    if (!screen)
        screen = QGuiApplication::screens().first();

    const QRect geom = screen->availableGeometry();

    QSize s = sizeHint();
    if (!s.isValid())
        s = QSize(850, 500);

    const int x = geom.center().x() - s.width() / 2;
    const int y = geom.center().y() - s.height() / 2;

    resize(s);
    move(x, y);

    show();
    raise();
    activateWindow();
}
