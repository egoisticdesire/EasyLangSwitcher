#include "mainBootstrapHelper.h"
#include "../core/config/appSettings.h"
#include "../core/config/logger.h"
#include "../ui/helpers/windowsToastIdentity.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <algorithm>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <KnownFolders.h>
#include <PropKey.h>
#include <Propvarutil.h>
#include <ShObjIdl_core.h>
#include <ShlObj_core.h>
#include <winreg.h>
#include <wrl/client.h>
#include <array>
#include <string>

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#endif

namespace {
#ifdef Q_OS_WIN
    QString currentExePath() {
        std::array<wchar_t, MAX_PATH> buffer{};
        const DWORD len = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (len == 0 || len >= buffer.size()) return {};
        return QString::fromWCharArray(buffer.data(), static_cast<int>(len));
    }

    QString appShortcutPath() {
        PWSTR rawProgramsPath = nullptr;
        if (const HRESULT hr = SHGetKnownFolderPath(FOLDERID_Programs, KF_FLAG_DEFAULT, nullptr, &rawProgramsPath);
            FAILED(hr) || !rawProgramsPath) {
            LOG_WARNING() << "Failed to get Start Menu Programs folder. HRESULT=0x"
                    << QString::number(static_cast<qulonglong>(hr), 16);
            return {};
        }

        const QString programsPath = QString::fromWCharArray(rawProgramsPath);
        CoTaskMemFree(rawProgramsPath);
        return QDir(programsPath).filePath(QString("%1.lnk").arg(AppSettings::APP_NAME));
    }

    QString toastLaunchArgument() {
        return QString("%1-toast-open-update").arg(QString(AppSettings::APP_NAME).toLower());
    }

    bool writeRegistryDefaultValue(const std::wstring &subKey, const std::wstring &value) {
        HKEY key = nullptr;
        const LONG createResult = RegCreateKeyExW(
            HKEY_CURRENT_USER,
            subKey.c_str(),
            0,
            nullptr,
            REG_OPTION_NON_VOLATILE,
            KEY_WRITE,
            nullptr,
            &key,
            nullptr
        );
        if (createResult != ERROR_SUCCESS) return false;

        const LONG setResult = RegSetValueExW(
            key,
            nullptr,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE *>(value.c_str()),
            static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t))
        );
        RegCloseKey(key);
        return setResult == ERROR_SUCCESS;
    }

    bool writeRegistryNamedValue(const std::wstring &subKey, const std::wstring &name, const std::wstring &value) {
        HKEY key = nullptr;
        const LONG createResult = RegCreateKeyExW(
            HKEY_CURRENT_USER,
            subKey.c_str(),
            0,
            nullptr,
            REG_OPTION_NON_VOLATILE,
            KEY_WRITE,
            nullptr,
            &key,
            nullptr
        );
        if (createResult != ERROR_SUCCESS) return false;

        const LONG setResult = RegSetValueExW(
            key,
            name.c_str(),
            0,
            REG_SZ,
            reinterpret_cast<const BYTE *>(value.c_str()),
            static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t))
        );
        RegCloseKey(key);
        return setResult == ERROR_SUCCESS;
    }
#else
    QString toastLaunchArgument() {
        return {};
    }
#endif
}

