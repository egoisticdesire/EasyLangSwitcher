#include "hoverWarning.h"
#include "../helpers/acrylicHelper.h"
#include <QScreen>
#include <QTimer>

KeyHoverWarning::KeyHoverWarning(QWidget *owner)
    : QWidget(owner), owner(owner) {
    ui.setupUi(this);

    QTimer::singleShot(0, this, [this] { AcrylicHelper::enableAcrylic(this); });

    m_content = ui.background_frame ? ui.background_frame : static_cast<QWidget *>(this);

    setFixedWidth(380);

    setWindowFlags(Qt::Tool
                   | Qt::FramelessWindowHint
                   | Qt::WindowDoesNotAcceptFocus
                   | Qt::BypassWindowManagerHint);

    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);

    fx = new QGraphicsOpacityEffect(this);
    fx->setOpacity(0.0);
    setGraphicsEffect(fx);

    animOpacity = new QPropertyAnimation(fx, "opacity", this);
    animOpacity->setDuration(160);
    animOpacity->setStartValue(0.0);
    animOpacity->setEndValue(1.0);

    animPos = new QPropertyAnimation(this, "pos", this);
    animPos->setDuration(180);
    animPos->setEasingCurve(QEasingCurve::OutCubic);

    animOpacityOut = new QPropertyAnimation(fx, "opacity", this);
    animOpacityOut->setDuration(140);
    animOpacityOut->setStartValue(1.0);
    animOpacityOut->setEndValue(0.0);

    animPosOut = new QPropertyAnimation(this, "pos", this);
    animPosOut->setDuration(160);
    animPosOut->setEasingCurve(QEasingCurve::InCubic);

    connect(animOpacityOut, &QPropertyAnimation::finished, this, [this] {
        hide();
        m_visible = false;
    });

    // Устанавливаем фильтр событий на родителя
    if (owner) {
        owner->installEventFilter(this);
    }
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
    animOpacity->stop();
    animPos->stop();
    animOpacityOut->stop();
    animPosOut->stop();

    hide();
    m_visible = false;
}

void KeyHoverWarning::setText(const QString &text) {
    ui.label_warning->setText(text);
    ui.label_warning->setWordWrap(true);
    ui.label_warning->setMargin(0);

    auto *layout = m_content->layout();
    if (!layout) return;

    constexpr QMargins margins(10, 12, 10, 6);
    layout->setContentsMargins(margins);
    layout->setSpacing(0);

    // Вычисления высоты
    QTextDocument doc;
    doc.setDefaultFont(ui.label_warning->font());
    doc.setHtml(text);
    doc.setDocumentMargin(0);

    const int textWidth = width() - margins.left() - margins.right();
    doc.setTextWidth(textWidth);

    const QFontMetricsF fm(ui.label_warning->font());
    const int textHeight = qCeil(doc.size().height() + fm.descent());

    constexpr int extraPadding = 3;
    const int newHeight = textHeight + margins.top() + margins.bottom() + extraPadding;

    setFixedHeight(newHeight);

    AcrylicHelper::updateRegion(this);

    this->repaint();
}

void KeyHoverWarning::showNear(const QWidget *anchor) {
    if (!anchor || m_visible) return;
    m_visible = true;

    const QPoint endPos =
            anchor->mapToGlobal(QPoint(anchor->width() + 12, 0));
    const QPoint startPos = endPos - QPoint(12, 0);

    animOpacity->stop();
    animPos->stop();
    animOpacityOut->stop();
    animPosOut->stop();

    move(startPos);
    fx->setOpacity(0.0);
    show();
    raise();

    animPos->setStartValue(startPos);
    animPos->setEndValue(endPos);

    animOpacity->start();
    animPos->start();
}

void KeyHoverWarning::hideNow() const {
    if (!m_visible) return;

    animOpacity->stop();
    animPos->stop();

    const QPoint startPos = pos();
    const QPoint endPos = startPos - QPoint(12, 0);

    animPosOut->setStartValue(startPos);
    animPosOut->setEndValue(endPos);

    animOpacityOut->start();
    animPosOut->start();
}
