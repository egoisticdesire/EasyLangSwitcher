#include "app_settings.h"
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

void AppSettings::load() {
    hotkeyMainVk = settings.value("hotkey/main_vk", defaultHotkeyMainVk).toInt();
    hotkeyModifiers = settings.value("hotkey/mods", defaultHotkeyModifiers).toInt();
    hotkeyName = settings.value("hotkey/name", defaultHotkeyName).toString();
    switchDelayMs = settings.value("delay", defaultSwitchDelayMs).toInt();
    previousHotkeyMainVk = settings.value("previous_hotkey/main_vk", previousHotkeyMainVk).toInt();
    previousHotkeyName = settings.value("previous_hotkey/name", previousHotkeyName).toString();

    LOG_DEBUG() << "Loaded settings: vk=" << hotkeyMainVk
    << "; name='" << hotkeyName << "'; delay=" << switchDelayMs << "; prevVk=" << previousHotkeyMainVk
    << "; prevName='" << previousHotkeyName << "'";
}

void AppSettings::save() {
    settings.setValue("hotkey/main_vk", hotkeyMainVk);
    settings.setValue("hotkey/mods", hotkeyModifiers);
    settings.setValue("hotkey/name", hotkeyName);
    settings.setValue("delay", switchDelayMs);
    settings.setValue("previous_hotkey/main_vk", previousHotkeyMainVk);
    settings.setValue("previous_hotkey/name", previousHotkeyName);

    LOG_DEBUG() << "Saved settings: vk=" << hotkeyMainVk
            << "; name='" << hotkeyName << "'; delay=" << switchDelayMs << "; prevVk=" << previousHotkeyMainVk
            << "; prevName='" << previousHotkeyName << "'";
}
