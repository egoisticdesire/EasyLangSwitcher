#include "animated_selector.h"
#include "../../core/config/logger.h"
#include <QPushButton>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QTimer>
#include <QStyle>
#include <QLineF>
#include <QGraphicsDropShadowEffect>
#include <QPointer>
#include <cmath>

AnimatedSelector::AnimatedSelector(QWidget *parent)
    : QObject(parent), m_parent(parent) {
}

void AnimatedSelector::bindToFrame(QFrame *frame, const QString &extraStyle) {
    m_frame = frame;
    if (!m_frame) return;

    LOG_DEBUG() << "Bound to frame: " << m_frame->objectName();

    m_indicator = new QFrame(m_parent);
    m_indicator->setObjectName("animatedIndicator");
    const QString finalStyle =
            "margin: 1px;"
            "border-radius: 8px;"
            "color: rgba(255, 255, 255, 255);"
            "background: rgba(255, 255, 255, 15);"
            + extraStyle;
    m_indicator->setStyleSheet(finalStyle);
    m_indicator->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_indicator->hide();

    m_shadow = new QGraphicsDropShadowEffect(m_indicator);
    m_shadow->setBlurRadius(6);
    m_shadow->setOffset(0, 4);
    m_shadow->setColor(QColor(0, 0, 0, 0));
    m_indicator->setGraphicsEffect(m_shadow);

    QList<QPushButton *> buttons = m_frame->findChildren<QPushButton *>();
    for (auto *b: buttons) b->setCheckable(true);

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);
    for (auto *b: buttons)
        m_group->addButton(b);

    connect(m_group, &QButtonGroup::buttonClicked,
            this, &AnimatedSelector::animateToButton);

    m_customEdit = m_frame->findChild<QLineEdit *>();
    if (m_customEdit)
        connect(m_customEdit, &QLineEdit::textChanged,
                this, &AnimatedSelector::onCustomEditChanged);
}

void AnimatedSelector::initPosition() {
    if (!m_group || !m_indicator) return;

    // Если инпут с текстом — индикатор не показываем, но сохраняем геометрию фрейма
    if (m_customEdit && !m_customEdit->text().trimmed().isEmpty()) {
        m_indicator->hide();
        const auto frameGeom = QRect(
            m_frame->mapTo(m_parent, QPoint(0, 0)),
            QSize(m_frame->size().width() - 18, m_frame->size().height() + 2)
        );
        m_indicatorGeometry = frameGeom; // <<< сохраняем геометрию
        updateButtonColors();
        return;
    }

    const QAbstractButton *btn = nullptr;
    for (const auto *b: m_group->buttons()) {
        if (b->isChecked()) {
            btn = b;
            break;
        }
    }
    if (!btn && !m_group->buttons().isEmpty()) btn = m_group->buttons().first();
    if (!btn) return;

    QRect g = btn->geometry();
    g.moveTopLeft(btn->mapTo(m_parent, QPoint(0, 0)));
    m_indicator->setGeometry(g);
    m_indicator->show();
    m_indicator->raise();

    m_indicatorGeometry = g;
    updateButtonColors();
}

