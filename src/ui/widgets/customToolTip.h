#pragma once
#include "ui_EasyLangSwitcher_settings_tooltip.h"
#include <QWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

class CustomToolTip final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QPoint pos READ pos WRITE move)

public:
    explicit CustomToolTip(QWidget *parent = nullptr);

    void showAt(const QWidget *target, const QString &langKey);

    void hideAnimated();

    void refreshTranslations();

private:
    void updateSize();

    Ui::tooltip_main_widget ui{};
    QString currentLangKey;

    QParallelAnimationGroup *animGroup;
    QPropertyAnimation *posAnim;
    QPropertyAnimation *opAnim;

    bool isClosing = false;
};
