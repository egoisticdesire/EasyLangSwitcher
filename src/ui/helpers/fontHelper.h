#pragma once
#include "../../core/config/logger.h"
#include <QApplication>
#include <QFontDatabase>
#include <QList>

class QApplication;

class FontManager {
public:
    static constexpr int DefaultSize = 10;
    static constexpr int xs = 8;
    static constexpr int s = 9;
    static constexpr int m = 11;
    static constexpr int l = 12;
    static constexpr int xl = 13;

    static void init(QApplication &app) {
        const int id = QFontDatabase::addApplicationFont(":/fonts/fonts/Inter-Regular.ttf");
        if (id == -1) return;

        const QStringList families = QFontDatabase::applicationFontFamilies(id);
        if (families.isEmpty()) {
            LOG_WARNING() << "Application font loaded but no font families resolved";
            return;
        }

        const QString family = families.first();
        QFont appFont(family);
        appFont.setPointSize(DefaultSize);
        appFont.setHintingPreference(QFont::PreferNoHinting);
        appFont.setStyleStrategy(QFont::PreferAntialias);
        QApplication::setFont(appFont);

        LOG_DEBUG() << "Font family: " << family << "; size: " << appFont.pointSize() << "pt";
    }

    static QFont getFont(const int size) {
        QFont f = QApplication::font();
        f.setPixelSize(size);
        return f;
    }

    static QFont Default() { return getFont(DefaultSize); }
    static QFont ExtraSmall() { return getFont(xs); }
    static QFont Small() { return getFont(s); }
    static QFont Medium() { return getFont(m); }
    static QFont Big() { return getFont(l); }
    static QFont ExtraBig() { return getFont(xl); }
};
