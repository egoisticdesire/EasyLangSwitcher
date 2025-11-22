#include "../config/app_settings.h"
#include "kb.h"
#include <QDebug>
#include <vector>
#include <unordered_set>

thread_local KeyboardHandler *KeyboardHandler::instance = nullptr;

KeyboardHandler::KeyboardHandler(QObject *parent)
    : QObject(parent) {
    // таймер долгого нажатия: если срабатывает => это долгий тап
    longPressTimer.setSingleShot(true);
    connect(&longPressTimer, &QTimer::timeout, this, [this]() {
        longPressDetected = true;
        // как только обнаружено долгое нажатие, любые ожидающие переключения отменяются
        pendingSwitch = false;
        qDebug() << "[KeyboardHandler] longPressDetected = true";
    });

    // таймер окна быстрого повторного нажатия: решает, выполнять переключение или нет
    doublePressTimer.setSingleShot(true);
    connect(&doublePressTimer, &QTimer::timeout, this, [this]() {
        // окно истекло: если есть pendingSwitch и не longPress и не подавлено => выполняем переключение
        qDebug() << "[KeyboardHandler] double-window expired. pendingSwitch =" << pendingSwitch
                << " longPressDetected =" << longPressDetected
                << " suppressedByRapidRepeat =" << suppressedByRapidRepeat;

        // если было ожидающее переключение и оно не подавлено/не longPress -> переключаем
        if (pendingSwitch && !longPressDetected && !suppressedByRapidRepeat) {
            switchKeyboardLayout();
            qDebug() << "[KeyboardHandler] delayed switch executed";
        }
        // очистка флагов
        pendingSwitch = false;
        suppressedByRapidRepeat = false;
    });
}

KeyboardHandler::~KeyboardHandler() {
    stop();
}

void KeyboardHandler::start() {
    instance = this;
    hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0);
    if (!hook) {
        qDebug() << "[KeyboardHandler] hook install failed";
    } else {
        qDebug() << "[KeyboardHandler] hook installed";
    }
}

void KeyboardHandler::stop() {
    if (hook) {
        UnhookWindowsHookEx(hook);
        hook = nullptr;
        instance = nullptr;
        qDebug() << "[KeyboardHandler] hook removed";
    }
}

// проверка, является ли клавиша модификатором
static bool isModifierVk(const int vk) {
    return vk == VK_LCONTROL || vk == VK_RCONTROL ||
           vk == VK_LSHIFT || vk == VK_RSHIFT ||
           vk == VK_LMENU || vk == VK_RMENU ||
           vk == VK_CAPITAL || vk == VK_LWIN || vk == VK_RWIN;
}

