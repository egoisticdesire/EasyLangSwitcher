#pragma once

#include <QStringList>

namespace MainBootstrapHelper {
    bool ensureToastShortcut(const wchar_t *appUserModelId);

    void ensureProtocolRegistration();

    QStringList startupArgsFromArgv(int argc, char *argv[]);

    QString instanceLockPath();

    bool isToastActivationLaunch(const QStringList &startupArgs);
}
