#pragma once
#include <QString>

namespace LangRu {
    const QString SELECT_KEY_LABEL = "Выберите клавишу";
    const QString DELAY_LABEL = "Укажите время задержки";
    const QString LANGUAGE_LABEL = "Язык приложения";

    const QString AUTOSTART_LABEL = "Автозапуск при старте Windows";
    const QString AUTOSTART_VALUE_ENABLED = "Вкл";
    const QString AUTOSTART_VALUE_DISABLED = "Выкл";

    const QString RESTORE_DEFAULT_LABEL = "Сбросить настройки";

    const QString MENU_GENERAL = "Общие";
    const QString MENU_EXCLUSIONS = "Исключения";
    const QString MENU_INFO = "Информация";

    const QString BTN_APPLY = "Применить";
    const QString SAVE_BUTTON = "Сохранить";
    const QString CANCEL_BUTTON = "Отмена";

    const QString ADD_APP_BUTTON = "  Добавить приложение...";
    const QString ADD_REGEX_BUTTON = "  Добавить регулярное выражение...";

    const QString TABLE_HEADER_TYPE = "Тип";
    const QString TABLE_HEADER_PATH = "Путь / Шаблон";
    const QString TABLE_HEADER_RULE = "Правило";
    const QString TABLE_HEADER_DELETE = "Удаление";

    const QString RULE_ON_BUTTON = "Вкл";
    const QString RULE_OFF_BUTTON = "Выкл";

    const QString ENABLE_ALL_RULES_BUTTON = "Включить все правила";
    const QString DISABLE_ALL_RULES_BUTTON = "Выключить все правила";
    const QString CLEAR_LIST_BUTTON = "Очистить список";

    const QString REGEX_PLACEHOLDER = "Введите имя процесса или регулярное выражение для исключения...";
    const QString CASE_SENSITIVITY_LABEL = "Чувствительность к регистру";
    const QString FULL_MATCH_LABEL = "Полное совпадение";

    const QString ALL_CHANGES_SAVED = "Все изменения сохранены автоматически";
    const QString CHANGES_NOT_SAVED = "Изменения не сохранены";

    const QString EXCLUSION_ADDED = "Правило исключения добавлено";
    const QString EXCLUSION_REMOVED = "Правило исключения удалено";
    const QString EXCLUSIONS_CLEARED = "Список правил очищен";
    const QString DUPLICATE_ENTRY = "Такая запись уже существует";

    const QString INVALID_INPUT = "Недопустимое значение";
    const QString REGEX_ERROR = "Ошибка в регулярном выражении";
    const QString MISSING_PATH = "Не указан путь";

    const QString LANGUAGE_CHANGED = "Язык интерфейса изменен";
    const QString SETTINGS_RESET = "Настройки сброшены до значений по умолчанию";

    const QString INFO_SWITCHES = "Переключений:";


    // Всплывающий попап с предупреждениями
    const QString WARNINGS_POPUP = R"(
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
    )";

    // Info блок
    const QString INFO_TEXT = R"(
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
        Приложение позволяет:
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
        <span style=" font-weight:700;">Идея приложения</span> возникла из личного опыта: удобная отдельная клавиша на <span style=" font-weight:700;">MacBook</span>, возможности <span style=" font-weight:700;">BetterTouchTool</span> и индикация из <span style=" font-weight:700;">Input Source Pro</span> вдохновили на создание такого решения для Windows.
        </p>
        </body></html>
    )";

    const QString INFO_FOOTER = R"(
        <html><head/><body>
        <p align="center" style="color: rgba(255, 255, 255, 200);">
        Приложение распространяется бесплатно под лицензией <span style=" font-weight:700;">MIT</span>.
        </p>
        <p align="center" style="color: rgba(255, 255, 255, 200);">
        Вы можете поддержать развитие проекта.
        </p>
        </body></html>
    )";
}
