#pragma once
#include "logger.h"
#include <QLoggingCategory>

class QtLoggerBridge {
public:
    static void install() {
        qInstallMessageHandler(messageHandler);
    }

private:
    static void messageHandler(const QtMsgType type, const QMessageLogContext &context, const QString &msg) {
        Logger::Level lvl = {};
        switch (type) {
            case QtDebugMsg: lvl = Logger::Level::DEBUG;
                break;
            case QtInfoMsg: lvl = Logger::Level::INFO;
                break;
            case QtWarningMsg: lvl = Logger::Level::WARN;
                break;
            case QtCriticalMsg:
            case QtFatalMsg: lvl = Logger::Level::ERR;
                break;
        }

        // Можно фильтровать конкретную категорию, если нужно
        // if (QString(context.category) != "qt.multimedia.ffmpeg") return;

        Logger(context.file, context.function, context.line, lvl) << msg;
    }
};
