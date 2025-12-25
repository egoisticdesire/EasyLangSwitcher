#include "customToolTip.h"
#include "notifications/inAppNotification.h"
#include "../../ui/helpers/acrylicHelper.h"
#include "../../core/i18n/lang.h"
#include <QScreen>


CustomToolTip::CustomToolTip(QWidget *parent) : QWidget(nullptr) {
    ui.setupUi(this);

    // parent не передаем в QWidget, но флаг ToolTip свяжет жизненный цикл при правильном удалении parent
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    ui.hlayout_background_frame->setContentsMargins(8, 0, 8, 0);
    ui.tooltip_label->setAlignment(Qt::AlignCenter);

    // Инициализируем акрил
    QTimer::singleShot(0, this, [this]() { AcrylicHelper::enableAcrylic(this); });

    // Создаем анимации один раз
    animGroup = new QParallelAnimationGroup(this);

    posAnim = new QPropertyAnimation(this, "pos", this);
    opAnim = new QPropertyAnimation(this, "windowOpacity", this);

    animGroup->addAnimation(posAnim);
    animGroup->addAnimation(opAnim);

    // Единственный коннект на закрытие
    connect(animGroup, &QParallelAnimationGroup::finished, this, [this]() {
        if (isClosing) this->hide();
    });
}

void CustomToolTip::updateSize() {
    const QFontMetrics fm(ui.tooltip_label->font());
    const int textWidth = fm.horizontalAdvance(ui.tooltip_label->text());
    this->setFixedSize(textWidth + 24, 28);
}

void CustomToolTip::showAt(const QWidget *target, const QString &langKey) {
    if (!target) return;

    animGroup->stop();
    isClosing = false;

    currentLangKey = langKey;
    ui.tooltip_label->setText(Lang::tr(langKey));
    updateSize();

    constexpr int padding = 12;
    const QPoint globalPos = target->mapToGlobal(QPoint(target->width() + padding, 0))
                             + QPoint(0, (target->height() - this->height()) / 2);
    const QPoint startPos = globalPos - QPoint(padding, 0);

    // Настройка анимации появления
    posAnim->setDuration(200);
    posAnim->setStartValue(startPos);
    posAnim->setEndValue(globalPos);
    posAnim->setEasingCurve(QEasingCurve::OutBack);

    opAnim->setDuration(140);
    opAnim->setStartValue(this->windowOpacity());
    opAnim->setEndValue(1.0);
    opAnim->setEasingCurve(QEasingCurve::OutCubic);

    if (!this->isVisible()) {
        this->setWindowOpacity(0.0);
        this->show();
    }

    AcrylicHelper::updateRegion(this);

    animGroup->start();
}

void CustomToolTip::hideAnimated() {
    if (isClosing || !this->isVisible()) return;
    isClosing = true;

    animGroup->stop();

    constexpr int padding = 12;

    posAnim->setDuration(180);
    posAnim->setStartValue(this->pos());
    posAnim->setEndValue(this->pos() - QPoint(padding, 0));
    posAnim->setEasingCurve(QEasingCurve::InBack);

    opAnim->setDuration(140);
    opAnim->setStartValue(this->windowOpacity());
    opAnim->setEndValue(0.0);
    opAnim->setEasingCurve(QEasingCurve::InCubic);

    animGroup->start();
}

void CustomToolTip::refreshTranslations() {
    if (!currentLangKey.isEmpty()) {
        ui.tooltip_label->setText(Lang::tr(currentLangKey));
        updateSize();

        AcrylicHelper::updateRegion(this);
    }
}
