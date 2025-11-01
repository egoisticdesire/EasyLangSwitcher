#pragma once
#include <QIcon>
#include <QSvgRenderer>
#include <QPainter>
#include <QSize>

class IconHelper {
public:
    static QIcon loadIcon(const QString &path, const QSize &size = QSize(22, 22)) {
        if (path.endsWith(".svg", Qt::CaseInsensitive)) {
            QSvgRenderer renderer(path);
            if (!renderer.isValid()) return QIcon();
            QPixmap pixmap(size);
            pixmap.fill(Qt::transparent);
            QPainter painter(&pixmap);
            renderer.render(&painter);
            return QIcon(pixmap);
        }

        // Для PNG/JPG и других растровых форматов
        QPixmap pixmap(path);
        if (!pixmap.isNull() && !size.isEmpty())
            pixmap = pixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        return QIcon(pixmap);
    }
};
