#include "tray.h"

#include "../../core/config/appSettings.h"
#include "../../core/config/logger.h"
#include "../../core/i18n/lang.h"
#include "../helpers/acrylicHelper.h"
#include "../helpers/iconHelper.h"
#include "../helpers/screenResolver.h"
#include "../helpers/trayHoverHelper.h"
#include "../helpers/windowsNotificationState.h"
#include "../helpers/windowsToastIdentity.h"
#include "../helpers/windowsToastNotification.h"
#include "../widgets/soundManager.h"

#include <QDateTime>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QScreen>
#include <array>

TrayManager::TrayManager(QWidget* parent)
    : QWidget(parent), updateManager(new UpdateManager(this)), audioEffectOn(new QSoundEffect(this)),
      audioEffectOff(new QSoundEffect(this)), audioEffectUpdateAlert(new QSoundEffect(this)),
      clickTimer(new QTimer(this))
{
    ui.setupUi(this);

    setWindowFlags(Qt::Popup | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::StrongFocus);

#ifdef Q_OS_WIN
    setupToastActivationBridge();
#endif

    connect(updateManager, &UpdateManager::updateAvailable, this, &TrayManager::handleUpdateAvailable);
    connect(updateManager, &UpdateManager::noUpdateAvailable, this, &TrayManager::handleNoUpdateAvailable);

    updateManager->start();

    setupAnimations();

    // Звуки
    audioEffectOn->setSource(QUrl("qrc:/sounds/sounds/on.wav"));
    audioEffectOn->setVolume(0.5F);
    audioEffectOff->setSource(QUrl("qrc:/sounds/sounds/off.wav"));
    audioEffectOff->setVolume(0.5F);
    audioEffectUpdateAlert->setSource(QUrl("qrc:/sounds/sounds/notification_alert.wav"));
    audioEffectUpdateAlert->setVolume(0.5F);

    soundManager::instance().registerEffect(audioEffectOn);
    soundManager::instance().registerEffect(audioEffectOff);
    soundManager::instance().registerEffect(audioEffectUpdateAlert);

    // Таймер для разделения Single и Double кликов
    clickTimer->setSingleShot(true);
    connect(clickTimer, &QTimer::timeout, this, [this]() {
        enabled = !enabled;
        soundManager::playEffect(enabled ? audioEffectOn : audioEffectOff);
        emit keyboardToggled(enabled);
        updateTrayIcon();
        updateInfo();
    });

    ui.info_frame->installEventFilter(this);

    setupUiBehavior();
    setupTrayIcon();

    setWindowOpacity(0.0);
    hide();
    updateInfo();

    LOG_DEBUG() << "TrayManager initialized";
}

TrayManager::~TrayManager()
{
#ifdef Q_OS_WIN
    if (m_toastActivationEvent != nullptr) {
        CloseHandle(m_toastActivationEvent);
        m_toastActivationEvent = nullptr;
    }
#endif
    delete settingsWindow;
}

void TrayManager::setupAnimations()
{
    fadeIn = new QPropertyAnimation(this, "windowOpacity", this);
    fadeIn->setDuration(180);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);

    posAnim = new QPropertyAnimation(this, "pos", this);
    posAnim->setDuration(250);
    posAnim->setEasingCurve(QEasingCurve::OutBack);

    showGroup = new QParallelAnimationGroup(this);
    showGroup->addAnimation(fadeIn);
    showGroup->addAnimation(posAnim);

    fadeOut = new QPropertyAnimation(this, "windowOpacity", this);
    fadeOut->setDuration(160);
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::InCubic);

    connect(fadeOut, &QPropertyAnimation::finished, this, [this]() {
        this->hide();
        m_isClosing = false;
    });
}

