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

    if (m_animating) return;
    m_animating = true;

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

        QPointF center = startCenter + delta * t;

        constexpr double yOffsetFactor = 0.33;
        center.setY(center.y() - delta.y() * yOffsetFactor * std::sin(t * M_PI));

        // ----- Коэффициенты -----
        constexpr double squashMidX = 0.6; // Сжатие по X
        constexpr double squashMidY = 0.6; // Сжатие по Y
        constexpr double stretchMidX = 0.8; // Растяжение по X
        constexpr double stretchMidY = 0.6; // Растяжение по Y
        // ------------------------------------

        const double sinTerm = std::sin(t * M_PI);
        const double arcTerm = (1.0 - std::abs(2.0 * t - 1.0));

        // Независимые squash
        const double squashX = 1.0 - (1.0 - squashMidX) * sinTerm;
        const double squashY = 1.0 - (1.0 - squashMidY) * sinTerm;

        // Независимые stretch
        const double stretchX = 1.0 + (1.0 / stretchMidX - 1.0) * arcTerm;
        const double stretchY = 1.0 + (1.0 / stretchMidY - 1.0) * arcTerm;

        const double baseW = startGeom.width() + (endGeom.width() - startGeom.width()) * t;
        const double baseH = startGeom.height() + (endGeom.height() - startGeom.height()) * t;

        const double newW = horizontal ? baseW * squashX : baseW * stretchX;
        const double newH = horizontal ? baseH * stretchY : baseH * squashY;

        const QRectF r(
            center.x() - newW / 2.0,
            center.y() - newH / 2.0,
            newW,
            newH
        );

        indicatorPtr->setGeometry(r.toAlignedRect());
        indicatorPtr->raise();

        const auto style = QString(
            "margin: 1px;"
            "border-radius: 8px;"
            "color: rgba(255,255,255,255);"
            "background: rgba(255,255,255,15);"
        );
        indicatorPtr->setStyleSheet(style);

        if (shadowPtr) {
            const double sp = sinTerm;
            shadowPtr->setBlurRadius(6 + 6 * sp);
            shadowPtr->setOffset(0, 4 + 6 * sp);
            shadowPtr->setColor(QColor(0, 0, 0, static_cast<int>(120 + 120 * sp)));
        }
    });

    connect(anim, &QVariantAnimation::finished, this, [this, indicatorPtr, shadowPtr, btn, endGeom, anim]() {
        m_animating = false;

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
    if (m_indicator->isVisible() && m_indicator->geometry().isValid()) {
        startGeom = m_indicator->geometry();
    } else if (m_indicatorGeometry.isValid()) {
        startGeom = m_indicatorGeometry;
    } else {
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

    // запрет повторного запуска
    if (m_animating) return;
    m_animating = true;

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
    anim->setDuration(1000);
    anim->setEasingCurve(QEasingCurve::InOutCubic);

    const QPointF cStart = startGeom.center();
    const QPointF cEnd = frameGeom.center();
    const QPointF delta = cEnd - cStart;

    connect(anim, &QVariantAnimation::valueChanged, this,
            [this, indicatorPtr, shadowPtr, cStart, delta, startGeom, frameGeom](const QVariant &v) {
                if (!indicatorPtr) return;
                const double t = v.toDouble();

                // Основные плавные термы
                const double sinTerm = std::sin(t * M_PI); // пик в середине
                const double arcTerm = (1.0 - std::abs(2.0 * t - 1.0)); // широкая дуга
                const double smoothT = 0.5 - 0.5 * std::cos(t * M_PI); // ease-in-out

                // Центр (с лёгким y-смещением, как у кнопок)
                QPointF center = cStart + delta * smoothT;
                constexpr double yOffsetFactor = -0.33;
                center.setY(center.y() - delta.y() * yOffsetFactor * sinTerm);

                // Базовая интерполяция размеров
                const double baseW = startGeom.width() + (frameGeom.width() - startGeom.width()) * smoothT;
                const double baseH = startGeom.height() + (frameGeom.height() - startGeom.height()) * smoothT;

                // ----- Коэффициенты -----
                constexpr double squashMidX = 0.125; // сжатие по X
                constexpr double squashMidY = 0.15; // сжатие по Y
                constexpr double stretchMidX = 0.275; // растяжение по X
                constexpr double stretchMidY = 0.2; // растяжение по Y
                // --------------------------------------------------

                // Независимые squash
                const double squashX = 1.0 - (1.0 - squashMidX) * sinTerm;
                const double squashY = 1.0 - (1.0 - squashMidY) * sinTerm;

                // Независимые stretch
                const double stretchX = 1.0 + (1.0 / stretchMidX - 1.0) * arcTerm;
                const double stretchY = 1.0 + (1.0 / stretchMidY - 1.0) * arcTerm;

                // Вычисление итоговых размеров:
                // при экспансии к фрейму хотим поведение, схожее с кнопочной анимацией,
                // поэтому используем комбинацию squash/stretch (умножаем для выразительности).
                const double newW = baseW * squashX * stretchX;
                const double newH = baseH * squashY * stretchY;

                const QRectF r(
                    center.x() - newW / 2.0,
                    center.y() - newH / 2.0,
                    newW,
                    newH
                );

                indicatorPtr->setGeometry(r.toAlignedRect());
                indicatorPtr->raise();

                // Радиус — arcsin-кривая (мягкий старт/финиш)
                double p = 2.0 * t - 1.0;
                if (p < -1.0) p = -1.0;
                if (p > 1.0) p = 1.0;
                const double crv = (std::asin(p) / (M_PI / 2.0) + 1.0) * 0.5; // [0..1]
                const int radius = static_cast<int>(8 + (12 - 8) * crv);

                // Небольшая коррекция прозрачности для мягкости (можно регулировать)
                const double opacityFactor = 1.0 - std::pow(1.0 - smoothT, 1.6);

                indicatorPtr->setStyleSheet(QString(
                    "margin: 1px;"
                    "border-radius: %1px;"
                    "color: rgba(255,255,255,255);"
                    "background: rgba(255,255,255,15);"
                ).arg(radius));

                if (m_opacity) m_opacity->setOpacity(1.0 - opacityFactor);

                // Тень — более мягкая, сильнее в середине
                if (shadowPtr) {
                    const double sh = std::sin(t * M_PI); // 0..1..0
                    shadowPtr->setBlurRadius(6 + 6 * sh);
                    shadowPtr->setOffset(0, 4 + 6 * sh);
                    shadowPtr->setColor(QColor(0, 0, 0, static_cast<int>(160 * sh)));
                }
            });

    connect(anim, &QVariantAnimation::finished, this, [this, indicatorPtr, shadowPtr, frameGeom, anim]() {
        m_animating = false;

        if (!indicatorPtr) return;

        // В финале скрываем индикатор, но сохраняем геометрию фрейма (как было)
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

    // Защита от реэнтрантных вызовов
    if (m_inTextHandler) return;
    m_inTextHandler = true;

    const bool hasText = !text.trimmed().isEmpty();

    // Остановим текущую анимацию, чтобы исключить гонки
    if (m_runningAnim) {
        m_runningAnim->stop();
        m_runningAnim->deleteLater();
        m_runningAnim = nullptr;
    }

    // Сбрасываем флаг animating — чтобы гарантировать старт новой анимации
    m_animating = false;

    if (hasText) {
        // Программно снимаем чеки, блокируя сигнал каждой кнопки, чтобы избежать вызовов animateToButton
        for (auto *b: m_group->buttons()) {
            if (b->isChecked()) {
                b->blockSignals(true);
                b->setChecked(false);
                b->blockSignals(false);
            }
        }

        // Устанавливаем геометрию индикатора на геометрию фрейма и прячем индикатор,
        // но сохраняем геометрию — это ключевой момент для повторного ввода.
        if (m_indicator) {
            m_indicator->hide();
        }

        // Установим флаг, чтобы animateToCustomEdit знала, что стартовая геометрия должна быть frameGeom
        m_forceCustomStartFromFrame = true;

        // Гарантированно запустить анимацию расширения по фрейму
        animateToCustomEdit();
    } else {
        // Текст пустой -> вернуть индикатор на кнопку (если есть выбранная)
        if (auto *btn = m_group->checkedButton()) {
            // Если есть активный custom — уже пустой — безопасно вызвать
            animateToButton(btn);
        } else {
            // Никаких действий, просто обновим цвета
            updateButtonColors();
        }
    }

    m_inTextHandler = false;
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
