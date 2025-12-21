#pragma once
#include <QtWidgets/QWidget>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QWindow>
#include <Windows.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

constexpr int ACRYLIC_WINDOW_RADIUS = 8;

// Сообщения для превью в панели задач
#ifndef WM_DWMSENDICONICTHUMBNAIL
#define WM_DWMSENDICONICTHUMBNAIL 0x0323
#endif
#ifndef WM_DWMSENDICONICLIVEPREVIEWBITMAP
#define WM_DWMSENDICONICLIVEPREVIEWBITMAP 0x0326
#endif

// WinAPI структуры и типы для недокументированного акрила
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

struct WINDOWCOMPOSITIONATTRIBDATA {
    DWORD Attribute; // WCA_ACCENT_POLICY = 19
    PVOID Data;
    SIZE_T SizeOfData;
};

using pSetWindowCompositionAttribute = BOOL(WINAPI*)(HWND, WINDOWCOMPOSITIONATTRIBDATA *);
using pRtlGetVersion = LONG(WINAPI*)(PRTL_OSVERSIONINFOEXW);

class AcrylicHelper {
    // Структура для хранения версии ОС и указателей на функции (инициализируется один раз)
    struct WinInternal {
        bool isWin11;
        bool isWin10;
        pSetWindowCompositionAttribute setAttribPtr;

        WinInternal() {
            // 1. Получаем версию ОС
            isWin11 = false;
            isWin10 = false;
            if (const auto ntdll = GetModuleHandleW(L"ntdll.dll")) {
                if (const auto rtlGetVersion = reinterpret_cast<pRtlGetVersion>(
                    GetProcAddress(ntdll, "RtlGetVersion"))) {
                    RTL_OSVERSIONINFOEXW rovi = {sizeof(rovi)};
                    if (rtlGetVersion(&rovi) == 0) {
                        isWin10 = (rovi.dwMajorVersion == 10);
                        isWin11 = (isWin10 && rovi.dwBuildNumber >= 22000);
                    }
                }
            }
            // 2. Ищем функцию акрила
            setAttribPtr = reinterpret_cast<pSetWindowCompositionAttribute>(
                GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));
        }
    };

    // Статический доступ к внутренним данным (потокобезопасно в C++11+)
    static const WinInternal &info() {
        static WinInternal instance;
        return instance;
    }

