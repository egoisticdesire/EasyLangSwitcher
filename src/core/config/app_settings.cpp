#include "app_settings.h"
#include <Windows.h>
#include <QDebug>

int AppSettings::defaultHotkeyMainVk = VK_LCONTROL;
int AppSettings::defaultHotkeyModifiers = 0;
QString AppSettings::defaultHotkeyName = "Left Ctrl";
int AppSettings::defaultSwitchDelayMs = 250;

int AppSettings::hotkeyMainVk = AppSettings::defaultHotkeyMainVk;
int AppSettings::hotkeyModifiers = AppSettings::defaultHotkeyModifiers;
QString AppSettings::hotkeyName = AppSettings::defaultHotkeyName;
int AppSettings::switchDelayMs = AppSettings::defaultSwitchDelayMs;

void AppSettings::load() {
    hotkeyMainVk = settings.value("hotkey/main_vk", defaultHotkeyMainVk).toInt();
    hotkeyModifiers = settings.value("hotkey/mods", defaultHotkeyModifiers).toInt();
    hotkeyName = settings.value("hotkey/name", defaultHotkeyName).toString();
    switchDelayMs = settings.value("delay", defaultSwitchDelayMs).toInt();
    qDebug() << "AppSettings::load -> mainVk =" << hotkeyMainVk
            << "mods =" << hotkeyModifiers
            << "name =" << hotkeyName
            << "delay =" << switchDelayMs;
}

void AppSettings::save() {
    settings.setValue("hotkey/main_vk", hotkeyMainVk);
    settings.setValue("hotkey/mods", hotkeyModifiers);
    settings.setValue("hotkey/name", hotkeyName);
    settings.setValue("delay", switchDelayMs);
    qDebug() << "AppSettings::save -> persisted mainVk =" << hotkeyMainVk
            << "name =" << hotkeyName;
}
