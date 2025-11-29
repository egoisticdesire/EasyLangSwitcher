#pragma once
#include <QFrame>
#include <QLabel>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>

class HoverEffectHelper {
public:
    static void initializeHoverEffects(const QWidget *widget) {
        for (const auto *frame: widget->findChildren<QFrame *>()) {
            for (auto *lbl: frame->findChildren<QLabel *>()) {
                auto *eff = new QGraphicsOpacityEffect(lbl);
                eff->setOpacity(0.7);
                lbl->setGraphicsEffect(eff);
            }
        }
    }

    static void animateHover(const QFrame *frame, const bool enter) {
        for (const auto *lbl: frame->findChildren<QLabel *>()) {
            if (auto *eff = qobject_cast<QGraphicsOpacityEffect *>(lbl->graphicsEffect())) {
                auto *anim = new QPropertyAnimation(eff, "opacity");
                anim->setDuration(180);
                anim->setStartValue(eff->opacity());
                anim->setEndValue(enter ? 1.0 : 0.7);
                anim->setEasingCurve(QEasingCurve::InOutCubic);
                anim->start(QAbstractAnimation::DeleteWhenStopped);
            }
        }
    }
};
