#include "../core/config/logger.h"
#include "../core/handlers/kb.h"
#include "../ui/helpers/iconHelper.h"
#include "../ui/tray/tray.h"
#include "../core/config/app_settings.h"
#include <QApplication>
#include <Windows.h>
#include <fcntl.h>
#include <io.h>

int main(int argc, char *argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U16TEXT);

    Logger::_debug = true;
    LOG_INFO() << "Logger initialized with level: "
            << (Logger::_debug ? "DEBUG" : "INFO");

    const QApplication app(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false);
    QApplication::setWindowIcon(IconHelper::loadIcon(":/icons/icons/FlashSparkleFilled2.png"));

    AppSettings::load();

    TrayManager tray;
    tray.show();

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
