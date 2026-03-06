#pragma once
#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QObject>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPixmap>
#include <QtGui/QWindow>
#include <QtWidgets/QWidget>

#include <Windows.h>
#include <bit>
#include <cstdint>
#include <cstring>
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
enum class AccentState : std::uint8_t
{
    Disabled = 0,
    EnableGradient = 1,
    EnableTransparentGradient = 2,
    EnableBlurBehind = 3,
    EnableAcrylicBlurBehind = 4
};

struct ACCENT_POLICY
{
    DWORD AccentState;
    DWORD AccentFlags;
    DWORD GradientColor;
    DWORD AnimationId;
};

struct WINDOWCOMPOSITIONATTRIBDATA
{
    DWORD Attribute; // WCA_ACCENT_POLICY = 19
    PVOID Data;
    SIZE_T SizeOfData;
};

using pSetWindowCompositionAttribute = BOOL(WINAPI*)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);
using pRtlGetVersion = LONG(WINAPI*)(PRTL_OSVERSIONINFOEXW);

class AcrylicHelper final : public QObject
{
    // Структура для хранения версии ОС и указателей на функции (инициализируется один раз)
    struct WinInternal
    {
        bool isWin11;
        bool isWin10;
        pSetWindowCompositionAttribute setAttribPtr;

        WinInternal()
        {
            // Получаем версию ОС
            isWin11 = false;
            isWin10 = false;
            if (auto* const ntdll = GetModuleHandleW(L"ntdll.dll")) {
                if (const auto rtlGetVersion =
                            reinterpret_cast<pRtlGetVersion>(GetProcAddress(ntdll, "RtlGetVersion"))) {
                    RTL_OSVERSIONINFOEXW rovi{sizeof(rovi)};
                    if (rtlGetVersion(&rovi) == 0) {
                        isWin10 = (rovi.dwMajorVersion == 10);
                        isWin11 = (isWin10 && rovi.dwBuildNumber >= 22000);
                    }
                }
            }
            // Ищем функцию акрила
            setAttribPtr = reinterpret_cast<pSetWindowCompositionAttribute>(
                    GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));
        }
    };

    // Статический доступ к внутренним данным
    static const WinInternal& info()
    {
        static WinInternal instance;
        return instance;
    }

    // Синглтон для управления фильтром
    static AcrylicHelper* instance()
    {
        static AcrylicHelper inst;
        return &inst;
    }

    // Генерация текстуры шума для Win10
    static QPixmap& noiseTexture(const qreal dpr = 1.0)
    {
        static QPixmap pix;
        static qreal lastDpr = 0;

        // Пересоздаем текстуру только если изменился масштаб или её еще нет
        if (pix.isNull() || !qFuzzyCompare(dpr, lastDpr)) {
            lastDpr = dpr;
            constexpr int baseSize = 128;
            // Физический размер текстуры в пикселях
            const int physSize = qRound(baseSize * dpr);

            QImage img(physSize, physSize, QImage::Format_ARGB32_Premultiplied);
            img.fill(Qt::transparent);

            unsigned int x = 123456789;
            unsigned int y = 362436069;
            unsigned int z = 521288629;
            auto fastRand = [&]() {
                x ^= x << 16;
                x ^= x >> 5;
                x ^= x << 1;
                const unsigned int t = x;
                x = y;
                y = z;
                z = t ^ x ^ y;
                return z;
            };

            // Увеличиваем количество точек пропорционально площади (DPR^2)
            constexpr int dotsCount = 50000;
            const int iterations = qRound(dotsCount * dpr * dpr);

            QPainter p(&img);
            p.setRenderHint(QPainter::Antialiasing, false);

            for (int i = 0; i < iterations; ++i) {
                const int px = static_cast<int>(fastRand() % static_cast<unsigned int>(physSize));
                const int py = static_cast<int>(fastRand() % static_cast<unsigned int>(physSize));
                const int shade = static_cast<int>(fastRand() % 256U);
                p.setPen(QColor(shade, shade, shade, 1));
                p.drawPoint(px, py);
            }
            p.end();

            pix = QPixmap::fromImage(img);
            pix.setDevicePixelRatio(dpr); // Говорим Qt, что это HiDPI текстура
        }
        return pix;
    }