void AnimatedSelector::animateToButton(QAbstractButton *btn) {
    if (!m_indicator || !btn || !m_parent || !m_frame) return;

    if (m_customEdit && !m_customEdit->text().trimmed().isEmpty()) {
        m_customEdit->clear();
        updateEditStyle();
    }

    QRect endGeom = btn->geometry();
    endGeom.moveTopLeft(btn->mapTo(m_parent, QPoint(0, 0)));

    QRect startGeom;
    if (m_indicator->isVisible() && m_indicator->geometry().isValid())
        startGeom = m_indicator->geometry();
    else if (m_indicatorGeometry.isValid())
        startGeom = m_indicatorGeometry;
    else
        startGeom = endGeom;

    if (!m_indicator->isVisible()) {
        m_indicator->setGeometry(startGeom);
        m_indicator->show();
        m_indicator->raise();
    }

    const QPointF startCenter = startGeom.center();
    const QPointF endCenter = endGeom.center();
    const QPointF delta = endCenter - startCenter;
    const double distance = QLineF(startCenter, endCenter).length();
    const bool horizontal = std::abs(delta.x()) >= std::abs(delta.y());

    if (m_runningAnim) {
        m_runningAnim->stop();
        m_runningAnim->deleteLater();
        m_runningAnim = nullptr;
    }

    const QPointer indicatorPtr(m_indicator);
    const QPointer shadowPtr(m_shadow);

    auto *anim = new QVariantAnimation(this);
    m_runningAnim = anim;
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    const int dur = static_cast<int>(qBound(300.0, 500.0 + distance * 0.5, 700.0));
    anim->setDuration(dur);
    anim->setEasingCurve(QEasingCurve::InOutCubic);

    connect(anim, &QVariantAnimation::valueChanged, this, [=](const QVariant &v) {
        if (!indicatorPtr) return;
        const double t = v.toDouble();

        const QPointF center = startCenter + delta * t;

        constexpr double squashMid = 0.7;
        const double squashFactor = 1.0 - (1.0 - squashMid) * std::sin(t * M_PI);
        const double stretchFactor = 1.0 + (1.0 / squashMid - 1.0) * (1.0 - std::abs(2.0 * t - 1.0));

        const double w = startGeom.width() + (endGeom.width() - startGeom.width()) * t;
        const double h = startGeom.height() + (endGeom.height() - startGeom.height()) * t;

        const double newW = horizontal ? w * squashFactor : w * stretchFactor;
        const double newH = horizontal ? h * stretchFactor : h * squashFactor;

        const QRectF r(center.x() - newW / 2.0, center.y() - newH / 2.0, newW, newH);
        indicatorPtr->setGeometry(r.toAlignedRect());
        indicatorPtr->raise();

        // плавный радиус скругления
        // радиус при перемещении между кнопками всегда 8
        const auto style = QString(
            "margin: 1px;"
            "border-radius: 8px;"
            "color: rgba(255,255,255,255);"
            "background: rgba(255,255,255,15);"
        );
        indicatorPtr->setStyleSheet(style);

        if (shadowPtr) {
            const double sp = std::sin(t * M_PI);
            shadowPtr->setBlurRadius(6 + 6 * sp);
            shadowPtr->setOffset(0, 4 + 6 * sp);
            shadowPtr->setColor(QColor(0, 0, 0, static_cast<int>(120 + 120 * sp)));
        }
    });

    connect(anim, &QVariantAnimation::finished, this, [this, indicatorPtr, shadowPtr, btn, endGeom, anim]() {
        if (!indicatorPtr) return;
        indicatorPtr->setGeometry(endGeom);
        indicatorPtr->raise();
        btn->setChecked(true);
        updateButtonColors();
        m_indicatorGeometry = endGeom;

        if (shadowPtr) {
            shadowPtr->setBlurRadius(6);
            shadowPtr->setOffset(0, 4);
            shadowPtr->setColor(QColor(0, 0, 0, 0));
        }
        if (m_runningAnim == anim) m_runningAnim = nullptr;
        anim->deleteLater();
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void AnimatedSelector::animateToCustomEdit() {
    if (!m_indicator || !m_customEdit || !m_frame || !m_parent) return;

    QRect startGeom;
    if (m_indicator->isVisible() && m_indicator->geometry().isValid())
        startGeom = m_indicator->geometry();
    else if (m_indicatorGeometry.isValid())
        startGeom = m_indicatorGeometry;
    else {
        if (const auto *b = m_group ? m_group->checkedButton() : nullptr) {
            startGeom = b->geometry();
            startGeom.moveTopLeft(b->mapTo(m_parent, QPoint(0, 0)));
        } else {
            startGeom = QRect(m_frame->mapTo(m_parent, QPoint(0, 0)),
                              QSize(m_frame->size().width() - 18, m_frame->size().height() + 2));
        }
    }

    const auto frameGeom = QRect(m_frame->mapTo(m_parent, QPoint(0, 0)),
                                 QSize(m_frame->size().width() - 18, m_frame->size().height() + 2));

    if (!m_indicator->isVisible()) {
        m_indicator->setGeometry(startGeom);
        m_indicator->show();
        m_indicator->raise();
    }

    if (m_runningAnim) {
        m_runningAnim->stop();
        m_runningAnim->deleteLater();
        m_runningAnim = nullptr;
    }

    const QPointer indicatorPtr(m_indicator);
    const QPointer shadowPtr(m_shadow);

    auto *anim = new QVariantAnimation(this);
    m_runningAnim = anim;
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setDuration(500);
    anim->setEasingCurve(QEasingCurve::InOutCubic);

    const QPointF cStart = startGeom.center();
    const QPointF cEnd = frameGeom.center();
    const QPointF delta = cEnd - cStart;

    connect(anim, &QVariantAnimation::valueChanged, this,
            [this, indicatorPtr, shadowPtr, cStart, delta, startGeom, frameGeom](const QVariant &v) {
                if (!indicatorPtr) return;
                const double t = v.toDouble();

                const QPointF center = cStart + delta * t;
                const double w = startGeom.width() + (frameGeom.width() - startGeom.width()) * t;
                const double h = startGeom.height() + (frameGeom.height() - startGeom.height()) * t;

                const double squashFactor = 1.0 - 0.22 * std::sin(t * M_PI);
                const double stretchFactor = 1.0 + 0.22 * std::sin(t * M_PI);

                const double newW = w * squashFactor;
                const double newH = h * stretchFactor;

                const QRectF r(center.x() - newW / 2.0, center.y() - newH / 2.0, newW, newH);
                indicatorPtr->setGeometry(r.toAlignedRect());
                indicatorPtr->raise();

                // плавный радиус
                const int radius = static_cast<int>(8 + (10 - 8) * t);
                const QString style = QString(
                    "margin: 1px;"
                    "border-radius: %1px;"
                    "color: rgba(255,255,255,255);"
                    "background: rgba(255,255,255,15);"
                ).arg(radius);
                indicatorPtr->setStyleSheet(style);

                if (m_opacity) m_opacity->setOpacity(1.0 - t);

                if (shadowPtr) {
                    const double sp = 1.0 - t;
                    shadowPtr->setBlurRadius(6 + 6 * sp);
                    shadowPtr->setOffset(0, 4 + 6 * sp);
                    shadowPtr->setColor(QColor(0, 0, 0, static_cast<int>(120 + 120 * sp)));
                }
            });

    connect(anim, &QVariantAnimation::finished, this, [this, indicatorPtr, shadowPtr, frameGeom, anim]() {
        if (!indicatorPtr) return;
        indicatorPtr->hide();
        m_indicatorGeometry = frameGeom;

        if (m_opacity) m_opacity->setOpacity(1.0);
        if (shadowPtr) {
            shadowPtr->setBlurRadius(6);
            shadowPtr->setOffset(0, 4);
            shadowPtr->setColor(QColor(0, 0, 0, 0));
        }

        if (m_runningAnim == anim) m_runningAnim = nullptr;
        anim->deleteLater();
        updateButtonColors();
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void AnimatedSelector::onCustomEditChanged(const QString &text) {
    updateEditStyle();
    if (!m_group) return;

    if (!text.trimmed().isEmpty()) {
        for (auto *b: m_group->buttons()) b->setChecked(false);
        animateToCustomEdit();
    } else {
        if (auto *btn = m_group->checkedButton()) animateToButton(btn);
    }

    updateButtonColors();
}

void AnimatedSelector::updateButtonColors() const {
    if (!m_group) return;

    const bool hasCustom = m_customEdit && !m_customEdit->text().trimmed().isEmpty();
    for (auto *b: m_group->buttons()) {
        b->setProperty("customActive", hasCustom);
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
