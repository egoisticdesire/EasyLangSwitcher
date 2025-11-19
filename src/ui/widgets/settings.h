#pragma once
#include "../../ui/helpers/acrylicHelper.h"
#include "ui_EasyLangSwitcher_settings.h"
#include "animated_selector.h"
#include "window_dragger.h"
#include <QVector>

class SettingsWindow final : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = nullptr);

    ~SettingsWindow() override;

    void openCentered();

protected:
    void showEvent(QShowEvent *event) override;

    bool event(QEvent *ev) override;

private:
    Ui::main_widget ui{};

    QVector<AnimatedSelector *> selectors;

    WindowDragger *dragger = nullptr;

    void addSelectorForFrame(QFrame *frame, const QString &extraStyle = QString());
};
