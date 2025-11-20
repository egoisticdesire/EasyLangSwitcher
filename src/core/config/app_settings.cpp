#include "app_settings.h"

int AppSettings::defaultHotkeyVk = VK_LCONTROL;
QString AppSettings::defaultHotkeyName = "Left Ctrl";
int AppSettings::defaultSwitchDelayMs = 250;

int AppSettings::hotkeyVk = AppSettings::defaultHotkeyVk;
QString AppSettings::hotkeyName = AppSettings::defaultHotkeyName;
int AppSettings::switchDelayMs = AppSettings::defaultSwitchDelayMs;

void AppSettings::load() {
    hotkeyVk = settings.value("hotkey/vk", defaultHotkeyVk).toInt();
    hotkeyName = settings.value("hotkey/name", defaultHotkeyName).toString();
    switchDelayMs = settings.value("delay", defaultSwitchDelayMs).toInt();
}

void AppSettings::save() {
    settings.setValue("hotkey/vk", hotkeyVk);
    settings.setValue("hotkey/name", hotkeyName);
    settings.setValue("delay", switchDelayMs);
}
