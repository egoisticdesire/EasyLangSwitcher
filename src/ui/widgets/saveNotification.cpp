#include "saveNotification.h"
#include "settingsWindow.h"
#include "../helpers/iconHelper.h"
#include "../helpers/acrylicHelper.h"
#include <QTextDocument>
#include <QtMath>
#include <QScreen>
#include <QEvent>

QVector<SaveNotification *> SaveNotification::stack;

SaveNotification::SaveNotification(SettingsWindow *settings, const QString &text)
    : QWidget(settings), settings(settings) {
    ui.setupUi(this);

    // Фиксируем ширину виджета. Это важно сделать ДО расчетов текста.
    constexpr int totalWidth = 260;
    setFixedWidth(totalWidth);

    // Получаем корневой лейаут (тот, что управляет background_frame)
    if (auto *rootLayout = this->layout()) {
        // Убираем отступы у корня, чтобы ui.background_frame прилип к краям SaveNotification.
        // Это решит проблему "потери отступа от края экрана" и "странной рамки".
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(0);
    }

    // Определяем контейнер контента и его лейаут
    QWidget *contentWidget = ui.background_frame ? ui.background_frame : static_cast<QWidget *>(this);
    QLayout *innerLayout = contentWidget->layout();

    // Параметры "внутренней красоты" (padding внутри пузыря)
    constexpr int paddingW = 12; // Отступ от краев рамки до текста
    constexpr int paddingH = 6; // Отступ от краев рамки до текста
    constexpr int iconSpacing = 12; // Отступ между иконкой и текстом
    constexpr int iconSize = 32; // Размер иконки

    if (innerLayout) {
        // Задаем отступы только внутри фрейма
        innerLayout->setContentsMargins(paddingW, paddingH, paddingW, paddingH);
        innerLayout->setSpacing(iconSpacing);
        // Выравнивание, чтобы иконка и текст были по центру по вертикали (опционально)
        innerLayout->setAlignment(Qt::AlignVCenter);
    }

    ui.icon_label->setFixedSize(iconSize, iconSize);
    ui.icon_label->setIcon(
        IconHelper::loadIcon(":/icons/icons/checked.svg", QColor(91, 239, 91), QSize(iconSize, iconSize)));
    ui.icon_label->setAttribute(Qt::WA_TransparentForMouseEvents);

    ui.message_label->setText(text);
    ui.message_label->setWordWrap(true);
    ui.message_label->setContentsMargins(0, 0, 0, 0);

    // Вычисляем доступную ширину для текста:
    // Ширина всего виджета - (левый + правый отступ) - ширина иконки - отступ между иконкой и текстом
    constexpr int textAvailableWidth = totalWidth - (paddingW * 2) - iconSize - iconSpacing;

    QTextDocument doc;
    doc.setDefaultFont(ui.message_label->font());
    doc.setPlainText(text);
    doc.setDocumentMargin(0);
    doc.setTextWidth(textAvailableWidth);

    // Считаем высоту текста
    const QFontMetricsF fm(ui.message_label->font());
    // qCeil округляет вверх. descent() нужен для "хвостиков" букв (р, у, д, щ).
    int contentHeight = qCeil(doc.size().height());

    // Высота не должна быть меньше иконки
    if (contentHeight < iconSize) {
        contentHeight = iconSize;
    }

    int finalHeight = contentHeight + (paddingH * 2);

    constexpr int borderSafetyMargin = 2;

    finalHeight += borderSafetyMargin;

    setFixedHeight(finalHeight);

    m_content = contentWidget;

    installEventFilter(this);

    if (settings) {
        settings->installEventFilter(this);
        connect(settings, &QObject::destroyed, this, [] {
            for (auto *n: stack) {
                if (!n) continue;
                n->hide();
                n->deleteLater();
            }
            stack.clear();
        });
    }

    QTimer::singleShot(0, this, [this]() {
        AcrylicHelper::setAcrylicEnabled(this, true);
        AcrylicHelper::updateRegion(this);
    });

    setWindowFlags(Qt::FramelessWindowHint
                   | Qt::Tool
                   | Qt::WindowDoesNotAcceptFocus
                   | Qt::BypassWindowManagerHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);

    if (m_content != this) {
        fx = new QGraphicsOpacityEffect(m_content);
        fx->setOpacity(0.0);
        m_content->setGraphicsEffect(fx);
    } else {
        fx = new QGraphicsOpacityEffect(this);
        fx->setOpacity(0.0);
        setGraphicsEffect(fx);
    }

    animInOpacity = new QPropertyAnimation(fx, "opacity", this);
    animInOpacity->setDuration(200);
    animInOpacity->setStartValue(0.0);
    animInOpacity->setEndValue(1.0);
    animInOpacity->setEasingCurve(QEasingCurve::InOutCubic);

    animInPos = new QPropertyAnimation(this, "pos", this);
    animInPos->setDuration(250);
    animInPos->setEasingCurve(QEasingCurve::InOutCubic);

    animOutOpacity = new QPropertyAnimation(fx, "opacity", this);
    animOutOpacity->setDuration(200);
    animOutOpacity->setStartValue(1.0);
    animOutOpacity->setEndValue(0.0);
    animOutOpacity->setEasingCurve(QEasingCurve::InOutCubic);

    animOutPos = new QPropertyAnimation(this, "pos", this);
    animOutPos->setDuration(250);
    animOutPos->setEasingCurve(QEasingCurve::InOutCubic);

    hideTimer.setSingleShot(true);
    hideTimer.setInterval(3000);

    connect(&hideTimer, &QTimer::timeout,
            this, [this]() { startHideAnimation(true); });
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
    if (o == this && e->type() == QEvent::MouseButtonPress) {
        // Если уже запущена анимация закрытия — игнорируем повторные клики
        if (!m_closing) startHideAnimation(true);
        return true;
    }

    if (o == settings) {
        if (e->type() == QEvent::Move || e->type() == QEvent::Resize) {
            // перемещаем все уведомления без анимации, чтобы они точно следовали за окном
            for (int i = 0; i < stack.size(); ++i) {
                auto *n = stack[i];
                if (!n) continue;
                n->move(n->basePosition(i));
            }
            return false;
        }
        if (e->type() == QEvent::Close) {
            // удаляем все уведомления мгновенно
            for (auto *n: stack) {
                if (!n) continue;
                n->hide();
                n->deleteLater();
            }
            stack.clear();
            return false;
        }
    }
    return QObject::eventFilter(o, e);
}