void TrayManager::showAtCursor()
{
    m_isClosing = false;
    fadeOut->stop();

    updateInfo();

    // Подготовка размеров попапа до его появления
    layout()->activate();
    adjustSize();

    const QPoint cursorPos = QCursor::pos();
    const QScreen* screen = ScreenResolver::atPointOrPrimary(cursorPos);
    if (screen == nullptr) {
        return;
    }

    const QRect screenRect = screen->availableGeometry();
    QPoint finalPos = cursorPos;
    constexpr int padding = 3;
    constexpr int slideDist = 12;

    // Вычисление позиции
    bool isLeft = true;
    if (finalPos.x() + width() > screenRect.right()) {
        finalPos.setX(finalPos.x() - width() - padding);
        isLeft = false;
    }
    else {
        finalPos.setX(finalPos.x() + padding);
    }

    bool isTop = true;
    if (finalPos.y() + height() > screenRect.bottom()) {
        finalPos.setY(finalPos.y() - height() - padding);
        isTop = false;
    }
    else {
        finalPos.setY(finalPos.y() + padding);
    }

    // Анимация вылета
    QPoint startPos = finalPos;
    if (cursorPos.y() > screenRect.bottom() - 100 || cursorPos.y() < screenRect.top() + 100) {
        startPos.setY(isTop ? finalPos.y() - slideDist : finalPos.y() + slideDist);
    }
    else {
        startPos.setX(isLeft ? finalPos.x() - slideDist : finalPos.x() + slideDist);
    }

    move(startPos);
    setWindowOpacity(0.0);

    show();
    raise();
    activateWindow();

    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    posAnim->setStartValue(startPos);
    posAnim->setEndValue(finalPos);

    showGroup->start();

    QTimer::singleShot(0, this, [this]() { AcrylicHelper::enableAcrylic(this); });
}

void TrayManager::hideAnimated() const
{
    if (!isVisible() || m_isClosing) {
        return;
    }
    m_isClosing = true;
    showGroup->stop();
    fadeOut->setStartValue(this->windowOpacity());
    fadeOut->start();
}

void TrayManager::openSettings()
{
    hideAnimated();
    ensureSettingsWindow();
    clearPendingUpdate();
#ifdef Q_OS_WIN
    WindowsToastNotification::clearToastHistoryForApp();
#endif

    QTimer::singleShot(0, settingsWindow, [this]() {
        settingsWindow->openCentered();
        settingsWindow->raise();
        settingsWindow->activateWindow();
    });
}

void TrayManager::ensureSettingsWindow()
{
    if (settingsWindow != nullptr) {
        return;
    }

    settingsWindow = new SettingsWindow(nullptr);
    settingsWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    settingsWindow->setUpdateManager(updateManager);
    settingsWindow->setPendingUpdateHint(m_hasAvailableUpdate, m_availableUpdateVersion);

    connect(settingsWindow, &SettingsWindow::settingsSaved, this, [this]() {
        updateInfo();
        updateTrayIcon();
        if (m_currentGlobalNotif != nullptr) {
            m_currentGlobalNotif->refreshTranslations();
        }
    });
}

void TrayManager::setupTrayIcon()
{
    updateTrayIcon();
    trayIcon.setVisible(true);

    connect(&trayIcon, &QSystemTrayIcon::activated, this, [this](const QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            clickTimer->start(250);
        }
        else if (reason == QSystemTrayIcon::DoubleClick) {
            clickTimer->stop();
            openSettings();
        }
        else if (reason == QSystemTrayIcon::Context) {
            if (isVisible() && !m_isClosing) {
                hideAnimated();
            }
            else {
                showAtCursor();
            }
        }
    });

    connect(&trayIcon, &QSystemTrayIcon::messageClicked, this, [this]() {
        if (!m_hasAvailableUpdate || m_availableUpdateVersion.isEmpty()) {
            return;
        }
        LOG_INFO() << "System notification clicked: opening deferred update popup";
        showGlobalNotification(
                GlobalNotification::Mode::UpdateAvailable, m_availableUpdateVersion, m_availableUpdateUrl);
        clearPendingUpdate();
#ifdef Q_OS_WIN
        WindowsToastNotification::clearToastHistoryForApp();
#endif
    });
}

void TrayManager::updateTrayIcon()
{
    trayIcon.setIcon(buildTrayIcon());
    updateTrayToolTip();
}

void TrayManager::updateInfo() const
{
    ui.status_value->setText(enabled ? Lang::tr("TRAY_TOGGLE_ENABLED") : Lang::tr("TRAY_TOGGLE_DISABLED"));
    ui.hotkey_value->setText(AppSettings::hotkeyName);
    ui.delay_value->setText(QString::number(AppSettings::switchDelayMs));
    ui.toggle_btn->setText(enabled ? Lang::tr("TRAY_TOGGLE_PAUSE") : Lang::tr("TRAY_TOGGLE_RESUME"));

    ui.settings_btn->setText(Lang::tr("TRAY_SETTINGS"));
    ui.exit_btn->setText(Lang::tr("TRAY_EXIT"));
    ui.status_key->setText(Lang::tr("TRAY_LABEL_STATUS"));
    ui.hotkey_key->setText(Lang::tr("TRAY_LABEL_HOTKEY"));
    ui.delay_key->setText(Lang::tr("TRAY_LABEL_DELAY"));

    ui.toggle_btn->setIcon(IconHelper::loadIcon(enabled ? ":/icons/icons/FlashSparkleRegular.svg"
                                                        : ":/icons/icons/FlashSparkleFilled.svg"));
    ui.settings_btn->setIcon(IconHelper::loadIcon(":/icons/icons/FlashSettingsRegular.svg"));
    ui.exit_btn->setIcon(IconHelper::loadIcon(":/icons/icons/FlashOffRegular.svg"));
}

