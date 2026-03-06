#include "kb.h"

#include "../config/appSettings.h"
#include "../config/logger.h"

#include <QMetaObject>
#include <QThread>
#include <QTimer>

#include <Windows.h>

#include <array>
#include <bit>
#include <cstdint>
#include <unordered_set>
#include <utility>

namespace
{
QString hklToLangLabel(const HKL hkl)
{
    const LANGID langId = LOWORD(reinterpret_cast<DWORD_PTR>(hkl));
    std::array<WCHAR, LOCALE_NAME_MAX_LENGTH> name{};
    if (LCIDToLocaleName(MAKELCID(langId, SORT_DEFAULT), name.data(), static_cast<int>(name.size()), 0)) {
        return QString::fromWCharArray(name.data());
    }
    return {"Unknown"};
}

} // namespace

class KeyboardHookWorker final : public QObject
{
public:
    KeyboardHookWorker()
    {
        longPressTimer.setParent(this);
        doublePressTimer.setParent(this);

        longPressTimer.setSingleShot(true);
        connect(&longPressTimer, &QTimer::timeout, this, [this]() {
            isLongPress = true;
            switchPending = false;
        });

        doublePressTimer.setSingleShot(true);
        connect(&doublePressTimer, &QTimer::timeout, this, [this]() {
            if (switchPending && !isLongPress && !rapidRepeatSuppressed) {
                switchKeyboardLayout();
            }

            switchPending = false;
            rapidRepeatSuppressed = false;
        });
    }

    ~KeyboardHookWorker() override
    {
        stopHook();
    }

    void setActive(const bool value)
    {
        isActive = value;
    }

    void startHook()
    {
        if (keyboardHook != nullptr) {
            return;
        }

        instance = this;
        downKeys.clear();
        resetState();

        keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0);
        if (keyboardHook == nullptr) {
            LOG_WARNING() << "Hook installation failed";
        }
        else {
            LOG_DEBUG() << "Hook installed successfully on dedicated thread";
        }
    }

    void stopHook()
    {
        longPressTimer.stop();
        doublePressTimer.stop();
        downKeys.clear();
        resetState();

        if (keyboardHook != nullptr) {
            UnhookWindowsHookEx(keyboardHook);
            keyboardHook = nullptr;
            LOG_DEBUG() << "Hook removed";
        }

        if (instance == this) {
            instance = nullptr;
        }
    }

