#pragma once
#include <QTimer>

#include "ui_EasyLangSwitcher_settings_notification.h"

#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QVector>
#include <QWidget>

#include <cstdint>

class SettingsWindow;

class InAppNotification final : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(InAppNotification)
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)
    Q_PROPERTY(QPoint pos READ pos WRITE move)

public:
    ~InAppNotification() override = default;

    enum class Type : std::uint8_t
    {
        Success,
        Error,
        Info,
        Warning
    };

    explicit InAppNotification(SettingsWindow* settings, const QString& text, Type type = Type::Success);

    static void showFor(SettingsWindow* settings, const QString& text, Type type = Type::Success);

    static QVector<InAppNotification*> stack;

    [[nodiscard]] qreal progress() const
    {
        return m_progress;
    }

    void setProgress(const qreal v)
    {
        m_progress = v;
        update();
    }

protected:
    bool eventFilter(QObject* o, QEvent* e) override;

    void paintEvent(QPaintEvent* e) override;

private:
    void startShowAnimation();

    void startHideAnimation(bool removeFromStack = true);

    void animateTo(int newIndex);

    [[nodiscard]] QPoint basePosition(int index) const;

    Ui_notif_main_widget ui{};
    SettingsWindow* settings = nullptr;

    bool m_closing = false;
    qreal m_progress = 0.0;
    Type m_type;

    QParallelAnimationGroup* animGroupIn = nullptr;
    QParallelAnimationGroup* animGroupOut = nullptr;
    QPropertyAnimation* progressAnim = nullptr;

    QTimer hideTimer;
};
