#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <Windows.h>

struct SettingsData {
    int switchDelayMs = 300;
    int hotkeyVk = VK_LCONTROL;
    bool autostart = false;
    QString uiLanguage = "en";
};

class SettingsWindow final : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWindow(SettingsData &data, QWidget *parent = nullptr);
    QString getHotkeyName() const {
        // Преобразуем VK код в читаемое имя (например, только для Ctrl)
        return settings.hotkeyVk == VK_LCONTROL ? "Left Ctrl" : QString("VK %1").arg(settings.hotkeyVk);
    }
    int getSwitchDelayMs() const { return settings.switchDelayMs; }

private:
    SettingsData &settings;
    QLineEdit *keyEdit;
    QLineEdit *delayEdit;
    QPushButton *autostartBtn;

    void onSave();

    void toggleAutostart() const;
};
