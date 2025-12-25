#pragma once
#include "iconHelper.h"
#include <QPropertyAnimation>
#include <QPushButton>
#include <QPainter>
#include <QPointer>
#include <QMouseEvent>

class NotificationCloseButton final : public QPushButton {
    Q_OBJECT

public:

    ~NotificationCloseButton() override {
        LOG_DEBUG() << "CloseButton DESTROYED:" << this << "for target:" << m_target;
    }

    explicit NotificationCloseButton(QWidget *target)
        : QPushButton(nullptr), m_target(target) {
        setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);

        setFixedSize(32, 32);
        setWindowOpacity(0.0);

        setIcon(IconHelper::loadIcon(":/icons/icons/Close.svg", QColor(175, 175, 175), QSize(20, 20)));
        setStyleSheet("QPushButton { background: transparent; border: none; }");

        m_fadeAnim = new QPropertyAnimation(this, "windowOpacity", this);
        m_fadeAnim->setDuration(150);

        LOG_DEBUG() << "CloseButton CREATED:" << this << "for target:" << m_target;
    }

    void updatePosition() {
        if (m_target.isNull() || !m_target->isVisible()) return;
        // Центрируем 32x32 относительно угла основного окна
        this->move(m_target->x() - 10, m_target->y() - 10);
    }

    void setFade(const bool show) const {
        if (!m_fadeAnim) return;
        const qreal target = show ? 1.0 : 0.0;
        if (qFuzzyCompare(windowOpacity(), target)) return;
        m_fadeAnim->stop();
        m_fadeAnim->setEndValue(target);
        m_fadeAnim->start();
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Рисуем тень (Radial Gradient)
        QRadialGradient shadowGradient(16, 16, 16);
        shadowGradient.setColorAt(0.66, QColor(0, 0, 0, 100));
        shadowGradient.setColorAt(1.0, Qt::transparent);
        painter.setPen(Qt::NoPen);
        painter.setBrush(shadowGradient);
        painter.drawEllipse(0, 3, 32, 32);

        // Сама кнопка
        constexpr QRect btnRect(4, 4, 24, 24);
        const QColor bgColor = isDown() ? QColor(28, 28, 28) : (underMouse() ? QColor(42, 42, 42) : QColor(32, 32, 32));

        painter.setBrush(bgColor);
        painter.setPen(QPen(QColor(255, 255, 255, 50), 1.08));
        painter.drawEllipse(btnRect);

        QPushButton::paintEvent(event);
    }

    void enterEvent(QEnterEvent *event) override {
        setFade(true);
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override {
        if (m_target && !m_target->geometry().contains(QCursor::pos())) {
            setFade(false);
        }
        QPushButton::leaveEvent(event);
    }

private:
    QPointer<QWidget> m_target;
    QPropertyAnimation *m_fadeAnim = nullptr;
};