bool TrayManager::shouldDeferPopupNotification()
{
#ifdef Q_OS_WIN
    const auto [hasSystemNotificationState,
                systemStateHr,
                systemStateName,
                deferBySystemState,
                deferByFullscreenWindow,
                deferByQuietHoursService,
                deferByGlobalToastSetting,
                deferByFocusSession,
                toastSettingName,
                deferByToastSetting,
                quietHoursProfileName,
                deferByQuietHoursProfile,
                shouldDefer] = WindowsNotificationState::evaluatePopupDeferral();

    if (!hasSystemNotificationState) {
        LOG_WARNING() << "SHQueryUserNotificationState failed, continuing with fallback checks. HRESULT=0x"
                      << QString::number(static_cast<qulonglong>(systemStateHr), 16);
    }

    LOG_DEBUG() << "Notification state: " << systemStateName << "; defer by state=" << deferBySystemState
                << "; defer by fullscreen=" << deferByFullscreenWindow
                << "; defer by quiet-hours-service=" << deferByQuietHoursService
                << "; defer by global-toast-setting=" << deferByGlobalToastSetting
                << "; defer by focus-session=" << deferByFocusSession << "; toast-setting=" << toastSettingName
                << "; defer by toast-setting=" << deferByToastSetting
                << "; quiet-hours-profile=" << quietHoursProfileName
                << "; defer by quiet-hours-profile=" << deferByQuietHoursProfile << "; defer popup=" << shouldDefer;
    return shouldDefer;
#else
    return false;
#endif
}

void TrayManager::handleUpdateAvailable(const QString& version, const QString& url, const bool isManualCheck)
{
    LOG_INFO() << "Update available detected. version=" << version << "; manual=" << (isManualCheck ? "true" : "false");
    setAvailableUpdate(version, url);

    if (isManualCheck) {
        const bool suppressActive = shouldDeferPopupNotification();
        LOG_INFO() << "Manual check: showing custom global notification";
        clearPendingUpdate();
        showGlobalNotification(GlobalNotification::Mode::UpdateAvailable, version, url);
        if (!suppressActive) {
            playUpdateAvailableAlert();
        }
        return;
    }

    if (shouldDeferPopupNotification()) {
        LOG_INFO() << "Auto check: DND/fullscreen active, deferring popup and sending system notification";
        setPendingUpdate(version, url);
        showSystemUpdateMessage(version, false);
        return;
    }

    LOG_INFO() << "Auto check: notifications allowed, showing custom global notification";
    clearPendingUpdate();
    showGlobalNotification(GlobalNotification::Mode::UpdateAvailable, version, url);
    playUpdateAvailableAlert();
}

void TrayManager::handleNoUpdateAvailable(const QString& version, const bool isManualCheck)
{
    clearAvailableUpdate();
    clearPendingUpdate();

    if (!isManualCheck) {
        return;
    }

    if (shouldDeferPopupNotification()) {
        if (QSystemTrayIcon::supportsMessages()) {
            trayIcon.showMessage(Lang::tr("NOTIFICATION_UPD_NOT_AVAILABLE_TITLE"),
                                 Lang::tr("NOTIFICATION_UPD_NOT_AVAILABLE_DESC").arg(version),
                                 QSystemTrayIcon::Information,
                                 6000);
        }
        return;
    }

    showGlobalNotification(GlobalNotification::Mode::UpToDate, version, {});
}

void TrayManager::showGlobalNotification(const GlobalNotification::Mode mode,
                                         const QString& version,
                                         const QString& url)
{
    if (m_currentGlobalNotif) {
        m_currentGlobalNotif->close();
        m_currentGlobalNotif = nullptr;
    }

    m_currentGlobalNotif = new GlobalNotification(mode, version, url);
    m_currentGlobalNotif->show();
    m_currentGlobalNotif->raise();
}

void TrayManager::setAvailableUpdate(const QString& version, const QString& url)
{
    const bool changed = !m_hasAvailableUpdate || m_availableUpdateVersion != version || m_availableUpdateUrl != url;
    m_availableUpdateVersion = version;
    m_availableUpdateUrl = url;
    m_hasAvailableUpdate = true;
    if (settingsWindow != nullptr) {
        settingsWindow->setPendingUpdateHint(true, version);
    }
    if (changed) {
        LOG_INFO() << "Available update stored. version=" << version;
        updateTrayIcon();
    }
}

