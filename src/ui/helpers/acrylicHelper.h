#pragma once
#include <../../../src/core/config/logger.h>
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

// --- DWM константы для Iconic Bitmap ---
#ifndef WM_DWMSENDICONICTHUMBNAIL
#define WM_DWMSENDICONICTHUMBNAIL 0x0323
#endif
#ifndef WM_DWMSENDICONICLIVEPREVIEWBITMAP
#define WM_DWMSENDICONICLIVEPREVIEWBITMAP 0x0326
#endif

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
    // 0xCC (~80%) | 0xE0 (~88%) | 0xE3 (~90%) | 0xE6 (~92%) | 0xF0 (~94%) | 0xF3 (~96%) | 0xF6 (~98%) | 0xF9 (~100%)
    static void setAcrylicEnabled(
        const QWidget *widget,
        const bool enabled,
        const DWORD alphaActiveWin11 = 0x40,
        const DWORD rgbActiveWin11 = 0x202020,
        const DWORD alphaActiveWin10 = 0xE6,
        const DWORD rgbActiveWin10 = 0x151515,
        const DWORD alphaInactiveWin11 = 0xFF,
        const DWORD rgbInactiveWin11 = 0x101010,
        const DWORD alphaInactiveWin10 = 0xFF,
        const DWORD rgbInactiveWin10 = 0x090909
    ) {
        if (!widget) return;

        if (enabled) {
            enableAcrylic(widget, alphaActiveWin11, rgbActiveWin11, alphaActiveWin10, rgbActiveWin10);
            enableCustomPreview(widget);
        } else {
            enableAcrylic(widget, alphaInactiveWin11, rgbInactiveWin11, alphaInactiveWin10, rgbInactiveWin10);
        }
    }

    static void enableCustomPreview(const QWidget *widget) {
        if (!widget) return;
        const auto hwnd = reinterpret_cast<HWND>(widget->winId());

        constexpr BOOL fForceIconic = TRUE;
        constexpr BOOL fHasIconicBitmap = TRUE;

        // DWMWA_FORCE_ICONIC_REPRESENTATION = 7
        (void) DwmSetWindowAttribute(hwnd, 7, &fForceIconic, sizeof(fForceIconic));
        // DWMWA_HAS_ICONIC_BITMAP = 10
        (void) DwmSetWindowAttribute(hwnd, 10, &fHasIconicBitmap, sizeof(fHasIconicBitmap));
    }

private:
    static HBITMAP qtPixmapToHBitmap(const QPixmap &pix) {
        if (pix.isNull()) return nullptr;

        QImage img = pix.toImage();
        if (img.format() != QImage::Format_ARGB32_Premultiplied) {
            img = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        }

        const int w = img.width();
        const int h = img.height();

        BITMAPINFO bmi;
        ZeroMemory(&bmi, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = w;
        bmi.bmiHeader.biHeight = -h;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void *bits = nullptr;
        const HBITMAP hBitmap = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);

        if (hBitmap && bits) {
            const int bytesPerLine = w * 4;
            for (int y = 0; y < h; ++y) {
                memcpy(
                    static_cast<unsigned char *>(bits) + (y * bytesPerLine),
                    img.scanLine(y),
                    bytesPerLine
                );
            }
        }
        return hBitmap;
    }

    static QPixmap generateOpaqueScreenshot(QWidget *widget, const QSize &targetSize, const QColor &bgColor) {
        // Рендерим исходный виджет (он будет с острыми углами из-за QSS)
        QPixmap rawContent(widget->size());
        rawContent.fill(Qt::transparent);
        widget->render(&rawContent, QPoint(), QRegion(), QWidget::DrawChildren);

        // Готовим холст
        QPixmap finalPix(widget->size());
        finalPix.fill(Qt::transparent);

        QPainter painter(&finalPix);
        painter.setRenderHint(QPainter::Antialiasing);

        if (isWindows11OrGreater()) {
            // Win11: Эмулируем скругление DWM
            QPainterPath path;
            path.addRoundedRect(finalPix.rect(), ACRYLIC_WINDOW_RADIUS, ACRYLIC_WINDOW_RADIUS);

            // Заливаем фон ("акрил") внутри скругления
            painter.fillPath(path, bgColor);

            // Обрезаем контент виджета по скруглению
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, rawContent);

            painter.setClipping(false);

            const QPen borderPen(QColor(0x42, 0x42, 0x42), 1.0); // Серый бордер, 1px
            painter.setPen(borderPen);

            // Преобразуем QRect в QRectF для использования дробных смещений
            const QRectF borderRect = finalPix.rect().toRectF().adjusted(0.5, 0.5, -0.5, -0.5);

            QPainterPath borderPath;
            borderPath.addRoundedRect(borderRect, ACRYLIC_WINDOW_RADIUS, ACRYLIC_WINDOW_RADIUS);

            painter.drawPath(borderPath);
        } else {
            // Win10: Оставляем квадратом
            painter.fillRect(finalPix.rect(), bgColor);
            painter.drawPixmap(0, 0, rawContent);
        }

        painter.end();

        // Масштабируем
        if (finalPix.size() != targetSize) {
            return finalPix.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        return finalPix;
    }

public:
    static bool handleIconicMessages(QWidget *widget, void *message,
                                     const QColor &bgFallbackColor = QColor(32, 32, 32)) {
        const MSG *msg = static_cast<MSG *>(message);
        const HWND hwnd = msg->hwnd;

        if (msg->message == WM_DWMSENDICONICTHUMBNAIL) {
            int width = HIWORD(msg->lParam);
            int height = LOWORD(msg->lParam);

            if (width == 0 || height == 0) {
                width = widget->width() / 4;
                height = widget->height() / 4;
            }

            const QPixmap resultPix = generateOpaqueScreenshot(widget, QSize(width, height), bgFallbackColor);
            const HBITMAP hbm = qtPixmapToHBitmap(resultPix);
            (void) DwmSetIconicThumbnail(hwnd, hbm, 0);
            DeleteObject(hbm);
            return true;
        }
        if (msg->message == WM_DWMSENDICONICLIVEPREVIEWBITMAP) {
            const QPixmap resultPix = generateOpaqueScreenshot(widget, widget->size(), bgFallbackColor);
            const HBITMAP hbm = qtPixmapToHBitmap(resultPix);
            (void) DwmSetIconicLivePreviewBitmap(hwnd, hbm, nullptr, 0);
            DeleteObject(hbm);
            return true;
        }

        return false;
    }

    // 0xCC (~80%) | 0xE0 (~88%) | 0xE3 (~90%) | 0xE6 (~92%) | 0xF0 (~94%) | 0xF3 (~96%) | 0xF6 (~98%) | 0xF9 (~100%)
    static void enableAcrylic(
        const QWidget *widget,
        const DWORD alphaWin11 = 0x40,
        const DWORD rgbWin11 = 0x202020,
        const DWORD alphaWin10 = 0xE6,
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

            if (const HRGN hrgn = CreateRectRgn(
                0, 0, widget->width() + 1, widget->height() + 1))
                SetWindowRgn(hwnd, hrgn, TRUE);
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
};
