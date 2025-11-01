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

class AcrylicHelper {
public:
    static void enableAcrylic(const QWidget *widget, const DWORD alpha = 0x40, const DWORD rgb = 0x202020) {
        if (!widget) return;

        const auto hwnd = reinterpret_cast<HWND>(widget->winId());
        if (!hwnd) return;

        // Убираем WS_EX_LAYERED, если есть (чтобы DWM корректно применил blur)
        if (const LONG ex = GetWindowLongW(hwnd, GWL_EXSTYLE); ex & WS_EX_LAYERED) {
            SetWindowLongW(hwnd, GWL_EXSTYLE, ex & ~WS_EX_LAYERED);
        }

        // Скругление углов окна через DWM
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

        // Акрил через SetWindowCompositionAttribute
        const auto setWindowCompositionAttribute =
                reinterpret_cast<pSetWindowCompositionAttribute>(
                    GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));
        if (!setWindowCompositionAttribute) return;

        ACCENT_POLICY policy{};
        policy.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
        policy.GradientColor = (alpha << 24) | rgb;
        policy.AccentFlags = 2;

        WINDOWCOMPOSITIONATTRIBDATA data{};
        data.Attribute = WCA_ACCENT_POLICY;
        data.Data = &policy;
        data.SizeOfData = sizeof(policy);

        setWindowCompositionAttribute(hwnd, &data);

        // Скругление региона
        if (const HRGN hrgn = CreateRoundRectRgn(
            0,
            0,
            widget->width() + 1,
            widget->height() + 1,
            ACRYLIC_WINDOW_RADIUS,
            ACRYLIC_WINDOW_RADIUS
        )) {
            SetWindowRgn(hwnd, hrgn, TRUE);
        }
    }

    static void updateRegion(const QWidget *widget) {
        if (!widget) return;
        const auto hwnd = reinterpret_cast<HWND>(widget->winId());
        if (!hwnd) return;
        if (const HRGN hrgn = CreateRoundRectRgn(
            0,
            0,
            widget->width() + 1,
            widget->height() + 1,
            ACRYLIC_WINDOW_RADIUS,
            ACRYLIC_WINDOW_RADIUS
        )) {
            SetWindowRgn(hwnd, hrgn, TRUE);
        }
    }
};
