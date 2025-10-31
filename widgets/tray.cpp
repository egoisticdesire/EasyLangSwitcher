#include "tray.h"
#include <QIcon>

TrayManager::TrayManager(SettingsWindow &settingsWindow, QObject *parent)
    : QObject(parent),
      enableAction("Enable", this),
      disableAction("Disable", this),
      settingsAction("Settings", this),
      exitAction("Exit", this),
      settingsWindow(settingsWindow) {
    trayIcon.setIcon(QIcon(":/resources/icons/a1.png"));

    trayMenu.addAction(&enableAction);
    trayMenu.addAction(&disableAction);
    trayMenu.addSeparator();
    trayMenu.addAction(&settingsAction);
    trayMenu.addAction(&exitAction);

    trayIcon.setContextMenu(&trayMenu);

    setupActions();
}

void TrayManager::setupActions() {
    connect(&settingsAction, &QAction::triggered, [&]() {
        settingsWindow.show();
    });


    connect(&exitAction, &QAction::triggered, [&]() {
        emit exitRequested();
    });
}

void TrayManager::show() {
    trayIcon.show();
}
