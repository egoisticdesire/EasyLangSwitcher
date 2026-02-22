#include "customToolTip.h"
#include "../../ui/helpers/acrylicHelper.h"
#include "../../core/i18n/lang.h"
#include <QTimer>

namespace {
constexpr int kToolTipMinWidth = 220;
constexpr int kToolTipMaxWidth = 360;
constexpr int kToolTipHeightPadding = 10;
constexpr int kToolTipHSpacing = 12;
constexpr int kToolTipSlideOffset = 10;
}

CustomToolTip::CustomToolTip(QWidget *parent) : QWidget(nullptr) {
    Q_UNUSED(parent);
    ui.setupUi(this);

    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    ui.hlayout_background_frame->setContentsMargins(8, 0, 8, 0);
    ui.tooltip_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui.tooltip_label->setWordWrap(true);

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
    const int maxTextWidth = kToolTipMaxWidth - 16;
    const QRect textRect = fm.boundingRect(
        QRect(0, 0, maxTextWidth, 4096),
        Qt::AlignLeft | Qt::TextWordWrap,
        ui.tooltip_label->text()
    );

    const int tooltipWidth = qBound(kToolTipMinWidth, textRect.width() + 16, kToolTipMaxWidth);
    const int tooltipHeight = qMax(28, textRect.height() + kToolTipHeightPadding);
    ui.tooltip_label->setFixedWidth(tooltipWidth - 16);
    this->setFixedSize(tooltipWidth, tooltipHeight);
}

QString CustomToolTip::resolveText() const {
    if (currentLangKey.isEmpty()) return {};

    QString text = Lang::tr(currentLangKey);
    for (const QString &arg: currentLangArgs) {
        text = text.arg(arg);
    }
    return text;
}

void CustomToolTip::showAt(const QWidget *target, const QString &langKey) {
    showAt(target, langKey, {});
}

void CustomToolTip::showAt(const QWidget *target, const QString &langKey, const QString &arg) {
    showAt(target, langKey, arg, {});
}

void CustomToolTip::showAt(const QWidget *target, const QString &langKey, const QString &arg1, const QString &arg2) {
    if (!target) return;

    animGroup->stop();
    animGroup->clear();
    isClosing = false;

    currentLangKey = langKey;
    currentLangArgs.clear();
    if (!arg1.isEmpty()) currentLangArgs.push_back(arg1);
    if (!arg2.isEmpty()) currentLangArgs.push_back(arg2);
    ui.tooltip_label->setText(resolveText());
    updateSize();

    // Позиция: справа от кнопки, по вертикали в центр кнопки.
    const QPoint buttonTopLeft = target->mapToGlobal(QPoint(0, 0));
    const int x = buttonTopLeft.x() + target->width() + kToolTipHSpacing;
    const int y = buttonTopLeft.y() + (target->height() - this->height()) / 2;
    const QPoint globalPos(x, y);
    const QPoint startPos = globalPos - QPoint(kToolTipSlideOffset, 0);

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

    auto *posAnim = new QVariantAnimation(this);
    posAnim->setDuration(180);
    posAnim->setStartValue(this->pos());
    posAnim->setEndValue(this->pos() - QPoint(kToolTipSlideOffset, 0));
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
        ui.tooltip_label->setText(resolveText());
        updateSize();

        AcrylicHelper::updateRegion(this);
    }
}
