#include "windowsNotificationState.h"

#ifdef Q_OS_WIN
#include "../../core/config/appSettings.h"
#include <vector>
#include <windows.h>
#include <shellapi.h>
#include <roapi.h>
#include <windows.foundation.h>
#include <windows.ui.notifications.h>
#include <windows.ui.shell.h>
#include <wrl.h>
#include <wrl/wrappers/corewrappers.h>

#pragma comment(lib, "runtimeobject.lib")

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::UI::Shell;

namespace {
    enum class QuietHoursProfile {
        Unknown,
        Unrestricted,
        PriorityOnly,
        AlarmsOnly,
        Other,
    };

    bool ensureRuntimeInitialized() {
        static bool initialized = false;
        static HRESULT initResult = E_FAIL;

        if (!initialized) {
            initResult = RoInitialize(RO_INIT_MULTITHREADED);
            if (initResult == RPC_E_CHANGED_MODE) {
                initResult = RoInitialize(RO_INIT_SINGLETHREADED);
            }
            initialized = true;
        }

        return SUCCEEDED(initResult);
    }

    QString notificationStateName(const QUERY_USER_NOTIFICATION_STATE state) {
        switch (state) {
            case QUNS_NOT_PRESENT: return "not_present";
            case QUNS_BUSY: return "busy";
            case QUNS_RUNNING_D3D_FULL_SCREEN: return "running_d3d_full_screen";
            case QUNS_PRESENTATION_MODE: return "presentation_mode";
            case QUNS_ACCEPTS_NOTIFICATIONS: return "accepts_notifications";
            case QUNS_QUIET_TIME: return "quiet_time";
            case QUNS_APP: return "app";
            default: return "unknown";
        }
    }

    QString toastNotificationSettingName(const NotificationSetting setting) {
        switch (setting) {
            case NotificationSetting_Enabled: return "enabled";
            case NotificationSetting_DisabledForApplication: return "disabled_for_application";
            case NotificationSetting_DisabledForUser: return "disabled_for_user";
            case NotificationSetting_DisabledByGroupPolicy: return "disabled_by_group_policy";
            case NotificationSetting_DisabledByManifest: return "disabled_by_manifest";
            default: return "unknown";
        }
    }

    QString quietHoursProfileName(const QuietHoursProfile profile) {
        switch (profile) {
            case QuietHoursProfile::Unrestricted: return "unrestricted";
            case QuietHoursProfile::PriorityOnly: return "priority_only";
            case QuietHoursProfile::AlarmsOnly: return "alarms_only";
            case QuietHoursProfile::Other: return "other";
            case QuietHoursProfile::Unknown:
            default: return "unknown";
        }
    }

    bool readRegistryDwordValue(
        const HKEY rootKey,
        const wchar_t *subKey,
        const wchar_t *valueName,
        DWORD &value
    ) {
        DWORD data = 0;
        DWORD dataSize = sizeof(data);
        const LSTATUS status = RegGetValueW(
            rootKey,
            subKey,
            valueName,
            RRF_RT_REG_DWORD,
            nullptr,
            &data,
            &dataSize
        );
        if (status != ERROR_SUCCESS) return false;
        value = data;
        return true;
    }

    bool readRegistryBinaryValue(
        const HKEY rootKey,
        const wchar_t *subKey,
        const wchar_t *valueName,
        std::vector<BYTE> &value
    ) {
        DWORD dataSize = 0;
        LSTATUS status = RegGetValueW(
            rootKey,
            subKey,
            valueName,
            RRF_RT_REG_BINARY,
            nullptr,
            nullptr,
            &dataSize
        );
        if (status != ERROR_SUCCESS || dataSize == 0) return false;

        std::vector<BYTE> data(dataSize);
        status = RegGetValueW(
            rootKey,
            subKey,
            valueName,
            RRF_RT_REG_BINARY,
            nullptr,
            data.data(),
            &dataSize
        );
        if (status != ERROR_SUCCESS) return false;

        data.resize(dataSize);
        value = data;
        return true;
    }

