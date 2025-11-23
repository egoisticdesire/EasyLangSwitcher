#pragma once
#include <iostream>
#include <QString>
#include <QDateTime>
#include <QTextStream>

class Logger {
public:
    enum class Level { DEBUG, INFO, WARN, ERR };

    Logger(const char *className, int line, Level level = Level::DEBUG)
        : m_level(level) {
        // ANSI цвета
        m_reset = "\033[0m";
        m_green = "\033[38;2;135;225;110m";
        m_cyan = "\033[38;2;100;195;205m";
        m_blue = "\033[38;2;4;96;215m";
        m_lightGray = "\033[38;2;175;175;175m";
        m_gold = "\033[38;2;234;191;0m";
        m_red = "\033[38;2;239;41;41m";

        switch (level) {
            case Level::DEBUG: m_levelColor = m_blue;
                break;
            case Level::INFO: m_levelColor = m_lightGray;
                break;
            case Level::WARN: m_levelColor = m_gold;
                break;
            case Level::ERR: m_levelColor = m_red;
                break;
        }

        QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
        QString levelStr = levelToString(level).leftJustified(9, ' ');
        QString nameStr = QString(className).rightJustified(60, ' ');
        QString lineStr = QString::number(line).leftJustified(5, ' ');

        m_stream << m_green << "[" << timeStr << "]" << m_reset << " "
                << m_levelColor << levelStr << m_reset << " "
                << m_cyan << nameStr << m_reset
                << m_levelColor << ":" << m_reset
                << m_cyan << lineStr << m_reset
                << " ";
    }

    ~Logger() {
        const std::wstring wmsg = m_str.toStdWString();
        std::wcout << wmsg << std::endl;
    }

    template<typename T>
    Logger &operator<<(const T &value) {
        QTextStream(&m_str) << m_levelColor << value << m_reset;
        return *this;
    }

private:
    QString m_str;
    QTextStream m_stream{&m_str};
    Level m_level;
    QString m_levelColor;
    QString m_reset, m_green, m_cyan, m_blue, m_lightGray, m_gold, m_red;

    static QString levelToString(const Level lvl) {
        switch (lvl) {
            case Level::DEBUG: return "DEBUG";
            case Level::INFO: return "INFO";
            case Level::WARN: return "WARNING";
            case Level::ERR: return "ERROR";
        }
        return "UNKNOWN";
    }
};

#define LOG_DEBUG() Logger(__FUNCTION__, __LINE__, Logger::Level::DEBUG)
#define LOG_INFO()  Logger(__FUNCTION__, __LINE__, Logger::Level::INFO)
#define LOG_WARNING()  Logger(__FUNCTION__, __LINE__, Logger::Level::WARN)
#define LOG_ERROR() Logger(__FUNCTION__, __LINE__, Logger::Level::ERR)
