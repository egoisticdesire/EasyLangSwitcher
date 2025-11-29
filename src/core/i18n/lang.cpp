#include "lang.h"
#include <QHash>
#include <QString>
#include <algorithm>

// Реализация: таблица создаётся один раз в потокобезопасном локу функции.
static const QHash<QString, TranslationEntry> &translationsTable() {
    static const QHash<QString, TranslationEntry> table = []() -> QHash<QString, TranslationEntry> {
        QHash<QString, TranslationEntry> t;

        // Простые строки
        t.insert(QStringLiteral("SELECT_KEY_LABEL"), {
                     QStringLiteral("Выберите клавишу"),
                     QStringLiteral("Select Key")
                 });
        t.insert(QStringLiteral("DELAY_LABEL"), {
                     QStringLiteral("Укажите время задержки"),
                     QStringLiteral("Set switch delay")
                 });
        t.insert(QStringLiteral("LANGUAGE_LABEL"), {
                     QStringLiteral("Язык приложения"),
                     QStringLiteral("Application language")
                 });
        t.insert(QStringLiteral("AUTOSTART_LABEL"), {
                     QStringLiteral("Автозапуск при старте Windows"),
                     QStringLiteral("Launch at Windows startup")
                 });
        t.insert(QStringLiteral("AUTOSTART_VALUE_ENABLED"), {
                     QStringLiteral("Включить"),
                     QStringLiteral("Enable")
                 });
        t.insert(QStringLiteral("AUTOSTART_VALUE_DISABLED"), {
                     QStringLiteral("Выключить"),
                     QStringLiteral("Disable")
                 });
        t.insert(QStringLiteral("RESTORE_DEFAULT_LABEL"), {
                     QStringLiteral("Сбросить настройки"),
                     QStringLiteral("Restore default settings")
                 });
        t.insert(QStringLiteral("MENU_GENERAL"), {
                     QStringLiteral("Общие"),
                     QStringLiteral("General")
                 });
        t.insert(QStringLiteral("MENU_EXCLUSIONS"), {
                     QStringLiteral("Исключения"),
                     QStringLiteral("Exclusions")
                 });
        t.insert(QStringLiteral("MENU_INFO"), {
                     QStringLiteral("Информация"),
                     QStringLiteral("Info")
                 });
        t.insert(QStringLiteral("BTN_APPLY"), {
                     QStringLiteral("Применить"),
                     QStringLiteral("Apply")
                 });
        t.insert(QStringLiteral("SAVE_BUTTON"), {
                     QStringLiteral("Сохранить"),
                     QStringLiteral("Save")
                 });
        t.insert(QStringLiteral("CANCEL_BUTTON"), {
                     QStringLiteral("Отмена"),
                     QStringLiteral("Cancel")
                 });
        t.insert(QStringLiteral("ADD_APP_BUTTON"), {
                     QStringLiteral("  Добавить приложение..."),
                     QStringLiteral("  Add Application...")
                 });
        t.insert(QStringLiteral("ADD_REGEX_BUTTON"), {
                     QStringLiteral("  Добавить регулярное выражение..."),
                     QStringLiteral("  Add Regular Expression...")
                 });
        t.insert(QStringLiteral("TABLE_HEADER_TYPE"), {
                     QStringLiteral("Тип"),
                     QStringLiteral("Type")
                 });
        t.insert(QStringLiteral("TABLE_HEADER_PATH"), {
                     QStringLiteral("Путь / Шаблон"),
                     QStringLiteral("Path / Pattern")
                 });
        t.insert(QStringLiteral("TABLE_HEADER_RULE"), {
                     QStringLiteral("Правило"),
                     QStringLiteral("Rule")
                 });
        t.insert(QStringLiteral("TABLE_HEADER_DELETE"), {
                     QStringLiteral("Удаление"),
                     QStringLiteral("Delete")
                 });
        t.insert(QStringLiteral("ON_BUTTON"), {
                     QStringLiteral("Вкл"),
                     QStringLiteral("On")
                 });
        t.insert(QStringLiteral("OFF_BUTTON"), {
                     QStringLiteral("Выкл"),
                     QStringLiteral("Off")
                 });
        t.insert(QStringLiteral("ENABLE_ALL_RULES_BUTTON"), {
                     QStringLiteral("Включить все правила"),
                     QStringLiteral("Enable All Rules")
                 });
        t.insert(QStringLiteral("DISABLE_ALL_RULES_BUTTON"), {
                     QStringLiteral("Выключить все правила"),
                     QStringLiteral("Disable All Rules")
                 });
        t.insert(QStringLiteral("CLEAR_LIST_BUTTON"), {
                     QStringLiteral("Очистить список"),
                     QStringLiteral("Clear List")
                 });
        t.insert(QStringLiteral("REGEX_PLACEHOLDER"), {
                     QStringLiteral("Введите имя процесса или регулярное выражение для исключения..."),
                     QStringLiteral("Enter a process name or a regular expression to exclude...")
                 });
        t.insert(QStringLiteral("CASE_SENSITIVITY_LABEL"), {
                     QStringLiteral("Чувствительность к регистру"),
                     QStringLiteral("Case Sensitivity")
                 });
        t.insert(QStringLiteral("FULL_MATCH_LABEL"), {
                     QStringLiteral("Полное совпадение"),
                     QStringLiteral("Full Match")
                 });
        t.insert(QStringLiteral("ALL_CHANGES_SAVED"), {
                     QStringLiteral("Все изменения сохранены автоматически"),
                     QStringLiteral("All changes saved automatically")
                 });
        t.insert(QStringLiteral("CHANGES_NOT_SAVED"), {
                     QStringLiteral("Изменения не сохранены"),
                     QStringLiteral("Changes not saved")
                 });
        t.insert(QStringLiteral("EXCLUSION_ADDED"), {
                     QStringLiteral("Правило исключения добавлено"),
                     QStringLiteral("Exclusion rule added")
                 });
        t.insert(QStringLiteral("EXCLUSION_REMOVED"), {
                     QStringLiteral("Правило исключения удалено"),
                     QStringLiteral("Exclusion rule removed")
                 });
        t.insert(QStringLiteral("EXCLUSIONS_CLEARED"), {
                     QStringLiteral("Список правил очищен"),
                     QStringLiteral("Exclusions list cleared")
                 });
        t.insert(QStringLiteral("DUPLICATE_ENTRY"), {
                     QStringLiteral("Такая запись уже существует"),
                     QStringLiteral("This entry already exists")
                 });
        t.insert(QStringLiteral("INVALID_INPUT"), {
                     QStringLiteral("Недопустимое значение"),
                     QStringLiteral("Invalid input")
                 });
        t.insert(QStringLiteral("REGEX_ERROR"), {
                     QStringLiteral("Ошибка в регулярном выражении"),
                     QStringLiteral("Regular expression error")
                 });
        t.insert(QStringLiteral("MISSING_PATH"), {
                     QStringLiteral("Не указан путь"),
                     QStringLiteral("Path not specified")
                 });
        t.insert(QStringLiteral("LANGUAGE_CHANGED"), {
                     QStringLiteral("Язык интерфейса изменен"),
                     QStringLiteral("Interface language changed")
                 });
        t.insert(QStringLiteral("SETTINGS_RESET"), {
                     QStringLiteral("Настройки сброшены до значений по умолчанию"),
                     QStringLiteral("Settings reset to default")
                 });
        t.insert(QStringLiteral("INFO_SWITCHES"), {
                     QStringLiteral("Переключений:"),
                     QStringLiteral("Switches:")
                 });
        t.insert(QStringLiteral("ENABLE_INDICATOR_LABEL"), {
                     QStringLiteral("Включить индикатор"),
                     QStringLiteral("Enable Indicator")
                 });
        t.insert(QStringLiteral("DISABLE_INDICATOR_LABEL"), {
                     QStringLiteral("Выключить индикатор"),
                     QStringLiteral("Disable Indicator")
                 });
        t.insert(QStringLiteral("FONT_COLOR_LABEL"), {
                     QStringLiteral("Цвет шрифта"),
                     QStringLiteral("Font Color")
                 });
        t.insert(QStringLiteral("BACKGROUND_COLOR_LABEL"), {
                     QStringLiteral("Цвет фона"),
                     QStringLiteral("Background Color")
                 });
        t.insert(QStringLiteral("BORDER_COLOR_LABEL"), {
                     QStringLiteral("Цвет рамки"),
                     QStringLiteral("Border Color")
                 });
        t.insert(QStringLiteral("DONATE_BUTTON_LABEL"), {
                     QStringLiteral("Поддержать проект"),
                     QStringLiteral("Support Project")
                 });
        t.insert(QStringLiteral("DONATE_BUTTON_TOOLTIP"), {
                     QStringLiteral("Открыть страницу поддержки проекта"),
                     QStringLiteral("Open the project support page")
                 });
        t.insert(QStringLiteral("CONFIRMATION_LABEL"), {
                     QStringLiteral("Вы уверены?"),
                     QStringLiteral("Are you sure?")
                 });
        t.insert(QStringLiteral("YES_BUTTON_LABEL"), {
                     QStringLiteral("Да"),
                     QStringLiteral("Yes")
                 });
        t.insert(QStringLiteral("NO_BUTTON_LABEL"), {
                     QStringLiteral("Нет"),
                     QStringLiteral("No")
                 });
        t.insert(QStringLiteral("KEY_SEQUENCE"), {
                     QStringLiteral("Клавиша..."),
                     QStringLiteral("Key...")
                 });

        // WARNINGS_POPUP / INFO_TEXT / INFO_FOOTER — многострочные HTML; используем fromUtf8 + raw string literals
        t.insert(QStringLiteral("WARNINGS_POPUP"), {
                     QString::fromUtf8(R"(
                <html><head/><body>
                <div style="color: rgba(255, 255, 255, 200); font-size: 12px;">
                <p align="justify" style="font-size: 14px;">
                <span style=" font-weight:700;">Предупреждения:</span>
                </p>
                <ul style="margin-top: 0px; margin-bottom: 0px; margin-left: 0px; margin-right: 0px; -qt-list-indent: 1;">
                <li align="justify" style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">
                <p align="justify">
                при выборе клавиши <code style="color: rgba(74, 144, 226, 255); font-size: 13px;"><span style=" font-weight:700;">Alt</span></code> в качестве модификатора переключения, её стандартное поведение будет отключено — а именно, активация верхнего меню окна;
                </p>
                </li>
                <li align="justify" style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">
                <p align="justify">
                при выборе клавиши <code style="color: rgba(74, 144, 226, 255); font-size: 13px;"><span style=" font-weight:700;">Caps Lock</span></code> в качестве модификатора переключения, её стандартное поведение будет изменено: короткое нажатие будет переключать раскладку, а длительное — выполнять стандартное действие.
                </p>
                </li>
                </ul>
                </div>
                </body></html>
            )"),
                     QString::fromUtf8(R"(
                <html><head/><body>
                <div style="color: rgba(255, 255, 255, 200); font-size: 12px;">
                <p align="justify" style="font-size: 14px;">
                <span style=" font-weight:700;">Warnings:</span>
                </p>
                <ul style="margin-top: 0px; margin-bottom: 0px; margin-left: 0px; margin-right: 0px; -qt-list-indent: 1;">
                <li align="justify" style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">
                <p align="justify">
                When selecting the <code style="color: rgba(74, 144, 226, 255); font-size: 13px;"><span style=" font-weight:700;">Alt</span></code> key as a layout switch modifier, its default behavior will be disabled — specifically, the activation of the window’s top menu;
                </p>
                </li>
                <li align="justify" style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">
                <p align="justify">
                When selecting the <code style="color: rgba(74, 144, 226, 255); font-size: 13px;"><span style=" font-weight:700;">Caps Lock</span></code>  key as a layout switch modifier, its default behavior will change: a short press will switch the layout, while a long press will perform the standard action.
                </p>
                </li>
                </ul>
                </div>
                </body></html>
            )")
                 });

        t.insert(QStringLiteral("INFO_TEXT"), {
                     QString::fromUtf8(R"(
                <html><head/><body>
                <p align="justify">
                <span style=" font-weight:700;">EasyLangSwitcher</span> — простое и лёгкое приложение для Windows, которое позволяет переключать раскладку клавиатуры <span style=" font-weight:700;">одной клавишей</span>. Переключение происходит циклично по всем языкам, добавленным в системе:
                </p>
                <ul style="margin-top: 0px; margin-bottom: 0px; margin-left: 0px; margin-right: 0px; -qt-list-indent: 1;">
                <li align="justify" style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">
                <span style=" font-weight:700;">Краткое нажатие</span> — смена текущей раскладки.
                </li>
                <li align="justify" style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">
                <span style=" font-weight:700;">Длительное нажатие</span> или использование комбинации (например, <span style=" font-weight:700;">Ctrl+C</span>) с назначенной клавишей — стандартное поведение кнопки сохраняется.
                </li>
                </ul>
                <p align="justify">
                Приложение позволяет вам:
                </p>
                <ul style="margin-top: 0px; margin-bottom: 0px; margin-left: 0px; margin-right: 0px; -qt-list-indent: 1;">
                <li align="justify" style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">
                Использовать любую клавишу для переключения раскладки без привычных сочетаний <span style=" font-weight:700;">Alt+Shift</span> или <span style=" font-weight:700;">Win+Space</span>.
                </li>
                <li align="justify" style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">
                Добавлять программы в <span style=" font-weight:700;">исключения</span>, чтобы переключение раскладки не мешало работе (например, в играх или специальных приложениях).
                </li>
                <li align="justify" style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">
                Видеть текущую раскладку благодаря удобной индикации.
                </li>
                </ul>
                <p align="justify">
                <span style=" font-weight:700;">Идея приложения</span> возникла из личного опыта: удобная отдельная клавиша на <span style=" font-weight:700;">MacBook</span>, возможности <span style=" font-weight:700;">BetterTouchTool</span> и индикация из <span style=" font-weight:700;">Input Source Pro</span> вдохновили на создание этого решения для Windows.
                </p>
                </body></html>
            )"),
                     QString::fromUtf8(R"(
                <html><head/><body>
                <p align="justify">
                <span style=" font-weight:700;">EasyLangSwitcher</span> is a simple and lightweight Windows application that allows you to switch keyboard layouts using <span style=" font-weight:700;">a single key</span>. Switching cycles through all languages added in the system:
                </p>
                <ul style="margin-top: 0px; margin-bottom: 0px; margin-left: 0px; margin-right: 0px; -qt-list-indent: 1;">
                <li align="justify" style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">
                <span style=" font-weight:700;">Short press</span> — changes the current layout.
                </li>
                <li align="justify" style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">
                <span style=" font-weight:700;">Long press</span> or using a combination (for example, <span style=" font-weight:700;">Ctrl+C</span>) with the assigned key preserves the standard behavior of that key.
                </li>
                </ul>
                <p align="justify">
                The application allows you to:
                </p>
                <ul style="margin-top: 0px; margin-bottom: 0px; margin-left: 0px; margin-right: 0px; -qt-list-indent: 1;">
                <li align="justify" style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">
                Use any key to switch layouts without the usual combinations <span style=" font-weight:700;">Alt+Shift</span> or <span style=" font-weight:700;">Win+Space</span>.
                </li>
                <li align="justify" style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">
                Add programs to <span style=" font-weight:700;">exceptions</span> so layout switching does not interfere with work (for example, in games or specialized applications).
                </li>
                <li align="justify" style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">
                See the current layout thanks to convenient visual indication.
                </li>
                </ul>
                <p align="justify">
                <span style=" font-weight:700;">The idea behind the app</span> came from personal experience: a dedicated key on the <span style=" font-weight:700;">MacBook</span>, features of <span style=" font-weight:700;">BetterTouchTool</span>, and the visual layout indicator from <span style=" font-weight:700;">Input Source Pro</span> inspired the creation of this solution for Windows.
                </p>
                </body></html>
            )")
                 });

        t.insert(QStringLiteral("INFO_FOOTER"), {
                     QString::fromUtf8(R"(
                <html><head/><body>
                <p align="center" style="color: rgba(255, 255, 255, 200);">
                Приложение распространяется бесплатно под лицензией <span style=" font-weight:700;">MIT</span>.
                </p>
                <p align="center" style="color: rgba(255, 255, 255, 200);">
                Вы можете поддержать развитие проекта.
                </p>
                </body></html>
            )"),
                     QString::fromUtf8(R"(
                <html><head/><body>
                <p align="center" style="color: rgba(255, 255, 255, 200);">
                The application is distributed for free under the <span style=" font-weight:700;">MIT license</span>.
                </p>
                <p align="center" style="color: rgba(255, 255, 255, 200);">
                You can support the further development of the project.
                </p>
                </body></html>
            )")
                 });

        return t;
    }();
    return table;
}

QString Lang::tr(const QString &key, const Locale locale) {
    const auto &t = translationsTable();
    auto it = t.constFind(key);
    if (it == t.constEnd()) {
        // Попробуем ключ в upper-case (на случай разных стилей)
        const QString up = key.toUpper();
        it = t.constFind(up);
        if (it == t.constEnd())
            return key; // fallback — вернуть ключ
    }

    return (locale == Locale::EN) ? it->en : it->ru;
}

QString Lang::tr(const QString &key, const QString &localeCode) {
    const QString code = localeCode.trimmed().toLower();
    const Locale loc = (code == QLatin1String("en")) ? Locale::EN : Locale::RU;
    return tr(key, loc);
}

const QHash<QString, TranslationEntry> &Lang::allTranslations() {
    return translationsTable();
}