LRESULT CALLBACK KeyboardHandler::LowLevelKeyboardProc(const int nCode, const WPARAM wParam, const LPARAM lParam) {
    if (nCode == HC_ACTION && instance && instance->active) {
        thread_local std::unordered_set<int> downKeys;
        const auto kb = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        const bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        const bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
        const int vk = static_cast<int>(kb->vkCode);

        // отслеживаем состояние Alt отдельно
        if (vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU) {
            if (isDown) instance->altDown = true;
            if (isUp) instance->altDown = false;
        }

        if (isDown) downKeys.insert(vk);
        if (isUp) downKeys.erase(vk);

        const int configuredMain = AppSettings::hotkeyMainVk;
        const DWORD now = GetTickCount();

        // обработка нажатия основной клавиши
        if (isDown && vk == configuredMain) {
            // проверяем, нажаты ли другие клавиши => комбо
            bool otherKeyDown = false;
            for (const int d: downKeys) {
                if (d == vk) continue;
                if (isModifierVk(d)) continue;
                otherKeyDown = true;
                break;
            }

            if (otherKeyDown) {
                qDebug() << "[KeyboardHandler] hotkey down ignored (combo)";
            } else {
                // проверяем интервал между нажатиями для быстрого повторного нажатия
                const DWORD intervalSinceLastDown = now - instance->lastDownTime;

                if (const DWORD doublePress = static_cast<DWORD>(AppSettings::switchDelayMs);
                    intervalSinceLastDown <= doublePress) {
                    // обнаружено быстрое повторное нажатие: подавляем переключение, ведём себя как обычные клавиши
                    qDebug() << "[KeyboardHandler] rapid repeat detected, suppress switch";
                    instance->suppressedByRapidRepeat = true;

                    // отменяем любые предыдущие действия
                    instance->pendingSwitch = false;
                    instance->longPressDetected = false;
                    instance->longPressTimer.stop();
                    instance->doublePressTimer.stop();

                    // не считаем это специальным нажатием триггера
                    instance->keyPressed = false;
                } else {
                    // возможное первое нажатие одиночного/длительного нажатия;
                    // старт таймера долгого нажатия и окна повторного нажатия
                    instance->keyPressed = true;
                    instance->longPressDetected = false;
                    instance->suppressedByRapidRepeat = false;
                    instance->pressTime = now;
                    instance->pendingSwitch = false;
                    instance->longPressTimer.start(AppSettings::switchDelayMs);

                    // старт таймера окна повторного нажатия
                    instance->doublePressTimer.start(doublePress);

                    qDebug() << "[KeyboardHandler] hotkey down detected vk=" << vk
                            << " start longPressTimer and doublePress";
                }

                // обновляем время последнего нажатия
                instance->lastDownTime = now;
            }
        }
        // обработка отпускания основной клавиши
        else if (isUp && vk == configuredMain) {
            // если down не был потенциальным триггером, ничего не делаем
            if (!instance->keyPressed) {
                qDebug() << "[KeyboardHandler] hotkey up normal";
            } else {
                // останавливаем таймер долгого нажатия
                instance->longPressTimer.stop();

                const DWORD pressDuration = now - instance->pressTime;
                qDebug() << "[KeyboardHandler] hotkey up vk=" << vk << " duration=" << pressDuration
                        << " longPressDetected=" << instance->longPressDetected
                        << " suppressedByRapidRepeat=" << instance->suppressedByRapidRepeat;

                // если долгий тап -> стандартное поведение
                if (instance->longPressDetected) {
                    instance->keyPressed = false;
                    instance->pendingSwitch = false;
                    instance->suppressedByRapidRepeat = false;
                    qDebug() << "[KeyboardHandler] long press - normal";
                } else {
                    // короткое нажатие -> потенциальное переключение, ждём окно doublePress
                    if (!instance->doublePressTimer.isActive()) {
                        // окно уже истекло => переключаем сразу
                        if (!instance->suppressedByRapidRepeat) {
                            switchKeyboardLayout();
                            qDebug() << "[KeyboardHandler] immediate switch executed";
                        } else {
                            qDebug() << "[KeyboardHandler] suppressed - do nothing";
                        }
                        instance->keyPressed = false;
                        instance->pendingSwitch = false;
                        instance->suppressedByRapidRepeat = false;
                    } else {
                        // помечаем как ожидающее переключение
                        instance->pendingSwitch = true;
                        qDebug() << "[KeyboardHandler] short press pendingSwitch";
                        // keyPressed будет очищен после обработки doublePressTimer
                        instance->keyPressed = false;
                    }
                }
            }
        }

        // обрабатываем как комбо, если нажата другая клавиша при удержании триггера
        else if (isDown && instance->keyPressed && vk != configuredMain) {
            instance->keyPressed = false;
            instance->longPressTimer.stop();
            instance->doublePressTimer.stop();
            instance->pendingSwitch = false;
            instance->longPressDetected = false;
            instance->suppressedByRapidRepeat = false;

            qDebug() << "[KeyboardHandler] combo detected - cancel special";
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

    // 1. стандартная попытка переключения
    PostMessage(hwnd, WM_INPUTLANGCHANGEREQUEST, 0, reinterpret_cast<LPARAM>(next));

    // 2. проверка через короткую задержку, изменилась ли раскладка
    QTimer::singleShot(fallbackCheckMs, [threadId, next]() {
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
    qDebug() << "[KeyboardHandler] switch requested HKL:" << QString::number(
        reinterpret_cast<qulonglong>(next), 16);
}
