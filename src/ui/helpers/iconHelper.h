#pragma once

#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QSvgRenderer>
#include <QFile>
#include <QSize>
#include <QString>

class IconHelper {
public:
    static QIcon loadIcon(
        const QString &path,
        const QColor &color = QColor(200, 200, 200),
        const QSize &size = QSize(18, 18)
    ) {
        if (path.endsWith(".svg", Qt::CaseInsensitive)) {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                return QIcon();

            QString svgData = file.readAll();
            file.close();

            if (color.isValid()) {
                const QString hex = color.name(QColor::HexRgb);
                svgData.replace("fill=\"currentColor\"", QString("fill=\"%1\"").arg(hex), Qt::CaseInsensitive);
            }

            QSvgRenderer renderer(svgData.toUtf8());
            if (!renderer.isValid())
                return QIcon();

            const QSize finalSize = size.isEmpty() ? renderer.defaultSize() : size;
            if (finalSize.isEmpty())
                return QIcon();

            QPixmap pixmap(finalSize);
            pixmap.fill(Qt::transparent);

            QPainter painter(&pixmap);
            renderer.render(&painter);

            return QIcon(pixmap);
        }

        // PNG/JPG и другие растровые форматы
        QPixmap pixmap(path);
        if (!pixmap.isNull() && !size.isEmpty())
            pixmap = pixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        return QIcon(pixmap);
    }
};
