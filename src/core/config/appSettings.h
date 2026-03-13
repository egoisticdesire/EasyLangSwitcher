#pragma once
#include <cstdint>

#include <QSettings>
#include <QString>

class AppSettings
{
public:
    static constexpr auto APP_NAME = "EasyLangSwitcher";

    static void logger(const QString& action);

    enum class UpdateFrequency : std::uint8_t
    {
        Never,
        Daily,
        Weekly,
        Monthly
    };

    static QString updateFrequencyToString(UpdateFrequency value, bool localized = true);

    static void load();

    static void save();

    static bool isDirty();

    static void reset();

    static bool autoStartup;

    static int hotkeyMainVk;

    static int hotkeyModifiers;

    static QString hotkeyName;

    static int switchDelayMs;

    static int previousHotkeyMainVk;

    static QString previousHotkeyName;

    static int defaultHotkeyModifiers;

    static int defaultSwitchDelayMs;

    static int defaultHotkeyMainVk;

    static QString appLang;

    static UpdateFrequency defaultUpdateFrequency;

    static UpdateFrequency updateFrequency;

    static QDate& lastUpdateCheckDate();

    static QDateTime& lastUpdateCheckDateTime();

    static constexpr auto GITHUB_REPO = "egoisticdesire/EasyLangSwitcher";

private:
    static QSettings& settingsStorage();
};
