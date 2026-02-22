#pragma once
#include <QString>

struct WindowsNotificationEvaluation {
    bool hasSystemNotificationState = false;
    long systemStateHr = 0;
    QString systemStateName = "unavailable";

    bool deferBySystemState = false;
    bool deferByFullscreenWindow = false;
    bool deferByQuietHoursService = false;
    bool deferByGlobalToastSetting = false;
    bool deferByFocusSession = false;
    QString toastSettingName = "unknown";
    bool deferByToastSetting = false;
    QString quietHoursProfileName = "unknown";
    bool deferByQuietHoursProfile = false;

    bool shouldDefer = false;
};

namespace WindowsNotificationState {
    WindowsNotificationEvaluation evaluatePopupDeferral();
}
