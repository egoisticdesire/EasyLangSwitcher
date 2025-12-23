#include "saveNotification.h"
#include "settingsWindow.h"
#include "../helpers/iconHelper.h"
#include "../helpers/acrylicHelper.h"
#include <QTextDocument>
#include <QtMath>
#include <QScreen>
#include <QEvent>
#include <QPainter>
#include <QGuiApplication>

QVector<SaveNotification *> SaveNotification::stack;

SaveNotification::SaveNotification(SettingsWindow *settings, const QString &text)
    : QWidget(nullptr), settings(settings) {
    ui.setupUi(this);

    setWindowFlags(Qt::ToolTip | Qt::NoDropShadowWindowHint | Qt::WindowDoesNotAcceptFocus | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);

    // Включаем отслеживание мыши, чтобы Enter/Leave работали стабильно
    setAttribute(Qt::WA_Hover);

    constexpr int totalWidth = 250;
    setFixedWidth(totalWidth);
    if (auto *rootLayout = this->layout()) {
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(0);
    }

    constexpr int paddingW = 12, paddingH = 6, iconSpacing = 12, iconSize = 32;
    const QWidget *contentWidget = ui.background_frame ? ui.background_frame : static_cast<QWidget *>(this);
    if (auto *innerLayout = contentWidget->layout()) {
        innerLayout->setContentsMargins(paddingW, paddingH, paddingW, paddingH);
        innerLayout->setSpacing(iconSpacing);
        innerLayout->setAlignment(Qt::AlignVCenter);
    }

    ui.icon_label->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui.icon_label->setFixedSize(iconSize, iconSize);
    ui.icon_label->setIcon(IconHelper::loadIcon(":/icons/icons/checked.svg", QColor(91, 239, 91),
                                                QSize(iconSize, iconSize)));
    ui.message_label->setText(text);
    ui.message_label->setWordWrap(true);

    QTextDocument doc;
    doc.setDefaultFont(ui.message_label->font());
    doc.setPlainText(text);
    doc.setTextWidth(totalWidth - (paddingW * 2) - iconSize - iconSpacing);
    const int finalHeight = qMax(static_cast<int>(iconSize), qCeil(doc.size().height())) + (paddingH * 2) + 2;
    setFixedHeight(finalHeight);

    setupAnimations();
    installEventFilter(this);

    if (settings) settings->installEventFilter(this);

    hideTimer.setSingleShot(true);
    hideTimer.setInterval(3000);
    connect(&hideTimer, &QTimer::timeout, this, [this]() { startHideAnimation(true); });
}

void SaveNotification::setupAnimations() {
    animGroupIn = new QParallelAnimationGroup(this);
    animInPos = new QPropertyAnimation(this, "pos", this);
    animInOpacity = new QPropertyAnimation(this, "windowOpacity", this);
    animInPos->setDuration(200);
    animInPos->setEasingCurve(QEasingCurve::OutBack);
    animInOpacity->setDuration(180);
    animInOpacity->setEasingCurve(QEasingCurve::OutCubic);
    animGroupIn->addAnimation(animInPos);
    animGroupIn->addAnimation(animInOpacity);

    animGroupOut = new QParallelAnimationGroup(this);
    animOutPos = new QPropertyAnimation(this, "pos", this);
    animOutOpacity = new QPropertyAnimation(this, "windowOpacity", this);
    animOutPos->setDuration(180);
    animOutPos->setEasingCurve(QEasingCurve::InBack);
    animOutOpacity->setDuration(160);
    animOutOpacity->setEasingCurve(QEasingCurve::InCubic);
    animGroupOut->addAnimation(animOutPos);
    animGroupOut->addAnimation(animOutOpacity);

    connect(animOutOpacity, &QPropertyAnimation::finished, this, [this]() {
        if (m_closing) {
            this->hide();
            if (const qsizetype idx = stack.indexOf(this); idx >= 0) stack.removeAt(idx);
            this->deleteLater();
            for (int i = 0; i < stack.size(); ++i) {
                if (stack[i]) stack[i]->animateTo(i);
            }
        }
    });
}

