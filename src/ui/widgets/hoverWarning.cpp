#include "hoverWarning.h"

#include <QTextDocument>
#include <QTimer>
#include <QtMath>

#include "../helpers/acrylicHelper.h"

#include <QScreen>

KeyHoverWarning::KeyHoverWarning(QWidget* owner)
    : QWidget(nullptr), owner(owner), m_content(this), animGroupIn(new QParallelAnimationGroup(this)),
      animGroupOut(new QParallelAnimationGroup(this))
{
    ui.setupUi(this);

    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);

    m_content = (ui.background_frame != nullptr) ? ui.background_frame : static_cast<QWidget*>(this);
    setFixedWidth(380);

    QTimer::singleShot(0, this, [this] { AcrylicHelper::enableAcrylic(this); });

    connect(animGroupOut, &QParallelAnimationGroup::finished, this, [this] {
        if (m_isClosing) {
            hide();
            m_visible = false;
        }
    });

    if (owner != nullptr) {
        owner->installEventFilter(this);
    }
}

bool KeyHoverWarning::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == owner) {
        if (event->type() == QEvent::Close || event->type() == QEvent::Hide) {
            hideImmediately();
        }
    }
    return QWidget::eventFilter(obj, event);
}

void KeyHoverWarning::hideImmediately()
{
    animGroupIn->stop();
    animGroupOut->stop();
    hide();
    m_visible = false;
    m_isClosing = false;
}

void KeyHoverWarning::setText(const QString& text)
{
    ui.label_warning->setText(text);
    ui.label_warning->setWordWrap(true);

    auto* layout = m_content->layout();
    if (layout == nullptr) {
        return;
    }

    constexpr QMargins margins(10, 12, 10, 6);
    layout->setContentsMargins(margins);
    layout->setSpacing(0);

    QTextDocument doc;
    doc.setDefaultFont(ui.label_warning->font());
    doc.setHtml(text);
    doc.setDocumentMargin(0);
    doc.setTextWidth(width() - margins.left() - margins.right());

    const QFontMetricsF fm(ui.label_warning->font());
    const int textHeight = qCeil(doc.size().height() + fm.descent());

    constexpr int extraPadding = 3;
    setFixedHeight(textHeight + margins.top() + margins.bottom() + extraPadding);

    AcrylicHelper::updateRegion(this);
}

void KeyHoverWarning::showNear(const QWidget* anchor)
{
    if (anchor == nullptr || m_visible) {
        return;
    }

    animGroupIn->stop();
    animGroupIn->clear();
    animGroupOut->stop();

    m_visible = true;
    m_isClosing = false;

    const QPoint endPos = anchor->mapToGlobal(QPoint(anchor->width() + 12, 0));
    const QPoint startPos = endPos - QPoint(12, 0);

    // Анимация позиции с поддержкой Акрила
    auto* posAnim = new QVariantAnimation(this);
    posAnim->setDuration(200);
    posAnim->setStartValue(startPos);
    posAnim->setEndValue(endPos);
    posAnim->setEasingCurve(QEasingCurve::OutBack);

    connect(posAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        this->move(v.toPoint());
        AcrylicHelper::updateRegion(this);
    });

    // Анимация прозрачности
    auto* opacityAnim = new QPropertyAnimation(this, "windowOpacity", this);
    opacityAnim->setDuration(140);
    opacityAnim->setStartValue(0.0);
    opacityAnim->setEndValue(1.0);
    opacityAnim->setEasingCurve(QEasingCurve::OutCubic);

    animGroupIn->addAnimation(posAnim);
    animGroupIn->addAnimation(opacityAnim);

    // Правильный порядок появления
    this->move(startPos);
    this->setWindowOpacity(0.0);
    this->show();

    AcrylicHelper::updateRegion(this);

    animGroupIn->start();
}

void KeyHoverWarning::hideNow()
{
    if (!m_visible || m_isClosing) {
        return;
    }

    animGroupIn->stop();
    animGroupOut->stop();
    animGroupOut->clear();

    m_isClosing = true;

    const QPoint startPos = pos();
    const QPoint endPos = startPos - QPoint(12, 0);

    auto* posAnim = new QVariantAnimation(this);
    posAnim->setDuration(180);
    posAnim->setStartValue(startPos);
    posAnim->setEndValue(endPos);
    posAnim->setEasingCurve(QEasingCurve::InBack);

    connect(posAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        this->move(v.toPoint());
        AcrylicHelper::updateRegion(this);
    });

    auto* opacityAnim = new QPropertyAnimation(this, "windowOpacity", this);
    opacityAnim->setDuration(140);
    opacityAnim->setStartValue(this->windowOpacity());
    opacityAnim->setEndValue(0.0);
    opacityAnim->setEasingCurve(QEasingCurve::InCubic);

    animGroupOut->addAnimation(posAnim);
    animGroupOut->addAnimation(opacityAnim);

    animGroupOut->start();
}
