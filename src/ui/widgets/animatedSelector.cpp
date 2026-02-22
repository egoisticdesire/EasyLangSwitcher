#include "animatedSelector.h"
#include "../../core/config/logger.h"
#include <QPushButton>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QTimer>
#include <QStyle>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPointer>

namespace {
    constexpr int kIndicatorRadiusPx = 8;
    constexpr int kIndicatorBgAlpha = 15;

    class IndicatorFrame final : public QFrame {
    public:
        explicit IndicatorFrame(QWidget *parent = nullptr) : QFrame(parent) {
            setFrameStyle(NoFrame);
            setAttribute(Qt::WA_TranslucentBackground);
        }

        void setFillOpacity(const qreal opacity) {
            const qreal clamped = qBound(0.0, opacity, 1.0);
            if (qFuzzyCompare(m_fillOpacity, clamped)) return;
            m_fillOpacity = clamped;
            update();
        }

    protected:
        void paintEvent(QPaintEvent *event) override {
            Q_UNUSED(event);
            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.setPen(Qt::NoPen);

            QColor fill(255, 255, 255, kIndicatorBgAlpha);
            const int alpha = qBound(
                0,
                static_cast<int>(std::lround(static_cast<double>(kIndicatorBgAlpha) * m_fillOpacity)),
                255
            );
            fill.setAlpha(alpha);
            p.setBrush(fill);

            QRectF r = rect();
            r.adjust(0.5, 0.5, -0.5, -0.5);
            p.drawRoundedRect(r, kIndicatorRadiusPx, kIndicatorRadiusPx);
        }

    private:
        qreal m_fillOpacity = 1.0;
    };

    void setIndicatorFillOpacity(const QPointer<QFrame> &frame, const qreal opacity) {
        if (!frame) return;
        if (auto *indicator = dynamic_cast<IndicatorFrame *>(frame.data())) {
            indicator->setFillOpacity(opacity);
        }
    }
} // namespace

AnimatedSelector::AnimatedSelector(QWidget *parent)
    : QObject(parent), m_parent(parent) {
}