QPoint SaveNotification::basePosition(const int index) const {
    constexpr int margin = 12;
    constexpr int spacing = 12;

    // если окно настроек отсутствует или скрыто — fallback по экрану
    if (!settings) {
        const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
        const int x = screen.right() - width() - margin;
        const int y = screen.bottom() - height() - margin - index * (height() + spacing);
        return QPoint(x, y);
    }

    // главное окно всегда даёт ГЛОБАЛЬНУЮ геометрию
    const QRect win = settings->geometry();

    // правая нижняя точка относительно главного окна
    const int x = win.right() - width() - margin;
    const int y = win.bottom() - height() - margin - index * (height() + spacing);

    return QPoint(x, y);
}

void SaveNotification::shiftUp(const int dy) {
    const auto a = new QPropertyAnimation(this, "pos");
    a->setDuration(270);
    a->setStartValue(pos());
    a->setEndValue(pos() - QPoint(0, dy));
    a->setEasingCurve(QEasingCurve::InOutCubic);
    a->start(QAbstractAnimation::DeleteWhenStopped);
}

void SaveNotification::startShowAnimation() {
    if (!settings || !settings->isVisible()) {
        deleteLater();
        return;
    }

    const int index = stack.indexOf(this);
    if (index < 0) return;

    // basePosition теперь возвращает позицию в координатах parent (settings)
    const QPoint endP = basePosition(index);

    // стартовая позиция — немного правее/сверху от конца, но в координатах parent
    const auto startP = QPoint(endP.x() + 10, endP.y());

    // перед показом ставим в позицию старта
    move(startP);
    show();

    auto *progressAnim = new QPropertyAnimation(this, "progress", this);
    progressAnim->setDuration(hideTimer.interval());
    progressAnim->setStartValue(0.0);
    progressAnim->setEndValue(1.0);
    progressAnim->setEasingCurve(QEasingCurve::Linear);
    progressAnim->start(QAbstractAnimation::DeleteWhenStopped);


    // защита от многократного запуска анимаций
    animInPos->stop();
    animInOpacity->stop();

    animInPos->setStartValue(startP);
    animInPos->setEndValue(endP);

    animInPos->start();
    animInOpacity->start();

    hideTimer.start(hideTimer.interval());
}

void SaveNotification::startHideAnimation(bool removeFromStack) {
    // защита: если уже запущено скрытие — ничего не делать
    if (m_closing) return;
    m_closing = true;

    if (!isVisible()) {
        // всё равно нужно очистить из стека если требуется
        if (removeFromStack) {
            if (const int idx = stack.indexOf(this); idx >= 0) stack.removeAt(idx);
        }
        deleteLater();
        return;
    }

    hideTimer.stop();

    // гарантированно удалить предыдущие слоты finished (защита от множественных connect)
    disconnect(animOutOpacity, &QPropertyAnimation::finished, nullptr, nullptr);

    animOutOpacity->stop();
    animOutPos->stop();

    const QPoint startP = pos();
    const QPoint endP = startP + QPoint(0, 10);

    animOutPos->setStartValue(startP);
    animOutPos->setEndValue(endP);

    // Подключаем единичный слот — срабатывает только для этого объекта
    connect(animOutOpacity, &QPropertyAnimation::finished, this, [this, removeFromStack]() {
        this->hide();

        if (!removeFromStack) {
            // если не удаляем из стека — просто сбрасываем флаг
            m_closing = false;
            return;
        }

        const qsizetype removedIndex = stack.indexOf(this);
        if (removedIndex >= 0) {
            stack.removeAt(removedIndex);
        }
        this->deleteLater();

        // смещаем уведомления ниже удалённого
        for (int i = (removedIndex < 0 ? 0 : removedIndex); i < stack.size(); ++i) {
            auto *n = stack[i];
            if (!n) continue;
            n->animateTo(i);
        }
    });

    animOutPos->start();
    animOutOpacity->start();
}

void SaveNotification::showFor(SettingsWindow *settings, const QString &text) {
    if (!settings) return;

    // если уже 3 — скрываем нижнее (но НЕ меняем ему индекс)
    if (stack.size() == 3) {
        stack.last()->startHideAnimation(true);
    }

    auto *n = new SaveNotification(settings, text);

    // сдвигаем старые вверх (+1 индекс) по анимации
    for (int i = 0; i < stack.size(); ++i) {
        auto *notif = stack[i];
        if (!notif) continue;
        notif->animateTo(i + 1);
    }

    // вставляем новый
    stack.prepend(n);
    n->startShowAnimation();
}

void SaveNotification::animateTo(const int newIndex) {
    const QPoint target = basePosition(newIndex);

    auto *a = new QPropertyAnimation(this, "pos");
    a->setDuration(250);
    a->setStartValue(pos());
    a->setEndValue(target);
    a->setEasingCurve(QEasingCurve::InOutCubic);
    a->start(QAbstractAnimation::DeleteWhenStopped);
}
