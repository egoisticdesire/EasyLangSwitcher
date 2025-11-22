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

    void setActive(const bool value) { active = value; }

private:
    bool keyPressed = false; // обнаружено нажатие триггера

    bool longPressDetected = false; // длинное удержание клавиши

    bool pendingSwitch = false; // ожидание переключения в окне double-window

    bool suppressedByRapidRepeat = false; // подавление переключения при быстром повторе

    DWORD pressTime = 0; // время последнего нажатия

    DWORD lastDownTime = 0; // время предыдущего нажатия

    static constexpr int fallbackCheckMs = 20; // интервал проверки fallback

    QTimer longPressTimer; // определяет удержание > порога

    QTimer doublePressTimer; // окно для проверки быстрого повторного нажатия

    HHOOK hook = nullptr;

    bool active = true;

    bool altDown = false;

    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    static void switchKeyboardLayout();

    static thread_local KeyboardHandler *instance;
};
