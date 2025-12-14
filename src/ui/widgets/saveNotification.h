#pragma once
#include "ui_EasyLangSwitcher_settings_notification.h"
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QVector>
#include <QTimer>

class SettingsWindow;

class SaveNotification final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)

public:
    explicit SaveNotification(SettingsWindow *settings, const QString &text);

    static void showFor(SettingsWindow *settings, const QString &text);

    static QVector<SaveNotification *> stack;

    qreal progress() const { return m_progress; }

    void setProgress(const qreal v) {
        m_progress = v;
        update();
    }

protected:
    bool eventFilter(QObject *o, QEvent *e) override;

    void paintEvent(QPaintEvent *e) override;

private:
    void startShowAnimation();

    void startHideAnimation(bool removeFromStack = true);

    void animateTo(int newIndex);

    bool m_closing = false;

    qreal m_progress = 0.0;

    QWidget *m_content = nullptr; // контейнер, который содержит текст+иконку в .ui

    void shiftUp(int dy); // анимация сдвига вверх (для стека)
    QPoint basePosition(int index) const;

    Ui_notif_main_widget ui;
    SettingsWindow *settings = nullptr;

    QGraphicsOpacityEffect *fx = nullptr;
    QPropertyAnimation *animInOpacity = nullptr;
    QPropertyAnimation *animInPos = nullptr;
    QPropertyAnimation *animOutOpacity = nullptr;
    QPropertyAnimation *animOutPos = nullptr;

    QTimer hideTimer;
};