    bool isForegroundFullscreenWindow() {
        const HWND hwnd = GetForegroundWindow();
        if (!hwnd || IsIconic(hwnd)) return false;

        RECT windowRect{};
        if (!GetWindowRect(hwnd, &windowRect)) return false;

        const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
        if (!monitor) return false;

        MONITORINFO monitorInfo{};
        monitorInfo.cbSize = sizeof(monitorInfo);
        if (!GetMonitorInfoW(monitor, &monitorInfo)) return false;

        const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        if (const bool framedWindow = (style & (WS_CAPTION | WS_THICKFRAME)) != 0; framedWindow && IsZoomed(hwnd))
            return false;

        return windowRect.left <= monitorInfo.rcMonitor.left
               && windowRect.top <= monitorInfo.rcMonitor.top
               && windowRect.right >= monitorInfo.rcMonitor.right
               && windowRect.bottom >= monitorInfo.rcMonitor.bottom;
    }

    QuietHoursProfile parseQuietHoursProfilePayload(const QString &payload) {
        if (payload.contains("Microsoft.QuietHoursProfile.AlarmsOnly", Qt::CaseInsensitive))
            return QuietHoursProfile::AlarmsOnly;
        if (payload.contains("Microsoft.QuietHoursProfile.PriorityOnly", Qt::CaseInsensitive))
            return QuietHoursProfile::PriorityOnly;
        if (payload.contains("Microsoft.QuietHoursProfile.Unrestricted", Qt::CaseInsensitive))
            return QuietHoursProfile::Unrestricted;
        if (payload.contains("Microsoft.QuietHoursProfile.", Qt::CaseInsensitive))
            return QuietHoursProfile::Other;
        return QuietHoursProfile::Unknown;
    }

    QString extractPrintablePayload(const std::vector<BYTE> &data) {
        QString payload;
        payload.reserve(static_cast<qsizetype>(data.size()));
        for (const BYTE byte: data) {
            if (byte == 0) continue;
            if (byte >= 32 && byte <= 126) {
                payload.append(QChar::fromLatin1(static_cast<char>(byte)));
            } else {
                payload.append(u' ');
            }
        }
        return payload;
    }

    QuietHoursProfile parseQuietHoursProfileBlob(const std::vector<BYTE> &data) {
        if (data.empty()) return QuietHoursProfile::Unknown;
        return parseQuietHoursProfilePayload(extractPrintablePayload(data));
    }

    QuietHoursProfile detectQuietHoursProfileFromRegistry(const wchar_t *subKey) {
        static constexpr wchar_t kDataValue[] = L"Data";

        std::vector<BYTE> rawData;
        if (!readRegistryBinaryValue(HKEY_CURRENT_USER, subKey, kDataValue, rawData)) {
            return QuietHoursProfile::Unknown;
        }
        return parseQuietHoursProfileBlob(rawData);
    }

    QuietHoursProfile detectQuietHoursProfileFromCloudStore() {
        static constexpr wchar_t kCurrentPath[] =
                L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CloudStore\\Store\\DefaultAccount\\Current\\default$windows.data.donotdisturb.quiethourssettings\\windows.data.donotdisturb.quiethourssettings";
        static constexpr wchar_t kLegacyCachePath[] =
                L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CloudStore\\Store\\Cache\\DefaultAccount\\$$windows.data.notifications.quiethourssettings\\Current";

        if (const QuietHoursProfile currentProfile = detectQuietHoursProfileFromRegistry(kCurrentPath);
            currentProfile != QuietHoursProfile::Unknown)
            return currentProfile;

        return detectQuietHoursProfileFromRegistry(kLegacyCachePath);
    }

    bool isQuietHoursServiceActive() {
        DWORD state = 0;
        if (!readRegistryDwordValue(
            HKEY_CURRENT_USER,
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Notifications\\QuietHours",
            L"QuietHoursServiceState",
            state
        )) {
            return false;
        }
        return state != 0;
    }

    bool areGlobalToastsDisabled() {
        DWORD enabled = 1;
        if (!readRegistryDwordValue(
            HKEY_CURRENT_USER,
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PushNotifications",
            L"ToastEnabled",
            enabled
        )) {
            return false;
        }
        return enabled == 0;
    }

