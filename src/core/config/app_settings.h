#pragma once
#include <QSettings>
#include <QString>

/*
AppSettings
— централизованное хранение настроек (runtime + defaults)
*/

class AppSettings {
public:
    static void load();

    static void save();

    // runtime
    static int hotkeyMainVk;

    static int hotkeyModifiers;

    static QString hotkeyName;

    static int switchDelayMs;

    // defaults
    static int defaultHotkeyModifiers;

    static QString defaultHotkeyName;

    static int defaultSwitchDelayMs;

    static int defaultHotkeyMainVk;

private:
    static inline QSettings settings{"EasyLangSwitcher", "EasyLangSwitcher"};
};
