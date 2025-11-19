#include "animated_selector.h"
#include <QPushButton>
#include <QPropertyAnimation>
#include <QTimer>
#include <QDebug>

AnimatedSelector::AnimatedSelector(QWidget *parent) : QObject(parent) {
}

void AnimatedSelector::bindToFrame(QFrame *frame, const QString &extraStyle) {
    m_frame = frame;
    if (!m_frame) return;

    m_indicator = new QFrame(m_frame);
    m_indicator->setObjectName("animatedIndicator");

    // БАЗОВЫЙ стиль (всегда активен)
    const QString baseStyle =
            "margin: 1px;"
            "color: rgba(255, 255, 255, 255);"
            "background-color: rgba(255, 255, 255, 15);"
            "border-radius: 8px;";

    // ЕСЛИ пользователь передал extraStyle → ДОБАВЛЯЕМ, а не заменяем
    QString finalStyle = baseStyle;
    if (!extraStyle.isEmpty())
        finalStyle += extraStyle;

    m_indicator->setStyleSheet(finalStyle);

    m_indicator->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_indicator->hide();

    // Кнопки
    QList<QPushButton *> buttons = m_frame->findChildren<QPushButton *>();
    for (auto *b: buttons) b->setCheckable(true);

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);
    for (auto *b: buttons) m_group->addButton(b);

    connect(m_group, &QButtonGroup::buttonClicked,
            this, &AnimatedSelector::animateToButton);
}


void AnimatedSelector::initPosition() const {
    if (!m_group || !m_indicator)
        return;

    const QAbstractButton *btn = nullptr;

    // ищем checked
    for (const auto *b: m_group->buttons()) {
        if (b->isChecked()) {
            btn = b;
            break;
        }
    }
    if (!btn && !m_group->buttons().isEmpty())
        btn = m_group->buttons().first();

    if (!btn)
        return;

    // вычисляем геометрию кнопки в координатах фрейма
    QRect g = btn->geometry();
    const QPoint mapped = btn->mapTo(m_frame, QPoint(0, 0));
    g.moveTopLeft(mapped);

    m_indicator->setGeometry(g);
    m_indicator->show();
    m_indicator->raise();
}

void AnimatedSelector::animateToButton(const QAbstractButton *btn) {
    if (!m_indicator || !btn || !m_frame)
        return;

    QRect endGeom = btn->geometry();
    const QPoint mapped = btn->mapTo(m_frame, QPoint(0, 0));
    endGeom.moveTopLeft(mapped);

    const QRect startGeom = m_indicator->geometry();

    if (!m_indicator->isVisible()) {
        m_indicator->setGeometry(endGeom);
        m_indicator->show();
        m_indicator->raise();
        return;
    }

    auto *anim = new QPropertyAnimation(m_indicator, "geometry");
    anim->setDuration(400);
    anim->setEasingCurve(QEasingCurve::InOutCubic);
    anim->setStartValue(startGeom);
    anim->setEndValue(endGeom);

    connect(anim, &QPropertyAnimation::finished, this, [this]() {
        m_indicator->raise();
    });

    anim->start(QPropertyAnimation::DeleteWhenStopped);
}
