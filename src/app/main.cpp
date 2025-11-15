#include "../core/handlers/kb.h"
#include "../ui/tray/tray.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    const QApplication app(argc, argv);

    // Трей-менеджер
    TrayManager tray;
    tray.show();

    // Хэндлер клавиатуры
    KeyboardHandler kbHandler;
    kbHandler.start();

    QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
        kbHandler.stop();
    });
    QObject::connect(&tray, &TrayManager::exitRequested, [&]() {
        QApplication::quit();
    });
    QObject::connect(&tray, &TrayManager::keyboardToggled, [&](const bool enabled) {
        kbHandler.setActive(enabled);
    });

    return QApplication::exec();
}
