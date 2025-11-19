#pragma once
#include <Windows.h>
#include <QString>

struct AppConfig {
    inline static int hotkeyVk = VK_LCONTROL;
    inline static int switchDelayMs = 250;
    inline static QString hotkeyName = "Left Ctrl";
};
