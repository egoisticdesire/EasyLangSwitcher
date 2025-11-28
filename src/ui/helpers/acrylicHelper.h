#pragma once
#include <../../../src/core/config/logger.h>
#include <QtWidgets/QWidget>
#include <Windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

constexpr int ACRYLIC_WINDOW_RADIUS = 8;

// Типы WinAPI для акрила
enum ACCENT_STATE {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4
};

struct ACCENT_POLICY {
    ACCENT_STATE AccentState;
    DWORD AccentFlags;
    DWORD GradientColor;
    DWORD AnimationId;
};

enum WINDOWCOMPOSITIONATTRIB {
    WCA_ACCENT_POLICY = 19
};

struct WINDOWCOMPOSITIONATTRIBDATA {
    WINDOWCOMPOSITIONATTRIB Attribute;
    PVOID Data;
    SIZE_T SizeOfData;
};

using pSetWindowCompositionAttribute = BOOL(WINAPI *)(HWND, WINDOWCOMPOSITIONATTRIBDATA *);

// Проверка версии Windows
inline bool isWindows11OrGreater() {
    typedef LONG (WINAPI*RtlGetVersionPtr)(PRTL_OSVERSIONINFOEXW);
    RTL_OSVERSIONINFOEXW rovi{};
    rovi.dwOSVersionInfoSize = sizeof(rovi);
    const auto rtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(
        GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
    if (rtlGetVersion && rtlGetVersion(&rovi) == 0) {
        LOG_DEBUG() << QString("Windows version: %1.%2.%3")
                        .arg(rovi.dwMajorVersion).arg(rovi.dwMinorVersion).arg(rovi.dwBuildNumber);
        return (rovi.dwMajorVersion == 10 && rovi.dwBuildNumber >= 22000);
    }
    return false;
}

inline bool isWindows10OrGreater() {
    typedef LONG (WINAPI*RtlGetVersionPtr)(PRTL_OSVERSIONINFOEXW);
    RTL_OSVERSIONINFOEXW rovi{};
    rovi.dwOSVersionInfoSize = sizeof(rovi);
    const auto rtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(
        GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
    if (rtlGetVersion && rtlGetVersion(&rovi) == 0) {
        LOG_DEBUG() << QString("Windows version: %1.%2.%3")
                        .arg(rovi.dwMajorVersion).arg(rovi.dwMinorVersion).arg(rovi.dwBuildNumber);
        return (rovi.dwMajorVersion == 10);
    }
    return false;
}

class AcrylicHelper {
public:
    static void setAcrylicEnabled( // 0xCC (~80%) | 0xE0 (~88%) | 0xF0 (~94%)
        const QWidget *widget,
        const bool enabled,
        const DWORD alphaActiveWin11 = 0x40,
        const DWORD rgbActiveWin11 = 0x202020,
        const DWORD alphaActiveWin10 = 0xE0,
        const DWORD rgbActiveWin10 = 0x151515,
        const DWORD alphaInactiveWin11 = 0xFF,
        const DWORD rgbInactiveWin11 = 0x101010,
        const DWORD alphaInactiveWin10 = 0xFF,
        const DWORD rgbInactiveWin10 = 0x090909
    ) {
        if (!widget) return;

        if (enabled)
            enableAcrylic(widget, alphaActiveWin11, rgbActiveWin11, alphaActiveWin10, rgbActiveWin10);
        else
            enableAcrylic(widget, alphaInactiveWin11, rgbInactiveWin11, alphaInactiveWin10, rgbInactiveWin10);
    }

    static void enableAcrylic( // 0xCC (~80%) | 0xE0 (~88%) | 0xF0 (~94%)
        const QWidget *widget,
        const DWORD alphaWin11 = 0x40,
        const DWORD rgbWin11 = 0x202020,
        const DWORD alphaWin10 = 0xE0,
        const DWORD rgbWin10 = 0x101010
    ) {
        if (!widget) return;

        const auto hwnd = reinterpret_cast<HWND>(widget->winId());
        if (!hwnd) return;

        // Убираем WS_EX_LAYERED
        if (const LONG ex = GetWindowLongW(hwnd, GWL_EXSTYLE); ex & WS_EX_LAYERED)
            SetWindowLongW(hwnd, GWL_EXSTYLE, ex & ~WS_EX_LAYERED);

        const auto setWindowCompositionAttribute =
                reinterpret_cast<pSetWindowCompositionAttribute>(
                    GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));
        if (!setWindowCompositionAttribute) return;

        ACCENT_POLICY policy{};

        if (isWindows11OrGreater()) {
            // Win11: акрил с радиусом углов
            policy.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
            policy.GradientColor = (alphaWin11 << 24) | rgbWin11;
            policy.AccentFlags = 2;

            // Скругление углов через DWM
            constexpr DWORD DWMWA_WINDOW_CORNER_PREFERENCE = 33;
            constexpr int DWMWCP_ROUND = 2;
            (void) DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &DWMWCP_ROUND, sizeof(DWMWCP_ROUND));

            // Скругление региона
            if (const HRGN hrgn = CreateRoundRectRgn(
                0, 0, widget->width() + 1, widget->height() + 1,
                ACRYLIC_WINDOW_RADIUS, ACRYLIC_WINDOW_RADIUS))
                SetWindowRgn(hwnd, hrgn, TRUE);
        } else if (isWindows10OrGreater()) {
            // Win10: простой blur без скруглений
            policy.AccentState = ACCENT_ENABLE_BLURBEHIND;
            policy.GradientColor = (alphaWin10 << 24) | rgbWin10;
            policy.AccentFlags = 2;

            // Win10: простой blur без скруглений
            if (const HRGN hrgn = CreateRectRgn(0, 0, widget->width() + 1, widget->height() + 1))
                SetWindowRgn(hwnd, hrgn, TRUE);

            // Добавляем шум поверх окна
            addNoiseForWin10(widget);
        } else {
            policy.AccentState = ACCENT_DISABLED;
            policy.GradientColor = 0;
            policy.AccentFlags = 0;
        }

        WINDOWCOMPOSITIONATTRIBDATA data{};
        data.Attribute = WCA_ACCENT_POLICY;
        data.Data = &policy;
        data.SizeOfData = sizeof(policy);

        setWindowCompositionAttribute(hwnd, &data);
    }

    static void updateRegion(const QWidget *widget) {
        if (!widget) return;
        const auto hwnd = reinterpret_cast<HWND>(widget->winId());
        if (!hwnd) return;

        if (isWindows11OrGreater()) {
            if (const HRGN hrgn = CreateRoundRectRgn(
                0, 0, widget->width() + 1, widget->height() + 1,
                ACRYLIC_WINDOW_RADIUS, ACRYLIC_WINDOW_RADIUS))
                SetWindowRgn(hwnd, hrgn, TRUE);
        } else if (isWindows10OrGreater()) {
            if (const HRGN hrgn = CreateRectRgn(0, 0, widget->width() + 1, widget->height() + 1))
                SetWindowRgn(hwnd, hrgn, TRUE);
        }
    }

    static void disableAcrylic(const QWidget *widget) {
        if (!widget) return;
        const auto hwnd = reinterpret_cast<HWND>(widget->winId());
        if (!hwnd) return;

        const auto setWindowCompositionAttribute =
                reinterpret_cast<pSetWindowCompositionAttribute>(
                    GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));
        if (!setWindowCompositionAttribute) return;

        ACCENT_POLICY policy{};
        policy.AccentState = ACCENT_DISABLED;
        policy.AccentFlags = 0;
        policy.GradientColor = 0;

        WINDOWCOMPOSITIONATTRIBDATA data{};
        data.Attribute = WCA_ACCENT_POLICY;
        data.Data = &policy;
        data.SizeOfData = sizeof(policy);

        setWindowCompositionAttribute(hwnd, &data);
    }

    static void addNoiseForWin10(const QWidget *widget, const BYTE intensity = 15) {
        if (!widget || !isWindows10OrGreater() || isWindows11OrGreater()) return;

        static QWidget *noiseOverlay = nullptr;
        if (!noiseOverlay) {
            noiseOverlay = new QWidget(const_cast<QWidget *>(widget));
            noiseOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
            noiseOverlay->setAttribute(Qt::WA_NoSystemBackground);
            noiseOverlay->setGeometry(widget->rect());
            noiseOverlay->show();

            std::srand(static_cast<unsigned int>(std::time(nullptr))); // инициализация генератора
        }

        QImage img(widget->size(), QImage::Format_ARGB32);
        img.fill(Qt::transparent);

        for (int y = 0; y < img.height(); ++y) {
            const auto line = reinterpret_cast<QRgb *>(img.scanLine(y));
            for (int x = 0; x < img.width(); ++x) {
                const int val = std::rand() % intensity;
                line[x] = qRgba(val, val, val, val);
            }
        }

        QPalette palette;
        palette.setBrush(noiseOverlay->backgroundRole(), QBrush(img));
        noiseOverlay->setPalette(palette);
        noiseOverlay->setAutoFillBackground(true);
    }
};
