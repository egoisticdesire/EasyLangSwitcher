#pragma once

#include <QString>

class TranslationEntry {
public:
    QString ru;
    QString en;
};

class Lang {
public:
    enum class Locale {
        RU,
        EN
    };

    // Вернуть перевод по ключу; если ключ не найден — вернуть сам ключ.
    static QString tr(const QString &key, Locale locale = Locale::RU);

    // Удобные перегрузки
    static QString tr(const char *key, const Locale locale = Locale::RU) {
        return tr(QString::fromUtf8(key), locale);
    }

    // localeCode: "ru" / "en" (нечувствительно к регистру)
    static QString tr(const QString &key, const QString &localeCode);

    // Полезно для дебага / перечисления ключей (const ref, не изменяемый)
    static const QHash<QString, TranslationEntry> &allTranslations();
};
