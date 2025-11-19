#include "window_dragger.h"

#include <QMouseEvent>

WindowDragger::WindowDragger(QWidget *target)
    : QObject(target), win(target) {
    win->installEventFilter(this);
}

void WindowDragger::addIgnoredWidget(QWidget *w) {
    ignore.insert(w);
}

bool WindowDragger::eventFilter(QObject *obj, QEvent *ev) {
    if (obj != win) return QObject::eventFilter(obj, ev);

    switch (ev->type()) {
        case QEvent::MouseButtonPress: {
            const auto *e = static_cast<QMouseEvent *>(ev);
            if (e->button() != Qt::LeftButton) break;

            if (QWidget *child = win->childAt(e->position().toPoint()); child && ignore.contains(child)) {
                dragging = false;
                break;
            }

            dragging = true;
            dragOffset = e->globalPosition().toPoint() - win->frameGeometry().topLeft();
            return true;
        }

        case QEvent::MouseMove: {
            if (dragging) {
                const auto *e = static_cast<QMouseEvent *>(ev);
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
