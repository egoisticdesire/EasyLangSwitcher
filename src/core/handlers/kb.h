#pragma once
#include <QObject>
#include <QTimer>
#include <Windows.h>

class KeyboardHandler final : public QObject {
    Q_OBJECT

public:
    explicit KeyboardHandler(QObject *parent = nullptr);

    ~KeyboardHandler() override;

    void start();

    void stop();

    void setActive(const bool value) { active = value; }

private:
    bool keyPressed = false;
    bool longPressDetected = false;
    DWORD pressTime = 0;
    QTimer timer;
    HHOOK hook = nullptr;
    bool active = true;

    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    static void switchKeyboardLayout();

    static thread_local KeyboardHandler *instance;
};
