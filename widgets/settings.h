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

private:
    SettingsData &settings;
    QLineEdit *keyEdit;
    QLineEdit *delayEdit;
    QPushButton *autostartBtn;

    void onSave();

    void toggleAutostart() const;
};
