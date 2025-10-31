#include "settings.h"
#include <QDebug>

SettingsWindow::SettingsWindow(SettingsData &data, QWidget *parent)
    : QWidget(parent), settings(data) {
    setWindowTitle("Settings");
    setFixedSize(300, 200);

    const auto layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Hotkey (VK code):"));
    keyEdit = new QLineEdit(QString::number(settings.hotkeyVk));
    layout->addWidget(keyEdit);

    layout->addWidget(new QLabel("Short press delay (ms):"));
    delayEdit = new QLineEdit(QString::number(settings.switchDelayMs));
    layout->addWidget(delayEdit);

    autostartBtn = new QPushButton(settings.autostart ? "Autostart: ON" : "Autostart: OFF");
    layout->addWidget(autostartBtn);

    const auto saveBtn = new QPushButton("Save");
    layout->addWidget(saveBtn);

    connect(saveBtn, &QPushButton::clicked, this, &SettingsWindow::onSave);
    connect(autostartBtn, &QPushButton::clicked, this, &SettingsWindow::toggleAutostart);
}

void SettingsWindow::onSave() {
    settings.hotkeyVk = keyEdit->text().toInt();
    settings.switchDelayMs = delayEdit->text().toInt();
    hide();
    qDebug() << "Settings saved";
}

void SettingsWindow::toggleAutostart() const {
    settings.autostart = !settings.autostart;
    autostartBtn->setText(settings.autostart ? "Autostart: ON" : "Autostart: OFF");
}
