#pragma once
#include <QHash>
#include <QKeySequence>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace VkMapper {
    inline int scanCodeToLatinVk(const int sc) {
        switch (sc) {
            // Цифровой ряд
            case 0x02: return '1';
            case 0x03: return '2';
            case 0x04: return '3';
            case 0x05: return '4';
            case 0x06: return '5';
            case 0x07: return '6';
            case 0x08: return '7';
            case 0x09: return '8';
            case 0x0A: return '9';
            case 0x0B: return '0';

            // Символы верхнего ряда
            case 0x29: return '`'; // `
            case 0x0C: return '-'; // -
            case 0x0D: return '='; // =

            // QWERTY
            case 0x10: return 'Q';
            case 0x11: return 'W';
            case 0x12: return 'E';
            case 0x13: return 'R';
            case 0x14: return 'T';
            case 0x15: return 'Y';
            case 0x16: return 'U';
            case 0x17: return 'I';
            case 0x18: return 'O';
            case 0x19: return 'P';

            // Блок справа от P
            case 0x1A: return '[';
            case 0x1B: return ']';
            case 0x2B: return '\\';

            // Home row
            case 0x1E: return 'A';
            case 0x1F: return 'S';
            case 0x20: return 'D';
            case 0x21: return 'F';
            case 0x22: return 'G';
            case 0x23: return 'H';
            case 0x24: return 'J';
            case 0x25: return 'K';
            case 0x26: return 'L';

            // Символы справа от L
            case 0x27: return ';';
            case 0x28: return '\'';

            // Нижний ряд
            case 0x2C: return 'Z';
            case 0x2D: return 'X';
            case 0x2E: return 'C';
            case 0x2F: return 'V';
            case 0x30: return 'B';
            case 0x31: return 'N';
            case 0x32: return 'M';

            // Символы справа от M
            case 0x33: return ',';
            case 0x34: return '.';
            case 0x35: return '/';

            case 0x39: return ' ';
            default: break;
        }
        return 0;
    }

    // scan → Qt::Key_A…Z, Qt::Key_0…9
    inline int scanCodeToQtKey(const int sc) {
        const int vk = scanCodeToLatinVk(sc);

        if (vk >= 'A' && vk <= 'Z')
            return Qt::Key_A + (vk - 'A');
        if (vk >= '0' && vk <= '9')
            return Qt::Key_0 + (vk - '0');

        switch (vk) {
            case '`': return Qt::Key_QuoteLeft;
            case '-': return Qt::Key_Minus;
            case '=': return Qt::Key_Equal;
            case '[': return Qt::Key_BracketLeft;
            case ']': return Qt::Key_BracketRight;
            case '\\': return Qt::Key_Backslash;
            case ';': return Qt::Key_Semicolon;
            case '\'': return Qt::Key_Apostrophe;
            case ',': return Qt::Key_Comma;
            case '.': return Qt::Key_Period;
            case '/': return Qt::Key_Slash;
            case ' ': return Qt::Key_Space;
            default: break;
        }

        return 0;
    }

    inline const QHash<int, QString> &vkToNameMap() {
        static const QHash<int, QString> map = {

            // Модификаторы
            {VK_LCONTROL, "Left Ctrl"},
            {VK_RCONTROL, "Right Ctrl"},
            {VK_LSHIFT, "Left Shift"},
            {VK_RSHIFT, "Right Shift"},
            {VK_LMENU, "Left Alt"},
            {VK_RMENU, "Right Alt"},
            {VK_CAPITAL, "Caps Lock"},

            // Стрелки
            {VK_LEFT, "Left"},
            {VK_RIGHT, "Right"},
            {VK_UP, "Up"},
            {VK_DOWN, "Down"},

            // Спец клавиши
            {VK_SPACE, "Space"},
            {VK_TAB, "Tab"},
            {VK_RETURN, "Enter"},
            {VK_ESCAPE, "Escape"},
            {VK_BACK, "Backspace"},
            {VK_DELETE, "Delete"},
            {VK_INSERT, "Insert"},
            {VK_HOME, "Home"},
            {VK_END, "End"},
            {VK_PRIOR, "Page Up"},
            {VK_NEXT, "Page Down"},

            // Функциональные (F1..F24)
            {VK_F1, "F1"}, {VK_F2, "F2"},
            {VK_F3, "F3"}, {VK_F4, "F4"},
            {VK_F5, "F5"}, {VK_F6, "F6"},
            {VK_F7, "F7"}, {VK_F8, "F8"},
            {VK_F9, "F9"}, {VK_F10, "F10"},
            {VK_F11, "F11"}, {VK_F12, "F12"},
            {VK_F13, "F13"}, {VK_F14, "F14"},
            {VK_F15, "F15"}, {VK_F16, "F16"},
            {VK_F17, "F17"}, {VK_F18, "F18"},
            {VK_F19, "F19"}, {VK_F20, "F20"},
            {VK_F21, "F21"}, {VK_F22, "F22"},
            {VK_F23, "F23"}, {VK_F24, "F24"},

            // Нумпад
            {VK_NUMPAD0, "Num 0"},
            {VK_NUMPAD1, "Num 1"},
            {VK_NUMPAD2, "Num 2"},
            {VK_NUMPAD3, "Num 3"},
            {VK_NUMPAD4, "Num 4"},
            {VK_NUMPAD5, "Num 5"},
            {VK_NUMPAD6, "Num 6"},
            {VK_NUMPAD7, "Num 7"},
            {VK_NUMPAD8, "Num 8"},
            {VK_NUMPAD9, "Num 9"},

            {VK_MULTIPLY, "Num *"},
            {VK_ADD, "Num +"},
            {VK_SUBTRACT, "Num -"},
            {VK_DECIMAL, "Num ."},
            {VK_DIVIDE, "Num /"},

            // Media клавиши
            {VK_VOLUME_UP, "Volume Up"},
            {VK_VOLUME_DOWN, "Volume Down"},
            {VK_VOLUME_MUTE, "Volume Mute"},

            // Браузерные
            {VK_BROWSER_BACK, "Browser Back"},
            {VK_BROWSER_FORWARD, "Browser Forward"},
            {VK_BROWSER_REFRESH, "Browser Refresh"},
            {VK_BROWSER_STOP, "Browser Stop"},
            {VK_BROWSER_HOME, "Browser Home"}
        };
        return map;
    }

    inline QString vkToName(const int vk) {
        if (const auto &map = vkToNameMap(); map.contains(vk))
            return map[vk];

        // A–Z
        if (vk >= 'A' && vk <= 'Z')
            return {QChar(vk)};

        // Цифры 0–9
        if (vk >= '0' && vk <= '9')
            return {QChar(vk)};

        // OEM-символы → используем OS-маппинг Qt
        const QKeySequence seq(vk);
        if (const QString text = seq.toString(QKeySequence::NativeText); !text.isEmpty())
            return text;

        return QString("VK_%1").arg(vk);
    }

    inline const QHash<QString, int> &nameToVkMap() {
        static QHash<QString, int> reverse;

        if (reverse.isEmpty()) {
            const auto &forward = vkToNameMap();
            for (auto it = forward.begin(); it != forward.end(); ++it)
                reverse[it.value()] = it.key();

            for (char c = 'A'; c <= 'Z'; c++)
                reverse[QString(c)] = static_cast<unsigned char>(c);

            for (char c = '0'; c <= '9'; c++)
                reverse[QString(c)] = static_cast<unsigned char>(c);
        }

        return reverse;
    }

    inline int nameToVk(const QString &name) {
        const auto &m = nameToVkMap();
        return m.value(name, 0);
    }

    inline int sequenceToVk(const QKeySequence &seq) {
        if (seq.isEmpty())
            return 0;

        // key() возвращает Qt::Key_..., которые совпадают с ASCII для букв/цифр

        // Если это Qt::Key_A … Qt::Key_Z или 0-9 — можно использовать напрямую
        if (const QKeyCombination comb = seq[0];
            (comb.key() >= Qt::Key_A && comb.key() <= Qt::Key_Z) ||
            (comb.key() >= Qt::Key_0 && comb.key() <= Qt::Key_9)) {
            return comb.key();
        }


        // Если это функциональная или спец-клавиша — сопоставление вручную
        const QString text = seq.toString(QKeySequence::NativeText);

        return nameToVk(text);
    }
}
