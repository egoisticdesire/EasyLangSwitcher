#include "lang.h"
#include "../config/appSettings.h"
#include <QHash>
#include <QString>

#include "../../ui/helpers/fontHelper.h"

static const QHash<QString, TranslationEntry> &translationsTable() {
    static const QHash<QString, TranslationEntry> table = []() -> QHash<QString, TranslationEntry> {
        QHash<QString, TranslationEntry> t;


        t.insert(
            QStringLiteral("SECOND_INSTANCE_ERROR"), {
                QStringLiteral("%1 is already running!"),
                QStringLiteral("%1 уже запущен!"),
            });


        t.insert(
            QStringLiteral("TRAY_LABEL_STATUS"), {
                QStringLiteral("Status"),
                QStringLiteral("Статус"),
            });
        t.insert(
            QStringLiteral("TRAY_LABEL_HOTKEY"), {
                QStringLiteral("Hotkey"),
                QStringLiteral("Гор. клавиша"),
            });
        t.insert(
            QStringLiteral("TRAY_LABEL_DELAY"), {
                QStringLiteral("Delay (ms)"),
                QStringLiteral("Задержка (мс)"),
            });


        t.insert(
            QStringLiteral("TRAY_TOGGLE_ENABLED"), {
                QStringLiteral("Enabled"),
                QStringLiteral("Включено"),
            });
        t.insert(
            QStringLiteral("TRAY_TOGGLE_DISABLED"), {
                QStringLiteral("Disabled"),
                QStringLiteral("Выключено"),
            });
        t.insert(
            QStringLiteral("TRAY_TOGGLE_RESUME"), {
                QStringLiteral("  Enable"),
                QStringLiteral("  Возобновить"),
            });
        t.insert(
            QStringLiteral("TRAY_TOGGLE_PAUSE"), {
                QStringLiteral("  Disable"),
                QStringLiteral("  Приостановить"),
            });
        t.insert(
            QStringLiteral("TRAY_SETTINGS"), {
                QStringLiteral("  Settings"),
                QStringLiteral("  Настройки"),
            });
        t.insert(
            QStringLiteral("TRAY_EXIT"), {
                QStringLiteral("  Exit"),
                QStringLiteral("  Выход"),
            });


        t.insert(
            QStringLiteral("SETTINGS_SELECT_KEY_LABEL"), {
                QStringLiteral("Select Hotkey"),
                QStringLiteral("Выберите горячую клавишу"),
            });
        t.insert(
            QStringLiteral("SETTINGS_SWITCH_DELAY_LABEL"), {
                QStringLiteral("Switching delay time"),
                QStringLiteral("Время задержки переключения"),
            });
        t.insert(
            QStringLiteral("SETTINGS_APP_STARTUP_LABEL"), {
                QStringLiteral("Launch at Windows startup"),
                QStringLiteral("Автозапуск при старте Windows"),
            });
        t.insert(
            QStringLiteral("SETTINGS_APP_LANG_LABEL"), {
                QStringLiteral("Application language"),
                QStringLiteral("Язык приложения"),
            });
        t.insert(
            QStringLiteral("SETTINGS_RESTORE_DEFAULT_LABEL"), {
                QStringLiteral("Restore default settings"),
                QStringLiteral("Восстановить настройки по умолчанию"),
            });
        t.insert(
            QStringLiteral("SETTINGS_KEY_SEQUENCE"), {
                QStringLiteral("Key..."),
                QStringLiteral("Клавиша..."),
            });
        t.insert(
            QStringLiteral("SETTINGS_ALL_CHANGES_SAVED"), {
                QStringLiteral("All changes are saved automatically"),
                QStringLiteral("Все изменения сохранены автоматически"),
            });
        t.insert(
            QStringLiteral("SETTINGS_SIDER_MENU_GENERAL"), {
                QStringLiteral("General"),
                QStringLiteral("Общие"),
            });
        t.insert(
            QStringLiteral("SETTINGS_SIDER_MENU_EXCLUSIONS"), {
                QStringLiteral("Exclusions"),
                QStringLiteral("Исключения"),
            });
        t.insert(
            QStringLiteral("SETTINGS_SIDER_MENU_INFO"), {
                QStringLiteral("Info"),
                QStringLiteral("Информация"),
            });
        t.insert(
            QStringLiteral("SETTINGS_SIDER_MENU_CLOSE"), {
                QStringLiteral("Close"),
                QStringLiteral("Закрыть"),
            });
        t.insert(
            QStringLiteral("SETTINGS_ENABLE_STARTUP_LABEL"), {
                QStringLiteral("Enable"),
                QStringLiteral("Включить"),
            });
        t.insert(
            QStringLiteral("SETTINGS_DISABLE_STARTUP_LABEL"), {
                QStringLiteral("Disable"),
                QStringLiteral("Выключить"),
            });


        t.insert(
            QStringLiteral("BTN_APPLY"), {
                QStringLiteral("Apply"),
                QStringLiteral("Применить"),
            });
        t.insert(
            QStringLiteral("SAVE_BUTTON"), {
                QStringLiteral("Save"),
                QStringLiteral("Сохранить"),
            });
        t.insert(
            QStringLiteral("CANCEL_BUTTON"), {
                QStringLiteral("Cancel"),
                QStringLiteral("Отмена"),
            });
        t.insert(
            QStringLiteral("ADD_APP_BUTTON"), {
                QStringLiteral("  Add Application..."),
                QStringLiteral("  Добавить приложение..."),
            });
        t.insert(
            QStringLiteral("ADD_REGEX_BUTTON"), {
                QStringLiteral("  Add Regular Expression..."),
                QStringLiteral("  Добавить регулярное выражение..."),
            });
        t.insert(
            QStringLiteral("TABLE_HEADER_TYPE"), {
                QStringLiteral("Type"),
                QStringLiteral("Тип"),
            });
        t.insert(
            QStringLiteral("TABLE_HEADER_PATH"), {
                QStringLiteral("Path / Pattern"),
                QStringLiteral("Путь / Шаблон"),
            });
        t.insert(
            QStringLiteral("TABLE_HEADER_RULE"), {
                QStringLiteral("Rule"),
                QStringLiteral("Правило"),
            });
        t.insert(
            QStringLiteral("TABLE_HEADER_DELETE"), {
                QStringLiteral("Delete"),
                QStringLiteral("Удаление"),
            });
        t.insert(
            QStringLiteral("ON_BUTTON"), {
                QStringLiteral("On"),
                QStringLiteral("Вкл"),
            });
        t.insert(
            QStringLiteral("OFF_BUTTON"), {
                QStringLiteral("Off"),
                QStringLiteral("Выкл"),
            });
        t.insert(
            QStringLiteral("ENABLE_ALL_RULES_BUTTON"), {
                QStringLiteral("Enable All Rules"),
                QStringLiteral("Включить все правила"),
            });
        t.insert(
            QStringLiteral("DISABLE_ALL_RULES_BUTTON"), {
                QStringLiteral("Disable All Rules"),
                QStringLiteral("Выключить все правила"),
            });
        t.insert(
            QStringLiteral("CLEAR_LIST_BUTTON"), {
                QStringLiteral("Clear List"),
                QStringLiteral("Очистить список"),
            });
        t.insert(
            QStringLiteral("REGEX_PLACEHOLDER"), {
                QStringLiteral("Enter a process name or a regular expression to exclude..."),
                QStringLiteral("Введите имя процесса или регулярное выражение для исключения..."),
            });
        t.insert(
            QStringLiteral("CASE_SENSITIVITY_LABEL"), {
                QStringLiteral("Case Sensitivity"),
                QStringLiteral("Чувствительность к регистру"),
            });
        t.insert(
            QStringLiteral("FULL_MATCH_LABEL"), {
                QStringLiteral("Full Match"),
                QStringLiteral("Полное совпадение"),
            });
        t.insert(
            QStringLiteral("CHANGES_NOT_SAVED"), {
                QStringLiteral("Changes not saved"),
                QStringLiteral("Изменения не сохранены"),
            });
        t.insert(
            QStringLiteral("EXCLUSION_ADDED"), {
                QStringLiteral("Exclusion rule added"),
                QStringLiteral("Правило исключения добавлено"),
            });
        t.insert(
            QStringLiteral("EXCLUSION_REMOVED"), {
                QStringLiteral("Exclusion rule removed"),
                QStringLiteral("Правило исключения удалено"),
            });
        t.insert(
            QStringLiteral("EXCLUSIONS_CLEARED"), {
                QStringLiteral("Exclusions list cleared"),
                QStringLiteral("Список правил очищен"),
            });
        t.insert(
            QStringLiteral("DUPLICATE_ENTRY"), {
                QStringLiteral("This entry already exists"),
                QStringLiteral("Такая запись уже существует"),
            });
        t.insert(
            QStringLiteral("INVALID_INPUT"), {
                QStringLiteral("Invalid input"),
                QStringLiteral("Недопустимое значение"),
            });
        t.insert(
            QStringLiteral("REGEX_ERROR"), {
                QStringLiteral("Regular expression error"),
                QStringLiteral("Ошибка в регулярном выражении"),
            });
        t.insert(
            QStringLiteral("MISSING_PATH"), {
                QStringLiteral("Path not specified"),
                QStringLiteral("Не указан путь"),
            });
        t.insert(
            QStringLiteral("LANGUAGE_CHANGED"), {
                QStringLiteral("Interface language changed"),
                QStringLiteral("Язык интерфейса изменен"),
            });
        t.insert(
            QStringLiteral("SETTINGS_RESET"), {
                QStringLiteral("Settings reset to default"),
                QStringLiteral("Настройки сброшены до значений по умолчанию"),
            });
        t.insert(
            QStringLiteral("INFO_SWITCHES"), {
                QStringLiteral("Switches:"),
                QStringLiteral("Переключений:"),
            });
        t.insert(
            QStringLiteral("ENABLE_INDICATOR_LABEL"), {
                QStringLiteral("Enable Indicator"),
                QStringLiteral("Включить индикатор"),
            });
        t.insert(
            QStringLiteral("DISABLE_INDICATOR_LABEL"), {
                QStringLiteral("Disable Indicator"),
                QStringLiteral("Выключить индикатор"),
            });
        t.insert(
            QStringLiteral("FONT_COLOR_LABEL"), {
                QStringLiteral("Font Color"),
                QStringLiteral("Цвет шрифта"),
            });
        t.insert(
            QStringLiteral("BACKGROUND_COLOR_LABEL"), {
                QStringLiteral("Background Color"),
                QStringLiteral("Цвет фона"),
            });
        t.insert(
            QStringLiteral("BORDER_COLOR_LABEL"), {
                QStringLiteral("Border Color"),
                QStringLiteral("Цвет рамки"),
            });
        t.insert(
            QStringLiteral("DONATE_BUTTON_LABEL"), {
                QStringLiteral("Support Project"),
                QStringLiteral("Поддержать проект"),
            });
        t.insert(
            QStringLiteral("DONATE_BUTTON_TOOLTIP"), {
                QStringLiteral("Open the project support page"),
                QStringLiteral("Открыть страницу поддержки проекта"),
            });
        t.insert(
            QStringLiteral("CONFIRMATION_LABEL"), {
                QStringLiteral("Are you sure?"),
                QStringLiteral("Вы уверены?"),
            });
        t.insert(
            QStringLiteral("YES_BUTTON_LABEL"), {
                QStringLiteral("Yes"),
                QStringLiteral("Да"),
            });
        t.insert(
            QStringLiteral("NO_BUTTON_LABEL"), {
                QStringLiteral("No"),
                QStringLiteral("Нет"),
            });


        t.insert(
            QStringLiteral("SETTINGS_KEY_HOVER_WARNING_POPUP"), {
                QString::fromUtf8(R"(
                    <html><head/><body style="%1 margin: 0; padding: 0;">
                    <div style="%1 %3 font-size: %5pt;">
                    <span style="%7 %8">Warnings:</span>
                    </div>
                    <ul style="margin: 0; padding: 0; list-style: none; %1>
                    <li>
                        <table cellspacing="0" cellpadding="0">
                        <tr>
                            <td width="16" valign="top" style="%2 %6 font-size: %4pt;">•</td>
                            <td align="justify" style="%1 %2 %3 %6 font-size: %4pt;">
                                When selecting the
                                <code style="%1 color: rgba(74,144,226,255);">Caps</code>
                                <code style="%1 color: rgba(74,144,226,255);">Lock</code>
                                key as a layout switch modifier, its default behavior will change:
                                a <span style="%7 %8">short</span> press will switch the layout, while
                                <span style="%7 %8">repeated</span> or <span style="%7 %8">long</span> presses
                                will perform the standard action.
                            </td>
                        </tr>
                        </table>
                    </li>
                    </ul>
                    </body></html>
            )")
                .arg("font-family: 'Inter', 'Segoe UI', sans-serif;")
                .arg("padding-top: 12px;")
                .arg("line-height: 1.2;")
                .arg(FontManager::Small().pixelSize())
                .arg(FontManager::Default().pixelSize())
                .arg("color:rgba(255, 255, 255, 175);")
                .arg("color:rgba(255, 255, 255, 200);")
                .arg("font-weight: 600;"),
                QString::fromUtf8(R"(
                    <html><head/><body style="%1 margin: 0; padding: 0;">
                    <div style="%1 %3 font-size: %5pt;">
                    <span style="%7 %8">Предупреждения:</span>
                    </div>
                    <ul style="margin: 0; padding: 0; list-style: none; %1>
                    <li>
                        <table cellspacing="0" cellpadding="0">
                        <tr>
                            <td width="16" valign="top" style="%2 %6 font-size: %4pt;">•</td>
                            <td align="justify" style="%1 %2 %3 %6 font-size: %4pt;">
                                При выборе клавиши
                                <code style="%1 color: rgba(74,144,226,255);">Caps</code>
                                <code style="%1 color: rgba(74,144,226,255);">Lock</code>
                                в качестве модификатора переключения, её стандартное поведение будет изменено:
                                <span style="%7 %8">короткое</span> нажатие будет переключать раскладку, а
                                <span style="%7 %8">многократное</span> или <span style="%7 %8">длительное</span> —
                                выполнять стандартное действие.
                            </td>
                        </tr>
                        </table>
                    </li>
                    </ul>
                    </body></html>
            )")
                .arg("font-family: 'Inter', 'Segoe UI', sans-serif;")
                .arg("padding-top: 12px;")
                .arg("line-height: 1.2;")
                .arg(FontManager::Small().pixelSize())
                .arg(FontManager::Default().pixelSize())
                .arg("color:rgba(255, 255, 255, 175);")
                .arg("color:rgba(255, 255, 255, 200);")
                .arg("font-weight: 600;"),
            });

        t.insert(
            QStringLiteral("INFO_TEXT"), {
                QString::fromUtf8(R"(
                    <html><head/><body>
                    <p align="justify">
                    <span style=" font-weight:700;">EasyLangSwitcher</span> is a simple and lightweight Windows application that allows you to switch keyboard layouts using <span style=" font-weight:700;">a single key</span>. Switching cycles through all languages added in the system:
                    </p>
                    <ul style="margin: 0; -qt-list-indent: 1;">
                    <li align="justify" style="margin: 12px 0;">
                    <span style=" font-weight:700;">Short press</span> — changes the current layout.
                    </li>
                    <li align="justify" style="margin: 12px 0;">
                    <span style=" font-weight:700;">Long press</span> or using a combination (for example, <span style=" font-weight:700;">Ctrl+C</span>) with the assigned key preserves the standard behavior of that key.
                    </li>
                    </ul>
                    <p align="justify">
                    The application allows you to:
                    </p>
                    <ul style="margin: 0; -qt-list-indent: 1;">
                    <li align="justify" style="margin: 12px 0;">
                    Use any key to switch layouts without the usual combinations <span style=" font-weight:700;">Alt+Shift</span> or <span style=" font-weight:700;">Win+Space</span>.
                    </li>
                    <li align="justify" style="margin: 12px 0;">
                    Add programs to <span style=" font-weight:700;">exceptions</span> so layout switching does not interfere with work (for example, in games or specialized applications).
                    </li>
                    <li align="justify" style="margin: 12px 0;">
                    See the current layout thanks to convenient visual indication.
                    </li>
                    </ul>
                    <p align="justify">
                    <span style=" font-weight:700;">The idea behind the app</span> came from personal experience: a dedicated key on the <span style=" font-weight:700;">MacBook</span>, features of <span style=" font-weight:700;">BetterTouchTool</span>, and the visual layout indicator from <span style=" font-weight:700;">Input Source Pro</span> inspired the creation of this solution for Windows.
                    </p>
                    </body></html>
            )"),
                QString::fromUtf8(R"(
                    <html><head/><body>
                    <p align="justify">
                    <span style=" font-weight:700;">EasyLangSwitcher</span> — простое и лёгкое приложение для Windows, которое позволяет переключать раскладку клавиатуры <span style=" font-weight:700;">одной клавишей</span>. Переключение происходит циклично по всем языкам, добавленным в системе:
                    </p>
                    <ul style="margin: 0; -qt-list-indent: 1;">
                    <li align="justify" style="margin: 12px 0;">
                    <span style=" font-weight:700;">Краткое нажатие</span> — смена текущей раскладки.
                    </li>
                    <li align="justify" style="margin: 12px 0;">
                    <span style=" font-weight:700;">Длительное нажатие</span> или использование комбинации (например, <span style=" font-weight:700;">Ctrl+C</span>) с назначенной клавишей — стандартное поведение кнопки сохраняется.
                    </li>
                    </ul>
                    <p align="justify">
                    Приложение позволяет вам:
                    </p>
                    <ul style="margin: 0; -qt-list-indent: 1;">
                    <li align="justify" style="margin: 12px 0;">
                    Использовать любую клавишу для переключения раскладки без привычных сочетаний <span style=" font-weight:700;">Alt+Shift</span> или <span style=" font-weight:700;">Win+Space</span>.
                    </li>
                    <li align="justify" style="margin: 12px 0;">
                    Добавлять программы в <span style=" font-weight:700;">исключения</span>, чтобы переключение раскладки не мешало работе (например, в играх или специальных приложениях).
                    </li>
                    <li align="justify" style="margin: 12px 0;">
                    Видеть текущую раскладку благодаря удобной индикации.
                    </li>
                    </ul>
                    <p align="justify">
                    <span style=" font-weight:700;">Идея приложения</span> возникла из личного опыта: удобная отдельная клавиша на <span style=" font-weight:700;">MacBook</span>, возможности <span style=" font-weight:700;">BetterTouchTool</span> и индикация из <span style=" font-weight:700;">Input Source Pro</span> вдохновили на создание этого решения для Windows.
                    </p>
                    </body></html>
            )"),
            });

        t.insert(
            QStringLiteral("INFO_FOOTER"), {
                QString::fromUtf8(R"(
                    <html><head/><body>
                    <p align="center" style="color: rgba(255, 255, 255, 200);">
                    The application is distributed for free under the <span style=" font-weight:700;">MIT license</span>.
                    </p>
                    <p align="center" style="color: rgba(255, 255, 255, 200);">
                    You can support the further development of the project.
                    </p>
                    </body></html>
            )"),
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
            });

        return t;
    }();
    return table;
}

QString Lang::tr(const QString &key) {
    return tr(key, AppSettings::appLang);
}

QString Lang::tr(const QString &key, const QString &localeCode) {
    const QString code = localeCode.trimmed().toLower();
    return tr(key, (code == "en") ? Locale::EN : Locale::RU);
}

QString Lang::tr(const QString &key, const Locale locale) {
    const auto &tbl = translationsTable();
    auto it = tbl.constFind(key);

    if (it == tbl.constEnd()) {
        const QString up = key.toUpper();
        it = tbl.constFind(up);
        if (it == tbl.constEnd())
            return key; // fallback
    }

    return (locale == Locale::EN) ? it->en : it->ru;
}

const QHash<QString, TranslationEntry> &Lang::allTranslations() {
    return translationsTable();
}
