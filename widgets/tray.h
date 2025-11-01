#pragma once

#include <QWidget>
#include <QSystemTrayIcon>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QSvgRenderer>
#include <QPainter>
#include "settings.h"
#include "../resources/ui_EasyLangSwitcher.h"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

class TrayManager final : public QWidget
{
    Q_OBJECT
public:
    explicit TrayManager(SettingsWindow &settingsWindow, QWidget *parent = nullptr);
    void showAtCursor();

    signals:
        void exitRequested();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    Ui::main_frame ui;
    SettingsWindow &settingsWindow;
    QSystemTrayIcon trayIcon;

    bool enabled = true;
    QPropertyAnimation *fadeIn = nullptr;
    QPropertyAnimation *fadeOut = nullptr;

    void setupTrayIcon();
    void setupUiBehavior();
    void updateInfo() const;
    void enableAcrylic();
    void animateToggleButton();
    void hideAnimated() const;

    static void startHoverBrightening(const QFrame *frame, bool enter);

    static QIcon loadSvgIcon(const QString &path, const QSize &size = QSize(22,22));
};
