#pragma once
#include "ui_EasyLangSwitcher_tray.h"
#include "../widgets/settings.h"
#include <QSystemTrayIcon>
#include <QPropertyAnimation>

class TrayManager final : public QWidget {
    Q_OBJECT

public:
    explicit TrayManager(QWidget *parent = nullptr);

    void showAtCursor();

signals:
    void exitRequested();

    void keyboardToggled(bool enabled);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

    void focusOutEvent(QFocusEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::main_frame ui{};
    QSystemTrayIcon trayIcon;
    SettingsWindow *settingsWindow = nullptr;
    bool enabled = true;
    QPropertyAnimation *fadeIn = nullptr;
    QPropertyAnimation *fadeOut = nullptr;

    void updateTrayIcon();

    void setupTrayIcon();

    void setupUiBehavior();

    void updateInfo() const;

    void animateToggleButton();

    void hideAnimated() const;
};
