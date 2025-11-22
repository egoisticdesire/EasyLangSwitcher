#include "../core/handlers/kb.h"
#include "../ui/helpers/iconHelper.h"
#include "../ui/tray/tray.h"
#include "../core/config/app_settings.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[]) {
    const QApplication app(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false);
    QApplication::setWindowIcon(IconHelper::loadIcon(":/icons/icons/FlashSparkleFilled2.png"));

    AppSettings::load();
    qDebug() << "main: loaded settings -> hotkey:" << AppSettings::hotkeyName << "vk=" << AppSettings::hotkeyMainVk;

    TrayManager tray;
    tray.show();

    KeyboardHandler kbHandler;
    kbHandler.start();
    qDebug() << "main: KeyboardHandler started.";

    QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
        kbHandler.stop();
        qDebug() << "main: KeyboardHandler stopped on exit.";
    });
    QObject::connect(&tray, &TrayManager::exitRequested, [&]() {
        QApplication::quit();
    });
    QObject::connect(&tray, &TrayManager::keyboardToggled, [&](const bool enabled) {
        kbHandler.setActive(enabled);
    });

    return QApplication::exec();
}
