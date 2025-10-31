#pragma once
#include <QObject>
#include <QTimer>
#include <Windows.h>

struct SettingsData;

class KeyboardHandler final : public QObject {
    Q_OBJECT

public:
    explicit KeyboardHandler(SettingsData &data, QObject *parent = nullptr);

    ~KeyboardHandler() override;

    void start();

    void stop();

private:
    SettingsData &settings;
    bool keyPressed = false;
    bool longPressDetected = false;
    DWORD pressTime = 0;
    QTimer timer;
    HHOOK hook = nullptr;

    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    static void switchKeyboardLayout();

    // Указатель на текущий объект для хука (thread-safe)
    static thread_local KeyboardHandler *instance;
};
