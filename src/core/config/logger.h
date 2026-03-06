#pragma once
#include <QThread>

#include <QFileInfo>
#include <QMutex>
#include <QQueue>
#include <QRegularExpression>
#include <QWaitCondition>
#include <atomic>
#include <iostream>
#include <type_traits>
#include <utility>

#include <cstdint>

namespace logger_detail
{
template <typename T, typename = void> struct has_qtextstream_insertion : std::false_type
{
};

template <typename T>
struct has_qtextstream_insertion<T, std::void_t<decltype(std::declval<QTextStream&>() << std::declval<const T&>())>>
    : std::true_type
{
};

template <typename T, typename = void> struct has_qdebug_insertion : std::false_type
{
};

template <typename T>
struct has_qdebug_insertion<T, std::void_t<decltype(std::declval<QDebug&>() << std::declval<const T&>())>>
    : std::true_type
{
};
} // namespace logger_detail

class ThreadedLogger final : public QThread
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ThreadedLogger)

public:
    enum class Level : std::uint8_t
    {
        DEBUG,
        INFO,
        WARN,
        ERR
    };

    static ThreadedLogger& instance()
    {
        static ThreadedLogger logger;
        return logger;
    }

    void enqueue(const QString& msg)
    {
        QMutexLocker locker(&m_mutex);
        m_queue.enqueue(msg);
        m_wait.wakeOne();
    }

    void run() override
    {
        for (;;) {
            QString msg;
            {
                QMutexLocker locker(&m_mutex);
                while (m_queue.isEmpty() && !m_terminate.load()) {
                    m_wait.wait(&m_mutex);
                }
                if (m_queue.isEmpty() && m_terminate.load()) {
                    break;
                }
                msg = m_queue.dequeue();
            }
            if (!msg.isEmpty()) {
                const std::wstring wmsg = msg.toStdWString();
                std::wcout << wmsg << L'\n';
                // QString threadStr = QString("LoggerThread=%1").arg(reinterpret_cast<quintptr>(currentThreadId()));
                // std::wcout << (threadStr + " " + msg).toStdWString() << std::endl;
            }
        }
    }

    void stop()
    {
        m_terminate.store(true);
        m_wait.wakeAll();
        if (isRunning()) {
            wait();
        }
    }

private:
    ThreadedLogger()
    {
        start();
    }
    ~ThreadedLogger() override
    {
        stop();
    }

    QQueue<QString> m_queue;
    QMutex m_mutex;
    QWaitCondition m_wait;
    std::atomic_bool m_terminate = false;
};

class Logger
{
    Q_DISABLE_COPY_MOVE(Logger)

public:
    enum class Level : std::uint8_t
    {
        DEBUG,
        INFO,
        WARN,
        ERR
    };

    static inline bool _debug = true;

    static QString cleanFunction(const QString& func)
    {
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
        if (const qsizetype idx = f.indexOf('('); idx != -1) {
            f = f.left(idx + 1) + "...)";
        }

        return f.trimmed();
    }

