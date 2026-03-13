#pragma once
#include "logger.h"

#include <QLoggingCategory>

class QtLoggerBridge
{
public:
    static void install()
    {
        qInstallMessageHandler(messageHandler);
    }

private:
    static void messageHandler(const QtMsgType type, const QMessageLogContext& context, const QString& msg)
    {
        auto lvl = Logger::Level::DEBUG;

        switch (type) {
            case QtDebugMsg:
                lvl = Logger::Level::DEBUG;
                break;
            case QtInfoMsg:
                lvl = Logger::Level::INFO;
                break;
            case QtWarningMsg:
                lvl = Logger::Level::WARN;
                break;
            case QtCriticalMsg:
            case QtFatalMsg:
                lvl = Logger::Level::ERR;
                break;
        }

        if (!Logger::_debug && lvl == Logger::Level::DEBUG) {
            return;
        }

        // Можно фильтровать конкретную категорию, если нужно
        // if (QString(context.category) != "qt.multimedia.ffmpeg") return;

        Logger(context.file ? context.file : "", context.function ? context.function : "", context.line, lvl) << msg;
    }
};
