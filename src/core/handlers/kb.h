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
    bool triggerKeyDown = false;

    bool isLongPress = false;

    bool switchPending = false;

    bool rapidRepeatSuppressed = false;

    DWORD pressStartTime = 0;

    DWORD lastTriggerDownTime = 0;

    static constexpr int fallbackCheckMs = 20;

    QTimer longPressTimer;

    QTimer doublePressTimer;

    HHOOK keyboardHook = nullptr;

    bool isActive = true;

    bool isAltDown = false;

    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    static void switchKeyboardLayout();

    static thread_local KeyboardHandler *instance;
};
