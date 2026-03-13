#pragma once

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QObject>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPixmap>
#include <QtWidgets/QWidget>

#include <Windows.h>
#include <bit>
#include <cstdint>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

constexpr int ACRYLIC_WINDOW_RADIUS = 8;
constexpr DWORD DWMWA_FORCE_ICONIC_REPRESENTATION_VALUE = 7;
constexpr DWORD DWMWA_HAS_ICONIC_BITMAP_VALUE = 10;
constexpr DWORD DWMWA_WINDOW_CORNER_PREFERENCE_VALUE = 33;
constexpr DWORD DWMWCP_ROUND_VALUE = 2;

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

    static const WinInternal& info()
    {
        static WinInternal instance;
        return instance;
    }

    static AcrylicHelper* instance()
    {
        static AcrylicHelper inst;
        return &inst;
    }

    static QPixmap& noiseTexture(const qreal dpr = 1.0)
    {
        static QPixmap pix;
        static qreal lastDpr = 0;

        if (pix.isNull() || !qFuzzyCompare(dpr, lastDpr)) {
            lastDpr = dpr;
            constexpr int baseSize = 128;
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
            pix.setDevicePixelRatio(dpr);
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

    static void enableActiveBackground(QWidget* widget)
    {
        if (widget == nullptr) {
            return;
        }

        enableAcrylic(widget);
        enableCustomPreview(widget);
    }

    static void
    enableInactiveBackground(QWidget* widget, const DWORD rgbWin11 = 0x202020, const DWORD rgbWin10 = 0x202020)
    {
        if (widget == nullptr || !info().setAttribPtr) {
            return;
        }

        widget->removeEventFilter(instance());

        auto* const hwnd = std::bit_cast<HWND>(static_cast<std::uintptr_t>(widget->winId()));
        if (hwnd == nullptr) {
            return;
        }

        ACCENT_POLICY policy{};
        if (info().isWin11) {
            policy.AccentState = static_cast<DWORD>(AccentState::EnableGradient);
            policy.GradientColor = (0xFFu << 24) | rgbWin11;
        }
        else if (info().isWin10) {
            policy.AccentState = static_cast<DWORD>(AccentState::EnableGradient);
            policy.GradientColor = (0xFFu << 24) | rgbWin10;
            updateRegion(widget);
        }
        else {
            policy.AccentState = static_cast<DWORD>(AccentState::Disabled);
        }

        WINDOWCOMPOSITIONATTRIBDATA data{19, &policy, sizeof(policy)};
        info().setAttribPtr(hwnd, &data);
    }

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

        if (info().isWin10 && !info().isWin11) {
            widget->removeEventFilter(instance());
            widget->installEventFilter(instance());
        }
        else {
            widget->removeEventFilter(instance());
        }

        if (const LONG ex = GetWindowLongW(hwnd, GWL_EXSTYLE); ex & WS_EX_LAYERED) {
            SetWindowLongW(hwnd, GWL_EXSTYLE, ex & ~WS_EX_LAYERED);
        }

        ACCENT_POLICY policy{};
        const auto& win = info();

        if (win.isWin11) {
            policy.AccentState = static_cast<DWORD>(AccentState::EnableAcrylicBlurBehind);
            policy.GradientColor = (alphaWin11 << 24) | rgbWin11;
            policy.AccentFlags = 2;

            constexpr DWORD roundWindowCornerPreference = DWMWCP_ROUND_VALUE;
            (void) DwmSetWindowAttribute(hwnd,
                                         DWMWA_WINDOW_CORNER_PREFERENCE_VALUE,
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
            return;
        }

        auto* const hwnd = std::bit_cast<HWND>(static_cast<std::uintptr_t>(widget->winId()));
        if (hwnd == nullptr) {
            return;
        }

        const qreal dpr = widget->devicePixelRatioF();
        const int physW = qRound(widget->width() * dpr);
        const int physH = qRound(widget->height() * dpr);

        if (const HRGN hrgn = CreateRectRgn(0, 0, physW + 1, physH + 1)) {
            if (SetWindowRgn(hwnd, hrgn, TRUE) == 0) {
                DeleteObject(hrgn);
            }
        }
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
        (void) DwmSetWindowAttribute(hwnd, DWMWA_FORCE_ICONIC_REPRESENTATION_VALUE, &fTrue, sizeof(fTrue));
        (void) DwmSetWindowAttribute(hwnd, DWMWA_HAS_ICONIC_BITMAP_VALUE, &fTrue, sizeof(fTrue));
        (void) DwmInvalidateIconicBitmaps(hwnd);
    }

    static bool handleIconicMessages(QWidget& widget, void* message, const QColor& bg = QColor(32, 32, 32))
    {
        if (message == nullptr) {
            return false;
        }

        const auto* msg = static_cast<MSG*>(message);
        if (msg->message == WM_DWMSENDICONICTHUMBNAIL) {
            const QSize requestedSize(static_cast<int>(HIWORD(msg->lParam)), static_cast<int>(LOWORD(msg->lParam)));
            const QSize targetSize =
                    normalizeThumbnailTargetSize(widget, requestedSize.width(), requestedSize.height());
            const QPixmap thumbnail = generateOpaqueScreenshot(widget, targetSize, bg);
            const HBITMAP hbm = qtPixmapToHBitmap(thumbnail);
            (void) DwmSetIconicThumbnail(msg->hwnd, hbm, 0);
            DeleteObject(hbm);
            return true;
        }
        if (msg->message == WM_DWMSENDICONICLIVEPREVIEWBITMAP) {
            const QSize liveSize = physicalWidgetSize(widget);
            const QPixmap livePreview = generateOpaqueScreenshot(widget, liveSize, bg);
            const HBITMAP hbm = qtPixmapToHBitmap(livePreview);
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
                const qreal dpr = widget->devicePixelRatioF();
                painter.drawTiledPixmap(widget->rect(), noiseTexture(dpr));
                return true;
            }
        }
        return QObject::eventFilter(obj, event);
    }

private:
    static QSize physicalWidgetSize(const QWidget& widget)
    {
        const qreal dpr = widget.devicePixelRatioF();
        const int width = qMax(1, qRound(widget.width() * dpr));
        const int height = qMax(1, qRound(widget.height() * dpr));
        return QSize(width, height);
    }

    static QSize
    normalizeThumbnailTargetSize(const QWidget& widget, const int requestedWidth, const int requestedHeight)
    {
        const QSize sourceSize = physicalWidgetSize(widget);
        if (!sourceSize.isValid()) {
            return QSize(qMax(1, requestedWidth), qMax(1, requestedHeight));
        }

        int width = requestedWidth;
        int height = requestedHeight;

        if (width <= 0 && height <= 0) {
            constexpr QSize fallbackBox(200, 200);
            return sourceSize.scaled(fallbackBox, Qt::KeepAspectRatio);
        }

        if (width <= 0) {
            width = qMax(1, qRound((static_cast<double>(sourceSize.width()) * height) / sourceSize.height()));
        }
        if (height <= 0) {
            height = qMax(1, qRound((static_cast<double>(sourceSize.height()) * width) / sourceSize.width()));
        }

        return QSize(width, height);
    }

    static HBITMAP qtPixmapToHBitmap(const QPixmap& pix)
    {
        if (pix.isNull()) {
            return nullptr;
        }
        QPixmap nativePix = pix;
        nativePix.setDevicePixelRatio(1.0);
        const QImage img = nativePix.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
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
            std::memcpy(bits, img.constBits(), static_cast<std::size_t>(img.sizeInBytes()));
        }
        return hBitmap;
    }

    static QPixmap renderPreviewSource(QWidget& widget, const QColor& bgColor)
    {
        const qreal dpr = widget.devicePixelRatioF();
        const qreal halfPixel = 0.5 / qMax(1.0, dpr);
        const QRectF hostRect(QPointF(halfPixel, halfPixel),
                              QSizeF(widget.size()) - QSizeF(halfPixel * 2.0, halfPixel * 2.0));

        QPixmap finalPix(physicalWidgetSize(widget));
        finalPix.fill(Qt::transparent);

        QPainter painter(&finalPix);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.scale(dpr, dpr);

        QPainterPath clipPath;
        if (isWindows11OrGreater()) {
            clipPath.addRoundedRect(hostRect, ACRYLIC_WINDOW_RADIUS, ACRYLIC_WINDOW_RADIUS);
            painter.fillPath(clipPath, bgColor);
            painter.setClipPath(clipPath);
        }
        else {
            painter.fillRect(QRectF(QPointF(0, 0), QSizeF(widget.size())), bgColor);
        }

        widget.render(&painter, QPoint(), QRegion(), QWidget::DrawWindowBackground | QWidget::DrawChildren);

        if (!clipPath.isEmpty()) {
            QPen pen(QColor(255, 255, 255, 38));
            pen.setWidthF(1.0 / qMax(1.0, dpr));
            painter.setClipping(false);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(clipPath);
        }

        painter.end();
        return finalPix;
    }

    static QPixmap generateOpaqueScreenshot(QWidget& widget, const QSize& targetSize, const QColor& bgColor)
    {
        const QPixmap source = renderPreviewSource(widget, bgColor);
        if (source.isNull() || targetSize.isEmpty() || source.size() == targetSize) {
            return source;
        }

        const QPixmap scaled = source.scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        if (scaled.size() == targetSize) {
            return scaled;
        }

        const int x = qMax(0, (scaled.width() - targetSize.width()) / 2);
        const int y = qMax(0, (scaled.height() - targetSize.height()) / 2);
        return scaled.copy(x, y, targetSize.width(), targetSize.height());
    }
};