void TrayManager::clearAvailableUpdate()
{
    const bool hadAvailable =
            m_hasAvailableUpdate || !m_availableUpdateVersion.isEmpty() || !m_availableUpdateUrl.isEmpty();
    m_hasAvailableUpdate = false;
    m_availableUpdateVersion.clear();
    m_availableUpdateUrl.clear();
    if (settingsWindow != nullptr) {
        settingsWindow->setPendingUpdateHint(false);
    }
    if (hadAvailable) {
        LOG_INFO() << "Available update state cleared";
        updateTrayIcon();
    }
}

void TrayManager::setPendingUpdate(const QString& version, const QString& url)
{
    m_pendingUpdateVersion = version;
    m_pendingUpdateUrl = url;
    m_hasPendingUpdate = true;
    LOG_INFO() << "Pending update stored in tray. version=" << version;
    updateTrayIcon();
}

void TrayManager::clearPendingUpdate()
{
    const bool hadPending = m_hasPendingUpdate || !m_pendingUpdateVersion.isEmpty() || !m_pendingUpdateUrl.isEmpty();
    m_hasPendingUpdate = false;
    m_pendingUpdateVersion.clear();
    m_pendingUpdateUrl.clear();
    if (hadPending) {
        LOG_INFO() << "Pending update cleared from tray";
    }
    if (hadPending) {
        updateTrayIcon();
    }
}

QIcon TrayManager::buildTrayIcon() const
{
    QIcon base = IconHelper::loadIcon(enabled ? ":/icons/icons/FlashSparkleFilled2.png"
                                              : ":/icons/icons/FlashSparkleRegular2.png");

    if (!m_hasPendingUpdate) {
        return base;
    }

    QIcon badgedIcon;
    for (constexpr std::array<int, 7> sizes = {16, 20, 24, 32, 40, 48, 64}; const int size : sizes) {
        QPixmap pixmap = base.pixmap(size, size);
        if (pixmap.isNull()) {
            continue;
        }

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const int gap = qMax(1, size / 16);
        const int diameter = qMax(6, size / 3);
        const int margin = qMax(0, size / 16);
        const QRect outerRect(
                size - diameter - margin - (gap * 2), margin - gap, diameter + (gap * 2), diameter + (gap * 2));
        const QRect badgeRect = outerRect.adjusted(gap, gap, -gap, -gap);

        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::transparent);
        painter.drawEllipse(outerRect);

        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.setBrush(QColor(56, 138, 255));
        painter.drawEllipse(badgeRect);

        badgedIcon.addPixmap(pixmap);
    }

    if (badgedIcon.isNull()) {
        return base;
    }
    return badgedIcon;
}

void TrayManager::updateTrayToolTip()
{
    QString tooltip = AppSettings::APP_NAME;
    if (m_hasAvailableUpdate) {
        tooltip += QString("\n%1").arg(Lang::tr("NOTIFICATION_UPD_AVAILABLE_TITLE"));
        if (!m_availableUpdateVersion.isEmpty()) {
            tooltip += QString("\n%1").arg(m_availableUpdateVersion);
        }
    }
    trayIcon.setToolTip(tooltip);
}

void TrayManager::showSystemUpdateMessage(const QString& version, const bool isManualCheck)
{
    LOG_INFO() << "Requesting system notification. supportsMessages="
               << (QSystemTrayIcon::supportsMessages() ? "true" : "false");

    const QString title = Lang::tr("NOTIFICATION_UPD_AVAILABLE_TITLE");
    QString message = Lang::tr("NOTIFICATION_UPD_SYSTEM_BODY").arg(version);
    message += QString("\n%1").arg(Lang::tr("NOTIFICATION_UPD_SYSTEM_CLICK_HINT"));

#ifdef Q_OS_WIN
    WindowsToastNotification::clearToastHistoryForApp();
    if (WindowsToastNotification::showToastNotification(title, message, {}, true)) {
        LOG_INFO() << "Native Windows toast notification delivered";
        return;
    }
#endif

    if (QSystemTrayIcon::supportsMessages()) {
        trayIcon.showMessage(title, message, QSystemTrayIcon::Information, isManualCheck ? 15000 : 10000);
        return;
    }

    LOG_WARNING() << "Failed to show system notification via tray and native toast";
}

void TrayManager::playUpdateAvailableAlert() const
{
    if (audioEffectUpdateAlert == nullptr) {
        return;
    }
    soundManager::playEffect(audioEffectUpdateAlert);
}

