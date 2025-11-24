#include "animated_selector.h"
#include "../../core/config/logger.h"
#include <QPushButton>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QTimer>
#include <QStyle>

AnimatedSelector::AnimatedSelector(QWidget *parent)
    : QObject(parent) {
}

void AnimatedSelector::bindToFrame(QFrame *frame, const QString &extraStyle) {
    m_frame = frame;
    if (!m_frame) return;

    LOG_DEBUG() << "Bound to frame: " << m_frame->objectName();

    m_indicator = new QFrame(m_frame);
    m_indicator->setObjectName("animatedIndicator");

    const QString finalStyle =
            "margin: 1px;"
            "border-radius: 8px;"
            "color: rgba(255, 255, 255, 255);"
            "background: rgba(255, 255, 255, 15);"
            + extraStyle;
    m_indicator->setStyleSheet(finalStyle);
    m_indicator->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_opacity = new QGraphicsOpacityEffect(m_indicator);
    m_indicator->setGraphicsEffect(m_opacity);
    m_opacity->setOpacity(1.0);
    m_indicator->hide();

    // кнопки
    QList<QPushButton *> buttons = m_frame->findChildren<QPushButton *>();
    for (auto *b: buttons) b->setCheckable(true);

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);
    for (auto *b: buttons)
        m_group->addButton(b);

    connect(m_group, &QButtonGroup::buttonClicked,
            this, &AnimatedSelector::animateToButton);

    m_customEdit = m_frame->findChild<QLineEdit *>();

    LOG_DEBUG() << "Found buttons: " << buttons.size() << "; found custom edit: " << (m_customEdit != nullptr);

    if (m_customEdit) {
        connect(m_customEdit, &QLineEdit::textChanged,
                this, &AnimatedSelector::onCustomEditChanged);
    }
}


void AnimatedSelector::initPosition() const {
    if (!m_group || !m_indicator) return;

    // Если кастомный текст есть — индикатор не нужен
    if (m_customEdit && !m_customEdit->text().trimmed().isEmpty()) {
        m_indicator->hide();
        updateButtonColors();
        return;
    }

    const QAbstractButton *btn = nullptr;

    // ищем выбранную кнопку
    for (const auto *b: m_group->buttons()) {
        if (b->isChecked()) {
            btn = b;
            break;
        }
    }
    if (!btn && !m_group->buttons().isEmpty()) btn = m_group->buttons().first();

    if (!btn) return;

    // вычисляем геометрию кнопки в координатах фрейма
    QRect g = btn->geometry();
    const QPoint mapped = btn->mapTo(m_frame, QPoint(0, 0));
    g.moveTopLeft(mapped);

    m_indicator->setGeometry(g);
    m_indicator->show();
    m_indicator->raise();

    updateButtonColors();
}

void AnimatedSelector::animateToButton(const QAbstractButton *btn) {
    if (!m_indicator || !btn || !m_frame) return;

    if (m_customEdit && !m_customEdit->text().isEmpty()) {
        m_customEdit->clear();
        updateEditStyle();
    }

    // Целевая геометрия кнопки
    QRect endGeom = btn->geometry();
    const QPoint mapped = btn->mapTo(m_frame, QPoint(0, 0));
    endGeom.moveTopLeft(mapped);

    LOG_DEBUG() << "Animating to button '" << btn->objectName()
            << "' at position: " << QString("(%1, %2)").arg(endGeom.x()).arg(endGeom.y());

    QRect startGeom = m_indicator->geometry();

    // Если индикатор был скрыт — начинаем с нулевой прозрачности и позиции кастома
    if (!m_indicator->isVisible()) {
        if (m_customEdit) {
            QRect from = m_customEdit->geometry();
            const QPoint fromMapped = m_customEdit->mapTo(m_frame, QPoint(0, 0));
            from.moveTopLeft(fromMapped);
            startGeom = from;
        }
        m_indicator->setGeometry(startGeom);
        m_opacity->setOpacity(0.0);
        m_indicator->show();
        m_indicator->raise();
    }

    auto *moveAnim = new QPropertyAnimation(m_indicator, "geometry");
    moveAnim->setDuration(400);
    moveAnim->setEasingCurve(QEasingCurve::InOutCubic);
    moveAnim->setStartValue(startGeom);
    moveAnim->setEndValue(endGeom);

    // Плавное появление
    auto *fadeAnim = new QPropertyAnimation(m_opacity, "opacity");
    fadeAnim->setDuration(400);
    fadeAnim->setStartValue(m_opacity->opacity());
    fadeAnim->setEndValue(1.0);

    auto *group = new QParallelAnimationGroup;
    group->addAnimation(moveAnim);
    group->addAnimation(fadeAnim);

    connect(group, &QParallelAnimationGroup::finished, this, [this, btn]() {
        const_cast<QAbstractButton *>(btn)->setChecked(true);
        updateButtonColors();
        m_indicator->raise();
    });

    group->start(QPropertyAnimation::DeleteWhenStopped);
}

