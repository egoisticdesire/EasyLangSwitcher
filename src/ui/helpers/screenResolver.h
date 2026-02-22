#pragma once
#include <QGuiApplication>
#include <QList>
#include <QScreen>

namespace ScreenResolver {
    inline const QScreen *primaryOrFirst() {
        if (const QScreen *primary = QGuiApplication::primaryScreen()) return primary;
        const QList<QScreen *> screens = QGuiApplication::screens();
        return screens.isEmpty() ? nullptr : screens.first();
    }

    inline const QScreen *atPointOrPrimary(const QPoint &point) {
        if (const QScreen *screen = QGuiApplication::screenAt(point)) return screen;
        return primaryOrFirst();
    }
}
