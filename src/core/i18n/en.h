#pragma once
#include <QString>

namespace LangEn {
    const QString SELECT_KEY_LABEL = "Select Key";
    const QString DELAY_LABEL = "Set switch delay";
    const QString LANGUAGE_LABEL = "Application language";

    const QString AUTOSTART_LABEL = "Launch at Windows startup";
    const QString AUTOSTART_VALUE_ENABLED = "Enable";
    const QString AUTOSTART_VALUE_DISABLED = "Disable";

    const QString RESTORE_DEFAULT_LABEL = "Restore default settings";

    const QString MENU_GENERAL = "General";
    const QString MENU_EXCLUSIONS = "Exclusions";
    const QString MENU_INFO = "Info";

    const QString BTN_APPLY = "Apply";
    const QString SAVE_BUTTON = "Save";
    const QString CANCEL_BUTTON = "Cancel";

    const QString ADD_APP_BUTTON = "  Add Application...";
    const QString ADD_REGEX_BUTTON = "  Add Regular Expression...";

    const QString TABLE_HEADER_TYPE = "Type";
    const QString TABLE_HEADER_PATH = "Path / Pattern";
    const QString TABLE_HEADER_RULE = "Rule";
    const QString TABLE_HEADER_DELETE = "Delete";

    const QString ON_BUTTON = "On";
    const QString OFF_BUTTON = "Off";

    const QString ENABLE_ALL_RULES_BUTTON = "Enable All Rules";
    const QString DISABLE_ALL_RULES_BUTTON = "Disable All Rules";
    const QString CLEAR_LIST_BUTTON = "Clear List";

    const QString REGEX_PLACEHOLDER = "Enter a process name or a regular expression to exclude...";
    const QString CASE_SENSITIVITY_LABEL = "Case Sensitivity";
    const QString FULL_MATCH_LABEL = "Full Match";

    const QString ALL_CHANGES_SAVED = "All changes saved automatically";
    const QString CHANGES_NOT_SAVED = "Changes not saved";

    const QString EXCLUSION_ADDED = "Exclusion rule added";
    const QString EXCLUSION_REMOVED = "Exclusion rule removed";
    const QString EXCLUSIONS_CLEARED = "Exclusions list cleared";
    const QString DUPLICATE_ENTRY = "This entry already exists";

    const QString INVALID_INPUT = "Invalid input";
    const QString REGEX_ERROR = "Regular expression error";
    const QString MISSING_PATH = "Path not specified";

    const QString LANGUAGE_CHANGED = "Interface language changed";
    const QString SETTINGS_RESET = "Settings reset to default";

    const QString INFO_SWITCHES = "Switches:";

    const QString ENABLE_INDICATOR_LABEL = "Enable Indicator";
    const QString DISABLE_INDICATOR_LABEL = "Disable Indicator";
    const QString FONT_COLOR_LABEL = "Font Color";
    const QString BACKGROUND_COLOR_LABEL = "Background Color";
    const QString BORDER_COLOR_LABEL = "Border Color";

    const QString DONATE_BUTTON_LABEL = "Support Project";

    const QString CONFIRMATION_LABEL = "Are you sure?";
    const QString YES_BUTTON_LABEL = "Yes";
    const QString NO_BUTTON_LABEL = "No";


    // Всплывающий попап с предупреждениями
    const QString WARNINGS_POPUP = R"(
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
    )";

    // Info блок
    const QString INFO_TEXT = R"(
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
    )";

    const QString INFO_FOOTER = R"(
        <html><head/><body>
        <p align="center" style="color: rgba(255, 255, 255, 200);">
        The application is distributed for free under the <span style=" font-weight:700;">MIT license</span>.
        </p>
        <p align="center" style="color: rgba(255, 255, 255, 200);">
        You can support the further development of the project.
        </p>
        </body></html>
    )";
}
