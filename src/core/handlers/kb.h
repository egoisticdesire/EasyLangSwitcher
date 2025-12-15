#pragma once
#include <QObject>
#include <QTimer>
#include <Windows.h>

/*
KeyboardHandler
— обработчик нажатий клавиш
*/

class KeyboardHandler final : public QObject {
    Q_OBJECT

public:
    explicit KeyboardHandler(QObject *parent = nullptr);

    ~KeyboardHandler() override;

    void start();

    void stop();

    void setActive(const bool value) { isActive = value; }

private:
    // state
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

    HHOOK keyboardHook = nullptr;

    // hook
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    // switching
    static void switchKeyboardLayout();

    static void sendWinSpace();

    static thread_local KeyboardHandler *instance;
};
