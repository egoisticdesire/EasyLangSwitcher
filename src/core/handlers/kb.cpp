#include "../config/logger.h"
#include "../config/app_settings.h"
#include "kb.h"
#include <vector>
#include <unordered_set>

static QString hklToLangLabel(HKL hkl) {
    const LANGID langId = LOWORD(reinterpret_cast<DWORD_PTR>(hkl));
    WCHAR name[LOCALE_NAME_MAX_LENGTH] = {};
    if (LCIDToLocaleName(MAKELCID(langId, SORT_DEFAULT), name, LOCALE_NAME_MAX_LENGTH, 0)) {
        return QString::fromWCharArray(name);
    }
    return QString("Unknown");
}

thread_local KeyboardHandler *KeyboardHandler::instance = nullptr;

KeyboardHandler::KeyboardHandler(QObject *parent)
    : QObject(parent) {
    // таймер долгого нажатия: если срабатывает => это долгий тап
    longPressTimer.setSingleShot(true);
    connect(&longPressTimer, &QTimer::timeout, this, [this]() {
        // как только обнаружено долгое нажатие, любые ожидающие переключения отменяются
        isLongPress = true;
        switchPending = false;

        LOG_DEBUG() << "Long press detected";
    });

    // таймер окна быстрого повторного нажатия: решает, выполнять переключение или нет
    doublePressTimer.setSingleShot(true);
    connect(&doublePressTimer, &QTimer::timeout, this, [this]() {
        // окно истекло: если есть switchPending и не longPress и не подавлено => выполняем переключение
        LOG_DEBUG() << "Double-press window expired"
                << ": switchPending=" << (switchPending ? "true" : "false")
                << "; isLongPress=" << (isLongPress ? "true" : "false")
                << "; rapidRepeatSuppressed=" << (rapidRepeatSuppressed ? "true" : "false");

        // если было ожидающее переключение и оно не подавлено/не longPress -> переключаем
        if (switchPending && !isLongPress && !rapidRepeatSuppressed) {
            switchKeyboardLayout();

            LOG_DEBUG() << "Switch executed after delay";
        }
        // очистка флагов
        switchPending = false;
        rapidRepeatSuppressed = false;
    });

    LOG_DEBUG() << "KeyboardHandler initialized";
}

KeyboardHandler::~KeyboardHandler() {
    stop();
}

void KeyboardHandler::start() {
    instance = this;
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0);
    if (!keyboardHook) {
        LOG_WARNING() << "Hook installation failed";
    } else {
        LOG_DEBUG() << "Hook installed successfully";
    }
}

void KeyboardHandler::stop() {
    if (keyboardHook) {
        UnhookWindowsHookEx(keyboardHook);
        keyboardHook = nullptr;
        instance = nullptr;

        LOG_DEBUG() << "Hook removed";
    }
}

