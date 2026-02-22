#include "windowDragger.h"
#include "../../core/config/logger.h"
#include <QMouseEvent>

WindowDragger::WindowDragger(QWidget *target)
    : QObject(target), win(target) {
    win->installEventFilter(this);
    LOG_DEBUG() << "WindowDragger initialized";
}

void WindowDragger::addIgnoredWidget(QWidget *w) {
    ignore.insert(w);
}

bool WindowDragger::eventFilter(QObject *obj, QEvent *ev) {
    if (obj != win) return QObject::eventFilter(obj, ev);

    switch (ev->type()) {
        case QEvent::MouseButtonPress: {
            const auto *e = dynamic_cast<QMouseEvent *>(ev);
            if (!e) break;
            if (e->button() != Qt::LeftButton) break;

            QWidget *child = win->childAt(e->position().toPoint());
            while (child) {
                if (ignore.contains(child)) {
                    dragging = false;
                    break;
                }
                child = child->parentWidget();
            }
            if (!child) dragging = true;

            if (dragging) {
                dragOffset = e->globalPosition().toPoint() - win->frameGeometry().topLeft();
                return true;
            }
            break;
        }

        case QEvent::MouseMove: {
            if (dragging) {
                const auto *e = dynamic_cast<QMouseEvent *>(ev);
                if (!e) break;
                win->move(e->globalPosition().toPoint() - dragOffset);
                return true;
            }
            break;
        }

        case QEvent::MouseButtonRelease:
            dragging = false;
            break;

        default:
            break;
    }

    return QObject::eventFilter(obj, ev);
}
