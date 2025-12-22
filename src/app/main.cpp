#include "../core/config/logger.h"
#include "../core/config/loggerQtBridge.h"
#include "../core/handlers/kb.h"
#include "../ui/helpers/fontHelper.h"
#include "../ui/helpers/iconHelper.h"
#include "../ui/helpers/warningHelper.h"
#include "../ui/tray/tray.h"
#include "../core/config/appSettings.h"
#include "../core/i18n/lang.h"
#include <QApplication>
#include <Windows.h>
#include <fcntl.h>
#include <io.h>

#ifndef APP_VERSION
#define APP_VERSION "1.0.0"
#endif

int main(int argc, char *argv[]) {
    Logger::_debug = false;

    QtLoggerBridge::install();
    LOG_INFO() << "Logger initialized with level: " << (Logger::_debug ? "DEBUG" : "INFO");

    SetConsoleOutputCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U16TEXT);

    QApplication app(argc, argv);
    QApplication::setStyle("Windows11");

    app.setApplicationVersion(APP_VERSION);
    app.setOrganizationName("e.d");
    app.setApplicationName(AppSettings::APP_NAME);

    FontManager::init(app);

    AppSettings::load();
    // для тестов
    // AppSettings::lastUpdateCheckDate = QDate::currentDate().addDays(-2);

    // Проверка на уже запущенный экземпляр
    const QString mutexName = QString("MyUnique%1Mutex").arg(AppSettings::APP_NAME);
    const HANDLE hMutex = CreateMutex(nullptr, TRUE, mutexName.toStdWString().c_str());
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        WarningDialog dlg;
        dlg.setTranslations(Lang::tr("SETTINGS_SIDER_MENU_CLOSE"));
        dlg.setText(Lang::tr("SECOND_INSTANCE_ERROR").arg(AppSettings::APP_NAME));
        dlg.openCentered();
        dlg.exec();
        return 0;
    }

    QApplication::setQuitOnLastWindowClosed(false);
    QApplication::setWindowIcon(IconHelper::loadIcon(":/icons/icons/FlashSparkleFilled2.png"));

    TrayManager tray;
    tray.show();

    KeyboardHandler kbHandler;
    kbHandler.start();

    QObject::connect(&app, &QApplication::aboutToQuit, [&] {
        kbHandler.stop();
    });
    QObject::connect(&tray, &TrayManager::exitRequested, [&] {
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
