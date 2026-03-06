#pragma once
#include <QHash>

#include <cstdint>

class TranslationEntry
{
public:
    QString en;
    QString ru;
};

class Lang
{
public:
    enum class Locale : std::uint8_t
    {
        EN,
        RU,
    };

    // Использовать язык из настроек (основной способ)
    static QString tr(const QString& key);

    // Прямой вариант с enum
    static QString tr(const QString& key, Locale locale);

    // "ru" / "en"
    static QString tr(const QString& key, QStringView localeCode);

    // Алиас под const char*
    static QString tr(const char* key)
    {
        return tr(QString::fromUtf8(key));
    }

    static const QHash<QString, TranslationEntry>& allTranslations();
};
