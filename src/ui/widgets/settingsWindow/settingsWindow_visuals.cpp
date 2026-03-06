#include "../../../core/config/appSettings.h"
#include "../../../core/config/logger.h"
#include "../../helpers/acrylicHelper.h"
#include "../../helpers/iconHelper.h"
#include "../../helpers/screenResolver.h"
#include "../../helpers/syncIconHelper.h"
#include "../notifications/inAppNotification.h"
#include "settingsWindow.h"

#include <QApplication>
#include <QScreen>
#include <QVariantAnimation>

void SettingsWindow::initVisuals()
{
    // Временно скрываем элементы
    ui.btn_exclusions_top_sider->hide();
    ui.btn_indicator_top_sider->hide();
    ui.btn_info_top_sider->hide();
    ui.app_theme_frame->hide();
    ui.app_theme_label->hide();
    // ---------------

    // Селекторы
    addSelectorForFrame(ui.key_select_frame);
    addSelectorForFrame(ui.app_startup_frame);
    // addSelectorForFrame(ui.app_theme_frame);
    addSelectorForFrame(ui.app_lang_frame);

    // Тень
    m_shadow = new QGraphicsDropShadowEffect(ui.content_container);
    m_shadow->setBlurRadius(12);
    m_shadow->setOffset(-2, 0);
    m_shadow->setColor(QColor(0, 0, 0, 120));
    ui.content_container->setGraphicsEffect(m_shadow);

    // Иконки
    ui.btn_general_top_sider->setIcon(
            IconHelper::loadIcon(":/icons/icons/SettingsFilled.svg", QColor(225, 225, 225), QSize(42, 42)));
    ui.key_select_label_img->setIcon(
            IconHelper::loadIcon(":/icons/icons/InfoRegular.svg", QColor(175, 175, 175), QSize(16, 16)));
    ui.btn_upd_manually->setIcon(
            IconHelper::loadIcon(":/icons/icons/SyncFilled.svg", QColor(175, 175, 175), QSize(18, 18)));
    updateManualCheckButtonIcon();

    // Драггер
    dragger = new WindowDragger(this);
    dragger->addIgnoredWidget(ui.btn_close_bot_sider);

    // Предупреждение при наведении
    keyHoverWarning = new KeyHoverWarning(this);
    ui.key_select_label_img->setAttribute(Qt::WA_Hover);
    ui.key_select_label_img->installEventFilter(this);

    // Слайдер задержки
    ui.key_delay_slider->setValue(AppSettings::switchDelayMs);
    ui.key_delay_spinbox->setValue(AppSettings::switchDelayMs);
    ui.key_delay_slider->setSingleStep(10);
    ui.key_delay_slider->setPageStep(50);
    ui.key_delay_slider->setTickInterval(10);
    ui.key_delay_slider->setTracking(true);
    ui.key_delay_spinbox->setSingleStep(10);

    // Ручной запуск проверки
    m_syncRotationAnim = new QVariantAnimation(this);
    m_syncRotationAnim->setStartValue(0);
    m_syncRotationAnim->setEndValue(360);
    m_syncRotationAnim->setDuration(1000);
    m_syncRotationAnim->setLoopCount(-1); // Бесконечно

    connect(m_syncRotationAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        updateSyncIconRotation(value.toInt());
    });

    connect(m_syncRotationAnim, &QVariantAnimation::finished, this, [this]() { updateManualCheckButtonIcon(); });
}

void SettingsWindow::addSelectorForFrame(QFrame* frame, const QString& extraStyle)
{
    if (frame == nullptr) {
        return;
    }
    auto* const sel = new AnimatedSelector(this);
    sel->bindToFrame(frame, extraStyle);
    selectors.append(sel);
    QTimer::singleShot(0, sel, &AnimatedSelector::initPosition);
}

void SettingsWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        AcrylicHelper::enableActiveBackground(this);
        AcrylicHelper::updateRegion(this);
    });
    QTimer::singleShot(0, this, [this]() {
        for (AnimatedSelector* sel : selectors) {
            if (sel != nullptr) {
                sel->initPosition();
            }
        }
    });
}