void TrayManager::handleToastActivationRequest()
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (m_lastToastActivationMs > 0 && (nowMs - m_lastToastActivationMs) < 1200) {
        LOG_INFO() << "Toast activation ignored: duplicate signal";
        return;
    }
    m_lastToastActivationMs = nowMs;

    LOG_INFO() << "Handling toast activation request";
    if (m_hasAvailableUpdate && !m_availableUpdateVersion.isEmpty()) {
        showGlobalNotification(
                GlobalNotification::Mode::UpdateAvailable, m_availableUpdateVersion, m_availableUpdateUrl);
        clearPendingUpdate();
#ifdef Q_OS_WIN
        WindowsToastNotification::clearToastHistoryForApp();
#endif
        return;
    }

    LOG_INFO() << "Toast activation requested but no cached update; forcing update check";
    if (updateManager != nullptr) {
        updateManager->checkForUpdatesForce();
    }
}

#ifdef Q_OS_WIN
std::wstring TrayManager::toastActivationEventNameWide()
{
    return WindowsToastIdentity::toastActivationEventNameWide();
}

void TrayManager::setupToastActivationBridge()
{
    const std::wstring eventName = toastActivationEventNameWide();
    m_toastActivationEvent = CreateEventW(nullptr, TRUE, FALSE, eventName.c_str());
    if (m_toastActivationEvent == nullptr) {
        LOG_WARNING() << "Failed to create/open toast activation event. GetLastError="
                      << static_cast<qulonglong>(GetLastError());
        return;
    }

    m_toastActivationPollTimer = new QTimer(this);
    m_toastActivationPollTimer->setInterval(250);
    connect(m_toastActivationPollTimer, &QTimer::timeout, this, &TrayManager::pollToastActivationSignal);
    m_toastActivationPollTimer->start();
}

void TrayManager::pollToastActivationSignal()
{
    if (m_toastActivationEvent == nullptr) {
        return;
    }

    if (const DWORD waitResult = WaitForSingleObject(m_toastActivationEvent, 0); waitResult != WAIT_OBJECT_0) {
        return;
    }

    ResetEvent(m_toastActivationEvent);
    LOG_INFO() << "Toast activation signal received from external launch";
    handleToastActivationRequest();
}
#endif

void TrayManager::animateToggleButton()
{
    auto* effect = new QGraphicsOpacityEffect(ui.toggle_btn);
    ui.toggle_btn->setGraphicsEffect(effect);

    auto* fadeOutBtn = new QPropertyAnimation(effect, "opacity");
    fadeOutBtn->setDuration(160);
    fadeOutBtn->setStartValue(1.0);
    fadeOutBtn->setEndValue(0.0);

    auto* fadeInBtn = new QPropertyAnimation(effect, "opacity");
    fadeInBtn->setDuration(200);
    fadeInBtn->setStartValue(0.0);
    fadeInBtn->setEndValue(1.0);

    connect(fadeOutBtn, &QPropertyAnimation::finished, this, [this, fadeInBtn]() {
        updateInfo();
        fadeInBtn->start(QAbstractAnimation::DeleteWhenStopped);
    });

    connect(fadeInBtn, &QPropertyAnimation::finished, this, [this]() { ui.toggle_btn->setGraphicsEffect(nullptr); });

    fadeOutBtn->start(QAbstractAnimation::DeleteWhenStopped);
}

void TrayManager::setupUiBehavior()
{
    connect(ui.settings_btn, &QPushButton::clicked, this, &TrayManager::openSettings);

    connect(ui.exit_btn, &QToolButton::clicked, this, &TrayManager::exitRequested);

    connect(ui.toggle_btn, &QToolButton::clicked, this, [this]() {
        enabled = !enabled;
        soundManager::playEffect(enabled ? audioEffectOn : audioEffectOff);
        emit keyboardToggled(enabled);
        animateToggleButton();
        updateTrayIcon();
    });

    TrayHoverHelper::initializeHover(this);
}

bool TrayManager::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == ui.info_frame) {
        if (event->type() == QEvent::Enter) {
            TrayHoverHelper::animateHover(ui.info_frame, true);
        }
        else if (event->type() == QEvent::Leave) {
            TrayHoverHelper::animateHover(ui.info_frame, false);
        }
    }
    return QWidget::eventFilter(obj, event);
}

void TrayManager::focusOutEvent(QFocusEvent* event)
{
    hideAnimated();
    QWidget::focusOutEvent(event);
}

void TrayManager::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    AcrylicHelper::updateRegion(this);
}
