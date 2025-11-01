#include "handlers/keyboard.h"
#include "widgets/settings.h"
#include "widgets/tray.h"

int main(int argc, char *argv[]) {
    const QApplication app(argc, argv);

    SettingsData settings;
    SettingsWindow settingsWindow(settings);

    // Трей-менеджер
    TrayManager tray(settingsWindow);
    tray.show();

    // Хэндлер клавиатуры
    KeyboardHandler kbHandler(settings);
    kbHandler.start();

    QObject::connect(&app, &QApplication::aboutToQuit, [&]() { kbHandler.stop(); });
    QObject::connect(&tray, &TrayManager::exitRequested, [&]() { QApplication::quit(); });
    // привязываем включение/выключение клавиши
    QObject::connect(&tray, &TrayManager::keyboardToggled, [&](bool enabled) {
        kbHandler.setActive(enabled);
    });

    return QApplication::exec();
}