public:
    static bool isWindows11OrGreater()
    {
        return info().isWin11;
    }

    static bool isWindows10OrGreater()
    {
        return info().isWin10;
    }

    static void setAcrylicEnabled(QWidget* widget, const bool enabled)
    {
        if (widget == nullptr) {
            return;
        }
        if (enabled) {
            enableAcrylic(widget);
            enableCustomPreview(widget);
        }
        else {
            disableAcrylic(widget);
        }
    }

    // 0xCC (~80%) | 0xE0 (~88%) | 0xE3 (~90%) | 0xE6 (~92%) | 0xF0 (~94%) | 0xF3 (~96%) | 0xF6 (~98%) | 0xF9 (~100%)
    static void enableAcrylic(QWidget* widget,
                              const DWORD alphaWin11 = 0x40,
                              const DWORD rgbWin11 = 0x202020,
                              const DWORD alphaWin10 = 0xE6,
                              const DWORD rgbWin10 = 0x141414)
    {
        if (widget == nullptr || !info().setAttribPtr) {
            return;
        }
        auto* const hwnd = std::bit_cast<HWND>(static_cast<std::uintptr_t>(widget->winId()));
        if (hwnd == nullptr) {
            return;
        }

        // Регистрация фильтра шума для Win10 прямо здесь
        if (info().isWin10 && !info().isWin11) {
            widget->removeEventFilter(instance());
            widget->installEventFilter(instance());
        }

        // Чистим флаг Layered, чтобы WinAPI акрил не конфликтовал с прозрачностью Qt
        if (const LONG ex = GetWindowLongW(hwnd, GWL_EXSTYLE); ex & WS_EX_LAYERED) {
            SetWindowLongW(hwnd, GWL_EXSTYLE, ex & ~WS_EX_LAYERED);
        }

        ACCENT_POLICY policy{};
        const auto& win = info();

        if (win.isWin11) {
            policy.AccentState = static_cast<DWORD>(AccentState::EnableAcrylicBlurBehind);
            policy.GradientColor = (alphaWin11 << 24) | rgbWin11;
            policy.AccentFlags = 2;

            // Нативное скругление Win11 (DWM масштабирует его сам)
            constexpr DWORD windowCornerPreferenceAttribute = 33;
            constexpr DWORD roundWindowCornerPreference = 2;
            (void) DwmSetWindowAttribute(hwnd,
                                         windowCornerPreferenceAttribute,
                                         &roundWindowCornerPreference,
                                         sizeof(roundWindowCornerPreference));
        }
        else if (win.isWin10) {
            policy.AccentState = static_cast<DWORD>(AccentState::EnableBlurBehind);
            policy.GradientColor = (alphaWin10 << 24) | rgbWin10;
            policy.AccentFlags = 2;
            updateRegion(widget);
        }

        WINDOWCOMPOSITIONATTRIBDATA data{19, &policy, sizeof(policy)};
        win.setAttribPtr(hwnd, &data);
    }

    static void updateRegion(const QWidget* widget)
    {
        if (widget == nullptr || isWindows11OrGreater()) {
            return; // Win11 сама справляется
        }

        auto* const hwnd = std::bit_cast<HWND>(static_cast<std::uintptr_t>(widget->winId()));
        if (hwnd == nullptr) {
            return;
        }

        // Расчет физических пикселей для корректного DPI на Win10
        const qreal dpr = widget->devicePixelRatio();
        const int physW = qRound(widget->width() * dpr);
        const int physH = qRound(widget->height() * dpr);

        if (const HRGN hrgn = CreateRectRgn(0, 0, physW + 1, physH + 1)) {
            if (SetWindowRgn(hwnd, hrgn, TRUE) == 0) {
                DeleteObject(hrgn);
            }
        }
    }

    static void disableAcrylic(QWidget* widget)
    {
        if (widget == nullptr) {
            return;
        }

        // Обязательно снимаем фильтр при выключении
        widget->removeEventFilter(instance());

        if (!info().setAttribPtr) {
            return;
        }
        auto* const hwnd = std::bit_cast<HWND>(static_cast<std::uintptr_t>(widget->winId()));
        if (hwnd == nullptr) {
            return;
        }

        ACCENT_POLICY policy{static_cast<DWORD>(AccentState::Disabled), 0, 0, 0};
        WINDOWCOMPOSITIONATTRIBDATA data{19, &policy, sizeof(policy)};
        info().setAttribPtr(hwnd, &data);
    }

    static void enableCustomPreview(const QWidget* widget)
    {
        if (widget == nullptr) {
            return;
        }
        auto* const hwnd = std::bit_cast<HWND>(static_cast<std::uintptr_t>(widget->winId()));
        if (hwnd == nullptr) {
            return;
        }
        constexpr BOOL fTrue = TRUE;
        (void) DwmSetWindowAttribute(hwnd, 7, &fTrue, sizeof(fTrue));  // FORCE_ICONIC
        (void) DwmSetWindowAttribute(hwnd, 10, &fTrue, sizeof(fTrue)); // HAS_ICONIC_BITMAP
    }

    static bool handleIconicMessages(QWidget* widget, void* message, const QColor& bg = QColor(32, 32, 32))
    {
        const auto* msg = static_cast<MSG*>(message);
        if (msg->message == WM_DWMSENDICONICTHUMBNAIL) {
            const auto widthWord = HIWORD(msg->lParam);
            const auto heightWord = LOWORD(msg->lParam);
            int w = static_cast<int>(widthWord);
            int h = static_cast<int>(heightWord);
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

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (event->type() == QEvent::Paint) {
            if (auto* const widget = dynamic_cast<QWidget*>(obj)) {
                widget->removeEventFilter(this);
                QCoreApplication::sendEvent(widget, event);
                widget->installEventFilter(this);

                QPainter painter(widget);
                // Получаем DPR (например, 1.25, 1.5 и т.д.)
                const qreal dpr = widget->devicePixelRatio();

                // Рисуем текстуру, которая соответствует физическим пикселям
                painter.drawTiledPixmap(widget->rect(), noiseTexture(dpr));
                return true;
            }
        }
        return QObject::eventFilter(obj, event);
    }

private:
    static HBITMAP qtPixmapToHBitmap(const QPixmap& pix)
    {
        if (pix.isNull()) {
            return nullptr;
        }
        QImage img = pix.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = img.width();
        bmi.bmiHeader.biHeight = -img.height();
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void* bits = nullptr;
        const HBITMAP hBitmap = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (hBitmap && bits) {
            memcpy(bits, img.bits(), img.sizeInBytes());
        }
        return hBitmap;
    }

    static QPixmap generateOpaqueScreenshot(QWidget* widget, const QSize& targetSize, const QColor& bgColor)
    {
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
        }
        else {
            painter.fillRect(finalPix.rect(), bgColor);
            widget->render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
        }
        painter.end();
        return (finalPix.size() == targetSize)
                       ? finalPix
                       : finalPix.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
};
