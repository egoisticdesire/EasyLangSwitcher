#pragma once
#include "ui_EasyLangSwitcher_settings_warning.h"
#include <QWidget>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

class KeyHoverWarning final : public QWidget {
    Q_OBJECT

public:
    explicit KeyHoverWarning(QWidget *owner);

    void setText(const QString &text);

    void showNear(const QWidget *anchor);

    void hideNow() const;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QWidget *owner = nullptr;
    QWidget *m_content = nullptr;

    bool m_visible = false;
    QGraphicsOpacityEffect *fx = nullptr;

    QPropertyAnimation *animOpacity = nullptr;
    QPropertyAnimation *animPos = nullptr;

    QPropertyAnimation *animOpacityOut = nullptr;
    QPropertyAnimation *animPosOut = nullptr;

    void hideImmediately();

    Ui_settings_warning_main_widget ui;
};