void AnimatedSelector::animateToCustomEdit() {
    if (!m_indicator || !m_customEdit || !m_frame)
        return;

    QRect endGeom = m_customEdit->geometry();
    const QPoint mapped = m_customEdit->mapTo(m_frame, QPoint(0, 0));
    endGeom.moveTopLeft(mapped);

    const QRect startGeom = m_indicator->geometry();

    auto *moveAnim = new QPropertyAnimation(m_indicator, "geometry");
    moveAnim->setDuration(400);
    moveAnim->setEasingCurve(QEasingCurve::InOutCubic);
    moveAnim->setStartValue(startGeom);
    moveAnim->setEndValue(endGeom);

    auto *fadeAnim = new QPropertyAnimation(m_opacity, "opacity");
    fadeAnim->setDuration(400);
    fadeAnim->setStartValue(1.0);
    fadeAnim->setEndValue(0.0);

    // Группируем параллельно
    auto *group = new QParallelAnimationGroup;
    group->addAnimation(moveAnim);
    group->addAnimation(fadeAnim);

    connect(group, &QParallelAnimationGroup::finished, this, [this]() {
        m_indicator->hide();
        m_opacity->setOpacity(1.0);
        updateButtonColors();
    });

    if (!m_indicator->isVisible()) {
        m_indicator->setGeometry(startGeom);
        m_opacity->setOpacity(1.0);
        m_indicator->show();
    }

    group->start(QPropertyAnimation::DeleteWhenStopped);
}


void AnimatedSelector::onCustomEditChanged(const QString &text) {
    const bool hasCustom = !text.trimmed().isEmpty();

    updateEditStyle();

    if (hasCustom) {
        // Снять выделение со всех кнопок
        for (auto *b: m_group->buttons()) b->setChecked(false);

        animateToCustomEdit();

        LOG_DEBUG() << "Indicator animated to input and hidden";
    } else {
        // Вернуть индикатор и состояние кнопок
        if (const auto *btn = qobject_cast<QAbstractButton *>(m_group->checkedButton())) {
            animateToButton(btn);
        }

        LOG_DEBUG() << "Indicator shown";
    }

    updateButtonColors();
}

void AnimatedSelector::updateButtonColors() const {
    if (!m_group) return;

    const bool hasCustom = m_customEdit && !m_customEdit->text().trimmed().isEmpty();
    for (auto *b: m_group->buttons()) {
        if (hasCustom) {
            b->setChecked(false); // снимаем выделение
            b->setProperty("customActive", true);
        } else {
            b->setProperty("customActive", false);
        }
        b->style()->unpolish(b);
        b->style()->polish(b);
    }
}

void AnimatedSelector::updateEditStyle() const {
    if (!m_customEdit) return;

    const bool hasText = !m_customEdit->text().trimmed().isEmpty();
    m_customEdit->setProperty("hasText", hasText);
    m_customEdit->style()->unpolish(m_customEdit);
    m_customEdit->style()->polish(m_customEdit);
}
