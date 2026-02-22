#pragma once
#include "iconHelper.h"
#include <QPropertyAnimation>
#include <QPushButton>
#include <QPainter>
#include <QPointer>
#include <QMouseEvent>
#include <QEasingCurve>
#include <QTimer>

class NotificationCloseButton final : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(qreal scale READ scale WRITE setScale)

public:
    ~NotificationCloseButton() override {
        LOG_DEBUG() << "NotificationCloseButton DESTROYED: " << this << " for target: " << m_target;
    }

    explicit NotificationCloseButton(QWidget *target)
        : QPushButton(nullptr), m_target(target) {
        setWindowFlags(
            Qt::ToolTip
            | Qt::FramelessWindowHint
            | Qt::NoDropShadowWindowHint
            | Qt::WindowStaysOnTopHint
            | Qt::WindowDoesNotAcceptFocus
        );
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setFocusPolicy(Qt::NoFocus);
        setCursor(Qt::ArrowCursor);

        setFixedSize(32, 32);
        setWindowOpacity(0.0);
        hide();

        setIcon(IconHelper::loadIcon(":/icons/icons/Close.svg", QColor(175, 175, 175), QSize(20, 20)));
        setStyleSheet("QPushButton { background: transparent; border: none; }");

        m_fadeAnim = new QPropertyAnimation(this, "windowOpacity", this);
        m_fadeAnim->setDuration(250);

        m_scaleAnim = new QPropertyAnimation(this, "scale", this);

        m_monitorTimer = new QTimer(this);
        connect(m_monitorTimer, &QTimer::timeout, this, &NotificationCloseButton::checkMousePosition);
        m_monitorTimer->start(50);

        LOG_DEBUG() << "NotificationCloseButton CREATED: " << this << " for target: " << m_target;
    }

    [[nodiscard]] qreal scale() const { return m_scale; }

    void setScale(const qreal s) {
        m_scale = s;
        update();
    }

    void updatePosition() {
        if (m_target.isNull() || !m_target->isVisible()) return;
        this->move(m_target->x() - 10, m_target->y() - 10);
    }

    void setFade(const bool shouldShow) {
        if (!m_fadeAnim || !m_scaleAnim) return;
        const qreal targetOpacity = shouldShow ? 1.0 : 0.0;
        const qreal targetScale = shouldShow ? 1.0 : 0.0;

        if (shouldShow) {
            setAttribute(Qt::WA_TransparentForMouseEvents, false);
            if (!isVisible()) {
                setWindowOpacity(0.0);
                show();
            }
            raise();
        } else {
            setAttribute(Qt::WA_TransparentForMouseEvents, true);
            if (!isVisible()) return;
        }

        // Если состояние уже соответствует целевому, выходим
        if (qFuzzyCompare(windowOpacity(), targetOpacity)) return;

        m_fadeAnim->stop();
        m_scaleAnim->stop();

        m_fadeAnim->setEndValue(targetOpacity);
        m_scaleAnim->setEndValue(targetScale);

        if (shouldShow) {
            m_scaleAnim->setEasingCurve(QEasingCurve::OutBack);
            m_scaleAnim->setDuration(350);
        } else {
            m_scaleAnim->setEasingCurve(QEasingCurve::InBack);
            m_scaleAnim->setDuration(200);
            m_isPopped = false;
        }

        m_fadeAnim->start();
        m_scaleAnim->start();
    }

    void animatePopIn() {
        if (m_isPopped) return;
        m_isPopped = true;

        m_scale = 0.0;
        update();
        setFade(true);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        painter.translate(rect().center());
        painter.scale(m_scale, m_scale);
        painter.translate(-rect().center());

        // Тень
        QRadialGradient shadowGradient(16, 16, 16);
        shadowGradient.setColorAt(0.66, QColor(0, 0, 0, 100));
        shadowGradient.setColorAt(1.0, Qt::transparent);
        painter.setPen(Qt::NoPen);
        painter.setBrush(shadowGradient);
        painter.drawEllipse(0, 3, 32, 32);

        // Кнопка
        constexpr QRect btnRect(4, 4, 24, 24);
        const QColor bgColor = isDown() ? QColor(28, 28, 28) : (underMouse() ? QColor(42, 42, 42) : QColor(32, 32, 32));

        painter.setBrush(bgColor);
        painter.setPen(QPen(QColor(255, 255, 255, 50), 1.08));
        painter.drawEllipse(btnRect);

        const QIcon::Mode mode = isDown() ? QIcon::Selected : QIcon::Normal;
        icon().paint(&painter, btnRect.adjusted(4, 4, -4, -4), Qt::AlignCenter, mode);
    }

    // События нажатия оставляем для интерактивности, но без анимации scale
    void mousePressEvent(QMouseEvent *event) override { QPushButton::mousePressEvent(event); }
    void mouseReleaseEvent(QMouseEvent *event) override { QPushButton::mouseReleaseEvent(event); }

private:
    [[nodiscard]] bool isPointerOverTargetOrSelf() const {
        if (m_target.isNull()) return false;

        const QPoint globalPos = QCursor::pos();
        const bool overTarget = m_target->geometry().contains(globalPos);
        const bool overSelf = isVisible() && windowOpacity() > 0.01 && geometry().contains(globalPos);
        return overTarget || overSelf;
    }

    void checkMousePosition() {
        if (isPointerOverTargetOrSelf()) {
            // Если мышь зашла, а мы не в процессе показа или уже скрыты
            if ((!isVisible() || windowOpacity() < 0.1) && m_fadeAnim->state() != QAbstractAnimation::Running) {
                animatePopIn();
            }
        } else {
            // Если мышь ушла, а мы еще видны
            if (isVisible() && windowOpacity() > 0.01 && m_fadeAnim->state() != QAbstractAnimation::Running) {
                setFade(false);
            }
        }
    }

    QPointer<QWidget> m_target;
    QPropertyAnimation *m_fadeAnim = nullptr;
    QPropertyAnimation *m_scaleAnim = nullptr;
    qreal m_scale = 1.0;
    bool m_isPopped = false;
    QTimer *m_monitorTimer;
};
