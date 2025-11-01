#pragma once
#include <QIcon>
#include <QSvgRenderer>
#include <QPainter>
#include <QSize>

class SvgHelper {
public:
    static QIcon loadSvgIcon(const QString &path, const QSize &size = QSize(22, 22)) {
        QSvgRenderer renderer(path);
        if (!renderer.isValid()) return QIcon();
        QPixmap pixmap(size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        renderer.render(&painter);
        return QIcon(pixmap);
    }
};