void SaveNotification::startShowAnimation() {
    const int index = stack.indexOf(this);
    if (index < 0 || !settings) return;

    const QPoint endP = basePosition(index);
    const QPoint startP = endP + QPoint(15, 0);

    move(startP);
    setWindowOpacity(0.0);
    show();
    raise();

    QTimer::singleShot(0, this, [this]() {
        AcrylicHelper::enableAcrylic(this);
        AcrylicHelper::updateRegion(this);
    });

    animInPos->setStartValue(startP);
    animInPos->setEndValue(endP);
    animInOpacity->setStartValue(0.0);
    animInOpacity->setEndValue(1.0);

    animGroupIn->start();

    // Прогресс-бар
    progressAnim = new QPropertyAnimation(this, "progress", this);
    progressAnim->setDuration(hideTimer.interval());
    progressAnim->setStartValue(0.0);
    progressAnim->setEndValue(1.0);
    progressAnim->start();

    hideTimer.start();
}

void SaveNotification::startHideAnimation(const bool removeFromStack) {
    if (m_closing) return;
    m_closing = removeFromStack;

    if (progressAnim) progressAnim->stop();
    animGroupIn->stop();
    hideTimer.stop();

    animOutPos->setStartValue(pos());
    animOutPos->setEndValue(pos() + QPoint(0, 15));
    animOutOpacity->setStartValue(windowOpacity());
    animOutOpacity->setEndValue(0.0);

    animGroupOut->start();
}

void SaveNotification::animateTo(const int newIndex) {
    if (m_closing) return;
    auto *a = new QPropertyAnimation(this, "pos", this);
    a->setDuration(200);
    a->setEndValue(basePosition(newIndex));
    a->setEasingCurve(QEasingCurve::OutBack);
    a->start(QAbstractAnimation::DeleteWhenStopped);
}

QPoint SaveNotification::basePosition(const int index) const {
    constexpr int margin = 12;
    constexpr int spacing = 12;
    const QRect win = settings ? settings->geometry() : QGuiApplication::primaryScreen()->availableGeometry();

    const int x = win.right() - width() - margin;
    const int y = win.bottom() - height() - margin - index * (height() + spacing);
    return QPoint(x, y);
}

void SaveNotification::paintEvent(QPaintEvent *e) {
    QWidget::paintEvent(e);
    if (m_progress <= 0.0) return;
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

bool SaveNotification::eventFilter(QObject *o, QEvent *e) {
    if (o == this) {
        switch (e->type()) {
            case QEvent::MouseButtonPress:
                startHideAnimation(true);
                return true;

            case QEvent::Enter:
                for (auto *n: stack) {
                    if (n && n->progressAnim) {
                        // Пауза только если она сейчас запущена
                        if (n->progressAnim->state() == QAbstractAnimation::Running) {
                            n->progressAnim->pause();
                        }
                    }
                    if (n) n->hideTimer.stop();
                }
                return true;

            case QEvent::Leave:
                for (auto *n: stack) {
                    if (n && n->progressAnim) {
                        // Возобновляем только если она реально на паузе
                        if (n->progressAnim->state() == QAbstractAnimation::Paused) {
                            n->progressAnim->resume();

                            // Запускаем таймер заново на остаток времени
                            if (const int rem = n->progressAnim->duration() * (1.0 - n->progress()); rem > 0) {
                                n->hideTimer.start(rem);
                            } else {
                                n->startHideAnimation(true);
                            }
                        } else if (n->progressAnim->state() == QAbstractAnimation::Stopped && !n->m_closing) {
                            // Если вдруг анимация на паузе, но мы не в процессе закрытия — закрываем
                            n->startHideAnimation(true);
                        }
                    }
                }
                return true;

            default: break;
        }
    }

    if (o == settings) {
        if (e->type() == QEvent::Move || e->type() == QEvent::Resize) {
            for (int i = 0; i < stack.size(); ++i) {
                if (stack[i]) stack[i]->move(stack[i]->basePosition(i));
            }
        }
        if (e->type() == QEvent::Close || e->type() == QEvent::Hide) {
            auto copy = stack;
            stack.clear();
            for (auto *n: copy) {
                if (n) {
                    n->hide();
                    n->deleteLater();
                }
            }
        }
    }
    return QWidget::eventFilter(o, e);
}

void SaveNotification::showFor(SettingsWindow *settings, const QString &text) {
    if (!settings) return;
    if (stack.size() >= 3) {
        if (auto *last = stack.last()) last->startHideAnimation(true);
    }
    auto *n = new SaveNotification(settings, text);
    for (int i = 0; i < stack.size(); ++i) {
        if (stack[i]) stack[i]->animateTo(i + 1);
    }
    stack.prepend(n);
    n->startShowAnimation();
}
