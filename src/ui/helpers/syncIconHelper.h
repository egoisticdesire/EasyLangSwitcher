#pragma once
#include "iconHelper.h"
#include <QColor>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <QSize>

namespace SyncIconHelper {
    inline constexpr int CanvasSize = 24;
    inline constexpr int IconSize = 18;

    inline QPixmap baseSyncPixmap(const QColor &color = QColor(175, 175, 175)) {
        return IconHelper::loadIcon(
            ":/icons/icons/SyncFilled.svg",
            color,
            QSize(IconSize, IconSize)
        ).pixmap(IconSize, IconSize);
    }

    inline void drawPendingBadge(QPainter &painter) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(72, 155, 255));
        constexpr int dot = 7;
        painter.drawEllipse(QRect(CanvasSize - dot - 1, 1, dot, dot));
    }

    inline QIcon buildRotated(const int angle, const bool hasPendingUpdate) {
        QPixmap canvas(CanvasSize, CanvasSize);
        canvas.fill(Qt::transparent);

        const QPixmap pix = baseSyncPixmap();

        QPainter painter(&canvas);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.translate(CanvasSize / 2.0, CanvasSize / 2.0);
        painter.rotate(angle);
        painter.drawPixmap(-IconSize / 2.0, -IconSize / 2.0, pix);

        if (hasPendingUpdate) {
            painter.resetTransform();
            drawPendingBadge(painter);
        }
        painter.end();

        return QIcon(canvas);
    }

    inline QIcon buildStatic(const bool hasPendingUpdate) {
        QPixmap canvas(CanvasSize, CanvasSize);
        canvas.fill(Qt::transparent);

        const QPixmap base = baseSyncPixmap();

        QPainter painter(&canvas);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.drawPixmap((CanvasSize - IconSize) / 2, (CanvasSize - IconSize) / 2, base);

        if (hasPendingUpdate) {
            drawPendingBadge(painter);
        }
        painter.end();

        return QIcon(canvas);
    }
}
