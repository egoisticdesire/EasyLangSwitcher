#include "inAppNotification.h"

#include <QTextDocument>
#include <QtMath>

#include "../../helpers/acrylicHelper.h"
#include "../../helpers/iconHelper.h"
#include "../../helpers/screenResolver.h"
#include "../settingsWindow/settingsWindow.h"

#include <QApplication>
#include <QEvent>
#include <QPainter>
#include <QScreen>

QVector<InAppNotification*> InAppNotification::stack;

namespace
{
bool isSettingsWindowContextActive(const SettingsWindow* settings)
{
    if (settings == nullptr) {
        return false;
    }
    if (!settings->isVisible() || settings->isMinimized()) {
        return false;
    }
    if (settings->isActiveWindow()) {
        return true;
    }

    const QWidget* active = QApplication::activeWindow();
    while (active) {
        if (active == settings) {
            return true;
        }
        active = active->parentWidget();
    }
    return false;
}

void clearInAppNotificationStack()
{
    const auto copy = InAppNotification::stack;
    InAppNotification::stack.clear();
    for (auto* n : copy) {
        if (n) {
            n->hide();
            n->deleteLater();
        }
    }
}
} // namespace

InAppNotification::InAppNotification(SettingsWindow* settings, const QString& text, Type type)
    : QWidget(nullptr), settings(settings), m_type(type)
{
    ui.setupUi(this);

    setWindowFlags(Qt::ToolTip | Qt::NoDropShadowWindowHint | Qt::WindowDoesNotAcceptFocus | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_Hover);

    constexpr int totalWidth = 250;
    setFixedWidth(totalWidth);
    if (auto* rootLayout = this->layout()) {
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(0);
    }

    constexpr int paddingW = 12;
    constexpr int paddingH = 6;
    constexpr int iconSpacing = 12;
    constexpr int iconSize = 32;
    const QWidget* contentWidget = ui.background_frame ? ui.background_frame : static_cast<QWidget*>(this);
    if (auto* innerLayout = contentWidget->layout()) {
        innerLayout->setContentsMargins(paddingW, paddingH, paddingW, paddingH);
        innerLayout->setSpacing(iconSpacing);
        innerLayout->setAlignment(Qt::AlignVCenter);
    }

    ui.icon_label->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui.icon_label->setFixedSize(iconSize, iconSize);

    // Настройка иконки и цвета в зависимости от типа
    QString iconPath;
    QColor iconColor;

    switch (m_type) {
        case Type::Success:
            iconPath = ":/icons/icons/checked.svg";
            iconColor = QColor(91, 239, 91);
            break;
        case Type::Error:
            iconPath = ":/icons/icons/CancelRounded.svg";
            iconColor = QColor(200, 70, 70);
            break;
        case Type::Warning:
            iconPath = ":/icons/icons/WarningFilled.svg";
            iconColor = QColor(234, 191, 0);
            break;
        case Type::Info:
        default:
            iconPath = ":/icons/icons/InfoFilled.svg";
            iconColor = QColor(50, 114, 191);
            break;
    }

    ui.icon_label->setIcon(IconHelper::loadIcon(iconPath, iconColor, QSize(iconSize, iconSize)));

    ui.message_label->setText(text);
    ui.message_label->setWordWrap(true);

    QTextDocument doc;
    doc.setDefaultFont(ui.message_label->font());
    doc.setPlainText(text);
    doc.setTextWidth(totalWidth - (paddingW * 2) - iconSize - iconSpacing);
    const int finalHeight = qMax(static_cast<int>(iconSize), qCeil(doc.size().height())) + (paddingH * 2) + 2;
    setFixedHeight(finalHeight);

    installEventFilter(this);
    if (settings != nullptr) {
        settings->installEventFilter(this);
    }

    hideTimer.setSingleShot(true);
    hideTimer.setInterval(3000);
    connect(&hideTimer, &QTimer::timeout, this, [this]() { startHideAnimation(true); });
}

void InAppNotification::startShowAnimation()
{
    const int index = static_cast<int>(stack.indexOf(this));
    if (index < 0 || settings == nullptr) {
        return;
    }

    const QPoint endP = basePosition(index);
    const QPoint startP = endP + QPoint(15, 0);

    move(startP);
    setWindowOpacity(0.0);
    show();
    raise();

    AcrylicHelper::enableAcrylic(this);
    AcrylicHelper::updateRegion(this);

    // Анимация позиции
    auto* posAnim = new QVariantAnimation(this);
    posAnim->setDuration(200);
    posAnim->setStartValue(startP);
    posAnim->setEndValue(endP);
    posAnim->setEasingCurve(QEasingCurve::OutBack);
    connect(posAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        this->move(v.toPoint());
        this->update();
        AcrylicHelper::updateRegion(this);
    });

    // Анимация прозрачности
    auto* opacityAnim = new QPropertyAnimation(this, "windowOpacity", this);
    opacityAnim->setDuration(180);
    opacityAnim->setStartValue(0.0);
    opacityAnim->setEndValue(1.0);
    opacityAnim->setEasingCurve(QEasingCurve::OutCubic);

    // Группа
    auto* group = new QParallelAnimationGroup(this);
    group->addAnimation(posAnim);
    group->addAnimation(opacityAnim);
    group->start(QAbstractAnimation::DeleteWhenStopped);

    // Полоска прогресса
    if (progressAnim) {
        progressAnim->stop();
    }
    else {
        progressAnim = new QPropertyAnimation(this, "progress", this);
    }

    progressAnim->setDuration(hideTimer.interval());
    progressAnim->setStartValue(0.0);
    progressAnim->setEndValue(1.0);
    progressAnim->start();

    hideTimer.start();
}

