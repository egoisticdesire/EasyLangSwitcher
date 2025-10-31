#pragma once
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include "settings.h"

class TrayManager final : public QObject {
    Q_OBJECT

public:
    explicit TrayManager(SettingsWindow &settingsWindow, QObject *parent = nullptr);

    void show();

signals:
    void exitRequested();

private:
    QSystemTrayIcon trayIcon;
    QMenu trayMenu;
    QAction enableAction;
    QAction disableAction;
    QAction settingsAction;
    QAction exitAction;

    SettingsWindow &settingsWindow;

    void setupActions();
};
