#include "hoverWarning.h"
#include "../helpers/acrylicHelper.h"
#include <QScreen>
#include <QTimer>
#include <QTextDocument>
#include <QtMath>

KeyHoverWarning::KeyHoverWarning(QWidget *owner)
    : QWidget(nullptr), owner(owner) {
    // nullptr parent для выхода за границы
    ui.setupUi(this);

    // Устанавливаем флаги как у тултипа
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);

    m_content = ui.background_frame ? ui.background_frame : static_cast<QWidget *>(this);
    setFixedWidth(380);

    QTimer::singleShot(0, this, [this] { AcrylicHelper::enableAcrylic(this); });

    setupAnimations();

    if (owner) {
        owner->installEventFilter(this);
    }
}

void KeyHoverWarning::setupAnimations() {
    // Группа появления
    animGroupIn = new QParallelAnimationGroup(this);
    animPosIn = new QPropertyAnimation(this, "pos", this);
    animOpacityIn = new QPropertyAnimation(this, "windowOpacity", this);

    animPosIn->setDuration(200);
    animPosIn->setEasingCurve(QEasingCurve::OutBack);
    animOpacityIn->setDuration(140);
    animOpacityIn->setEasingCurve(QEasingCurve::OutCubic);

    animGroupIn->addAnimation(animPosIn);
    animGroupIn->addAnimation(animOpacityIn);

    // Группа исчезновения
    animGroupOut = new QParallelAnimationGroup(this);
    animPosOut = new QPropertyAnimation(this, "pos", this);
    animOpacityOut = new QPropertyAnimation(this, "windowOpacity", this);

    animPosOut->setDuration(180);
    animPosOut->setEasingCurve(QEasingCurve::InBack);
    animOpacityOut->setDuration(140);
    animOpacityOut->setEasingCurve(QEasingCurve::InCubic);

    animGroupOut->addAnimation(animPosOut);
    animGroupOut->addAnimation(animOpacityOut);

    connect(animGroupOut, &QParallelAnimationGroup::finished, this, [this] {
        if (m_isClosing) {
            hide();
            m_visible = false;
        }
    });
}

bool KeyHoverWarning::eventFilter(QObject *obj, QEvent *event) {
    if (obj == owner) {
        if (event->type() == QEvent::Close || event->type() == QEvent::Hide) {
            hideImmediately();
        }
    }
    return QWidget::eventFilter(obj, event);
}

void KeyHoverWarning::hideImmediately() {
    animGroupIn->stop();
    animGroupOut->stop();
    hide();
    m_visible = false;
    m_isClosing = false;
}

void KeyHoverWarning::setText(const QString &text) {
    ui.label_warning->setText(text);
    ui.label_warning->setWordWrap(true);

    auto *layout = m_content->layout();
    if (!layout) return;

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

void KeyHoverWarning::showNear(const QWidget *anchor) {
    if (!anchor || m_visible) return;

    animGroupOut->stop();
    m_visible = true;
    m_isClosing = false;

    const QPoint endPos = anchor->mapToGlobal(QPoint(anchor->width() + 12, 0));
    const QPoint startPos = endPos - QPoint(12, 0);

    animPosIn->setStartValue(startPos);
    animPosIn->setEndValue(endPos);
    animOpacityIn->setStartValue(this->windowOpacity());
    animOpacityIn->setEndValue(1.0);

    if (!isVisible()) {
        setWindowOpacity(0.0);
        move(startPos);
        show();
    }

    animGroupIn->start();
}

void KeyHoverWarning::hideNow() {
    if (!m_visible || m_isClosing) return;

    animGroupIn->stop();
    m_isClosing = true;

    const QPoint startPos = pos();
    const QPoint endPos = startPos - QPoint(12, 0);

    animPosOut->setStartValue(startPos);
    animPosOut->setEndValue(endPos);
    animOpacityOut->setStartValue(this->windowOpacity());
    animOpacityOut->setEndValue(0.0);

    animGroupOut->start();
}
