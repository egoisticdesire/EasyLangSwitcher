#pragma once
#include <QString>

class AutoStartupManager {
public:
    static bool setAutoStartup(bool enable);

    static bool isAutoStartupEnabled();

private:
    static QString getExePath();
};