    Logger(const char* file, const char* function, const int line, const Level level = Level::DEBUG)
        : m_level(level), m_file(file), m_function(function), m_line(line)
    {
        // Быстрая проверка: нужно ли вообще логировать?
        const Level minLevel = _debug ? Level::DEBUG : Level::INFO;
        m_suppressed = (level < minLevel);
        if (m_suppressed) {
            return;
        }

        // Определяем цвет уровня (используем готовые статические строки)
        switch (level) {
            case Level::DEBUG:
                m_levelColor = blueColor();
                break;
            case Level::INFO:
                m_levelColor = grayColor();
                break;
            case Level::WARN:
                m_levelColor = goldColor();
                break;
            case Level::ERR:
                m_levelColor = redColor();
                break;
        }

        // Вспомогательная функция теперь внутри, чтобы не засорять класс
        auto visibleLength = [](const QString& s) {
            static const QRegularExpression reColors("\033\\[[0-9;]*m");
            QString plain = s;
            plain.remove(reColors);
            return plain.length();
        };

        // Формируем мета-данные только если лог активен
        // QString threadStr = QString("Thread=%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
        const QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
        const QString levelStr = levelToString(level).leftJustified(9, ' ');

        QString formattedFileName = QString("%1%2%3").arg(purpleColor(), QFileInfo(file).fileName(), resetColor());
        QString formattedFuncName = QString("%1%2%3").arg(cyanColor(), cleanFunction(function), resetColor());
        QString formattedName =
                QString("%1 %2→%3 %4").arg(formattedFileName, m_levelColor, resetColor(), formattedFuncName);

        // Выравнивание
        if (const qsizetype padRaw = 80 - visibleLength(formattedName); padRaw > 0) {
            formattedName = QString(static_cast<int>(padRaw), ' ') + formattedName;
        }

        const QString lineStr = QString::number(line).leftJustified(5, ' ');

        // Записываем в поток
        m_stream
                // << threadStr << " "
                << greenColor() << "[" << timeStr << "]" << resetColor() << " " << m_levelColor << levelStr
                << resetColor() << " " << formattedName << m_levelColor << "#" << resetColor() << cyanColor() << lineStr
                << resetColor() << " ";
    }

    ~Logger()
    {
        if (!m_suppressed) {
            ThreadedLogger::instance().enqueue(m_str);
        }
    }

    template <typename T> Logger& operator<<(const T& value)
    {
        if (m_suppressed) {
            return *this;
        }

        if constexpr (logger_detail::has_qtextstream_insertion<T>::value) {
            QTextStream(&m_str) << m_levelColor << value << resetColor();
        }
        else if constexpr (logger_detail::has_qdebug_insertion<T>::value) {
            QString rendered;
            {
                QDebug debugStream(&rendered);
                debugStream.noquote().nospace() << value;
            }
            QTextStream(&m_str) << m_levelColor << rendered << resetColor();
        }
        else {
            QTextStream(&m_str) << m_levelColor << "<unprintable-value>" << resetColor();
        }
        return *this;
    }

private:
    bool m_suppressed = false;
    QString m_str;
    QTextStream m_stream{&m_str};
    Level m_level;
    QString m_levelColor;

    [[nodiscard]] static QString resetColor()
    {
        return QStringLiteral("\033[0m");
    }
    [[nodiscard]] static QString greenColor()
    {
        return QStringLiteral("\033[38;2;135;225;110m");
    }
    [[nodiscard]] static QString cyanColor()
    {
        return QStringLiteral("\033[38;2;100;195;205m");
    }
    [[nodiscard]] static QString blueColor()
    {
        return QStringLiteral("\033[38;2;4;96;215m");
    }
    [[nodiscard]] static QString grayColor()
    {
        return QStringLiteral("\033[38;2;175;175;175m");
    }
    [[nodiscard]] static QString goldColor()
    {
        return QStringLiteral("\033[38;2;234;191;0m");
    }
    [[nodiscard]] static QString redColor()
    {
        return QStringLiteral("\033[38;2;239;41;41m");
    }
    [[nodiscard]] static QString purpleColor()
    {
        return QStringLiteral("\033[38;2;200;120;200m");
    }

    const char* m_file;
    const char* m_function;
    int m_line;

    static QString levelToString(const Level lvl)
    {
        switch (lvl) {
            case Level::DEBUG:
                return "DEBUG";
            case Level::INFO:
                return "INFO";
            case Level::WARN:
                return "WARNING";
            case Level::ERR:
                return "ERROR";
        }
        return "UNKNOWN";
    }
};

#define LOG_DEBUG()                                                                                                    \
    if (!Logger::_debug) {                                                                                             \
    }                                                                                                                  \
    else                                                                                                               \
        Logger(__FILE__, __FUNCTION__, __LINE__, Logger::Level::DEBUG)
#define LOG_INFO() Logger(__FILE__, __FUNCTION__, __LINE__, Logger::Level::INFO)
#define LOG_WARNING() Logger(__FILE__, __FUNCTION__, __LINE__, Logger::Level::WARN)
#define LOG_ERROR() Logger(__FILE__, __FUNCTION__, __LINE__, Logger::Level::ERR)