public:
    static bool isWindows11OrGreater() { return info().isWin11; }
    static bool isWindows10OrGreater() { return info().isWin10; }

    static void setAcrylicEnabled(const QWidget *widget, const bool enabled) {
        if (!widget) return;
        if (enabled) {
            enableAcrylic(widget);
            enableCustomPreview(widget);
        } else {
            disableAcrylic(widget);
        }
    }

    // 0xCC (~80%) | 0xE0 (~88%) | 0xE3 (~90%) | 0xE6 (~92%) | 0xF0 (~94%) | 0xF3 (~96%) | 0xF6 (~98%) | 0xF9 (~100%)
    static void enableAcrylic(
        const QWidget *widget,
        const DWORD alphaWin11 = 0x40, const DWORD rgbWin11 = 0x202020,
        const DWORD alphaWin10 = 0xE6, const DWORD rgbWin10 = 0x101010
    ) {
        if (!widget || !info().setAttribPtr) return;
        const auto hwnd = reinterpret_cast<HWND>(widget->winId());
        if (!hwnd) return;

        // Чистим флаг Layered, чтобы WinAPI акрил не конфликтовал с прозрачностью Qt
        if (const LONG ex = GetWindowLongW(hwnd, GWL_EXSTYLE); ex & WS_EX_LAYERED)
            SetWindowLongW(hwnd, GWL_EXSTYLE, ex & ~WS_EX_LAYERED);

        ACCENT_POLICY policy{};
        const auto &win = info();

        if (win.isWin11) {
            policy.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
            policy.GradientColor = (alphaWin11 << 24) | rgbWin11;
            policy.AccentFlags = 2;

            // Нативное скругление Win11 (DWM масштабирует его сам)
            constexpr DWORD DWMWA_WINDOW_CORNER_PREFERENCE = 33;
            constexpr int DWMWCP_ROUND = 2;
            (void) DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &DWMWCP_ROUND, sizeof(DWMWCP_ROUND));
        } else if (win.isWin10) {
            policy.AccentState = ACCENT_ENABLE_BLURBEHIND;
            policy.GradientColor = (alphaWin10 << 24) | rgbWin10;
            policy.AccentFlags = 2;
            updateRegion(widget); // На Win10 нужен физический регион обрезки
        }

        WINDOWCOMPOSITIONATTRIBDATA data{19, &policy, sizeof(policy)};
        win.setAttribPtr(hwnd, &data);
    }

    static void updateRegion(const QWidget *widget) {
        if (!widget || isWindows11OrGreater()) return; // Win11 сама справляется

        const auto hwnd = reinterpret_cast<HWND>(widget->winId());
        if (!hwnd) return;

        // Расчет физических пикселей для корректного DPI на Win10
        const qreal dpr = widget->devicePixelRatio();
        const int physW = qRound(widget->width() * dpr);
        const int physH = qRound(widget->height() * dpr);

        if (const HRGN hrgn = CreateRectRgn(0, 0, physW + 1, physH + 1))
            SetWindowRgn(hwnd, hrgn, TRUE);
    }

    static void disableAcrylic(const QWidget *widget) {
        if (!widget || !info().setAttribPtr) return;
        const auto hwnd = reinterpret_cast<HWND>(widget->winId());

        ACCENT_POLICY policy{ACCENT_DISABLED, 0, 0, 0};
        WINDOWCOMPOSITIONATTRIBDATA data{19, &policy, sizeof(policy)};
        info().setAttribPtr(hwnd, &data);
    }

    static void enableCustomPreview(const QWidget *widget) {
        if (!widget) return;
        const auto hwnd = reinterpret_cast<HWND>(widget->winId());
        constexpr BOOL fTrue = TRUE;
        (void) DwmSetWindowAttribute(hwnd, 7, &fTrue, sizeof(fTrue)); // FORCE_ICONIC
        (void) DwmSetWindowAttribute(hwnd, 10, &fTrue, sizeof(fTrue)); // HAS_ICONIC_BITMAP
    }

    static bool handleIconicMessages(QWidget *widget, void *message, const QColor &bg = QColor(32, 32, 32)) {
        const MSG *msg = static_cast<MSG *>(message);
        if (msg->message == WM_DWMSENDICONICTHUMBNAIL) {
            int w = HIWORD(msg->lParam);
            int h = LOWORD(msg->lParam);
            if (w == 0 || h == 0) {
                w = widget->width() / 4;
                h = widget->height() / 4;
            }
            const HBITMAP hbm = qtPixmapToHBitmap(generateOpaqueScreenshot(widget, QSize(w, h), bg));
            (void) DwmSetIconicThumbnail(msg->hwnd, hbm, 0);
            DeleteObject(hbm);
            return true;
        }
        if (msg->message == WM_DWMSENDICONICLIVEPREVIEWBITMAP) {
            const HBITMAP hbm = qtPixmapToHBitmap(generateOpaqueScreenshot(widget, widget->size(), bg));
            (void) DwmSetIconicLivePreviewBitmap(msg->hwnd, hbm, nullptr, 0);
            DeleteObject(hbm);
            return true;
        }
        return false;
    }

private:
    static HBITMAP qtPixmapToHBitmap(const QPixmap &pix) {
        if (pix.isNull()) return nullptr;
        QImage img = pix.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = img.width();
        bmi.bmiHeader.biHeight = -img.height();
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void *bits = nullptr;
        const HBITMAP hBitmap = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (hBitmap && bits) memcpy(bits, img.bits(), img.sizeInBytes());
        return hBitmap;
    }

    static QPixmap generateOpaqueScreenshot(QWidget *widget, const QSize &targetSize, const QColor &bgColor) {
        QPixmap finalPix(widget->size());
        finalPix.fill(Qt::transparent);
        QPainter painter(&finalPix);
        painter.setRenderHint(QPainter::Antialiasing);

        if (isWindows11OrGreater()) {
            QPainterPath path;
            path.addRoundedRect(finalPix.rect(), ACRYLIC_WINDOW_RADIUS, ACRYLIC_WINDOW_RADIUS);
            painter.fillPath(path, bgColor);
            painter.setClipPath(path);
            widget->render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
            painter.setClipping(false);
            painter.setPen(QPen(QColor(0x42, 0x42, 0x42), 1.0));
            painter.drawPath(path);
        } else {
            painter.fillRect(finalPix.rect(), bgColor);
            widget->render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
        }
        painter.end();
        return (finalPix.size() == targetSize)
                   ? finalPix
                   : finalPix.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
};