bool MainBootstrapHelper::ensureToastShortcut(const wchar_t *appUserModelId) {
#ifdef Q_OS_WIN
    const QString exePath = currentExePath();
    const QString shortcutPath = appShortcutPath();
    if (exePath.isEmpty() || shortcutPath.isEmpty()) return false;

    const HRESULT coInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool shouldUninit = SUCCEEDED(coInit);
    if (FAILED(coInit) && coInit != RPC_E_CHANGED_MODE) {
        LOG_WARNING() << "CoInitializeEx failed for shortcut creation. HRESULT=0x"
                << QString::number(static_cast<qulonglong>(coInit), 16);
        return false;
    }

    Microsoft::WRL::ComPtr<IShellLinkW> shellLink;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));
    if (FAILED(hr) || !shellLink) {
        LOG_WARNING() << "CoCreateInstance(CLSID_ShellLink) failed. HRESULT=0x"
                << QString::number(static_cast<qulonglong>(hr), 16);
        if (shouldUninit) CoUninitialize();
        return false;
    }

    const std::wstring exeWide = QDir::toNativeSeparators(exePath).toStdWString();
    hr = shellLink->SetPath(exeWide.c_str());
    if (SUCCEEDED(hr))
        hr = shellLink->SetWorkingDirectory(QFileInfo(exePath).absolutePath().toStdWString().c_str());
    if (SUCCEEDED(hr)) hr = shellLink->SetIconLocation(exeWide.c_str(), 0);

    Microsoft::WRL::ComPtr<IPropertyStore> propertyStore;
    if (SUCCEEDED(hr)) hr = shellLink.As(&propertyStore);

    PROPVARIANT appIdProp{};
    if (SUCCEEDED(hr)) hr = InitPropVariantFromString(appUserModelId, &appIdProp);
    if (SUCCEEDED(hr) && propertyStore) hr = propertyStore->SetValue(PKEY_AppUserModel_ID, appIdProp);
    if (SUCCEEDED(hr) && propertyStore) hr = propertyStore->Commit();
    if (const HRESULT clearHr = PropVariantClear(&appIdProp); FAILED(clearHr)) {
        LOG_WARNING() << "PropVariantClear failed. HRESULT=0x"
                << QString::number(static_cast<qulonglong>(clearHr), 16);
    }

    Microsoft::WRL::ComPtr<IPersistFile> persistFile;
    if (SUCCEEDED(hr)) hr = shellLink.As(&persistFile);
    if (SUCCEEDED(hr) && persistFile)
        hr = persistFile->Save(QDir::toNativeSeparators(shortcutPath).toStdWString().c_str(), TRUE);

    if (shouldUninit) CoUninitialize();

    if (FAILED(hr)) {
        LOG_WARNING() << "Failed to create toast shortcut. HRESULT=0x"
                << QString::number(static_cast<qulonglong>(hr), 16);
        return false;
    }

    LOG_INFO() << "Created Start Menu shortcut for toast notifications at " << shortcutPath;
    return true;
#else
    Q_UNUSED(appUserModelId);
    return false;
#endif
}

void MainBootstrapHelper::ensureProtocolRegistration() {
#ifdef Q_OS_WIN
    const QString exePath = currentExePath();
    if (exePath.isEmpty()) return;

    const QString scheme = WindowsToastIdentity::toastProtocolScheme();
    const std::wstring baseKey = QString("Software\\Classes\\%1").arg(scheme).toStdWString();
    const std::wstring command =
            QString("\"%1\" \"%2\"").arg(QDir::toNativeSeparators(exePath), "%1").toStdWString();

    bool ok = true;
    ok = writeRegistryDefaultValue(baseKey, QString("URL:%1 protocol").arg(AppSettings::APP_NAME).toStdWString()) &&
         ok;
    ok = writeRegistryNamedValue(baseKey, L"URL Protocol", L"") && ok;
    ok = writeRegistryDefaultValue(baseKey + L"\\DefaultIcon",
                                   QDir::toNativeSeparators(exePath).toStdWString() + L",0") && ok;
    ok = writeRegistryDefaultValue(baseKey + L"\\shell\\open\\command", command) && ok;

    if (!ok) {
        LOG_WARNING() << "Failed to register protocol handler for toast activation";
    }
#endif
}

QStringList MainBootstrapHelper::startupArgsFromArgv(const int argc, char *argv[]) {
    QStringList args;
    args.reserve(qMax(0, argc - 1));
    for (int i = 1; i < argc; ++i) {
        args.push_back(QString::fromLocal8Bit(argv[i]));
    }
    return args;
}

QString MainBootstrapHelper::instanceLockPath() {
    QString lockDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (lockDir.isEmpty()) lockDir = QDir::tempPath();
    return QDir(lockDir).filePath(
        QString("%1.single_instance.lock").arg(QString(AppSettings::APP_NAME).toLower())
    );
}

bool MainBootstrapHelper::isToastActivationLaunch(const QStringList &startupArgs) {
    const QString toastArg = toastLaunchArgument();
    const QString toastUri = WindowsToastIdentity::toastProtocolUri();
    return std::ranges::any_of(
        startupArgs, [&toastArg, &toastUri](const QString &arg) {
            const bool hasCustomArg = !toastArg.isEmpty() && arg.contains(toastArg, Qt::CaseInsensitive);
            const bool hasProtocolArg = !toastUri.isEmpty() && arg.contains(toastUri, Qt::CaseInsensitive);
            const bool hasDefaultToastArg = arg.compare("-ToastActivated", Qt::CaseInsensitive) == 0
                                            || arg.compare("/ToastActivated", Qt::CaseInsensitive) == 0
                                            || arg.compare("ToastActivated", Qt::CaseInsensitive) == 0;
            return hasCustomArg || hasDefaultToastArg || hasProtocolArg;
        }
    );
}
