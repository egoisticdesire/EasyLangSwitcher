#pragma once
#include <QSettings>
#include <QString>

/*
AppSettings
— централизованное хранение настроек (runtime + defaults)
*/

class AppSettings {
public:
    static constexpr auto APP_NAME = "EasyLangSwitcher";

    static void logger(const QString &action);

    static void load();

    static void save();

    static void reset();

    static bool autoStartup;

    static int hotkeyMainVk;

    static int hotkeyModifiers;

    static QString hotkeyName;

    static int switchDelayMs;

    static int previousHotkeyMainVk;

    static QString previousHotkeyName;

    static int defaultHotkeyModifiers;

    static QString defaultHotkeyName;

    static int defaultSwitchDelayMs;

    static int defaultHotkeyMainVk;

    static QString appLang;

    static QString defaultAppLang;

private:
    static inline QSettings settings{APP_NAME, APP_NAME};
};
