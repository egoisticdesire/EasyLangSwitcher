#include <QTimer>

#include "../core/config/appSettings.h"
#include "../core/config/logger.h"
#include "../core/config/loggerQtBridge.h"
#include "../core/handlers/kb.h"
#include "../core/i18n/lang.h"
#include "../ui/helpers/fontHelper.h"
#include "../ui/helpers/iconHelper.h"
#include "../ui/helpers/warningHelper.h"
#include "../ui/helpers/windowsToastIdentity.h"
#include "../ui/tray/tray.h"
#include "mainBootstrapHelper.h"

#include <QApplication>
#include <QDate>
#include <QLockFile>
#include <ShObjIdl_core.h>
#include <Windows.h>
#include <fcntl.h>
#include <io.h>
#include <exception>
#include <memory>
#include <span>
#include <thread>

#pragma comment(lib, "Shell32.lib")

#ifndef APP_VERSION
#define APP_VERSION "1.0.0"
#endif

namespace MainRuntimeFlags
{
constexpr bool kDebugLogsEnabled = false;

constexpr bool kEnableUpdateCheckTestOverride = false;
constexpr auto kForcedAppVersion = "1.1.1";
constexpr int kLastUpdateShiftDays = -7;
constexpr int kAsyncBootstrapDelayMs = 1500;
} // namespace MainRuntimeFlags

void applyUpdateCheckTestOverrideAfterSettingsLoad()
{
    // ReSharper disable once CppDFAUnreachableCode
    if (!MainRuntimeFlags::kEnableUpdateCheckTestOverride) {
        return;
    }
    // ReSharper disable once CppDFAUnreachableCode
    QApplication::setApplicationVersion(MainRuntimeFlags::kForcedAppVersion);
    AppSettings::lastUpdateCheckDate() = QDate::currentDate().addDays(MainRuntimeFlags::kLastUpdateShiftDays);
}

namespace
{
void runWindowsBootstrapTasks(const std::wstring& appUserModelId) noexcept
{
    try {
        MainBootstrapHelper::ensureToastShortcut(appUserModelId.c_str());
        MainBootstrapHelper::ensureProtocolRegistration();
    }
    catch (const std::exception& ex) {
        LOG_ERROR() << "Async bootstrap failed: " << ex.what();
    }
    catch (...) {
        LOG_ERROR() << "Async bootstrap failed with unknown exception";
    }
}
} // namespace

void runWindowsBootstrapTasksAsync()
{
#ifdef Q_OS_WIN
    const auto appUserModelId = std::make_shared<std::wstring>(WindowsToastIdentity::appUserModelIdWide());
    std::thread([appUserModelId]() noexcept { runWindowsBootstrapTasks(*appUserModelId); }).detach();
#endif
}

void releaseMutexIfNeeded(const HANDLE hMutex, const bool mutexCreated, const bool mutexAlreadyExists)
{
    if (!mutexCreated) {
        return;
    }
    if (!mutexAlreadyExists) {
        ReleaseMutex(hMutex);
    }
    CloseHandle(hMutex);
}

void forwardToastActivationToRunningInstance()
{
#ifdef Q_OS_WIN
    const std::wstring eventName = WindowsToastIdentity::toastActivationEventNameWide();
    if (const HANDLE activationEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, eventName.c_str())) {
        SetEvent(activationEvent);
        CloseHandle(activationEvent);
        LOG_INFO() << "Forwarded toast activation to running instance";
    }
    else {
        LOG_WARNING() << "Failed to open toast activation event. GetLastError="
                      << static_cast<qulonglong>(GetLastError());
    }
#endif
}

int handleExistingInstance(const bool hasRunningInstance,
                           const bool isToastLaunch,
                           int& argc,
                           const std::span<char*> argv,
                           const HANDLE hMutex,
                           const bool mutexCreated,
                           const bool mutexAlreadyExists)
{
    if (!hasRunningInstance) {
        return -1;
    }

    if (isToastLaunch) {
        forwardToastActivationToRunningInstance();
        releaseMutexIfNeeded(hMutex, mutexCreated, mutexAlreadyExists);
        return 0;
    }

    const QApplication app(argc, argv.data());
    QApplication::setStyle("Windows11");
    FontManager::init(app);
    AppSettings::load();

    WarningDialog dlg;
    dlg.setTranslations(Lang::tr("SETTINGS_SIDER_MENU_CLOSE"));
    dlg.setText(Lang::tr("SECOND_INSTANCE_ERROR").arg(AppSettings::APP_NAME));
    dlg.openCentered();
    dlg.exec();

    releaseMutexIfNeeded(hMutex, mutexCreated, mutexAlreadyExists);
    return 0;
}

