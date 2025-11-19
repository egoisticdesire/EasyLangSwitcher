#pragma once

#include "ui_EasyLangSwitcher_settings.h"
#include <QPoint>

class SettingsWindow : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = nullptr);

    ~SettingsWindow() override;

    void openCentered();

protected:
    void mousePressEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void showEvent(QShowEvent *event) override;

    bool event(QEvent *ev) override;

private:
    Ui::main_widget ui{};

    bool dragging = false;
    QPoint dragStartPos;
};
