#pragma once
#include "ui_EasyLangSwitcher_settings_notification.h"
#include <QWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QVector>
#include <QTimer>

class SettingsWindow;

class SaveNotification final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)
    Q_PROPERTY(QPoint pos READ pos WRITE move)

public:
    explicit SaveNotification(SettingsWindow *settings, const QString &text);

    static void showFor(SettingsWindow *settings, const QString &text);

    static QVector<SaveNotification *> stack;

    [[nodiscard]] qreal progress() const { return m_progress; }

    void setProgress(const qreal v) {
        m_progress = v;
        update();
    }

protected:
    bool eventFilter(QObject *o, QEvent *e) override;

    void paintEvent(QPaintEvent *e) override;

private:
    void setupAnimations();

    void startShowAnimation();

    void startHideAnimation(bool removeFromStack = true);

    void animateTo(int newIndex);

    [[nodiscard]] QPoint basePosition(int index) const;

    Ui_notif_main_widget ui{};
    SettingsWindow *settings = nullptr;

    bool m_closing = false;
    qreal m_progress = 0.0;

    // Анимации
    QParallelAnimationGroup *animGroupIn = nullptr;
    QParallelAnimationGroup *animGroupOut = nullptr;
    QPropertyAnimation *animInPos = nullptr;
    QPropertyAnimation *animInOpacity = nullptr;
    QPropertyAnimation *animOutPos = nullptr;
    QPropertyAnimation *animOutOpacity = nullptr;
    QPropertyAnimation *progressAnim = nullptr;

    QTimer hideTimer;
};
