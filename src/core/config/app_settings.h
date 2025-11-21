#pragma once
#include <QSettings>

class AppSettings {
public:
    static void load();

    static void save();

    // текущие значения
    static int hotkeyVk;
    static QString hotkeyName;
    static int switchDelayMs;

    // дефолтные значения
    static int defaultHotkeyVk;
    static QString defaultHotkeyName;
    static int defaultSwitchDelayMs;

private:
    static inline QSettings settings{
        "EasyLangSwitcher", "EasyLangSwitcher"
    };
};
