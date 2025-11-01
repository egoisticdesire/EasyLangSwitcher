#include "keyboard.h"
#include "../widgets/settings.h"
#include <QDebug>

thread_local KeyboardHandler *KeyboardHandler::instance = nullptr;

KeyboardHandler::KeyboardHandler(SettingsData &data, QObject *parent)
    : QObject(parent)
      , settings(data) {
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, [this]() {
        longPressDetected = true;
        qDebug() << "Long press - treat as normal Ctrl";
    });
}

KeyboardHandler::~KeyboardHandler() {
    stop(); // на всякий случай
}

void KeyboardHandler::start() {
    instance = this;
    hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0);
    if (!hook) {
        qDebug() << "Failed to install hook";
    }
}

void KeyboardHandler::stop() {
    if (hook) {
        UnhookWindowsHookEx(hook);
        hook = nullptr;
        instance = nullptr;
    }
}

LRESULT CALLBACK KeyboardHandler::LowLevelKeyboardProc(const int nCode, const WPARAM wParam, const LPARAM lParam) {
    if (nCode == HC_ACTION && instance) {
        const auto kb = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        const bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        const bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);

        if (kb->vkCode == instance->settings.hotkeyVk) {
            if (isDown && !instance->keyPressed) {
                instance->keyPressed = true;
                instance->longPressDetected = false;
                instance->pressTime = GetTickCount();
                instance->timer.start(instance->settings.switchDelayMs);
            } else if (isUp && instance->keyPressed) {
                instance->timer.stop();
                instance->keyPressed = false;

                if (!instance->longPressDetected) {
                    switchKeyboardLayout(); // короткое нажатие
                } else {
                    qDebug() << "Long press released - normal Ctrl";
                }
            }
        } else if (instance->keyPressed && (isDown || isUp)) {
            // комбинация с другими клавишами
            instance->keyPressed = false;
            instance->timer.stop();
            instance->longPressDetected = false;
            qDebug() << "Combo detected - normal Ctrl";
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void KeyboardHandler::switchKeyboardLayout() {
    const HWND hwnd = GetForegroundWindow();
    if (!hwnd) return;

    const DWORD threadId = GetWindowThreadProcessId(hwnd, nullptr);
    const HKL current = GetKeyboardLayout(threadId);

    const int n = GetKeyboardLayoutList(0, nullptr);
    if (n <= 0) return;
    std::vector<HKL> list(n);
    GetKeyboardLayoutList(n, list.data());

    int idx = 0;
    for (int i = 0; i < n; ++i)
        if (list[i] == current) {
            idx = i;
            break;
        }
    HKL next = list[(idx + 1) % n];

    PostMessage(hwnd, WM_INPUTLANGCHANGEREQUEST, 0, reinterpret_cast<LPARAM>(next));

    qDebug() << "Requested layout switch (HKL next):" << QString::number(reinterpret_cast<qulonglong>(next), 16);
}
