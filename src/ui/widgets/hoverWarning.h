#pragma once
#include "ui_EasyLangSwitcher_settings_warning.h"
#include <QWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

class KeyHoverWarning final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QPoint pos READ pos WRITE move)

public:
    explicit KeyHoverWarning(QWidget *owner);

    void setText(const QString &text);

    void showNear(const QWidget *anchor);

    void hideNow();

    void hideImmediately();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setupAnimations();

    Ui_settings_warning_main_widget ui{};
    QWidget *owner = nullptr;
    QWidget *m_content = nullptr;

    bool m_visible = false;
    bool m_isClosing = false;

    QParallelAnimationGroup *animGroupIn = nullptr;
    QParallelAnimationGroup *animGroupOut = nullptr;

    QPropertyAnimation *animPosIn = nullptr;
    QPropertyAnimation *animOpacityIn = nullptr;

    QPropertyAnimation *animPosOut = nullptr;
    QPropertyAnimation *animOpacityOut = nullptr;
};
