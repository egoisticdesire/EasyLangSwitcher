#pragma once
#include "../../core/config/appSettings.h"

#include <QString>
#include <string>

namespace WindowsToastIdentity
{
inline std::wstring appUserModelIdWide()
{
    return QString("%1.Desktop").arg(AppSettings::APP_NAME).toStdWString();
}

inline QString toastProtocolScheme()
{
    return QString(AppSettings::APP_NAME).toLower();
}

inline QString toastProtocolUri()
{
    return QString("%1://toast/open-update").arg(toastProtocolScheme());
}

inline std::wstring toastActivationEventNameWide()
{
    return QString("Local\\%1.ToastActivation").arg(AppSettings::APP_NAME).toStdWString();
}
} // namespace WindowsToastIdentity