void AnimatedSelector::bindToFrame(QFrame *frame, const QString &extraStyle) {
    m_frame = frame;
    if (!m_frame) return;
    m_extraStyle = extraStyle;

    LOG_DEBUG() << "Bound to frame: " << m_frame->objectName();

    m_indicator = new IndicatorFrame(m_parent);
    m_indicator->setObjectName("animatedIndicator");
    setIndicatorFillOpacity(QPointer<QFrame>(m_indicator), 1.0);
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
    m_customHasText = m_customEdit && !m_customEdit->text().trimmed().isEmpty();

    // Если инпут с текстом — индикатор не показываем, но сохраняем геометрию поля
    if (m_customHasText) {
        m_indicator->hide();
        QRect editGeom = m_customEdit->geometry();
        editGeom.moveTopLeft(m_customEdit->mapTo(m_parent, QPoint(0, 0)));
        editGeom.adjust(-2, -1, 2, 1);
        m_indicatorGeometry = editGeom;
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

    if (m_animating && m_animTargetButton == btn) return;
    m_animating = true;
    m_animTargetButton = btn;

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

    QRect customGeom;
    bool hasCustomGeom = false;
    if (m_customEdit) {
        customGeom = m_customEdit->geometry();
        customGeom.moveTopLeft(m_customEdit->mapTo(m_parent, QPoint(0, 0)));
        customGeom.adjust(-2, -1, 2, 1);
        hasCustomGeom = true;
    }

    const auto nearRect = [](const QRect &a, const QRect &b, const int eps = 3) -> bool {
        return std::abs(a.left() - b.left()) <= eps &&
               std::abs(a.top() - b.top()) <= eps &&
               std::abs(a.width() - b.width()) <= eps &&
               std::abs(a.height() - b.height()) <= eps;
    };
    const bool fadeInFromCustom = !m_indicator->isVisible() &&
                                  hasCustomGeom &&
                                  startGeom.isValid() &&
                                  nearRect(startGeom, customGeom);

    if (!m_indicator->isVisible()) {
        m_indicator->setGeometry(startGeom);
        m_indicator->show();
        m_indicator->raise();
    }
    setIndicatorFillOpacity(QPointer<QFrame>(m_indicator), fadeInFromCustom ? 0.0 : 1.0);

    if (m_indicator->graphicsEffect() != m_shadow) m_indicator->setGraphicsEffect(m_shadow);

    const QPointF startCenter = startGeom.center();
    const QPointF endCenter = endGeom.center();
    const QPointF delta = endCenter - startCenter;
    const double distance = std::hypot(delta.x(), delta.y());
    const bool sameTarget = nearRect(startGeom, endGeom, 2);

    if (m_runningAnim) {
        m_runningAnim->stop();
        m_runningAnim = nullptr;
    }

    const QPointer indicatorPtr(m_indicator);
    const QPointer shadowPtr(m_shadow);
    const QPointer<QAbstractButton> btnPtr(btn);
    // Пиксельная коррекция позиционирования индикатора во время анимации.
    // Эквивалент старого выражения "... - (size / 2.0 - 1)".
    constexpr double geometryOffsetX = 1.0;
    constexpr double geometryOffsetY = 1.0;

    if (sameTarget && !fadeInFromCustom) {
        auto *reclickAnim = new QVariantAnimation(this);
        m_runningAnim = reclickAnim;
        reclickAnim->setStartValue(0.0);
        reclickAnim->setEndValue(1.0);

        // Параметры анимации reclick (регулируемые)
        constexpr int reclickDurationMs = 320; // Общая длительность пульса
        constexpr double reclickSqueezeX = 0.20; // Сжатие по горизонтали на пике
        constexpr double reclickStretchY = 0.10; // Растяжение по вертикали на пике
        constexpr double reclickShadowBoost = 0.35; // Усиление тени на пике

        reclickAnim->setDuration(reclickDurationMs);
        reclickAnim->setEasingCurve(QEasingCurve::Linear);

        connect(reclickAnim, &QVariantAnimation::valueChanged, this, [=](const QVariant &v) {
            if (!indicatorPtr) return;
            const double t = v.toDouble();
            const double smooth = t * t * (3.0 - 2.0 * t); // smoothstep для мягкого старта/финиша
            const double pulse = std::sin(smooth * M_PI); // 0..1..0

            const double w = std::max(1.0, endGeom.width() * (1.0 - reclickSqueezeX * pulse));
            const double h = std::max(1.0, endGeom.height() * (1.0 + reclickStretchY * pulse));
            const QPointF c = endGeom.center();
            const QRectF r(c.x() - w * 0.5 + geometryOffsetX, c.y() - h * 0.5 + geometryOffsetY, w, h);

            indicatorPtr->setGeometry(r.toAlignedRect());
            indicatorPtr->raise();
            setIndicatorFillOpacity(indicatorPtr, 1.0);

            if (shadowPtr) {
                const double sh = pulse * reclickShadowBoost;
                shadowPtr->setBlurRadius(6 + 6 * sh);
                shadowPtr->setOffset(0, 4 + 4 * sh);
                shadowPtr->setColor(QColor(0, 0, 0, static_cast<int>(160 * sh)));
            }
        });

        connect(reclickAnim, &QVariantAnimation::finished, this, [this, indicatorPtr, shadowPtr, btnPtr, endGeom, reclickAnim]() {
            m_animating = false;
            m_animTargetButton = nullptr;
            if (!indicatorPtr) return;

            indicatorPtr->setGeometry(endGeom);
            indicatorPtr->raise();
            setIndicatorFillOpacity(indicatorPtr, 1.0);
            if (btnPtr) btnPtr->setChecked(true);
            updateButtonColors();
            m_indicatorGeometry = endGeom;

            if (shadowPtr) {
                shadowPtr->setBlurRadius(6);
                shadowPtr->setOffset(0, 4);
                shadowPtr->setColor(QColor(0, 0, 0, 0));
            }
            if (m_runningAnim == reclickAnim) m_runningAnim = nullptr;
        });

        reclickAnim->start(QAbstractAnimation::DeleteWhenStopped);
        return;
    }

    auto *anim = new QVariantAnimation(this);
    m_runningAnim = anim;
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    const int dur = static_cast<int>(qBound(320.0, 480.0 + distance * 0.40, 760.0));
    anim->setDuration(dur);
    anim->setEasingCurve(QEasingCurve::Linear);
    const QEasingCurve posEase(QEasingCurve::InOutCubic);
    const QEasingCurve backEase(QEasingCurve::InOutBack);

    // Параметры анимации (регулируемые)
    constexpr double tailDelay = 0.3; // Задержка старта хвостовой границы
    constexpr double maxStretchRatio = 2.33; // Лимит растяжения вдоль направления
    constexpr double squeezeGain = 0.66; // Сжатие перпендикулярной оси от растяжения
    constexpr double backNorm = 0.11; // Нормализация разницы back/база
    constexpr double leadBackFactor = 0.16; // Амплитуда back для ведущей границы
    constexpr double tailBackFactor = 0.09; // Амплитуда back для хвостовой границы
    constexpr double appearStart = 0.02; // Старт проявления при переходе из custom-поля
    constexpr double appearDuration = 0.30; // Длительность проявления

    connect(anim, &QVariantAnimation::valueChanged, this, [=](const QVariant &v) {
        if (!indicatorPtr) return;
        const double t = v.toDouble();
        const double sinTerm = std::sin(t * M_PI);
        const double leadT = posEase.valueForProgress(t);
        const double tailRawT = qBound(0.0, (t - tailDelay) / std::max(0.0001, 1.0 - tailDelay), 1.0);
        const double tailT = posEase.valueForProgress(tailRawT);

        const auto lerp = [](const double a, const double b, const double p) -> double {
            return a + (b - a) * p;
        };

        double left = 0.0;
        double right = 0.0;
        double top = 0.0;
        double bottom = 0.0;

        const bool moveX = std::abs(delta.x()) > 0.5;
        const bool moveY = std::abs(delta.y()) > 0.5;

        const double pxX = moveX ? tailT : leadT;
        const double pxY = moveY ? tailT : leadT;

        if (delta.x() >= 0.0) {
            right = lerp(startGeom.right(), endGeom.right(), leadT);
            left = lerp(startGeom.left(), endGeom.left(), pxX);
        } else {
            left = lerp(startGeom.left(), endGeom.left(), leadT);
            right = lerp(startGeom.right(), endGeom.right(), pxX);
        }

        if (delta.y() >= 0.0) {
            bottom = lerp(startGeom.bottom(), endGeom.bottom(), leadT);
            top = lerp(startGeom.top(), endGeom.top(), pxY);
        } else {
            top = lerp(startGeom.top(), endGeom.top(), leadT);
            bottom = lerp(startGeom.bottom(), endGeom.bottom(), pxY);
        }

        const double weightSum = std::max(1.0, std::abs(delta.x()) + std::abs(delta.y()));
        const double wX = std::abs(delta.x()) / weightSum;
        const double wY = std::abs(delta.y()) / weightSum;
        const double baseW = lerp(startGeom.width(), endGeom.width(), leadT);
        const double baseH = lerp(startGeom.height(), endGeom.height(), leadT);
        const double minDim = std::min(baseW, baseH);
        const double backDelta = (backEase.valueForProgress(t) - leadT) / backNorm;
        const double leadBackPx = qBound(3.0, minDim * leadBackFactor, 8.0);
        const double tailBackPx = qBound(1.0, minDim * tailBackFactor, 5.0);

        if (moveX) {
            const double sx = (delta.x() >= 0.0) ? 1.0 : -1.0;
            if (delta.x() >= 0.0) {
                right += sx * leadBackPx * backDelta * wX;
                left += sx * tailBackPx * backDelta * wX;
            } else {
                left += sx * leadBackPx * backDelta * wX;
                right += sx * tailBackPx * backDelta * wX;
            }
        }
        if (moveY) {
            const double sy = (delta.y() >= 0.0) ? 1.0 : -1.0;
            if (delta.y() >= 0.0) {
                bottom += sy * leadBackPx * backDelta * wY;
                top += sy * tailBackPx * backDelta * wY;
            } else {
                top += sy * leadBackPx * backDelta * wY;
                bottom += sy * tailBackPx * backDelta * wY;
            }
        }

        double curW = std::max(1.0, right - left);
        double curH = std::max(1.0, bottom - top);
        const double maxW = std::max(1.0, baseW * maxStretchRatio);
        const double maxH = std::max(1.0, baseH * maxStretchRatio);
        if (curW > maxW) {
            if (delta.x() >= 0.0) left = right - maxW;
            else right = left + maxW;
            curW = maxW;
        }
        if (curH > maxH) {
            if (delta.y() >= 0.0) top = bottom - maxH;
            else bottom = top + maxH;
            curH = maxH;
        }

        const double stretchX = curW / std::max(1.0, baseW) - 1.0;
        const double stretchY = curH / std::max(1.0, baseH) - 1.0;
        const double squeezeY = qBound(0.0, stretchX * squeezeGain, 0.20);
        const double squeezeX = qBound(0.0, stretchY * squeezeGain, 0.20);
        const double cx = 0.5 * (left + right);
        const double cy = 0.5 * (top + bottom);
        const double halfW = std::max(1.0, curW * (1.0 - squeezeX) * 0.5);
        const double halfH = std::max(1.0, curH * (1.0 - squeezeY) * 0.5);
        const QRectF r(cx - halfW + geometryOffsetX, cy - halfH + geometryOffsetY, 2.0 * halfW, 2.0 * halfH);

        indicatorPtr->setGeometry(r.toAlignedRect());
        indicatorPtr->raise();

        if (fadeInFromCustom) {
            const double u = qBound(0.0, (leadT - appearStart) / std::max(0.0001, appearDuration), 1.0);
            const double smooth = u * u * (3.0 - 2.0 * u); // smoothstep
            setIndicatorFillOpacity(indicatorPtr, smooth);
        } else {
            setIndicatorFillOpacity(indicatorPtr, 1.0);
        }

        if (shadowPtr) {
            const double sp = sinTerm;
            shadowPtr->setBlurRadius(6 + 6 * sp);
            shadowPtr->setOffset(0, 4 + 6 * sp);
            shadowPtr->setColor(QColor(0, 0, 0, static_cast<int>(160 * sp)));
        }
    });

    connect(anim, &QVariantAnimation::finished, this, [this, indicatorPtr, shadowPtr, btnPtr, endGeom, anim]() {
        m_animating = false;
        m_animTargetButton = nullptr;

        if (!indicatorPtr) return;

        indicatorPtr->setGeometry(endGeom);
        setIndicatorFillOpacity(indicatorPtr, 1.0);
        indicatorPtr->raise();
        if (btnPtr) btnPtr->setChecked(true);
        updateButtonColors();
        m_indicatorGeometry = endGeom;

        if (shadowPtr) {
            shadowPtr->setBlurRadius(6);
            shadowPtr->setOffset(0, 4);
            shadowPtr->setColor(QColor(0, 0, 0, 0));
        }
        if (m_runningAnim == anim) m_runningAnim = nullptr;
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

    QRect targetGeom = m_customEdit->geometry();
    targetGeom.moveTopLeft(m_customEdit->mapTo(m_parent, QPoint(0, 0)));
    targetGeom.adjust(-2, -1, 2, 1);

    if (!m_indicator->isVisible()) {
        m_indicator->setGeometry(startGeom);
        m_indicator->show();
        m_indicator->raise();
    }
    setIndicatorFillOpacity(QPointer<QFrame>(m_indicator), 1.0);

    // запрет повторного запуска
    if (m_animating) return;
    m_animating = true;
    m_animTargetButton = nullptr;

    if (m_runningAnim) {
        m_runningAnim->stop();
        m_runningAnim = nullptr;
    }

    const QPointer indicatorPtr(m_indicator);
    const QPointer shadowPtr(m_shadow);

    auto *anim = new QVariantAnimation(this);
    m_runningAnim = anim;
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    const QPointF cStart = startGeom.center();
    const QPointF cEnd = targetGeom.center();
    const QPointF delta = cEnd - cStart;
    const double pathLen = std::hypot(delta.x(), delta.y());
    anim->setDuration(static_cast<int>(qBound(420.0, 620.0 + pathLen * 0.45, 980.0)));
    anim->setEasingCurve(QEasingCurve::Linear);
    const QEasingCurve backEase(QEasingCurve::InOutBack);
    const QEasingCurve posEase(QEasingCurve::InOutCubic);

    // Параметры анимации (регулируемые)
    constexpr double tailDelay = 0.30; // Задержка старта хвостовой границы
    constexpr double maxStretchRatio = 2.33; // Лимит растяжения вдоль направления
    constexpr double squeezeGain = 0.66; // Сжатие перпендикулярной оси от растяжения
    constexpr double backNorm = 0.11; // Нормализация разницы back/база
    constexpr double leadBackFactor = 0.17; // Амплитуда back для ведущей границы
    constexpr double tailBackFactor = 0.09; // Амплитуда back для хвостовой границы
    constexpr double fadeStart = 0.80; // Начало плавного исчезновения в конце анимации

    connect(anim, &QVariantAnimation::valueChanged, this,
            [indicatorPtr, shadowPtr, delta, startGeom, targetGeom, backEase, posEase](
        const QVariant &v) {
                if (!indicatorPtr) return;
                const double t = v.toDouble();
                const double sinTerm = std::sin(t * M_PI);
                const double leadT = posEase.valueForProgress(t);
                const double tailRawT = qBound(0.0, (t - tailDelay) / std::max(0.0001, 1.0 - tailDelay), 1.0);
                const double tailT = posEase.valueForProgress(tailRawT);

                const auto lerp = [](const double a, const double b, const double p) -> double {
                    return a + (b - a) * p;
                };

                double left = 0.0;
                double right = 0.0;
                double top = 0.0;
                double bottom = 0.0;

                const bool moveX = std::abs(delta.x()) > 0.5;
                const bool moveY = std::abs(delta.y()) > 0.5;

                const double pxX = moveX ? tailT : leadT;
                const double pxY = moveY ? tailT : leadT;

                if (delta.x() >= 0.0) {
                    right = lerp(startGeom.right(), targetGeom.right(), leadT);
                    left = lerp(startGeom.left(), targetGeom.left(), pxX);
                } else {
                    left = lerp(startGeom.left(), targetGeom.left(), leadT);
                    right = lerp(startGeom.right(), targetGeom.right(), pxX);
                }

                if (delta.y() >= 0.0) {
                    bottom = lerp(startGeom.bottom(), targetGeom.bottom(), leadT);
                    top = lerp(startGeom.top(), targetGeom.top(), pxY);
                } else {
                    top = lerp(startGeom.top(), targetGeom.top(), leadT);
                    bottom = lerp(startGeom.bottom(), targetGeom.bottom(), pxY);
                }

                const double weightSum = std::max(1.0, std::abs(delta.x()) + std::abs(delta.y()));
                const double wX = std::abs(delta.x()) / weightSum;
                const double wY = std::abs(delta.y()) / weightSum;
                const double baseW = lerp(startGeom.width(), targetGeom.width(), leadT);
                const double baseH = lerp(startGeom.height(), targetGeom.height(), leadT);
                const double minDim = std::min(baseW, baseH);
                const double backDelta = (backEase.valueForProgress(t) - leadT) / backNorm;
                const double leadBackPx = qBound(4.0, minDim * leadBackFactor, 10.0);
                const double tailBackPx = qBound(1.0, minDim * tailBackFactor, 6.0);

                if (moveX) {
                    const double sx = (delta.x() >= 0.0) ? 1.0 : -1.0;
                    if (delta.x() >= 0.0) {
                        right += sx * leadBackPx * backDelta * wX;
                        left += sx * tailBackPx * backDelta * wX;
                    } else {
                        left += sx * leadBackPx * backDelta * wX;
                        right += sx * tailBackPx * backDelta * wX;
                    }
                }
                if (moveY) {
                    const double sy = (delta.y() >= 0.0) ? 1.0 : -1.0;
                    if (delta.y() >= 0.0) {
                        bottom += sy * leadBackPx * backDelta * wY;
                        top += sy * tailBackPx * backDelta * wY;
                    } else {
                        top += sy * leadBackPx * backDelta * wY;
                        bottom += sy * tailBackPx * backDelta * wY;
                    }
                }

                double curW = std::max(1.0, right - left);
                double curH = std::max(1.0, bottom - top);
                const double maxW = std::max(1.0, baseW * maxStretchRatio);
                const double maxH = std::max(1.0, baseH * maxStretchRatio);
                if (curW > maxW) {
                    if (delta.x() >= 0.0) left = right - maxW;
                    else right = left + maxW;
                    curW = maxW;
                }
                if (curH > maxH) {
                    if (delta.y() >= 0.0) top = bottom - maxH;
                    else bottom = top + maxH;
                    curH = maxH;
                }

                const double stretchX = curW / std::max(1.0, baseW) - 1.0;
                const double stretchY = curH / std::max(1.0, baseH) - 1.0;
                const double squeezeY = qBound(0.0, stretchX * squeezeGain, 0.20);
                const double squeezeX = qBound(0.0, stretchY * squeezeGain, 0.20);
                const double cx = 0.5 * (left + right);
                const double cy = 0.5 * (top + bottom);
                const double halfW = std::max(1.0, curW * (1.0 - squeezeX) * 0.5);
                const double halfH = std::max(1.0, curH * (1.0 - squeezeY) * 0.5);
                const QRectF r(cx - halfW, cy - halfH, 2.0 * halfW, 2.0 * halfH);

                indicatorPtr->setGeometry(r.toAlignedRect());
                indicatorPtr->raise();

                // Мягкое затухание перед hide(), без покадрового setStyleSheet (чтобы убрать белые вспышки).
                double fadeMul = 1.0;
                if (leadT > fadeStart) {
                    const double u = qBound(0.0, (leadT - fadeStart) / std::max(0.0001, 1.0 - fadeStart), 1.0);
                    const double smooth = u * u * (3.0 - 2.0 * u); // smoothstep
                    fadeMul = 1.0 - smooth;
                }
                setIndicatorFillOpacity(indicatorPtr, fadeMul);

                // Тень — более мягкая, сильнее в середине; fadeMul приглушает хвост анимации.
                if (shadowPtr) {
                    const double sh = sinTerm; // 0..1..0
                    shadowPtr->setBlurRadius(6 + 6 * sh);
                    shadowPtr->setOffset(0, 4 + 6 * sh);
                    shadowPtr->setColor(QColor(0, 0, 0, static_cast<int>(160 * sh * fadeMul)));
                }
            });

    connect(anim, &QVariantAnimation::finished, this, [this, indicatorPtr, shadowPtr, targetGeom, anim]() {
        m_animating = false;
        m_animTargetButton = nullptr;

        if (!indicatorPtr) return;

        // В финале скрываем индикатор, но сохраняем геометрию поля.
        indicatorPtr->hide();
        m_indicatorGeometry = targetGeom;
        setIndicatorFillOpacity(indicatorPtr, 1.0);

        if (shadowPtr && indicatorPtr->graphicsEffect() != shadowPtr) indicatorPtr->setGraphicsEffect(shadowPtr);
        if (shadowPtr) {
            shadowPtr->setBlurRadius(6);
            shadowPtr->setOffset(0, 4);
            shadowPtr->setColor(QColor(0, 0, 0, 0));
        }

        if (m_runningAnim == anim) m_runningAnim = nullptr;
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
    if (hasText == m_customHasText) {
        updateButtonColors();
        m_inTextHandler = false;
        return;
    }
    m_customHasText = hasText;

    // Остановим текущую анимацию, чтобы исключить гонки
    if (m_runningAnim) {
        m_runningAnim->stop();
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

        // Установим флаг, чтобы animateToCustomEdit знала, что стартовая геометрия берётся из сохранённого состояния
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

QFrame *AnimatedSelector::boundFrame() const {
    return m_frame;
}

void AnimatedSelector::stopAndResetAnimation() {
    // остановить текущую анимацию безопасно
    if (m_runningAnim) {
        m_runningAnim->stop();
        m_runningAnim = nullptr;
    }
    // снять флаги блокировки, чтобы new animation могла стартовать
    m_animating = false;
    m_animTargetButton = nullptr;
    m_inTextHandler = false;
    m_forceCustomStartFromFrame = false;
}

void AnimatedSelector::animateToCurrentState() {
    if (!m_group) return;
    m_customHasText = m_customEdit && !m_customEdit->text().trimmed().isEmpty();

    // гарантируем сброс блокировок перед стартом
    if (m_customHasText) {
        stopAndResetAnimation();
        animateToCustomEdit();
        return;
    }

    // если есть выбранная кнопка — воспроизвести анимацию к ней
    if (auto *btn = m_group->checkedButton()) {
        stopAndResetAnimation();
        animateToButton(btn);
        return;
    }

    // fallback: первая кнопка
    if (const auto all = m_group->buttons(); !all.isEmpty()) {
        stopAndResetAnimation();
        animateToButton(all.first());
    }
}
