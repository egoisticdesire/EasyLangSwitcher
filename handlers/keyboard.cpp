#include "keyboard.h"
#include "../widgets/settings.h"
#include <QDebug>

thread_local KeyboardHandler *KeyboardHandler::instance = nullptr;

KeyboardHandler::KeyboardHandler(SettingsData &data, QObject *parent)
    : QObject(parent),
      settings(data) {
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

    // 1. Пытаемся стандартным способом
    PostMessage(hwnd, WM_INPUTLANGCHANGEREQUEST, 0, reinterpret_cast<LPARAM>(next));

    // 2. Проверяем через короткую задержку, изменилась ли раскладка
    QTimer::singleShot(20, [threadId, next]() {
        if (const HKL now = GetKeyboardLayout(threadId); now != next) {
            // fallback: эмулируем Win+Space
            INPUT inputs[4] = {};
            // Win down
            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = VK_LWIN;
            // Space down
            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = VK_SPACE;
            // Space up
            inputs[2].type = INPUT_KEYBOARD;
            inputs[2].ki.wVk = VK_SPACE;
            inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
            // Win up
            inputs[3].type = INPUT_KEYBOARD;
            inputs[3].ki.wVk = VK_LWIN;
            inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

            SendInput(4, inputs, sizeof(INPUT));
        }
    });
    qDebug() << "Requested layout switch:" << QString::number(reinterpret_cast<qulonglong>(next), 16);
}
