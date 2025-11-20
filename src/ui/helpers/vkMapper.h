#pragma once
#include <QHash>
#include <QKeySequence>

/*
    Модуль для конвертации Virtual-Key ↔ человекочитаемое имя.
*/

namespace VkMapper {
    // =============================
    // 1. Основные VK значения
    // =============================
#ifndef VK_LBUTTON
#define VK_LBUTTON        0x01
#define VK_RBUTTON        0x02
#define VK_CANCEL         0x03
#define VK_MBUTTON        0x04
#define VK_XBUTTON1       0x05
#define VK_XBUTTON2       0x06
#define VK_BACK           0x08
#define VK_TAB            0x09
#define VK_CLEAR          0x0C
#define VK_RETURN         0x0D
#define VK_SHIFT          0x10
#define VK_CONTROL        0x11
#define VK_MENU           0x12
#define VK_PAUSE          0x13
#define VK_CAPITAL        0x14
#define VK_ESCAPE         0x1B
#define VK_SPACE          0x20
#define VK_PRIOR          0x21
#define VK_NEXT           0x22
#define VK_END            0x23
#define VK_HOME           0x24
#define VK_LEFT           0x25
#define VK_UP             0x26
#define VK_RIGHT          0x27
#define VK_DOWN           0x28
#define VK_SELECT         0x29
#define VK_PRINT          0x2A
#define VK_EXECUTE        0x2B
#define VK_SNAPSHOT       0x2C
#define VK_INSERT         0x2D
#define VK_DELETE         0x2E
#define VK_HELP           0x2F

#define VK_LWIN           0x5B
#define VK_RWIN           0x5C
#define VK_APPS           0x5D
#define VK_SLEEP          0x5F

#define VK_NUMPAD0        0x60
#define VK_NUMPAD1        0x61
#define VK_NUMPAD2        0x62
#define VK_NUMPAD3        0x63
#define VK_NUMPAD4        0x64
#define VK_NUMPAD5        0x65
#define VK_NUMPAD6        0x66
#define VK_NUMPAD7        0x67
#define VK_NUMPAD8        0x68
#define VK_NUMPAD9        0x69

#define VK_MULTIPLY       0x6A
#define VK_ADD            0x6B
#define VK_SEPARATOR      0x6C
#define VK_SUBTRACT       0x6D
#define VK_DECIMAL        0x6E
#define VK_DIVIDE         0x6F

#define VK_F1             0x70
#define VK_F2             0x71
#define VK_F3             0x72
#define VK_F4             0x73
#define VK_F5             0x74
#define VK_F6             0x75
#define VK_F7             0x76
#define VK_F8             0x77
#define VK_F9             0x78
#define VK_F10            0x79
#define VK_F11            0x7A
#define VK_F12            0x7B
#define VK_F13            0x7C
#define VK_F14            0x7D
#define VK_F15            0x7E
#define VK_F16            0x7F
#define VK_F17            0x80
#define VK_F18            0x81
#define VK_F19            0x82
#define VK_F20            0x83
#define VK_F21            0x84
#define VK_F22            0x85
#define VK_F23            0x86
#define VK_F24            0x87

#define VK_NUMLOCK        0x90
#define VK_SCROLL         0x91

#define VK_LSHIFT         0xA0
#define VK_RSHIFT         0xA1
#define VK_LCONTROL       0xA2
#define VK_RCONTROL       0xA3
#define VK_LMENU          0xA4
#define VK_RMENU          0xA5

#define VK_BROWSER_BACK        0xA6
#define VK_BROWSER_FORWARD     0xA7
#define VK_BROWSER_REFRESH     0xA8
#define VK_BROWSER_STOP        0xA9
#define VK_BROWSER_SEARCH      0xAA
#define VK_BROWSER_FAVORITES   0xAB
#define VK_BROWSER_HOME        0xAC

#define VK_VOLUME_MUTE         0xAD
#define VK_VOLUME_DOWN         0xAE
#define VK_VOLUME_UP           0xAF
#endif

    // =======================================
    // 2. Таблица статических текстовых имен
    // =======================================
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
            {VK_F1, "F1"}, {VK_F2, "F2"}, {VK_F3, "F3"}, {VK_F4, "F4"},
            {VK_F5, "F5"}, {VK_F6, "F6"}, {VK_F7, "F7"}, {VK_F8, "F8"},
            {VK_F9, "F9"}, {VK_F10, "F10"}, {VK_F11, "F11"}, {VK_F12, "F12"},
            {VK_F13, "F13"}, {VK_F14, "F14"}, {VK_F15, "F15"}, {VK_F16, "F16"},
            {VK_F17, "F17"}, {VK_F18, "F18"}, {VK_F19, "F19"}, {VK_F20, "F20"},
            {VK_F21, "F21"}, {VK_F22, "F22"}, {VK_F23, "F23"}, {VK_F24, "F24"},

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

    // ====================================================
    // 3. Преобразование VK → человекочитаемое имя
    // ====================================================
    inline QString vkToName(const int vk) {
        if (const auto &map = vkToNameMap(); map.contains(vk))
            return map[vk];

        // A–Z
        if (vk >= 'A' && vk <= 'Z')
            return QString(QChar(vk));

        // Цифры 0–9
        if (vk >= '0' && vk <= '9')
            return QString(QChar(vk));

        // OEM-символы → используем OS-мэппинг Qt
        const QKeySequence seq(vk);
        if (const QString text = seq.toString(QKeySequence::NativeText); !text.isEmpty())
            return text;

        return QString("VK_%1").arg(vk);
    }

    // ====================================================
    // 4. Обратная таблица: имя → VK
    // ====================================================
    inline const QHash<QString, int> &nameToVkMap() {
        static QHash<QString, int> reverse;

        if (reverse.isEmpty()) {
            const auto &forward = vkToNameMap();
            for (auto it = forward.begin(); it != forward.end(); ++it)
                reverse[it.value()] = it.key();

            for (char c = 'A'; c <= 'Z'; c++)
                reverse[QString(c)] = c;

            for (char c = '0'; c <= '9'; c++)
                reverse[QString(c)] = c;
        }

        return reverse;
    }

    inline int nameToVk(const QString &name) {
        const auto &m = nameToVkMap();
        return m.value(name, 0);
    }

    // ====================================================
    // 5. QKeySequence → VK
    // ====================================================
    inline int sequenceToVk(const QKeySequence &seq) {
        if (seq.isEmpty())
            return 0;

        // key() возвращает Qt::Key_..., которые совпадают с ASCII для букв/цифр

        // Если это Qt::Key_A … Qt::Key_Z или 0-9 — можно использовать напрямую
        if (const QKeyCombination comb = seq[0];
            comb.key() >= Qt::Key_A && comb.key() <= Qt::Key_Z ||
            comb.key() >= Qt::Key_0 && comb.key() <= Qt::Key_9) {
            return comb.key();
        }


        // Если это функциональная или спец-клавиша — сопоставление вручную
        const QKeySequence ks = seq;
        const QString text = ks.toString(QKeySequence::NativeText);

        return nameToVk(text);
    }
}
