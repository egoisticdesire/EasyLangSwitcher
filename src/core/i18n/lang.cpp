#include "lang.h"
#include "../config/appSettings.h"
#include <QHash>
#include <QString>
#include <initializer_list>

#include "../../ui/helpers/fontHelper.h"

namespace {
    // ReSharper disable once CppDFATimeOver
    QString trWithOptionalArgs(const QString &value, const std::initializer_list<QString> args) {
        QString result = value;
        for (const QString &arg: args) {
            result = result.arg(arg);
        }
        return result;
    }
}

#define TR(key, en, ru, ...) \
    t.insert( \
        QStringLiteral(key), { \
            trWithOptionalArgs(QStringLiteral(en), {__VA_ARGS__}), \
            trWithOptionalArgs(QStringLiteral(ru), {__VA_ARGS__}), \
        } \
    )

static const QHash<QString, TranslationEntry> &translationsTable() {
    static const QHash<QString, TranslationEntry> table = []() -> QHash<QString, TranslationEntry> {
        QHash<QString, TranslationEntry> t;


        TR("SECOND_INSTANCE_ERROR",
           "%1 is already running!",
           "%1 уже запущен!"
        );


        TR("TRAY_LABEL_STATUS",
           "Status",
           "Статус"
        );
        TR("TRAY_LABEL_HOTKEY",
           "Hotkey",
           "Гор. клавиша"
        );
        TR("TRAY_LABEL_DELAY",
           "Delay (ms)",
           "Задержка (мс)"
        );


        TR("TRAY_TOGGLE_ENABLED",
           "Enabled",
           "Включено"
        );
        TR("TRAY_TOGGLE_DISABLED",
           "Disabled",
           "Выключено"
        );
        TR("TRAY_TOGGLE_RESUME",
           "  Enable",
           "  Возобновить"
        );
        TR("TRAY_TOGGLE_PAUSE",
           "  Disable",
           "  Приостановить"
        );
        TR("TRAY_SETTINGS",
           "  Settings",
           "  Настройки"
        );
        TR("TRAY_EXIT",
           "  Exit",
           "  Выход"
        );


        TR("SETTINGS_SELECT_KEY_LABEL",
           "Select Hotkey",
           "Выберите горячую клавишу"
        );
        TR("SETTINGS_SWITCH_DELAY_LABEL",
           "Switching delay time",
           "Время задержки переключения"
        );
        TR("SETTINGS_APP_STARTUP_LABEL",
           "Launch at Windows startup",
           "Автозапуск при старте Windows"
        );
        TR("SETTINGS_APP_LANG_LABEL",
           "Application language",
           "Язык приложения"
        );
        TR("SETTINGS_APP_UPD_CHECK_LABEL",
           "Automatically check for updates",
           "Автоматически проверять обновления"
        );
        TR("SETTINGS_TOOLTIP_CHECK_NOW",
           "Check for updates now",
           "Проверить обновления сейчас"
        );
        TR("SETTINGS_TOOLTIP_UPDATE_AVAILABLE",
           "A new version %1 is available.\nClick to open update actions.",
           "Доступна новая версия %1.\nНажмите, чтобы открыть действия обновления."
        );
        TR("SETTINGS_TOOLTIP_UPDATE_AVAILABLE_WITH_LAST_CHECK",
           "A new version %1 is available.\nLast check: %2",
           "Доступна новая версия %1.\nПоследняя проверка: %2"
        );
        TR("SETTINGS_TOOLTIP_LAST_CHECK",
           "The latest version is installed.\nLast check: %1",
           "Установлена актуальная версия.\nПоследняя проверка: %1"
        );
        TR("SETTINGS_APP_UPD_CHECK_NEVER",
           "Never",
           "Никогда"
        );
        TR("SETTINGS_APP_UPD_CHECK_DAILY",
           "Daily",
           "Ежедневно"
        );
        TR("SETTINGS_APP_UPD_CHECK_WEEKLY",
           "Weekly",
           "Еженедельно"
        );
        TR("SETTINGS_APP_UPD_CHECK_MONTHLY",
           "Monthly",
           "Ежемесячно"
        );
        TR("SETTINGS_RESTORE_DEFAULT_LABEL",
           "Restore default settings",
           "Восстановить настройки по умолчанию"
        );
        TR("SETTINGS_KEY_SEQUENCE",
           "Key...",
           "Клавиша..."
        );
        TR("SETTINGS_ALL_CHANGES_SAVED",
           "All changes are saved automatically",
           "Все изменения сохранены автоматически"
        );
        TR("SETTINGS_SIDER_MENU_GENERAL",
           "General",
           "Общие"
        );
        TR("SETTINGS_SIDER_MENU_EXCLUSIONS",
           "Exclusions",
           "Исключения"
        );
        TR("SETTINGS_SIDER_MENU_INFO",
           "Info",
           "Информация"
        );
        TR("SETTINGS_SIDER_MENU_CLOSE",
           "Close",
           "Закрыть"
        );
        TR("SETTINGS_ENABLE_STARTUP_LABEL",
           "Enable",
           "Включить"
        );
        TR("SETTINGS_DISABLE_STARTUP_LABEL",
           "Disable",
           "Выключить"
        );


        TR("NOTIFICATION_UPD_BTN_DOWNLOAD",
           "  Download",
           "  Загрузить"
        );
        TR("NOTIFICATION_UPD_BTN_RELEASES",
           "  What's new",
           "  Что нового"
        );
        TR("NOTIFICATION_UPD_BTN_OPEN_FOLDER",
           "  Open folder",
           "  Открыть папку"
        );
        TR("NOTIFICATION_UPD_BTN_SAVE_AS_TOOLTIP",
           "Save to a specific location...",
           "Сохранить в выбранную папку..."
        );
        TR("NOTIFICATION_UPD_SAVE_FILE_TITLE",
           "Save Installer",
           "Сохранить файл установки"
        );
        TR("NOTIFICATION_UPD_AVAILABLE_TITLE",
           "A new version of %1 is ready",
           "Доступна новая версия %1",
           AppSettings::APP_NAME
        );
        TR("NOTIFICATION_UPD_AVAILABLE_DESC",
           "Download version %1 now or view the changes on the release page!",
           "Загрузите версию %1 прямо сейчас или посмотрите список изменений в описании релиза!"
        );
        TR("NOTIFICATION_UPD_NOT_AVAILABLE_TITLE",
           "The latest version of %1 is installed",
           "Уже установлена последняя версия %1",
           AppSettings::APP_NAME
        );
        TR("NOTIFICATION_UPD_NOT_AVAILABLE_DESC",
           "The current version %1 is the latest, nothing needs to be updated!",
           "Версия %1 — самая свежая, обновлений не требуется!"
        );
        TR("NOTIFICATION_UPD_DOWNLOAD_COMPLETE",
           "Download completed successfully!",
           "Загрузка успешно завершена!"
        );
        TR("NOTIFICATION_UPD_DOWNLOAD_PROGRESS",
           "Downloading update...",
           "Загрузка обновления..."
        );
        TR("NOTIFICATION_UPD_DOWNLOAD_ERROR",
           "Failed to download update",
           "Не удалось загрузить обновление"
        );
        TR("NOTIFICATION_UPD_SYSTEM_BODY",
           "New update notification for version %1",
           "Новое уведомление об обновлении для версии %1"
        );
        TR("NOTIFICATION_UPD_SYSTEM_CLICK_HINT",
           "Click to open update actions.",
           "Нажмите, чтобы открыть действия обновления."
        );


        TR("BTN_APPLY",
           "Apply",
           "Применить"
        );
        TR("SAVE_BUTTON",
           "Save",
           "Сохранить"
        );
        TR("CANCEL_BUTTON",
           "Cancel",
           "Отмена"
        );
        TR("ADD_APP_BUTTON",
           "  Add Application...",
           "  Добавить приложение..."
        );
        TR("ADD_REGEX_BUTTON",
           "  Add Regular Expression...",
           "  Добавить регулярное выражение..."
        );
        TR("TABLE_HEADER_TYPE",
           "Type",
           "Тип"
        );
        TR("TABLE_HEADER_PATH",
           "Path / Pattern",
           "Путь / Шаблон"
        );
        TR("TABLE_HEADER_RULE",
           "Rule",
           "Правило"
        );
        TR("TABLE_HEADER_DELETE",
           "Delete",
           "Удаление"
        );
        TR("ON_BUTTON",
           "On",
           "Вкл"
        );
        TR("OFF_BUTTON",
           "Off",
           "Выкл"
        );
        TR("ENABLE_ALL_RULES_BUTTON",
           "Enable All Rules",
           "Включить все правила"
        );
        TR("DISABLE_ALL_RULES_BUTTON",
           "Disable All Rules",
           "Выключить все правила"
        );
        TR("CLEAR_LIST_BUTTON",
           "Clear List",
           "Очистить список"
        );
        TR("REGEX_PLACEHOLDER",
           "Enter a process name or a regular expression to exclude...",
           "Введите имя процесса или регулярное выражение для исключения..."
        );
        TR("CASE_SENSITIVITY_LABEL",
           "Case Sensitivity",
           "Чувствительность к регистру"
        );
        TR("FULL_MATCH_LABEL",
           "Full Match",
           "Полное совпадение"
        );
        TR("CHANGES_NOT_SAVED",
           "Changes not saved",
           "Изменения не сохранены"
        );
        TR("EXCLUSION_ADDED",
           "Exclusion rule added",
           "Правило исключения добавлено"
        );
        TR("EXCLUSION_REMOVED",
           "Exclusion rule removed",
           "Правило исключения удалено"
        );
        TR("EXCLUSIONS_CLEARED",
           "Exclusions list cleared",
           "Список правил очищен"
        );
        TR("DUPLICATE_ENTRY",
           "This entry already exists",
           "Такая запись уже существует"
        );
        TR("INVALID_INPUT",
           "Invalid input",
           "Недопустимое значение"
        );
        TR("REGEX_ERROR",
           "Regular expression error",
           "Ошибка в регулярном выражении"
        );
        TR("MISSING_PATH",
           "Path not specified",
           "Не указан путь"
        );
        TR("LANGUAGE_CHANGED",
           "Interface language changed",
           "Язык интерфейса изменен"
        );
        TR("SETTINGS_RESET",
           "Settings reset to default",
           "Настройки сброшены до значений по умолчанию"
        );
        TR("INFO_SWITCHES",
           "Switches:",
           "Переключений:"
        );
        TR("ENABLE_INDICATOR_LABEL",
           "Enable Indicator",
           "Включить индикатор"
        );
        TR("DISABLE_INDICATOR_LABEL",
           "Disable Indicator",
           "Выключить индикатор"
        );
        TR("FONT_COLOR_LABEL",
           "Font Color",
           "Цвет шрифта"
        );
        TR("BACKGROUND_COLOR_LABEL",
           "Background Color",
           "Цвет фона"
        );
        TR("BORDER_COLOR_LABEL",
           "Border Color",
           "Цвет рамки"
        );
        TR("DONATE_BUTTON_LABEL",
           "Support Project",
           "Поддержать проект"
        );
        TR("DONATE_BUTTON_TOOLTIP",
           "Open the project support page",
           "Открыть страницу поддержки проекта"
        );
        TR("CONFIRMATION_LABEL",
           "Are you sure?",
           "Вы уверены?"
        );
        TR("YES_BUTTON_LABEL",
           "Yes",
           "Да"
        );
        TR("NO_BUTTON_LABEL",
           "No",
           "Нет"
        );


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
            )").arg("font-family: 'Inter', 'Segoe UI', sans-serif;", "padding-top: 12px;", "line-height: 1.2;")
                .arg(FontManager::Small().pixelSize())
                .arg(FontManager::Default().pixelSize())
                .arg("color:rgba(255, 255, 255, 175);", "color:rgba(255, 255, 255, 200);", "font-weight: 600;"),
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
            )").arg("font-family: 'Inter', 'Segoe UI', sans-serif;", "padding-top: 12px;", "line-height: 1.2;")
                .arg(FontManager::Small().pixelSize())
                .arg(FontManager::Default().pixelSize())
                .arg("color:rgba(255, 255, 255, 175);", "color:rgba(255, 255, 255, 200);", "font-weight: 600;"),
            });

        t.insert(
            QStringLiteral("INFO_TEXT"), {
                QString::fromUtf8(R"(
                    <html><head/><body>
                    <p align="justify">
                    <span style=" font-weight:700;">%1</span> is a simple and lightweight Windows application that allows you to switch keyboard layouts using <span style=" font-weight:700;">a single key</span>. Switching cycles through all languages added in the system:
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
            )").arg(AppSettings::APP_NAME),
                QString::fromUtf8(R"(
                    <html><head/><body>
                    <p align="justify">
                    <span style=" font-weight:700;">%1</span> — простое и лёгкое приложение для Windows, которое позволяет переключать раскладку клавиатуры <span style=" font-weight:700;">одной клавишей</span>. Переключение происходит циклично по всем языкам, добавленным в системе:
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
            )").arg(AppSettings::APP_NAME),
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

#undef TR

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