private:
    static LRESULT CALLBACK LowLevelKeyboardProc(const int nCode, const WPARAM wParam, const LPARAM lParam)
    {
        auto* const self = instance;
        if (nCode != HC_ACTION || self == nullptr || !self->isActive) {
            return CallNextHookEx(nullptr, nCode, wParam, lParam);
        }

        const auto* const kb = std::bit_cast<KBDLLHOOKSTRUCT*>(static_cast<std::uintptr_t>(lParam));
        const bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        const bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
        const int vk = static_cast<int>(kb->vkCode);

        if (vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU) {
            if (isDown) {
                self->isAltDown = true;
            }
            if (isUp) {
                self->isAltDown = false;
            }
        }

        if (isDown) {
            self->downKeys.insert(vk);
        }
        if (isUp) {
            self->downKeys.erase(vk);
        }

        const int configuredMain = AppSettings::hotkeyMainVk;
        const DWORD now = GetTickCount();

        if (isDown && vk == configuredMain) {
            bool otherKeyDown = false;
            for (const int downVk : self->downKeys) {
                if (downVk == vk) {
                    continue;
                }

                otherKeyDown = true;
                break;
            }

            if (otherKeyDown) {
                self->cancelTriggerState();
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
            }

            const DWORD intervalSinceLastDown = now - self->lastTriggerDownTime;
            const int doublePress = AppSettings::switchDelayMs;

            if (const auto doublePressMs = static_cast<DWORD>(doublePress);
                std::cmp_less_equal(intervalSinceLastDown, doublePressMs)) {
                self->rapidRepeatSuppressed = true;
                self->switchPending = false;
                self->isLongPress = false;
                self->longPressTimer.stop();
                self->doublePressTimer.stop();
                self->triggerKeyDown = false;
                self->lastTriggerDownTime = now;
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
            }

            self->triggerKeyDown = true;
            self->isLongPress = false;
            self->rapidRepeatSuppressed = false;
            self->pressStartTime = now;
            self->switchPending = false;

            self->longPressTimer.start(AppSettings::switchDelayMs);
            self->doublePressTimer.start(doublePress);
            self->lastTriggerDownTime = now;

            if (configuredMain == VK_CAPITAL) {
                return 1;
            }
            return CallNextHookEx(nullptr, nCode, wParam, lParam);
        }

        if (isUp && vk == configuredMain) {
            if (!self->triggerKeyDown) {
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
            }

            self->longPressTimer.stop();

            if (self->isLongPress) {
                self->triggerKeyDown = false;
                self->switchPending = false;
                self->rapidRepeatSuppressed = false;
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
            }

            if (!self->doublePressTimer.isActive()) {
                if (!self->rapidRepeatSuppressed) {
                    self->switchKeyboardLayout();
                }

                self->triggerKeyDown = false;
                self->switchPending = false;
                self->rapidRepeatSuppressed = false;

                if (configuredMain == VK_CAPITAL) {
                    return 1;
                }
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
            }

            self->switchPending = true;
            self->triggerKeyDown = false;

            if (configuredMain == VK_CAPITAL) {
                return 1;
            }
            return CallNextHookEx(nullptr, nCode, wParam, lParam);
        }

        if (isDown && self->triggerKeyDown && vk != configuredMain) {
            self->cancelTriggerState();
            return CallNextHookEx(nullptr, nCode, wParam, lParam);
        }

        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    void cancelTriggerState()
    {
        triggerKeyDown = false;
        switchPending = false;
        isLongPress = false;
        rapidRepeatSuppressed = false;
        longPressTimer.stop();
        doublePressTimer.stop();
    }

    void resetState()
    {
        isAltDown = false;
        triggerKeyDown = false;
        isLongPress = false;
        switchPending = false;
        rapidRepeatSuppressed = false;
        pressStartTime = 0;
        lastTriggerDownTime = 0;
    }

    void switchKeyboardLayout() const
    {
        const HWND hwnd = GetForegroundWindow();
        if (hwnd == nullptr) {
            return;
        }

        const DWORD threadId = GetWindowThreadProcessId(hwnd, nullptr);
        const HKL before = GetKeyboardLayout(threadId);

        sendWinSpace();

        QTimer::singleShot(20, this, [threadId, before]() {
            const HKL after = GetKeyboardLayout(threadId);

            LOG_DEBUG() << QString("Layout switched: '%1' (%2) ? '%3' (%4)")
                                   .arg(hklToLangLabel(before),
                                        QString::number(reinterpret_cast<qulonglong>(before), 16),
                                        hklToLangLabel(after),
                                        QString::number(reinterpret_cast<qulonglong>(after), 16));
        });
    }

    static void sendWinSpace()
    {
        std::array<INPUT, 4> in{};

        in.at(0).type = INPUT_KEYBOARD;
        in.at(0).ki.wVk = VK_LWIN;

        in.at(1).type = INPUT_KEYBOARD;
        in.at(1).ki.wVk = VK_SPACE;

        in.at(2).type = INPUT_KEYBOARD;
        in.at(2).ki.wVk = VK_SPACE;
        in.at(2).ki.dwFlags = KEYEVENTF_KEYUP;

        in.at(3).type = INPUT_KEYBOARD;
        in.at(3).ki.wVk = VK_LWIN;
        in.at(3).ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput(static_cast<UINT>(in.size()), in.data(), sizeof(INPUT));
    }

    bool isActive = true;
    bool isAltDown = false;
    bool triggerKeyDown = false;
    bool isLongPress = false;
    bool switchPending = false;
    bool rapidRepeatSuppressed = false;

    DWORD pressStartTime = 0;
    DWORD lastTriggerDownTime = 0;

    QTimer longPressTimer;
    QTimer doublePressTimer;
    std::unordered_set<int> downKeys;
    HHOOK keyboardHook = nullptr;

    static thread_local KeyboardHookWorker* instance;
};

thread_local KeyboardHookWorker* KeyboardHookWorker::instance = nullptr;

KeyboardHandler::KeyboardHandler(QObject* parent) : QObject(parent)
{
    LOG_DEBUG() << "KeyboardHandler initialized";
}

KeyboardHandler::~KeyboardHandler()
{
    stop();
}

void KeyboardHandler::start()
{
    if (hookThread != nullptr) {
        return;
    }

    hookThread = new QThread();
    worker = new KeyboardHookWorker();
    worker->moveToThread(hookThread);

    hookThread->start();

    QMetaObject::invokeMethod(
            worker,
            [worker = worker, active = isActive]() {
                worker->setActive(active);
                worker->startHook();
            },
            Qt::BlockingQueuedConnection);
}

void KeyboardHandler::stop()
{
    KeyboardHookWorker* workerToDelete = worker;
    worker = nullptr;

    if (workerToDelete != nullptr) {
        QMetaObject::invokeMethod(
                workerToDelete,
                [workerToDelete]() {
                    workerToDelete->stopHook();
                    delete workerToDelete;
                },
                Qt::BlockingQueuedConnection);
    }

    if (hookThread != nullptr) {
        hookThread->quit();
        hookThread->wait();
        delete hookThread;
        hookThread = nullptr;
    }
}

void KeyboardHandler::setActive(const bool value)
{
    isActive = value;

    if (worker != nullptr) {
        auto* const workerTarget = worker;
        QMetaObject::invokeMethod(
                workerTarget, [workerTarget, value]() { workerTarget->setActive(value); }, Qt::QueuedConnection);
    }
}