LRESULT CALLBACK KeyboardHandler::LowLevelKeyboardProc(const int nCode, const WPARAM wParam, const LPARAM lParam) {
    if (nCode == HC_ACTION && instance && instance->isActive) {
        thread_local std::unordered_set<int> downKeys;
        const auto kb = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        const bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        const bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
        const int vk = static_cast<int>(kb->vkCode);

        // отслеживаем состояние Alt отдельно
        if (vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU) {
            if (isDown) instance->isAltDown = true;
            if (isUp) instance->isAltDown = false;
        }

        if (isDown) downKeys.insert(vk);
        if (isUp) downKeys.erase(vk);

        const int configuredMain = AppSettings::hotkeyMainVk;
        const DWORD now = GetTickCount();

        // обработка нажатия основной клавиши
        if (isDown && vk == configuredMain) {
            // проверяем, нажаты ли другие клавиши => комбо
            // считаем комбо при наличии любой другой зажатой клавише,
            // включая модификаторы (чтобы "Modifier -> Trigger" тоже было комбо).
            bool otherKeyDown = false;
            for (const int d: downKeys) {
                if (d == vk) continue;
                otherKeyDown = true;
                break;
            }

            if (otherKeyDown) {
                // Комбо — сбрасываем сценарий триггера и разрешаем нативное поведение
                instance->triggerKeyDown = false;
                instance->isLongPress = false;
                instance->switchPending = false;
                instance->rapidRepeatSuppressed = false;
                instance->longPressTimer.stop();
                instance->doublePressTimer.stop();

                LOG_DEBUG() << "Combo: cancel trigger";

                // нативное поведение разрешено для всех клавиш в сценарии комбо
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
            }
            // проверяем интервал между нажатиями для быстрого повторного нажатия
            const DWORD intervalSinceLastDown = now - instance->lastTriggerDownTime;

            if (const DWORD doublePress = static_cast<DWORD>(AppSettings::switchDelayMs);
                intervalSinceLastDown <= doublePress) {
                // обнаружено быстрое повторное нажатие: трактуем как отмену переключения
                // и возвращаем нативное поведение
                LOG_DEBUG() << "Rapid repeat: native key allowed";

                instance->rapidRepeatSuppressed = true;

                // отменяем любые предыдущие действия
                instance->switchPending = false;
                instance->isLongPress = false;
                instance->longPressTimer.stop();
                instance->doublePressTimer.stop();
                instance->triggerKeyDown = false;
                instance->lastTriggerDownTime = now;

                // разрешаем нативное поведение
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
            } else {
                // возможное первое нажатие одиночного/длительного нажатия;
                // старт таймера долгого нажатия и окна повторного нажатия
                instance->triggerKeyDown = true;
                instance->isLongPress = false;
                instance->rapidRepeatSuppressed = false;
                instance->pressStartTime = now;
                instance->switchPending = false;

                // запускаем longPressTimer and doublePressTimer
                instance->longPressTimer.start(AppSettings::switchDelayMs);
                instance->doublePressTimer.start(doublePress);

                LOG_DEBUG() << "Hotkey down vk=" << vk << ": start longPressTimer and doublePressTimer";

                instance->lastTriggerDownTime = now;

                // Подавлять нативное действие нужно только если триггер — CapsLock или Alt
                // (Если триггер — Shift/Ctrl/Win и т.д., нативное поведение оставляем)
                if (configuredMain == VK_CAPITAL
                    || configuredMain == VK_MENU
                    || configuredMain == VK_LMENU
                    || configuredMain == VK_RMENU
                ) {
                    // suppress keydown — чтобы CapsLock не переключался при коротком нажатии
                    return 1;
                }
                // для остальных клавиш разрешаем нативное поведение
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
            }
        }

        // обработка отпускания основной клавиши
        if (isUp && vk == configuredMain) {
            // если down не был потенциальным триггером, ничего не делаем
            if (!instance->triggerKeyDown) {
                LOG_DEBUG() << "Hotkey up: normal";
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
            }
            // останавливаем таймер долгого нажатия
            instance->longPressTimer.stop();

            const DWORD pressDuration = now - instance->pressStartTime;

            LOG_DEBUG() << "Hotkey up vk=" << vk
                        << ": duration=" << pressDuration
                        << "; isLongPress=" << (instance->isLongPress ? "true" : "false")
                        << "; rapidRepeatSuppressed=" << (instance->rapidRepeatSuppressed ? "true" : "false")
                        << "; switchPending=" << (instance->switchPending ? "true" : "false");

            // если долгое нажатие -> стандартное поведение клавиши
            if (instance->isLongPress) {
                instance->triggerKeyDown = false;
                instance->switchPending = false;
                instance->rapidRepeatSuppressed = false;

                LOG_DEBUG() << "Long press: cancel trigger";
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
            }
            // короткое нажатие -> потенциальное переключение, ждём окно doublePress
            if (!instance->doublePressTimer.isActive()) {
                // окно уже истекло => переключаем сразу
                if (!instance->rapidRepeatSuppressed) {
                    switchKeyboardLayout();

                    LOG_DEBUG() << "Switch executed immediately";
                } else {
                    LOG_DEBUG() << "Switch suppressed: cancel trigger";
                }
                instance->triggerKeyDown = false;
                instance->switchPending = false;
                instance->rapidRepeatSuppressed = false;

                // если триггер — CapsLock или Alt, подавляем keyup чтобы Caps не переключился,
                // иначе разрешаем нативное поведение (Shift/Ctrl/Alt остаются изначально активными)
                if (configuredMain == VK_CAPITAL
                    || configuredMain == VK_MENU
                    || configuredMain == VK_LMENU
                    || configuredMain == VK_RMENU
                ) {
                    return 1;
                }
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
            }
            // помечаем как ожидающее переключение
            instance->switchPending = true;
            LOG_DEBUG() << "Short press: pending switch";

            // triggerKeyDown будет очищен после обработки doublePressTimer
            instance->triggerKeyDown = false;

            // suppress keyup только для CapsLock и Alt (иначе нативное поведение)
            if (configuredMain == VK_CAPITAL
                || configuredMain == VK_MENU
                || configuredMain == VK_LMENU
                || configuredMain == VK_RMENU
            ) {
                return 1;
            }
            return CallNextHookEx(nullptr, nCode, wParam, lParam);
        }

        // обрабатываем как комбо, если нажата другая клавиша при удержании триггера
        if (isDown && instance->triggerKeyDown && vk != configuredMain) {
            // Комбо — отменяем сценарий триггера, но разрешаем нативное поведение
            instance->triggerKeyDown = false;
            instance->longPressTimer.stop();
            instance->doublePressTimer.stop();
            instance->switchPending = false;
            instance->isLongPress = false;
            instance->rapidRepeatSuppressed = false;

            LOG_DEBUG() << "Combo: cancel trigger";

            // нативное поведение для модификаторов/других клавиш должно срабатывать сразу,
            // чтобы, например, Shift позволял получить заглавную букву без задержек.
            return CallNextHookEx(nullptr, nCode, wParam, lParam);
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void KeyboardHandler::switchKeyboardLayout() {
    const HWND hwnd = GetForegroundWindow();
    if (!hwnd) return;

    DWORD threadId = GetWindowThreadProcessId(hwnd, nullptr);
    HKL current = GetKeyboardLayout(threadId);

    const int n = GetKeyboardLayoutList(0, nullptr);
    if (n <= 0) return;
    std::vector<HKL> layouts(n);
    GetKeyboardLayoutList(n, layouts.data());

    int idx = 0;
    for (int i = 0; i < n; ++i)
        if (layouts[i] == current) {
            idx = i;
            break;
        }
    HKL next = layouts[(idx + 1) % n];

    // Проверка типа окна: UWP/Store apps и т.п.
    wchar_t className[256] = {};
    GetClassName(hwnd, className, sizeof(className) / sizeof(wchar_t));

    bool useDirectFallback = false;
    if (const std::wstring cls(className);
        cls.starts_with(L"ApplicationFrameWindow") // большинство UWP окон
        || cls.starts_with(L"Windows.UI.Core.CoreWindow")
        || cls.starts_with(L"Xaml")
        // || cls.starts_with(L"Calculator") // конкретные "проблемные" окна
    ) {
        useDirectFallback = true;
    }

    if (!useDirectFallback) {
        // 1. Пытаемся PostMessage
        PostMessage(hwnd, WM_INPUTLANGCHANGEREQUEST, 0, reinterpret_cast<LPARAM>(next));
    }

    // 2. Проверка через короткий таймер, при необходимости fallback через Win+Space
    QTimer::singleShot(fallbackCheckMs, [threadId, next, useDirectFallback]() {
        if (const HKL now = GetKeyboardLayout(threadId); now != next || useDirectFallback) {
            // fallback: эмуляция Win+Space
            INPUT inputs[4] = {};
            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = VK_LWIN;
            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = VK_SPACE;
            inputs[2].type = INPUT_KEYBOARD;
            inputs[2].ki.wVk = VK_SPACE;
            inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
            inputs[3].type = INPUT_KEYBOARD;
            inputs[3].ki.wVk = VK_LWIN;
            inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(4, inputs, sizeof(INPUT));

            LOG_DEBUG() << "Switch layout: fallback win+space";
        }
    });

    LOG_DEBUG() << "Switch layout: old='" << hklToLangLabel(current)
                << "' (" << QString::number(reinterpret_cast<qulonglong>(current), 16) << ")"
                << "; new='" << hklToLangLabel(next)
                << "' (" << QString::number(reinterpret_cast<qulonglong>(next), 16) << ")"
                << "; directFallback=" << useDirectFallback;
}
