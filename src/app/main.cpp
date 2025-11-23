#include "../core/config/logger.h"
#include "../core/handlers/kb.h"
#include "../ui/helpers/iconHelper.h"
#include "../ui/helpers/warning.h"
#include "../ui/tray/tray.h"
#include "../core/config/app_settings.h"
#include <QApplication>
#include <Windows.h>
#include <fcntl.h>
#include <io.h>

int main(int argc, char *argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U16TEXT);

    const QApplication app(argc, argv);

    // Проверка на уже запущенный экземпляр
    const HANDLE hMutex = CreateMutex(nullptr, TRUE, L"MyUniqueFlashSparkleMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        WarningDialog dlg;
        dlg.setText("EasyLangSwitcher is already running!");
        dlg.openCentered();
        dlg.exec();
        return 0;
    }

    Logger::_debug = true;
    LOG_INFO() << "Logger initialized with level: "
            << (Logger::_debug ? "DEBUG" : "INFO");

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

    const int result = QApplication::exec();

    // Освобождаем мьютекс при выходе
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

    return result;
}
