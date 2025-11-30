#include "appSettings.h"
#include "logger.h"
#include <Windows.h>

int AppSettings::defaultHotkeyMainVk = VK_LCONTROL;
int AppSettings::defaultHotkeyModifiers = 0;
QString AppSettings::defaultHotkeyName = "Left Ctrl";
int AppSettings::defaultSwitchDelayMs = 250;

int AppSettings::hotkeyMainVk = defaultHotkeyMainVk;
int AppSettings::hotkeyModifiers = defaultHotkeyModifiers;
QString AppSettings::hotkeyName = defaultHotkeyName;
int AppSettings::switchDelayMs = defaultSwitchDelayMs;

int AppSettings::previousHotkeyMainVk = 0;
QString AppSettings::previousHotkeyName = "";

bool AppSettings::autoStartup = false;

void AppSettings::logger(const QString &action) {
    LOG_DEBUG() << QString("%1 settings: vk=%2; name='%3'; delay=%4; prevVk=%5; prevName='%6'; autoStartup=%7")
                    .arg(action)
                    .arg(hotkeyMainVk)
                    .arg(hotkeyName)
                    .arg(switchDelayMs)
                    .arg(previousHotkeyMainVk)
                    .arg(previousHotkeyName)
                    .arg(autoStartup ? "true" : "false");
}

void AppSettings::load() {
    hotkeyMainVk = settings.value("hotkey/main_vk", defaultHotkeyMainVk).toInt();
    hotkeyModifiers = settings.value("hotkey/mods", defaultHotkeyModifiers).toInt();
    hotkeyName = settings.value("hotkey/name", defaultHotkeyName).toString();
    switchDelayMs = settings.value("delay", defaultSwitchDelayMs).toInt();
    previousHotkeyMainVk = settings.value("previous_hotkey/main_vk", previousHotkeyMainVk).toInt();
    previousHotkeyName = settings.value("previous_hotkey/name", previousHotkeyName).toString();
    autoStartup = settings.value("auto_startup", autoStartup).toBool();

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

    logger("Saved");
}

void AppSettings::reset() {
    hotkeyMainVk = defaultHotkeyMainVk;
    hotkeyModifiers = defaultHotkeyModifiers;
    hotkeyName = defaultHotkeyName;
    switchDelayMs = defaultSwitchDelayMs;
    previousHotkeyMainVk = 0;
    previousHotkeyName = "";
    autoStartup = false;

    logger("Reset");
}
