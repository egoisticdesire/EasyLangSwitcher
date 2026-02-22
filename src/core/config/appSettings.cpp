#include "appSettings.h"
#include "logger.h"
#include <Windows.h>

#include "../i18n/lang.h"

int AppSettings::defaultHotkeyMainVk = VK_LCONTROL;
int AppSettings::defaultHotkeyModifiers = 0;
QString AppSettings::defaultHotkeyName = "Left Ctrl";
int AppSettings::defaultSwitchDelayMs = 250;
QString AppSettings::defaultAppLang = "en";
AppSettings::UpdateFrequency AppSettings::defaultUpdateFrequency = UpdateFrequency::Never;
QDate AppSettings::defaultLastUpdateCheckDate = QDate();
QDate AppSettings::lastUpdateCheckDate = defaultLastUpdateCheckDate;
QDateTime AppSettings::defaultLastUpdateCheckDateTime = QDateTime();
QDateTime AppSettings::lastUpdateCheckDateTime = defaultLastUpdateCheckDateTime;

int AppSettings::hotkeyMainVk = defaultHotkeyMainVk;
int AppSettings::hotkeyModifiers = defaultHotkeyModifiers;
QString AppSettings::hotkeyName = defaultHotkeyName;
int AppSettings::switchDelayMs = defaultSwitchDelayMs;
QString AppSettings::appLang = defaultAppLang;
AppSettings::UpdateFrequency AppSettings::updateFrequency = defaultUpdateFrequency;

int AppSettings::previousHotkeyMainVk = 0;
QString AppSettings::previousHotkeyName = "";

bool AppSettings::autoStartup = false;


struct UpdateFrequencyMeta {
    const char *logValue;
    const char *trKey;
};

static const QHash<AppSettings::UpdateFrequency, UpdateFrequencyMeta>
updateFrequencyMeta = {
    {AppSettings::UpdateFrequency::Never, {"never", "SETTINGS_APP_UPD_CHECK_NEVER"}},
    {AppSettings::UpdateFrequency::Daily, {"daily", "SETTINGS_APP_UPD_CHECK_DAILY"}},
    {AppSettings::UpdateFrequency::Weekly, {"weekly", "SETTINGS_APP_UPD_CHECK_WEEKLY"}},
    {AppSettings::UpdateFrequency::Monthly, {"monthly", "SETTINGS_APP_UPD_CHECK_MONTHLY"}},
};

void AppSettings::logger(const QString &action) {
    LOG_DEBUG() << QString("%1 settings: vk=%2; name='%3'; delay=%4; prevVk=%5; prevName='%6'; "
                   "autoStartup=%7; lang='%8'; updateFrequency='%9'; lastUpdCheck='%10'; lastUpdCheckTs='%11'")
                    .arg(action)
                    .arg(hotkeyMainVk)
                    .arg(hotkeyName.toLower())
                    .arg(switchDelayMs)
                    .arg(previousHotkeyMainVk)
                    .arg(previousHotkeyName.toLower())
                    .arg(autoStartup ? "true" : "false")
                    .arg(appLang.toLower(), updateFrequencyToString(updateFrequency, false).toLower())
                    .arg(lastUpdateCheckDate.isValid() ? lastUpdateCheckDate.toString(Qt::ISODate) : "never")
                    .arg(lastUpdateCheckDateTime.isValid() ? lastUpdateCheckDateTime.toString(Qt::ISODate) : "never");
}

QString AppSettings::updateFrequencyToString(
    const UpdateFrequency value,
    const bool localized
) {
    const auto it = updateFrequencyMeta.constFind(value);
    if (it == updateFrequencyMeta.constEnd())
        return "unknown";

    return localized ? Lang::tr(it->trKey) : it->logValue;
}


void AppSettings::load() {
    hotkeyMainVk = settings.value("hotkey/main_vk", defaultHotkeyMainVk).toInt();
    hotkeyModifiers = settings.value("hotkey/mods", defaultHotkeyModifiers).toInt();
    hotkeyName = settings.value("hotkey/name", defaultHotkeyName).toString();
    switchDelayMs = settings.value("delay", defaultSwitchDelayMs).toInt();
    previousHotkeyMainVk = settings.value("previous_hotkey/main_vk", 0).toInt();
    previousHotkeyName = settings.value("previous_hotkey/name", QString{}).toString();
    autoStartup = settings.value("auto_startup", false).toBool();
    appLang = settings.value("app/lang", defaultAppLang).toString();
    updateFrequency = static_cast<UpdateFrequency>(settings.value("updates/frequency",
                                                                  static_cast<int>(defaultUpdateFrequency)).toInt());
    lastUpdateCheckDate = settings.value("updates/last_check_date", defaultLastUpdateCheckDate).toDate();
    lastUpdateCheckDateTime = settings.value("updates/last_check_datetime", defaultLastUpdateCheckDateTime).toDateTime();

    logger("Loaded");
}

void AppSettings::save() {
    settings.setValue("hotkey/main_vk", hotkeyMainVk);
    settings.setValue("hotkey/mods", hotkeyModifiers);
    settings.setValue("hotkey/name", hotkeyName);
    settings.setValue("delay", switchDelayMs);
    settings.setValue("previous_hotkey/main_vk", previousHotkeyMainVk);
    settings.setValue("previous_hotkey/name", previousHotkeyName);
    settings.setValue("auto_startup", autoStartup);
    settings.setValue("app/lang", appLang);
    settings.setValue("updates/frequency", static_cast<int>(updateFrequency));
    settings.setValue("updates/last_check_date", lastUpdateCheckDate);
    settings.setValue("updates/last_check_datetime", lastUpdateCheckDateTime);

    logger("Saved");
}

bool AppSettings::isDirty() {
    // Сравниваем каждое значение с тем, что сейчас в реестре
    if (settings.value("hotkey/main_vk", defaultHotkeyMainVk).toInt() != hotkeyMainVk) return true;
    if (settings.value("hotkey/mods", defaultHotkeyModifiers).toInt() != hotkeyModifiers) return true;
    if (settings.value("hotkey/name", defaultHotkeyName).toString() != hotkeyName) return true;
    if (settings.value("delay", defaultSwitchDelayMs).toInt() != switchDelayMs) return true;
    if (settings.value("auto_startup", false).toBool() != autoStartup) return true;
    if (settings.value("app/lang", defaultAppLang).toString() != appLang) return true;
    if (static_cast<UpdateFrequency>(settings.value("updates/frequency",
        static_cast<int>(defaultUpdateFrequency)).toInt()) != updateFrequency) return true;

    // Если ничего не подошло
    return false;
}

void AppSettings::reset() {
    hotkeyMainVk = defaultHotkeyMainVk;
    hotkeyModifiers = defaultHotkeyModifiers;
    hotkeyName = defaultHotkeyName;
    switchDelayMs = defaultSwitchDelayMs;
    previousHotkeyMainVk = 0;
    previousHotkeyName = "";

    logger("Reset");
}