int main(int argc, char* argv[])
{
    try {
        Logger::_debug = MainRuntimeFlags::kDebugLogsEnabled;

        QtLoggerBridge::install();
        LOG_INFO() << "Logger initialized with level: " << (Logger::_debug ? "DEBUG" : "INFO");

        SetConsoleOutputCP(CP_UTF8);
        _setmode(_fileno(stdout), _O_U16TEXT);

        const QStringList startupArgs = MainBootstrapHelper::startupArgsFromArgv(argc, argv);
        LOG_INFO() << "Startup arguments (pre-Qt): " << startupArgs.join(" | ");
        const bool isToastLaunch = MainBootstrapHelper::isToastActivationLaunch(startupArgs);

        auto processLock = std::make_unique<QLockFile>(MainBootstrapHelper::instanceLockPath());
        processLock->setStaleLockTime(0);
        const bool lockAcquired = processLock->tryLock(0);

        const QString mutexName = QString("MyUnique%1Mutex").arg(AppSettings::APP_NAME);
        auto* const hMutex = CreateMutex(nullptr, TRUE, mutexName.toStdWString().c_str());
        const bool mutexCreated = hMutex != nullptr;
        const bool mutexAlreadyExists = mutexCreated && GetLastError() == ERROR_ALREADY_EXISTS;
        const bool hasRunningInstance = mutexAlreadyExists || !lockAcquired;

        if (!mutexCreated) {
            LOG_WARNING() << "CreateMutex failed. GetLastError=" << static_cast<qulonglong>(GetLastError());
        }

        const auto argvSpan = std::span<char*>(argv, static_cast<std::size_t>(argc));

        if (const int existingResult = handleExistingInstance(
                    hasRunningInstance, isToastLaunch, argc, argvSpan, hMutex, mutexCreated, mutexAlreadyExists);
            existingResult >= 0) {
            return existingResult;
        }

        const std::wstring appUserModelId = WindowsToastIdentity::appUserModelIdWide();
        if (const HRESULT appIdResult = SetCurrentProcessExplicitAppUserModelID(appUserModelId.c_str());
            FAILED(appIdResult)) {
            LOG_WARNING() << "SetCurrentProcessExplicitAppUserModelID failed. HRESULT=0x"
                          << QString::number(static_cast<qulonglong>(appIdResult), 16);
        }
        else {
            LOG_INFO() << "SetCurrentProcessExplicitAppUserModelID result=0x"
                       << QString::number(static_cast<qulonglong>(appIdResult), 16);
        }

        QApplication app(argc, argv);
        QApplication::setStyle("Windows11");
        QApplication::setApplicationVersion(APP_VERSION);

        FontManager::init(app);

        AppSettings::load();

        applyUpdateCheckTestOverrideAfterSettingsLoad();

        QApplication::setQuitOnLastWindowClosed(false);
        QApplication::setWindowIcon(IconHelper::loadIcon(":/icons/icons/FlashSparkleFilled2.png"));

        TrayManager tray;

        QTimer::singleShot(MainRuntimeFlags::kAsyncBootstrapDelayMs, &app, [] { runWindowsBootstrapTasksAsync(); });

        if (isToastLaunch) {
            QTimer::singleShot(0, &tray, [&tray]() { tray.handleToastActivationRequest(); });
        }

        KeyboardHandler kbHandler;
        QTimer::singleShot(0, &app, [&kbHandler]() { kbHandler.start(); });

        QObject::connect(&app, &QApplication::aboutToQuit, [&] { kbHandler.stop(); });
        QObject::connect(&tray, &TrayManager::exitRequested, [&] { QApplication::quit(); });
        QObject::connect(
                &tray, &TrayManager::keyboardToggled, [&](const bool enabled) { kbHandler.setActive(enabled); });

        const int result = QApplication::exec();

        // Освобождаем мьютекс при выходе
        releaseMutexIfNeeded(hMutex, mutexCreated, mutexAlreadyExists);

        return result;
    }
    catch (const std::exception& ex) {
        LOG_ERROR() << "Fatal error: " << ex.what();
    }
    catch (...) {
        LOG_ERROR() << "Fatal error: unknown exception";
    }
    return 1;
}
