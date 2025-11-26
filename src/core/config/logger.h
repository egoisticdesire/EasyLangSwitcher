#pragma once
#include <iostream>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDateTime>
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QTextStream>

class ThreadedLogger final : public QThread {
    Q_OBJECT

public:
    enum class Level { DEBUG, INFO, WARN, ERR };

    static ThreadedLogger &instance() {
        static ThreadedLogger logger;
        return logger;
    }

    void enqueue(const QString &msg) {
        QMutexLocker locker(&m_mutex);
        m_queue.enqueue(msg);
        m_wait.wakeOne();
    }

    void run() override {
        while (!m_terminate) {
            QString msg;
            {
                QMutexLocker locker(&m_mutex);
                if (m_queue.isEmpty()) {
                    m_wait.wait(&m_mutex, 100);
                }
                if (!m_queue.isEmpty()) msg = m_queue.dequeue();
            }
            if (!msg.isEmpty()) {
                const std::wstring wmsg = msg.toStdWString();
                std::wcout << wmsg << std::endl;
                // QString threadStr = QString("LoggerThread=%1").arg(reinterpret_cast<quintptr>(currentThreadId()));
                // std::wcout << (threadStr + " " + msg).toStdWString() << std::endl;
            }
        }
    }

    void stop() {
        m_terminate = true;
        m_wait.wakeOne();
        wait();
    }

private:
    ThreadedLogger() { start(); }
    ~ThreadedLogger() override { stop(); }

    QQueue<QString> m_queue;
    QMutex m_mutex;
    QWaitCondition m_wait;
    bool m_terminate = false;
};

class Logger {
public:
    enum class Level { DEBUG, INFO, WARN, ERR };

    static inline bool _debug = true;

    static QString cleanFunction(const QString &func) {
        static const QRegularExpression reCtor(R"(\{[^\}]+\})");
        static const QRegularExpression reLambda(R"(<lambda_[^>]+>)");
        static const QRegularExpression reOpCall(R"(::operator\s*\([^\)]*\))");
        static const QRegularExpression reOpIndex(R"(::operator\[\])");
        static const QRegularExpression reColons(R"(:{2,})");
        static const QRegularExpression reTrailing(R"(::$)");
        static const QRegularExpression reQtModifiers(R"(\b__\w+\b)"); // __cdecl, __stdcall
        static const QRegularExpression reQtClass(R"(\b(const\s+)?class\b)");
        static const QRegularExpression reExtraSpaces(R"(\s+)");

        QString f = func;

        f.replace(reCtor, "");
        f.replace(reLambda, "");
        f.replace(reOpCall, "");
        f.replace(reOpIndex, "");
        f.replace(reColons, "::");
        f.replace(reTrailing, "");

        // очистка Qt-модификаторов и лишних ключевых слов
        f.replace(reQtModifiers, "");
        f.replace(reQtClass, "");
        f.replace(reExtraSpaces, " ");

        // упрощаем параметры функций до (...)
        if (const int idx = f.indexOf('('); idx != -1) {
            f = f.left(idx + 1) + "...)";
        }

        return f.trimmed();
    }


    Logger(const char *file, const char *function, int line, Level level = Level::DEBUG)
        : m_level(level), m_file(file), m_function(function), m_line(line) {
        // минимальный уровень в зависимости от тумблера
        Level minLevel = _debug ? Level::DEBUG : Level::INFO;
        m_suppressed = (level < minLevel);
        if (m_suppressed) return;

        // цвета
        m_reset = "\033[0m";
        m_red = "\033[38;2;239;41;41m";
        m_green = "\033[38;2;135;225;110m";
        m_blue = "\033[38;2;4;96;215m";
        m_cyan = "\033[38;2;100;195;205m";
        m_gold = "\033[38;2;234;191;0m";
        m_purple = "\033[38;2;200;120;200m";
        m_gray = "\033[38;2;175;175;175m";

        switch (level) {
            case Level::DEBUG: m_levelColor = m_blue;
                break;
            case Level::INFO: m_levelColor = m_gray;
                break;
            case Level::WARN: m_levelColor = m_gold;
                break;
            case Level::ERR: m_levelColor = m_red;
                break;
        }

        auto visibleLength = [](const QString &s) {
            static const QRegularExpression reColors("\033\\[[0-9;]*m");
            QString plain = s;
            plain.remove(QRegularExpression(reColors));
            return plain.length();
        };

        // QString threadStr = QString("Thread=%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));

        QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
        QString levelStr = levelToString(level).leftJustified(9, ' ');

        QString formattedFileName = QString("%1%2%3").arg(m_purple, QFileInfo(file).fileName(), m_reset);
        QString formattedFuncName = QString("%1%2%3").arg(m_cyan, cleanFunction(function), m_reset);
        QString formattedName = QString("%1 %2→%3 %4").arg(formattedFileName, m_levelColor, m_reset, formattedFuncName);

        qsizetype padRaw = 100 - visibleLength(formattedName);
        if (int pad = static_cast<int>(padRaw); pad > 0)
            formattedName = QString(pad, ' ') + formattedName;

        QString lineStr = QString::number(line).leftJustified(5, ' ');

        m_stream
                // << threadStr << " "
                << m_green << "[" << timeStr << "]" << m_reset << " "
                << m_levelColor << levelStr << m_reset << " "
                << formattedName
                << m_levelColor << ":" << m_reset
                << m_cyan << lineStr << m_reset
                << " ";
    }

    ~Logger() {
        if (!m_suppressed) ThreadedLogger::instance().enqueue(m_str);
    }

    template<typename T>
    Logger &operator<<(const T &value) {
        if (!m_suppressed) QTextStream(&m_str) << m_levelColor << value << m_reset;
        return *this;
    }

private:
    bool m_suppressed = false;

    QString m_str;
    QTextStream m_stream{&m_str};

    Level m_level;
    QString m_levelColor;

    QString m_reset, m_green, m_cyan, m_blue, m_gray, m_gold, m_red, m_purple;

    const char *m_file;
    const char *m_function;
    int m_line;

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

#define LOG_DEBUG() if (!Logger::_debug) ; else Logger(__FILE__, __FUNCTION__, __LINE__, Logger::Level::DEBUG)
#define LOG_INFO() Logger(__FILE__, __FUNCTION__, __LINE__, Logger::Level::INFO)
#define LOG_WARNING() Logger(__FILE__, __FUNCTION__, __LINE__, Logger::Level::WARN)
#define LOG_ERROR() Logger(__FILE__, __FUNCTION__, __LINE__, Logger::Level::ERR)