bool SettingsWindow::event(QEvent* ev)
{
    if (ev->type() == QEvent::WindowActivate || ev->type() == QEvent::WindowDeactivate) {
        bool active = (ev->type() == QEvent::WindowActivate);
        QTimer::singleShot(0, this, [this, active]() {
            if (active) {
                AcrylicHelper::enableActiveBackground(this);
            }
            else {
                AcrylicHelper::enableInactiveBackground(this);
            }
            AcrylicHelper::updateRegion(this);
        });
        LOG_DEBUG() << "Settings window is " << (active ? "active" : "inactive");
    }
    return QWidget::event(ev);
}

bool SettingsWindow::eventFilter(QObject* watched, QEvent* event)
{
    // Логика для предупреждения о клавишах
    if (watched == ui.key_select_label_img && keyHoverWarning) {
        if (event->type() == QEvent::Enter) {
            keyHoverWarning->showNear(ui.key_select_label_img);
        }
        else if (event->type() == QEvent::Leave) {
            keyHoverWarning->hideNow();
        }
    }
    // Логика для тултипа кнопки обновления
    else if (watched == ui.btn_upd_manually && updateBtnToolTip) {
        if (event->type() == QEvent::Enter) {
            // Показываем только если стек уведомлений пуст
            if (InAppNotification::stack.isEmpty()) {
                refreshUpdateButtonTooltipLive();
            }
        }
        else if (event->type() == QEvent::Leave) {
            // Скрываем всегда. Если мышь ушла, тултип не должен оставаться,
            // даже если в этот момент появилось уведомление.
            updateBtnToolTip->hideAnimated();
        }
    }

    return QWidget::eventFilter(watched, event);
}

bool SettingsWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
    if (eventType == "windows_generic_MSG") {
        if (AcrylicHelper::handleIconicMessages(this, message, QColor(32, 32, 32))) {
            *result = 0;
            return true;
        }
    }
    return QWidget::nativeEvent(eventType, message, result);
}

void SettingsWindow::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    if (updateBtnToolTip != nullptr) {
        updateBtnToolTip->hide();
    }
    if (keyHoverWarning != nullptr) {
        keyHoverWarning->hideNow();
    }
    if (updPopup != nullptr) {
        updPopup->hide();
    }
}

void SettingsWindow::openCentered()
{
    const QScreen* screen = ScreenResolver::primaryOrFirst();
    if (screen == nullptr) {
        return;
    }

    const QRect geom = screen->availableGeometry();

    // Принудительно заставляем Qt пересчитать размеры контента
    if (layout()) {
        layout()->activate();
    }
    adjustSize();

    const QSize s = size();

    // Считаем центр
    const int x = geom.left() + (geom.width() - s.width()) / 2;
    const int y = geom.top() + (geom.height() - s.height()) / 2;

    // Сначала двигаем, потом показываем
    move(x, y);

    if (isHidden()) {
        show();
    }
    else {
        if (isMinimized()) {
            showNormal();
        }
        raise();
        activateWindow();
    }

    // Фокус и акрил
    QTimer::singleShot(50, this, [this]() {
        if (this->isActiveWindow() || (QApplication::activeWindow() == this)) {
            AcrylicHelper::enableActiveBackground(this);
        }
        else {
            AcrylicHelper::enableInactiveBackground(this);
        }
        AcrylicHelper::updateRegion(this);
    });
}

void SettingsWindow::updateSyncIconRotation(const int angle) const
{
    ui.btn_upd_manually->setIconSize(QSize(SyncIconHelper::CanvasSize, SyncIconHelper::CanvasSize));
    ui.btn_upd_manually->setIcon(SyncIconHelper::buildRotated(angle, m_hasPendingUpdate));
}

void SettingsWindow::updateManualCheckButtonIcon() const
{
    if (isManualCheckActive())
        return;

    ui.btn_upd_manually->setIconSize(QSize(SyncIconHelper::CanvasSize, SyncIconHelper::CanvasSize));
    ui.btn_upd_manually->setIcon(SyncIconHelper::buildStatic(m_hasPendingUpdate));
}

void SettingsWindow::stopSyncAnimation() const
{
    if (m_syncRotationAnim == nullptr) {
        return;
    }

    // Вместо мгновенной остановки (stop) завершить текущий цикл
    m_syncRotationAnim->setLoopCount(1);
}
