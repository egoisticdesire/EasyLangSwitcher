#pragma once
#include <QString>
#include <QSettings>
#include <Windows.h>

class AppSettings {
public:
    static void load();

    static void save();

    // текущие значения (runtime)
    static int hotkeyVk;
    static QString hotkeyName;
    static int switchDelayMs;

    // дефолты в том же файле
    static int defaultHotkeyVk;
    static QString defaultHotkeyName;
    static int defaultSwitchDelayMs;

private:
    static inline QSettings settings{
        "EasyLangSwitcher", "EasyLangSwitcher"
    };
};
