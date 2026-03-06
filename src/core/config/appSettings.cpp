#include "appSettings.h"

#include "../i18n/lang.h"
#include "logger.h"

#include <Windows.h>
#include <QTextStream>

namespace
{
const QString& defaultHotkeyName()
{
    static const auto value = QStringLiteral("Left Ctrl");
    return value;
}

const QString& defaultAppLang()
{
    static const auto value = QStringLiteral("en");
    return value;
}

const QDate& defaultLastUpdateCheckDate()
{
    static constexpr QDate value;
    return value;
}

const QDateTime& defaultLastUpdateCheckDateTime()
{
    static const QDateTime value;
    return value;
}
} // namespace

int AppSettings::defaultHotkeyMainVk = VK_LCONTROL;
int AppSettings::defaultHotkeyModifiers = 0;
int AppSettings::defaultSwitchDelayMs = 250;
AppSettings::UpdateFrequency AppSettings::defaultUpdateFrequency = UpdateFrequency::Never;

int AppSettings::hotkeyMainVk = defaultHotkeyMainVk;
int AppSettings::hotkeyModifiers = defaultHotkeyModifiers;
QString AppSettings::hotkeyName;
int AppSettings::switchDelayMs = defaultSwitchDelayMs;
QString AppSettings::appLang;
AppSettings::UpdateFrequency AppSettings::updateFrequency = defaultUpdateFrequency;

int AppSettings::previousHotkeyMainVk = 0;
QString AppSettings::previousHotkeyName;

bool AppSettings::autoStartup = false;

QDate& AppSettings::lastUpdateCheckDate()
{
    static QDate value;
    return value;
}

QDateTime& AppSettings::lastUpdateCheckDateTime()
{
    static QDateTime value;
    return value;
}

struct UpdateFrequencyMeta
{
    const char* logValue;
    const char* trKey;
};

const QHash<AppSettings::UpdateFrequency, UpdateFrequencyMeta>& updateFrequencyMeta()
{
    static const QHash<AppSettings::UpdateFrequency, UpdateFrequencyMeta> value = {
            {AppSettings::UpdateFrequency::Never,
             UpdateFrequencyMeta{.logValue = "never", .trKey = "SETTINGS_APP_UPD_CHECK_NEVER"}},
            {AppSettings::UpdateFrequency::Daily,
             UpdateFrequencyMeta{.logValue = "daily", .trKey = "SETTINGS_APP_UPD_CHECK_DAILY"}},
            {AppSettings::UpdateFrequency::Weekly,
             UpdateFrequencyMeta{.logValue = "weekly", .trKey = "SETTINGS_APP_UPD_CHECK_WEEKLY"}},
            {AppSettings::UpdateFrequency::Monthly,
             UpdateFrequencyMeta{.logValue = "monthly", .trKey = "SETTINGS_APP_UPD_CHECK_MONTHLY"}},
    };
    return value;
}

QSettings& AppSettings::settingsStorage()
{
    static QSettings settings(APP_NAME, APP_NAME);
    return settings;
}

void AppSettings::logger(const QString& action)
{
    QString message;
    QTextStream stream(&message);
    stream << action << " settings: vk=" << hotkeyMainVk << "; name='" << hotkeyName.toLower()
           << "'; delay=" << switchDelayMs << "; prevVk=" << previousHotkeyMainVk << "; prevName='"
           << previousHotkeyName.toLower() << "'; autoStartup=" << (autoStartup ? "true" : "false") << "; lang='"
           << appLang.toLower() << "'; updateFrequency='" << updateFrequencyToString(updateFrequency, false).toLower()
           << "'; lastUpdCheck='"
           << (lastUpdateCheckDate().isValid() ? lastUpdateCheckDate().toString(Qt::ISODate) : QStringLiteral("never"))
           << "'; lastUpdCheckTs='"
           << (lastUpdateCheckDateTime().isValid() ? lastUpdateCheckDateTime().toString(Qt::ISODate)
                                                   : QStringLiteral("never"))
           << "'";
    LOG_DEBUG() << message;
}