    bool isFocusSessionActive() {
        if (!ensureRuntimeInitialized()) return false;

        ComPtr<IFocusSessionManagerStatics> focusManagerStatics;
        HRESULT hr = RoGetActivationFactory(
            HStringReference(RuntimeClass_Windows_UI_Shell_FocusSessionManager).Get(),
            IID_PPV_ARGS(&focusManagerStatics)
        );
        if (FAILED(hr) || !focusManagerStatics) return false;

        boolean isSupported = false;
        hr = focusManagerStatics->get_IsSupported(&isSupported);
        if (FAILED(hr) || !isSupported) return false;

        ComPtr<IFocusSessionManager> focusManager;
        hr = focusManagerStatics->GetDefault(&focusManager);
        if (FAILED(hr) || !focusManager) return false;

        boolean isActive = false;
        hr = focusManager->get_IsFocusActive(&isActive);
        if (FAILED(hr)) return false;
        return isActive;
    }

    NotificationSetting queryToastNotificationSetting() {
        if (!ensureRuntimeInitialized()) return NotificationSetting_Enabled;

        ComPtr<IToastNotificationManagerStatics> toastManager;
        HRESULT hr = RoGetActivationFactory(
            HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),
            IID_PPV_ARGS(&toastManager)
        );
        if (FAILED(hr) || !toastManager) return NotificationSetting_Enabled;

        const std::wstring appId = QString("%1.Desktop").arg(AppSettings::APP_NAME).toStdWString();
        if (appId.empty()) return NotificationSetting_Enabled;

        ComPtr<IToastNotifier> notifier;
        hr = toastManager->CreateToastNotifierWithId(HStringReference(appId.c_str()).Get(), &notifier);
        if (FAILED(hr) || !notifier) return NotificationSetting_Enabled;

        NotificationSetting setting = NotificationSetting_Enabled;
        hr = notifier->get_Setting(&setting);
        if (FAILED(hr)) return NotificationSetting_Enabled;
        return setting;
    }
}

WindowsNotificationEvaluation WindowsNotificationState::evaluatePopupDeferral() {
    WindowsNotificationEvaluation result;

    QUERY_USER_NOTIFICATION_STATE state = QUNS_ACCEPTS_NOTIFICATIONS;
    const HRESULT hr = SHQueryUserNotificationState(&state);
    result.systemStateHr = static_cast<long>(hr);
    result.hasSystemNotificationState = SUCCEEDED(hr);
    if (result.hasSystemNotificationState) {
        result.systemStateName = notificationStateName(state);
    }

    result.deferBySystemState =
            (result.hasSystemNotificationState && state == QUNS_NOT_PRESENT)
            || (result.hasSystemNotificationState && state == QUNS_BUSY)
            || (result.hasSystemNotificationState && state == QUNS_RUNNING_D3D_FULL_SCREEN)
            || (result.hasSystemNotificationState && state == QUNS_PRESENTATION_MODE)
            || (result.hasSystemNotificationState && state == QUNS_QUIET_TIME);

    result.deferByFullscreenWindow = isForegroundFullscreenWindow();
    result.deferByQuietHoursService = isQuietHoursServiceActive();
    result.deferByGlobalToastSetting = areGlobalToastsDisabled();
    result.deferByFocusSession = isFocusSessionActive();

    const NotificationSetting toastSetting = queryToastNotificationSetting();
    result.toastSettingName = toastNotificationSettingName(toastSetting);
    result.deferByToastSetting = toastSetting != NotificationSetting_Enabled;

    const QuietHoursProfile quietHoursProfile = detectQuietHoursProfileFromCloudStore();
    result.quietHoursProfileName = quietHoursProfileName(quietHoursProfile);
    result.deferByQuietHoursProfile = quietHoursProfile != QuietHoursProfile::Unknown
                                      && quietHoursProfile != QuietHoursProfile::Unrestricted;

    result.shouldDefer =
            result.deferBySystemState
            || result.deferByFullscreenWindow
            || result.deferByQuietHoursService
            || result.deferByGlobalToastSetting
            || result.deferByFocusSession
            || result.deferByToastSetting
            || result.deferByQuietHoursProfile;

    return result;
}

#else
WindowsNotificationEvaluation WindowsNotificationState::evaluatePopupDeferral() {
    return {};
}
#endif