void InAppNotification::startHideAnimation(const bool removeFromStack)
{
    if (m_closing) {
        return;
    }
    m_closing = removeFromStack;

    // Останавливаем прогресс и таймер
    if (progressAnim) {
        progressAnim->stop();
    }
    hideTimer.stop();

    // Создаем анимации "на лету"
    auto* outPos = new QPropertyAnimation(this, "pos", this);
    outPos->setDuration(180);
    outPos->setStartValue(this->pos());
    outPos->setEndValue(this->pos() + QPoint(0, 15));
    outPos->setEasingCurve(QEasingCurve::InBack);

    auto* outOpacity = new QPropertyAnimation(this, "windowOpacity", this);
    outOpacity->setDuration(160);
    outOpacity->setStartValue(this->windowOpacity());
    outOpacity->setEndValue(0.0);
    outOpacity->setEasingCurve(QEasingCurve::InCubic);

    auto* group = new QParallelAnimationGroup(this);
    group->addAnimation(outPos);
    group->addAnimation(outOpacity);

    // Важно: логика удаления после завершения анимации
    connect(group, &QParallelAnimationGroup::finished, this, [this]() {
        if (m_closing) {
            this->hide();
            if (const qsizetype idx = stack.indexOf(this); idx >= 0) {
                stack.removeAt(idx);
            }

            this->deleteLater();

            // Сдвигаем оставшиеся уведомления
            for (int i = 0; i < stack.size(); ++i) {
                if (auto* const notification = stack.at(i); notification != nullptr) {
                    notification->animateTo(i);
                }
            }
        }
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void InAppNotification::animateTo(const int newIndex)
{
    if (m_closing) {
        return;
    }

    auto* a = new QVariantAnimation(this);
    a->setDuration(200);
    a->setStartValue(this->pos());
    a->setEndValue(basePosition(newIndex));
    a->setEasingCurve(QEasingCurve::OutBack);

    connect(a, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        this->move(value.toPoint());
        this->update();
        AcrylicHelper::updateRegion(this);
    });

    a->start(QAbstractAnimation::DeleteWhenStopped);
}

QPoint InAppNotification::basePosition(const int index) const
{
    constexpr int margin = 12;
    constexpr int spacing = 12;
    QRect win;
    if (settings) {
        win = settings->geometry();
    }
    else if (const QScreen* screen = ScreenResolver::primaryOrFirst()) {
        win = screen->availableGeometry();
    }
    if (!win.isValid()) {
        return {};
    }

    const int x = win.right() - width() - margin;
    const int y = win.bottom() - height() - margin - index * (height() + spacing);
    return {x, y};
}

void InAppNotification::paintEvent(QPaintEvent* e)
{
    QWidget::paintEvent(e);
    if (m_progress <= 0.0) {
        return;
    }
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF r = rect();
    r.setWidth(r.width() * m_progress);

    QLinearGradient grad(r.topLeft(), r.topRight());
    grad.setColorAt(1.0, QColor(255, 255, 255, 0));
    grad.setColorAt(0.98, QColor(255, 255, 255, 6));
    grad.setColorAt(0.0, QColor(255, 255, 255, 3));

    p.setPen(Qt::NoPen);
    p.setBrush(grad);
    p.drawRect(r);
}

bool InAppNotification::eventFilter(QObject* o, QEvent* e)
{
    if (o == this) {
        if (e->type() == QEvent::MouseButtonPress) {
            startHideAnimation(true);
            return true;
        }
        if (e->type() == QEvent::Enter) {
            for (auto* n : stack) {
                if (n && n->progressAnim && n->progressAnim->state() == QAbstractAnimation::Running) {
                    n->progressAnim->pause();
                }
                if (n) {
                    n->hideTimer.stop();
                }
            }
            return true;
        }
        if (e->type() == QEvent::Leave) {
            for (auto* n : stack) {
                if (n && n->progressAnim && n->progressAnim->state() == QAbstractAnimation::Paused) {
                    n->progressAnim->resume();
                    if (const int rem = qRound(n->progressAnim->duration() * (1.0 - n->progress())); rem > 0) {
                        n->hideTimer.start(rem);
                    }
                    else {
                        n->startHideAnimation(true);
                    }
                }
            }
            return true;
        }
    }

    if (o == settings) {
        if (e->type() == QEvent::Move || e->type() == QEvent::Resize) {
            for (int i = 0; i < stack.size(); ++i) {
                if (auto* const notification = stack.at(i); notification != nullptr) {
                    notification->move(notification->basePosition(i));
                }
            }
        }
        if (e->type() == QEvent::Close || e->type() == QEvent::Hide || e->type() == QEvent::WindowDeactivate) {
            clearInAppNotificationStack();
        }
    }
    return QWidget::eventFilter(o, e);
}

void InAppNotification::showFor(SettingsWindow* settings, const QString& text, const Type type)
{
    if (!isSettingsWindowContextActive(settings)) {
        return;
    }
    if (stack.size() >= 3) {
        if (auto* last = stack.last()) {
            last->startHideAnimation(true);
        }
    }
    auto* n = new InAppNotification(settings, text, type);
    for (int i = 0; i < stack.size(); ++i) {
        if (auto* const notification = stack.at(i); notification != nullptr) {
            notification->animateTo(i + 1);
        }
    }
    stack.prepend(n);
    n->startShowAnimation();
}
