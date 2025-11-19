#pragma once

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
        return (rovi.dwMajorVersion == 10);
    }
    return false;
}

class AcrylicHelper {
public:
    static void enableAcrylic(const QWidget *widget, const DWORD alpha = 0x40, const DWORD rgb = 0x202020) {
        if (!widget) return;

        const auto hwnd = reinterpret_cast<HWND>(widget->winId());
        if (!hwnd) return;

        // Убираем WS_EX_LAYERED
        if (const LONG ex = GetWindowLongW(hwnd, GWL_EXSTYLE); ex & WS_EX_LAYERED) {
            SetWindowLongW(hwnd, GWL_EXSTYLE, ex & ~WS_EX_LAYERED);
        }

        const auto setWindowCompositionAttribute =
                reinterpret_cast<pSetWindowCompositionAttribute>(
                    GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));
        if (!setWindowCompositionAttribute) return;

        ACCENT_POLICY policy{};

        if (isWindows11OrGreater()) {
            // Win11: акрил с радиусом углов
            policy.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
            policy.GradientColor = (alpha << 24) | rgb;
            policy.AccentFlags = 2;

            // Скругление углов через DWM
            constexpr DWORD DWMWA_WINDOW_CORNER_PREFERENCE = 33;
            constexpr int DWMWCP_ROUND = 2;
            const HRESULT hrCorner = DwmSetWindowAttribute(
                hwnd,
                DWMWA_WINDOW_CORNER_PREFERENCE,
                &DWMWCP_ROUND,
                sizeof(DWMWCP_ROUND)
            );
            if (FAILED(hrCorner)) {
                // qDebug() << "DwmSetWindowAttribute corner failed:" << hrCorner;
            }

            // Скругление региона
            if (const HRGN hrgn = CreateRoundRectRgn(
                0, 0, widget->width() + 1, widget->height() + 1,
                ACRYLIC_WINDOW_RADIUS, ACRYLIC_WINDOW_RADIUS
            )) {
                SetWindowRgn(hwnd, hrgn, TRUE);
            }
        } else if (isWindows10OrGreater()) {
            // Win10: простой blur без скруглений
            policy.AccentState = ACCENT_ENABLE_BLURBEHIND;
            policy.GradientColor = (0xE0 << 24) | rgb; // 0xCC (~80%) | 0xE0 (~88%) | 0xF0 (~94%)
            policy.AccentFlags = 2;

            // Прямоугольная область
            if (const HRGN hrgn = CreateRectRgn(0, 0, widget->width() + 1, widget->height() + 1)) {
                SetWindowRgn(hwnd, hrgn, TRUE);
            }
        } else {
            return; // ниже Win10 не поддерживаем
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
                ACRYLIC_WINDOW_RADIUS, ACRYLIC_WINDOW_RADIUS
            )) {
                SetWindowRgn(hwnd, hrgn, TRUE);
            }
        } else if (isWindows10OrGreater()) {
            if (const HRGN hrgn = CreateRectRgn(0, 0, widget->width() + 1, widget->height() + 1)) {
                SetWindowRgn(hwnd, hrgn, TRUE);
            }
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

    static void setAcrylicEnabled(
        const QWidget *widget,
        const bool enabled,
        const DWORD activeAlpha = 0x40,
        const DWORD activeRgb = 0x202020,
        const DWORD inactiveAlpha = 0xFF,
        const DWORD inactiveRgb = 0x101010
    ) {
        if (!widget) return;

        if (enabled) {
            // акрил — окно активно
            enableAcrylic(widget, activeAlpha, activeRgb);
        } else {
            // статичный фон — окно не активно
            enableAcrylic(widget, inactiveAlpha, inactiveRgb);
        }
    }
};