QString AppSettings::updateFrequencyToString(const UpdateFrequency value, const bool localized)
{
    const auto& meta = updateFrequencyMeta();
    const auto it = meta.constFind(value);
    if (it == meta.constEnd()) {
        return "unknown";
    }

    return localized ? Lang::tr(it->trKey) : it->logValue;
}

void AppSettings::load()
{
    const QSettings& settings = settingsStorage();
    hotkeyMainVk = settings.value("hotkey/main_vk", defaultHotkeyMainVk).toInt();
    hotkeyModifiers = settings.value("hotkey/mods", defaultHotkeyModifiers).toInt();
    hotkeyName = settings.value("hotkey/name", defaultHotkeyName()).toString();
    switchDelayMs = settings.value("delay", defaultSwitchDelayMs).toInt();
    previousHotkeyMainVk = settings.value("previous_hotkey/main_vk", 0).toInt();
    previousHotkeyName = settings.value("previous_hotkey/name", QString{}).toString();
    autoStartup = settings.value("auto_startup", false).toBool();
    appLang = settings.value("app/lang", defaultAppLang()).toString();
    updateFrequency = static_cast<UpdateFrequency>(
            settings.value("updates/frequency", static_cast<int>(defaultUpdateFrequency)).toInt());
    lastUpdateCheckDate() = settings.value("updates/last_check_date", defaultLastUpdateCheckDate()).toDate();
    lastUpdateCheckDateTime() =
            settings.value("updates/last_check_datetime", defaultLastUpdateCheckDateTime()).toDateTime();

    logger("Loaded");
}

void AppSettings::save()
{
    QSettings& settings = settingsStorage();
    settings.setValue("hotkey/main_vk", hotkeyMainVk);
    settings.setValue("hotkey/mods", hotkeyModifiers);
    settings.setValue("hotkey/name", hotkeyName);
    settings.setValue("delay", switchDelayMs);
    settings.setValue("previous_hotkey/main_vk", previousHotkeyMainVk);
    settings.setValue("previous_hotkey/name", previousHotkeyName);
    settings.setValue("auto_startup", autoStartup);
    settings.setValue("app/lang", appLang);
    settings.setValue("updates/frequency", static_cast<int>(updateFrequency));
    settings.setValue("updates/last_check_date", lastUpdateCheckDate());
    settings.setValue("updates/last_check_datetime", lastUpdateCheckDateTime());

    logger("Saved");
}

bool AppSettings::isDirty()
{
    const QSettings& settings = settingsStorage();
    // Сравниваем каждое значение с тем, что сейчас в реестре
    if (settings.value("hotkey/main_vk", defaultHotkeyMainVk).toInt() != hotkeyMainVk) {
        return true;
    }
    if (settings.value("hotkey/mods", defaultHotkeyModifiers).toInt() != hotkeyModifiers) {
        return true;
    }
    if (settings.value("hotkey/name", defaultHotkeyName()).toString() != hotkeyName) {
        return true;
    }
    if (settings.value("delay", defaultSwitchDelayMs).toInt() != switchDelayMs) {
        return true;
    }
    if (settings.value("auto_startup", false).toBool() != autoStartup) {
        return true;
    }
    if (settings.value("app/lang", defaultAppLang()).toString() != appLang) {
        return true;
    }
    if (static_cast<UpdateFrequency>(
                settings.value("updates/frequency", static_cast<int>(defaultUpdateFrequency)).toInt()) !=
        updateFrequency) {
        return true;
    }

    // Если ничего не подошло
    return false;
}

void AppSettings::reset()
{
    hotkeyMainVk = defaultHotkeyMainVk;
    hotkeyModifiers = defaultHotkeyModifiers;
    hotkeyName = defaultHotkeyName();
    switchDelayMs = defaultSwitchDelayMs;
    previousHotkeyMainVk = 0;
    previousHotkeyName.clear();

    logger("Reset");
}
