#include "animated_selector.h"
#include <QPushButton>
#include <QPropertyAnimation>
#include <QTimer>
#include <QDebug>
#include <qstyle.h>

AnimatedSelector::AnimatedSelector(QWidget *parent)
    : QObject(parent) {
}

void AnimatedSelector::bindToFrame(QFrame *frame, const QString &extraStyle) {
    m_frame = frame;
    if (!m_frame) return;

    m_indicator = new QFrame(m_frame);
    m_indicator->setObjectName("animatedIndicator");

    const QString finalStyle =
            "margin: 1px;"
            "color: rgba(255, 255, 255, 255);"
            "background-color: rgba(255, 255, 255, 15);"
            "border-radius: 8px;"
            + extraStyle;
    m_indicator->setStyleSheet(finalStyle);
    m_indicator->setAttribute(Qt::WA_TransparentForMouseEvents);
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
    if (m_customEdit) {
        connect(m_customEdit, &QLineEdit::textChanged,
                this, &AnimatedSelector::onCustomEditChanged);
    }
}


void AnimatedSelector::initPosition() const {
    if (!m_group || !m_indicator)
        return;

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

    updateButtonColors();
}

void AnimatedSelector::animateToButton(const QAbstractButton *btn) {
    if (!m_indicator || !btn || !m_frame)
        return;

    // если была кастомная строка → очищаем
    if (m_customEdit && !m_customEdit->text().isEmpty()) {
        m_customEdit->clear();
        updateEditStyle();
    }

    QRect endGeom = btn->geometry();
    const QPoint mapped = btn->mapTo(m_frame, QPoint(0, 0));
    endGeom.moveTopLeft(mapped);

    const QRect startGeom = m_indicator->geometry();

    auto *anim = new QPropertyAnimation(m_indicator, "geometry");
    anim->setDuration(400);
    anim->setEasingCurve(QEasingCurve::InOutCubic);
    anim->setStartValue(startGeom);
    anim->setEndValue(endGeom);

    connect(anim, &QPropertyAnimation::finished, this, [this, btn]() {
        m_indicator->raise();
        // обновляем стили после завершения анимации
        const_cast<QAbstractButton *>(btn)->setChecked(true);
        updateButtonColors();
    });

    if (!m_indicator->isVisible()) {
        m_indicator->setGeometry(endGeom);
        m_indicator->show();
        m_indicator->raise();
        const_cast<QAbstractButton *>(btn)->setChecked(true);
        updateButtonColors();
        return;
    }

    anim->start(QPropertyAnimation::DeleteWhenStopped);
}


void AnimatedSelector::onCustomEditChanged(const QString &text) const {
    const bool hasCustom = !text.trimmed().isEmpty();

    updateEditStyle();

    if (hasCustom) {
        // Снять выделение со всех кнопок
        for (auto *b: m_group->buttons()) {
            b->setChecked(false);
        }

        m_indicator->hide();
    } else {
        // Вернуть индикатор и состояние кнопок
        initPosition();
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
