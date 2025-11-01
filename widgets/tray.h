#pragma once
#include <QSystemTrayIcon>
#include "../resources/ui_EasyLangSwitcher.h"
#include "../helpers/svg.h"
#include "../helpers/acrylicEffect.h"
#include "../helpers/hoverEffect.h"
#include "settings.h"

class TrayManager final : public QWidget {
    Q_OBJECT

public:
    explicit TrayManager(SettingsWindow &settingsWindow, QWidget *parent = nullptr);

    void showAtCursor();

signals:
    void exitRequested();
    void keyboardToggled(bool enabled);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

    void focusOutEvent(QFocusEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;

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

    void animateToggleButton();

    void hideAnimated() const;
};
