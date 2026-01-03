#include "customToolTip.h"
#include "notifications/inAppNotification.h"
#include "../../ui/helpers/acrylicHelper.h"
#include "../../core/i18n/lang.h"
#include <QScreen>


CustomToolTip::CustomToolTip(QWidget *parent) : QWidget(nullptr) {
    ui.setupUi(this);

    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    ui.hlayout_background_frame->setContentsMargins(8, 0, 8, 0);
    ui.tooltip_label->setAlignment(Qt::AlignCenter);

    QTimer::singleShot(0, this, [this]() { AcrylicHelper::enableAcrylic(this); });

    // Создаем только группу
    animGroup = new QParallelAnimationGroup(this);

    // Один коннект на скрытие по окончанию
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
    animGroup->clear();
    isClosing = false;

    currentLangKey = langKey;
    ui.tooltip_label->setText(Lang::tr(langKey));
    updateSize();

    // Считаем позиции
    constexpr int padding = 12;
    const QPoint globalPos = target->mapToGlobal(QPoint(target->width() + padding, 0))
                             + QPoint(0, (target->height() - this->height()) / 2);
    const QPoint startPos = globalPos - QPoint(padding, 0);

    // Сначала перемещаем и скрываем, потом показываем
    this->move(startPos);
    this->setWindowOpacity(0.0);

    // Создаем анимации
    auto *posAnim = new QVariantAnimation(this);
    posAnim->setDuration(200);
    posAnim->setStartValue(startPos);
    posAnim->setEndValue(globalPos);
    posAnim->setEasingCurve(QEasingCurve::OutBack);

    connect(posAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        this->move(v.toPoint());
        AcrylicHelper::updateRegion(this);
    });

    auto *opAnim = new QPropertyAnimation(this, "windowOpacity", this);
    opAnim->setDuration(140);
    opAnim->setStartValue(0.0);
    opAnim->setEndValue(1.0);
    opAnim->setEasingCurve(QEasingCurve::OutCubic);

    animGroup->addAnimation(posAnim);
    animGroup->addAnimation(opAnim);

    // Теперь показываем — окно уже стоит в startPos и оно прозрачное
    this->show();

    // Сразу обновляем акрил, чтобы он "прилип" к новым координатам
    AcrylicHelper::updateRegion(this);

    animGroup->start();
}

void CustomToolTip::hideAnimated() {
    if (isClosing || !this->isVisible()) return;
    isClosing = true;

    animGroup->stop();
    animGroup->clear();

    constexpr int padding = 12;

    auto *posAnim = new QVariantAnimation(this);
    posAnim->setDuration(180);
    posAnim->setStartValue(this->pos());
    posAnim->setEndValue(this->pos() - QPoint(padding, 0));
    posAnim->setEasingCurve(QEasingCurve::InBack);

    connect(posAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        this->move(v.toPoint());
        AcrylicHelper::updateRegion(this);
    });

    auto *opAnim = new QPropertyAnimation(this, "windowOpacity", this);
    opAnim->setDuration(140);
    opAnim->setStartValue(this->windowOpacity());
    opAnim->setEndValue(0.0);
    opAnim->setEasingCurve(QEasingCurve::InCubic);

    animGroup->addAnimation(posAnim);
    animGroup->addAnimation(opAnim);

    animGroup->start();
}

void CustomToolTip::hideNow() {
    animGroup->stop();
    isClosing = false;
    this->setWindowOpacity(0.0);
    this->hide();
}

void CustomToolTip::refreshTranslations() {
    if (!currentLangKey.isEmpty()) {
        ui.tooltip_label->setText(Lang::tr(currentLangKey));
        updateSize();

        AcrylicHelper::updateRegion(this);
    }
}
