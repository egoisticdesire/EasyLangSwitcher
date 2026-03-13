#pragma once
#include "ui_EasyLangSwitcher_settings_tooltip.h"

#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QStringList>
#include <QWidget>

class CustomToolTip final : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QPoint pos READ pos WRITE move)

public:
    explicit CustomToolTip(const QWidget* parent = nullptr);

    void showAt(const QWidget* target, const QString& langKey);

    void showAt(const QWidget* target, const QString& langKey, const QString& arg);

    void showAt(const QWidget* target, const QString& langKey, const QString& arg1, const QString& arg2);

    void hideAnimated();

    void hideNow();

    void refreshTranslations();

private:
    Ui::tooltip_main_widget ui{};
    bool isClosing = false;
    QString currentLangKey;
    QStringList currentLangArgs;
    QParallelAnimationGroup* animGroup = nullptr;

    void updateSize();

    [[nodiscard]] QString resolveText() const;
};
