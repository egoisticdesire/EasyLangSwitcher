#include "tray.h"
#include "../widgets/soundManager.h"
#include "../../core/config/logger.h"
#include "../../core/config/appSettings.h"
#include "../../core/i18n/lang.h"
#include "../helpers/acrylicHelper.h"
#include "../helpers/trayHoverHelper.h"
#include "../helpers/iconHelper.h"
#include <QApplication>
#include <QMouseEvent>
#include <QScreen>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QDateTime>
#include <array>
#include <functional>
#include <string>
#include <vector>

#ifdef Q_OS_WIN
#include <shellapi.h>
#include <roapi.h>
#include <windows.data.xml.dom.h>
#include <windows.foundation.h>
#include <windows.ui.notifications.h>
#include <windows.ui.shell.h>
#include <wrl.h>
#include <wrl/event.h>
#include <wrl/implements.h>
#include <wrl/wrappers/corewrappers.h>
#pragma comment(lib, "runtimeobject.lib")
#endif

namespace {
#ifdef Q_OS_WIN
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::UI::Shell;
using ToastActivatedHandler = __FITypedEventHandler_2_Windows__CUI__CNotifications__CToastNotification_IInspectable;

ComPtr<IToastNotification> g_activeToast;
ComPtr<ToastActivatedHandler> g_activeToastActivatedHandler;
EventRegistrationToken g_activeToastActivatedToken{};

bool ensureToastRuntimeInitialized() {
    static bool initialized = false;
    static HRESULT initResult = E_FAIL;

    if (!initialized) {
        initResult = RoInitialize(RO_INIT_MULTITHREADED);
        if (initResult == RPC_E_CHANGED_MODE) {
            initResult = RoInitialize(RO_INIT_SINGLETHREADED);
        }
        initialized = true;
    }

    if (FAILED(initResult)) {
        LOG_WARNING() << "RoInitialize failed for toast. HRESULT=0x"
                      << QString::number(static_cast<qulonglong>(initResult), 16);
        return false;
    }
    return true;
}

void resetActiveToastHandlers() {
    if (g_activeToast && g_activeToastActivatedHandler) {
        g_activeToast->remove_Activated(g_activeToastActivatedToken);
    }
    g_activeToast.Reset();
    g_activeToastActivatedHandler.Reset();
    g_activeToastActivatedToken = {};
}

std::wstring appUserModelIdWide() {
    return QString("%1.Desktop").arg(AppSettings::APP_NAME).toStdWString();
}

QString toastProtocolUri() {
    return QString("%1://toast/open-update").arg(QString(AppSettings::APP_NAME).toLower());
}

std::wstring toastActivationEventNameWide() {
    return QString("Local\\%1.ToastActivation").arg(AppSettings::APP_NAME).toStdWString();
}

QString notificationStateName(const QUERY_USER_NOTIFICATION_STATE state) {
    switch (state) {
        case QUNS_NOT_PRESENT: return "not_present";
        case QUNS_BUSY: return "busy";
        case QUNS_RUNNING_D3D_FULL_SCREEN: return "running_d3d_full_screen";
        case QUNS_PRESENTATION_MODE: return "presentation_mode";
        case QUNS_ACCEPTS_NOTIFICATIONS: return "accepts_notifications";
        case QUNS_QUIET_TIME: return "quiet_time";
        case QUNS_APP: return "app";
        default: return "unknown";
    }
}

QString toastNotificationSettingName(const NotificationSetting setting) {
    switch (setting) {
        case NotificationSetting_Enabled: return "enabled";
        case NotificationSetting_DisabledForApplication: return "disabled_for_application";
        case NotificationSetting_DisabledForUser: return "disabled_for_user";
        case NotificationSetting_DisabledByGroupPolicy: return "disabled_by_group_policy";
        case NotificationSetting_DisabledByManifest: return "disabled_by_manifest";
        default: return "unknown";
    }
}

bool isForegroundFullscreenWindow() {
    const HWND hwnd = GetForegroundWindow();
    if (!hwnd || IsIconic(hwnd)) return false;

    RECT windowRect{};
    if (!GetWindowRect(hwnd, &windowRect)) return false;

    const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
    if (!monitor) return false;

    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (!GetMonitorInfoW(monitor, &monitorInfo)) return false;

    const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    const bool framedWindow = (style & (WS_CAPTION | WS_THICKFRAME)) != 0;
    if (framedWindow && IsZoomed(hwnd)) return false;

    return windowRect.left <= monitorInfo.rcMonitor.left
           && windowRect.top <= monitorInfo.rcMonitor.top
           && windowRect.right >= monitorInfo.rcMonitor.right
           && windowRect.bottom >= monitorInfo.rcMonitor.bottom;
}

bool readRegistryDwordValue(
    const HKEY rootKey,
    const wchar_t *subKey,
    const wchar_t *valueName,
    DWORD &value
) {
    DWORD data = 0;
    DWORD dataSize = sizeof(data);
    const LSTATUS status = RegGetValueW(
        rootKey,
        subKey,
        valueName,
        RRF_RT_REG_DWORD,
        nullptr,
        &data,
        &dataSize
    );
    if (status != ERROR_SUCCESS) return false;
    value = data;
    return true;
}

bool readRegistryBinaryValue(
    const HKEY rootKey,
    const wchar_t *subKey,
    const wchar_t *valueName,
    std::vector<BYTE> &value
) {
    DWORD dataSize = 0;
    LSTATUS status = RegGetValueW(
        rootKey,
        subKey,
        valueName,
        RRF_RT_REG_BINARY,
        nullptr,
        nullptr,
        &dataSize
    );
    if (status != ERROR_SUCCESS || dataSize == 0) return false;

    std::vector<BYTE> data(dataSize);
    status = RegGetValueW(
        rootKey,
        subKey,
        valueName,
        RRF_RT_REG_BINARY,
        nullptr,
        data.data(),
        &dataSize
    );
    if (status != ERROR_SUCCESS) return false;

    data.resize(dataSize);
    value = data;
    return true;
}

enum class QuietHoursProfile {
    Unknown,
    Unrestricted,
    PriorityOnly,
    AlarmsOnly,
    Other,
};

QuietHoursProfile parseQuietHoursProfilePayload(const QString &payload) {
    if (payload.contains("Microsoft.QuietHoursProfile.AlarmsOnly", Qt::CaseInsensitive))
        return QuietHoursProfile::AlarmsOnly;
    if (payload.contains("Microsoft.QuietHoursProfile.PriorityOnly", Qt::CaseInsensitive))
        return QuietHoursProfile::PriorityOnly;
    if (payload.contains("Microsoft.QuietHoursProfile.Unrestricted", Qt::CaseInsensitive))
        return QuietHoursProfile::Unrestricted;
    if (payload.contains("Microsoft.QuietHoursProfile.", Qt::CaseInsensitive))
        return QuietHoursProfile::Other;
    return QuietHoursProfile::Unknown;
}

QString extractPrintablePayload(const std::vector<BYTE> &data) {
    QString payload;
    payload.reserve(static_cast<qsizetype>(data.size()));
    for (const BYTE byte : data) {
        if (byte == 0) continue;
        if (byte >= 32 && byte <= 126) {
            payload.append(QChar::fromLatin1(static_cast<char>(byte)));
        } else {
            payload.append(u' ');
        }
    }
    return payload;
}

QuietHoursProfile parseQuietHoursProfileBlob(const std::vector<BYTE> &data) {
    if (data.empty()) return QuietHoursProfile::Unknown;
    return parseQuietHoursProfilePayload(extractPrintablePayload(data));
}

QuietHoursProfile detectQuietHoursProfileFromRegistry(const wchar_t *subKey) {
    static constexpr wchar_t kDataValue[] = L"Data";

    std::vector<BYTE> rawData;
    if (!readRegistryBinaryValue(HKEY_CURRENT_USER, subKey, kDataValue, rawData)) {
        return QuietHoursProfile::Unknown;
    }
    return parseQuietHoursProfileBlob(rawData);
}

QuietHoursProfile detectQuietHoursProfileFromCloudStore() {
    static constexpr wchar_t kCurrentPath[] =
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CloudStore\\Store\\DefaultAccount\\Current\\default$windows.data.donotdisturb.quiethourssettings\\windows.data.donotdisturb.quiethourssettings";
    static constexpr wchar_t kLegacyCachePath[] =
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CloudStore\\Store\\Cache\\DefaultAccount\\$$windows.data.notifications.quiethourssettings\\Current";

    const QuietHoursProfile currentProfile = detectQuietHoursProfileFromRegistry(kCurrentPath);
    if (currentProfile != QuietHoursProfile::Unknown) return currentProfile;

    return detectQuietHoursProfileFromRegistry(kLegacyCachePath);
}

bool isFocusSessionActive() {
    if (!ensureToastRuntimeInitialized()) return false;

    ComPtr<IFocusSessionManagerStatics> focusManagerStatics;
    HRESULT hr = RoGetActivationFactory(
        HStringReference(RuntimeClass_Windows_UI_Shell_FocusSessionManager).Get(),
        IID_PPV_ARGS(&focusManagerStatics)
    );
    if (FAILED(hr) || !focusManagerStatics) return false;

    boolean isSupported = false;
    hr = focusManagerStatics->get_IsSupported(&isSupported);
    if (FAILED(hr) || !isSupported) return false;

    ComPtr<IFocusSessionManager> focusManager;
    hr = focusManagerStatics->GetDefault(&focusManager);
    if (FAILED(hr) || !focusManager) return false;

    boolean isActive = false;
    hr = focusManager->get_IsFocusActive(&isActive);
    if (FAILED(hr)) return false;
    return isActive;
}

NotificationSetting queryToastNotificationSetting() {
    if (!ensureToastRuntimeInitialized()) return NotificationSetting_Enabled;

    ComPtr<IToastNotificationManagerStatics> toastManager;
    HRESULT hr = RoGetActivationFactory(
        HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),
        IID_PPV_ARGS(&toastManager)
    );
    if (FAILED(hr) || !toastManager) return NotificationSetting_Enabled;

    const std::wstring appId = appUserModelIdWide();
    if (appId.empty()) return NotificationSetting_Enabled;

    ComPtr<IToastNotifier> notifier;
    hr = toastManager->CreateToastNotifierWithId(HStringReference(appId.c_str()).Get(), &notifier);
    if (FAILED(hr) || !notifier) return NotificationSetting_Enabled;

    NotificationSetting setting = NotificationSetting_Enabled;
    hr = notifier->get_Setting(&setting);
    if (FAILED(hr)) return NotificationSetting_Enabled;
    return setting;
}

QString quietHoursProfileName(const QuietHoursProfile profile) {
    switch (profile) {
        case QuietHoursProfile::Unrestricted: return "unrestricted";
        case QuietHoursProfile::PriorityOnly: return "priority_only";
        case QuietHoursProfile::AlarmsOnly: return "alarms_only";
        case QuietHoursProfile::Other: return "other";
        case QuietHoursProfile::Unknown:
        default: return "unknown";
    }
}

bool isQuietHoursServiceActive() {
    DWORD state = 0;
    if (!readRegistryDwordValue(
        HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Notifications\\QuietHours",
        L"QuietHoursServiceState",
        state
    )) {
        return false;
    }
    return state != 0;
}

bool areGlobalToastsDisabled() {
    DWORD enabled = 1;
    if (!readRegistryDwordValue(
        HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PushNotifications",
        L"ToastEnabled",
        enabled
    )) {
        return false;
    }
    return enabled == 0;
}

bool appendToastTextNode(const ComPtr<IXmlDocument> &xml, const int index, const QString &text) {
    ComPtr<IXmlNodeList> textNodes;
    HRESULT hr = xml->GetElementsByTagName(HStringReference(L"text").Get(), &textNodes);
    if (FAILED(hr) || !textNodes) return false;

    ComPtr<IXmlNode> textNode;
    hr = textNodes->Item(index, &textNode);
    if (FAILED(hr) || !textNode) return false;

    ComPtr<IXmlText> textValue;
    const std::wstring wideText = text.toStdWString();
    hr = xml->CreateTextNode(HStringReference(wideText.c_str()).Get(), &textValue);
    if (FAILED(hr) || !textValue) return false;

    ComPtr<IXmlNode> textValueNode;
    hr = textValue.As(&textValueNode);
    if (FAILED(hr) || !textValueNode) return false;

    ComPtr<IXmlNode> appended;
    hr = textNode->AppendChild(textValueNode.Get(), &appended);
    return SUCCEEDED(hr);
}

bool setToastLaunchAttributes(const ComPtr<IXmlDocument> &xml, const QString &launchArgument, const bool suppressPopup) {
    ComPtr<IXmlElement> root;
    HRESULT hr = xml->get_DocumentElement(&root);
    if (FAILED(hr) || !root) return false;

    const std::wstring launch = launchArgument.toStdWString();
    hr = root->SetAttribute(HStringReference(L"launch").Get(), HStringReference(launch.c_str()).Get());
    if (FAILED(hr)) return false;

    hr = root->SetAttribute(HStringReference(L"activationType").Get(), HStringReference(L"protocol").Get());
    if (FAILED(hr)) return false;

    if (suppressPopup) {
        hr = root->SetAttribute(HStringReference(L"suppressPopup").Get(), HStringReference(L"true").Get());
        if (FAILED(hr)) return false;
    }

    return true;
}

bool showWindowsToastNotification(
    const QString &title,
    const QString &body,
    const std::function<void()> &onActivated,
    const bool suppressPopup
) {
    if (!ensureToastRuntimeInitialized()) return false;
    resetActiveToastHandlers();

    const std::wstring appId = appUserModelIdWide();
    if (appId.empty()) {
        return false;
    }

    ComPtr<IToastNotificationManagerStatics> toastManager;
    HRESULT hr = RoGetActivationFactory(
        HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),
        IID_PPV_ARGS(&toastManager)
    );
    if (FAILED(hr) || !toastManager) {
        LOG_WARNING() << "Toast manager activation failed. HRESULT=0x"
                      << QString::number(static_cast<qulonglong>(hr), 16);
        return false;
    }

    ComPtr<IXmlDocument> toastXml;
    hr = toastManager->GetTemplateContent(ToastTemplateType_ToastText02, &toastXml);
    if (FAILED(hr) || !toastXml) {
        LOG_WARNING() << "Toast XML template failed. HRESULT=0x"
                      << QString::number(static_cast<qulonglong>(hr), 16);
        return false;
    }

    if (!appendToastTextNode(toastXml, 0, title) || !appendToastTextNode(toastXml, 1, body)) {
        LOG_WARNING() << "Toast XML text setup failed";
        return false;
    }

    if (!setToastLaunchAttributes(toastXml, toastProtocolUri(), suppressPopup)) {
        LOG_WARNING() << "Failed to set toast launch attribute";
    }

    ComPtr<IToastNotificationFactory> toastFactory;
    hr = RoGetActivationFactory(
        HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(),
        IID_PPV_ARGS(&toastFactory)
    );
    if (FAILED(hr) || !toastFactory) {
        LOG_WARNING() << "Toast factory activation failed. HRESULT=0x"
                      << QString::number(static_cast<qulonglong>(hr), 16);
        return false;
    }

    ComPtr<IToastNotification> toast;
    hr = toastFactory->CreateToastNotification(toastXml.Get(), &toast);
    if (FAILED(hr) || !toast) {
        LOG_WARNING() << "Toast creation failed. HRESULT=0x"
                      << QString::number(static_cast<qulonglong>(hr), 16);
        return false;
    }

    if (onActivated) {
        g_activeToastActivatedHandler =
            Callback<ToastActivatedHandler>(
                [onActivated](ABI::Windows::UI::Notifications::IToastNotification *, IInspectable *) -> HRESULT {
                    QMetaObject::invokeMethod(
                        qApp,
                        [onActivated]() { onActivated(); },
                        Qt::QueuedConnection
                    );
                    return S_OK;
                });

        if (!g_activeToastActivatedHandler.Get()) {
            LOG_WARNING() << "Toast activation callback creation failed";
            return false;
        }

        hr = toast->add_Activated(
            g_activeToastActivatedHandler.Get(),
            &g_activeToastActivatedToken
        );
        if (FAILED(hr)) {
            LOG_WARNING() << "Toast activation callback registration failed. HRESULT=0x"
                          << QString::number(static_cast<qulonglong>(hr), 16);
            g_activeToastActivatedHandler.Reset();
            g_activeToastActivatedToken = {};
        }
    }

    ComPtr<IToastNotifier> notifier;
    hr = toastManager->CreateToastNotifierWithId(HStringReference(appId.c_str()).Get(), &notifier);
    if (FAILED(hr) || !notifier) {
        LOG_WARNING() << "Toast notifier creation failed. HRESULT=0x"
                      << QString::number(static_cast<qulonglong>(hr), 16);
        return false;
    }

    hr = notifier->Show(toast.Get());
    if (FAILED(hr)) {
        LOG_WARNING() << "Toast show failed. HRESULT=0x"
                      << QString::number(static_cast<qulonglong>(hr), 16);
        resetActiveToastHandlers();
        return false;
    }

    g_activeToast = toast;
    return true;
}

void clearWindowsToastHistoryForApp() {
    if (!ensureToastRuntimeInitialized()) return;

    ComPtr<IToastNotificationManagerStatics> toastManager;
    HRESULT hr = RoGetActivationFactory(
        HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),
        IID_PPV_ARGS(&toastManager)
    );
    if (FAILED(hr) || !toastManager) return;

    ComPtr<IToastNotificationManagerStatics2> toastManager2;
    hr = toastManager.As(&toastManager2);
    if (FAILED(hr) || !toastManager2) return;

    ComPtr<IToastNotificationHistory> history;
    hr = toastManager2->get_History(&history);
    if (FAILED(hr) || !history) return;

    const std::wstring appId = appUserModelIdWide();
    if (!appId.empty()) {
        history->ClearWithId(HStringReference(appId.c_str()).Get());
    }
    history->Clear();
    resetActiveToastHandlers();
}
#endif
}

TrayManager::TrayManager(QWidget *parent)
    : QWidget(parent) {
    ui.setupUi(this);

    setWindowFlags(Qt::Popup | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::StrongFocus);

#ifdef Q_OS_WIN
    setupToastActivationBridge();
#endif

    updateManager = new UpdateManager(this);
    settingsWindow = new SettingsWindow(nullptr);
    settingsWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    settingsWindow->setUpdateManager(updateManager);

    connect(updateManager, &UpdateManager::updateAvailable, this, &TrayManager::handleUpdateAvailable);
    connect(updateManager, &UpdateManager::noUpdateAvailable, this, &TrayManager::handleNoUpdateAvailable);

    updateManager->start();


    setupAnimations();

    // Звуки
    audioEffectOn = new QSoundEffect(this);
    audioEffectOn->setSource(QUrl("qrc:/sounds/sounds/on.wav"));
    audioEffectOn->setVolume(0.5f);
    audioEffectOff = new QSoundEffect(this);
    audioEffectOff->setSource(QUrl("qrc:/sounds/sounds/off.wav"));
    audioEffectOff->setVolume(0.5f);
    audioEffectUpdateAlert = new QSoundEffect(this);
    audioEffectUpdateAlert->setSource(QUrl("qrc:/sounds/sounds/notification_alert.wav"));
    audioEffectUpdateAlert->setVolume(0.5f);

    soundManager::instance().registerEffect(audioEffectOn);
    soundManager::instance().registerEffect(audioEffectOff);
    soundManager::instance().registerEffect(audioEffectUpdateAlert);

    // Таймер для разделения Single и Double кликов
    clickTimer = new QTimer(this);
    clickTimer->setSingleShot(true);
    connect(clickTimer, &QTimer::timeout, this, [this]() {
        enabled = !enabled;
        soundManager::instance().playEffect(enabled ? audioEffectOn : audioEffectOff);
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

TrayManager::~TrayManager() {
#ifdef Q_OS_WIN
    if (m_toastActivationEvent) {
        CloseHandle(m_toastActivationEvent);
        m_toastActivationEvent = nullptr;
    }
#endif
    delete settingsWindow;
}

void TrayManager::setupAnimations() {
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

void TrayManager::showAtCursor() {
    m_isClosing = false;
    fadeOut->stop();

    updateInfo();

    // Подготовка размеров попапа до его появления
    layout()->activate();
    adjustSize();

    const QPoint cursorPos = QCursor::pos();
    const QScreen *screen = QGuiApplication::screenAt(cursorPos);
    if (!screen) screen = QGuiApplication::primaryScreen();
    if (!screen) {
        const QList<QScreen *> screens = QGuiApplication::screens();
        if (!screens.isEmpty()) {
            screen = screens.first();
        }
    }
    if (!screen) return;

    const QRect screenRect = screen->availableGeometry();
    QPoint finalPos = cursorPos;
    constexpr int padding = 3;
    constexpr int slideDist = 12;

    // Вычисление позиции
    bool isLeft = true;
    if (finalPos.x() + width() > screenRect.right()) {
        finalPos.setX(finalPos.x() - width() - padding);
        isLeft = false;
    } else {
        finalPos.setX(finalPos.x() + padding);
    }

    bool isTop = true;
    if (finalPos.y() + height() > screenRect.bottom()) {
        finalPos.setY(finalPos.y() - height() - padding);
        isTop = false;
    } else {
        finalPos.setY(finalPos.y() + padding);
    }

    // Анимация вылета
    QPoint startPos = finalPos;
    if (cursorPos.y() > screenRect.bottom() - 100 || cursorPos.y() < screenRect.top() + 100) {
        startPos.setY(isTop ? finalPos.y() - slideDist : finalPos.y() + slideDist);
    } else {
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

void TrayManager::hideAnimated() const {
    if (!isVisible() || m_isClosing) return;
    m_isClosing = true;
    showGroup->stop();
    fadeOut->setStartValue(this->windowOpacity());
    fadeOut->start();
}

void TrayManager::openSettings() {
    hideAnimated();
    if (!settingsWindow) return;
    clearPendingUpdate();
#ifdef Q_OS_WIN
    clearWindowsToastHistoryForApp();
#endif

    QTimer::singleShot(0, settingsWindow, [this]() {
        settingsWindow->openCentered();
        settingsWindow->raise();
        settingsWindow->activateWindow();
    });
}

void TrayManager::setupTrayIcon() {
    updateTrayIcon();
    trayIcon.setVisible(true);

    connect(&trayIcon, &QSystemTrayIcon::activated, this, [this](const QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            clickTimer->start(250);
        } else if (reason == QSystemTrayIcon::DoubleClick) {
            clickTimer->stop();
            openSettings();
        } else if (reason == QSystemTrayIcon::Context) {
            if (isVisible() && !m_isClosing) hideAnimated();
            else showAtCursor();
        }
    });

    connect(&trayIcon, &QSystemTrayIcon::messageClicked, this, [this]() {
        if (!m_hasAvailableUpdate || m_availableUpdateVersion.isEmpty()) return;
        LOG_INFO() << "System notification clicked: opening deferred update popup";
        showGlobalNotification(GlobalNotification::UpdateAvailable, m_availableUpdateVersion, m_availableUpdateUrl);
        clearPendingUpdate();
#ifdef Q_OS_WIN
        clearWindowsToastHistoryForApp();
#endif
    });
}

void TrayManager::updateTrayIcon() {
    trayIcon.setIcon(buildTrayIcon());
    updateTrayToolTip();
}

void TrayManager::updateInfo() const {
    ui.status_value->setText(enabled ? Lang::tr("TRAY_TOGGLE_ENABLED") : Lang::tr("TRAY_TOGGLE_DISABLED"));
    ui.hotkey_value->setText(AppSettings::hotkeyName);
    ui.delay_value->setText(QString::number(AppSettings::switchDelayMs));
    ui.toggle_btn->setText(enabled ? Lang::tr("TRAY_TOGGLE_PAUSE") : Lang::tr("TRAY_TOGGLE_RESUME"));

    ui.settings_btn->setText(Lang::tr("TRAY_SETTINGS"));
    ui.exit_btn->setText(Lang::tr("TRAY_EXIT"));
    ui.status_key->setText(Lang::tr("TRAY_LABEL_STATUS"));
    ui.hotkey_key->setText(Lang::tr("TRAY_LABEL_HOTKEY"));
    ui.delay_key->setText(Lang::tr("TRAY_LABEL_DELAY"));

    ui.toggle_btn->setIcon(IconHelper::loadIcon(enabled
                                                    ? ":/icons/icons/FlashSparkleRegular.svg"
                                                    : ":/icons/icons/FlashSparkleFilled.svg"));
    ui.settings_btn->setIcon(IconHelper::loadIcon(":/icons/icons/FlashSettingsRegular.svg"));
    ui.exit_btn->setIcon(IconHelper::loadIcon(":/icons/icons/FlashOffRegular.svg"));
}

bool TrayManager::shouldDeferPopupNotification() const {
#ifdef Q_OS_WIN
    QUERY_USER_NOTIFICATION_STATE state = QUNS_ACCEPTS_NOTIFICATIONS;
    const HRESULT hr = SHQueryUserNotificationState(&state);
    const bool hasSystemNotificationState = SUCCEEDED(hr);
    if (!hasSystemNotificationState) {
        LOG_WARNING() << "SHQueryUserNotificationState failed, continuing with fallback checks. HRESULT=0x"
                      << QString::number(static_cast<qulonglong>(hr), 16);
    }

    const bool deferBySystemState =
        (hasSystemNotificationState && state == QUNS_NOT_PRESENT)
        || (hasSystemNotificationState && state == QUNS_BUSY)
        || (hasSystemNotificationState && state == QUNS_RUNNING_D3D_FULL_SCREEN)
        || (hasSystemNotificationState && state == QUNS_PRESENTATION_MODE)
        || (hasSystemNotificationState && state == QUNS_QUIET_TIME);
    const bool deferByFullscreenWindow = isForegroundFullscreenWindow();
    const bool deferByQuietHoursService = isQuietHoursServiceActive();
    const bool deferByGlobalToastSetting = areGlobalToastsDisabled();
    const bool deferByFocusSession = isFocusSessionActive();
    const NotificationSetting toastSetting = queryToastNotificationSetting();
    const bool deferByToastSetting = toastSetting != NotificationSetting_Enabled;
    const QuietHoursProfile quietHoursProfile = detectQuietHoursProfileFromCloudStore();
    const bool deferByQuietHoursProfile = quietHoursProfile != QuietHoursProfile::Unknown
                                          && quietHoursProfile != QuietHoursProfile::Unrestricted;
    const bool shouldDefer =
        deferBySystemState
        || deferByFullscreenWindow
        || deferByQuietHoursService
        || deferByGlobalToastSetting
        || deferByFocusSession
        || deferByToastSetting
        || deferByQuietHoursProfile;

    LOG_DEBUG() << "Notification state:"
                << (hasSystemNotificationState ? notificationStateName(state) : "unavailable")
                << "; defer by state=" << deferBySystemState
                << "; defer by fullscreen=" << deferByFullscreenWindow
                << "; defer by quiet-hours-service=" << deferByQuietHoursService
                << "; defer by global-toast-setting=" << deferByGlobalToastSetting
                << "; defer by focus-session=" << deferByFocusSession
                << "; toast-setting=" << toastNotificationSettingName(toastSetting)
                << "; defer by toast-setting=" << deferByToastSetting
                << "; quiet-hours-profile=" << quietHoursProfileName(quietHoursProfile)
                << "; defer by quiet-hours-profile=" << deferByQuietHoursProfile
                << "; defer popup=" << shouldDefer;
    return shouldDefer;
#else
    return false;
#endif
}

void TrayManager::handleUpdateAvailable(const QString &version, const QString &url, const bool isManualCheck) {
    LOG_INFO() << "Update available detected. version=" << version
               << "; manual=" << (isManualCheck ? "true" : "false");
    setAvailableUpdate(version, url);

    if (isManualCheck) {
        const bool suppressActive = shouldDeferPopupNotification();
        LOG_INFO() << "Manual check: showing custom global notification";
        clearPendingUpdate();
        showGlobalNotification(GlobalNotification::UpdateAvailable, version, url);
        if (!suppressActive) playUpdateAvailableAlert();
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
    showGlobalNotification(GlobalNotification::UpdateAvailable, version, url);
    playUpdateAvailableAlert();
}

void TrayManager::handleNoUpdateAvailable(const QString &version, const bool isManualCheck) {
    clearAvailableUpdate();
    clearPendingUpdate();

    if (!isManualCheck) return;

    if (shouldDeferPopupNotification()) {
        if (QSystemTrayIcon::supportsMessages()) {
            trayIcon.showMessage(
                Lang::tr("NOTIFICATION_UPD_NOT_AVAILABLE_TITLE"),
                Lang::tr("NOTIFICATION_UPD_NOT_AVAILABLE_DESC").arg(version),
                QSystemTrayIcon::Information,
                6000
            );
        }
        return;
    }

    showGlobalNotification(GlobalNotification::UpToDate, version, {});
}

void TrayManager::showGlobalNotification(const GlobalNotification::Mode mode, const QString &version, const QString &url) {
    if (m_currentGlobalNotif) {
        m_currentGlobalNotif->close();
        m_currentGlobalNotif = nullptr;
    }

    m_currentGlobalNotif = new GlobalNotification(mode, version, url);
    m_currentGlobalNotif->show();
}

void TrayManager::setAvailableUpdate(const QString &version, const QString &url) {
    const bool changed = !m_hasAvailableUpdate || m_availableUpdateVersion != version || m_availableUpdateUrl != url;
    m_availableUpdateVersion = version;
    m_availableUpdateUrl = url;
    m_hasAvailableUpdate = true;
    if (settingsWindow) settingsWindow->setPendingUpdateHint(true, version);
    if (changed) {
        LOG_INFO() << "Available update stored. version=" << version;
        updateTrayIcon();
    }
}

void TrayManager::clearAvailableUpdate() {
    const bool hadAvailable = m_hasAvailableUpdate || !m_availableUpdateVersion.isEmpty() || !m_availableUpdateUrl.isEmpty();
    m_hasAvailableUpdate = false;
    m_availableUpdateVersion.clear();
    m_availableUpdateUrl.clear();
    if (settingsWindow) settingsWindow->setPendingUpdateHint(false);
    if (hadAvailable) {
        LOG_INFO() << "Available update state cleared";
        updateTrayIcon();
    }
}

void TrayManager::setPendingUpdate(const QString &version, const QString &url) {
    m_pendingUpdateVersion = version;
    m_pendingUpdateUrl = url;
    m_hasPendingUpdate = true;
    LOG_INFO() << "Pending update stored in tray. version=" << version;
    updateTrayIcon();
}

void TrayManager::clearPendingUpdate() {
    const bool hadPending = m_hasPendingUpdate || !m_pendingUpdateVersion.isEmpty() || !m_pendingUpdateUrl.isEmpty();
    m_hasPendingUpdate = false;
    m_pendingUpdateVersion.clear();
    m_pendingUpdateUrl.clear();
    if (hadPending) LOG_INFO() << "Pending update cleared from tray";
    if (hadPending) updateTrayIcon();
}

QIcon TrayManager::buildTrayIcon() const {
    const QIcon base = IconHelper::loadIcon(
        enabled
            ? ":/icons/icons/FlashSparkleFilled2.png"
            : ":/icons/icons/FlashSparkleRegular2.png"
    );

    if (!m_hasPendingUpdate) return base;

    QIcon badgedIcon;
    constexpr std::array<int, 7> sizes = {16, 20, 24, 32, 40, 48, 64};
    for (const int size: sizes) {
        QPixmap pixmap = base.pixmap(size, size);
        if (pixmap.isNull()) continue;

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const int gap = qMax(1, size / 16);
        const int diameter = qMax(6, size / 3);
        const int margin = qMax(0, size / 16);
        const QRect outerRect(size - diameter - margin - gap * 2, margin - gap, diameter + gap * 2, diameter + gap * 2);
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

    return badgedIcon.isNull() ? base : badgedIcon;
}

void TrayManager::updateTrayToolTip() {
    QString tooltip = AppSettings::APP_NAME;
    if (m_hasAvailableUpdate) {
        tooltip += QString("\n%1").arg(Lang::tr("NOTIFICATION_UPD_AVAILABLE_TITLE"));
        if (!m_availableUpdateVersion.isEmpty())
            tooltip += QString("\n%1").arg(m_availableUpdateVersion);
    }
    trayIcon.setToolTip(tooltip);
}

void TrayManager::showSystemUpdateMessage(const QString &version, const bool isManualCheck) {
    LOG_INFO() << "Requesting system notification. supportsMessages="
               << (QSystemTrayIcon::supportsMessages() ? "true" : "false");

    const QString title = Lang::tr("NOTIFICATION_UPD_AVAILABLE_TITLE");
    QString message = Lang::tr("NOTIFICATION_UPD_SYSTEM_BODY").arg(version);
    message += QString("\n%1").arg(Lang::tr("NOTIFICATION_UPD_SYSTEM_CLICK_HINT"));

#ifdef Q_OS_WIN
    clearWindowsToastHistoryForApp();
    if (showWindowsToastNotification(title, message, {}, true)) {
        LOG_INFO() << "Native Windows toast notification delivered";
        return;
    }
#endif

    if (QSystemTrayIcon::supportsMessages()) {
        trayIcon.showMessage(
            title,
            message,
            QSystemTrayIcon::Information,
            isManualCheck ? 15000 : 10000
        );
        return;
    }

    LOG_WARNING() << "Failed to show system notification via tray and native toast";
}

void TrayManager::playUpdateAvailableAlert() const {
    if (!audioEffectUpdateAlert) return;
    soundManager::instance().playEffect(audioEffectUpdateAlert);
}

void TrayManager::handleToastActivationRequest() {
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (m_lastToastActivationMs > 0 && (nowMs - m_lastToastActivationMs) < 1200) {
        LOG_INFO() << "Toast activation ignored: duplicate signal";
        return;
    }
    m_lastToastActivationMs = nowMs;

    LOG_INFO() << "Handling toast activation request";
    if (m_hasAvailableUpdate && !m_availableUpdateVersion.isEmpty()) {
        showGlobalNotification(GlobalNotification::UpdateAvailable, m_availableUpdateVersion, m_availableUpdateUrl);
        clearPendingUpdate();
#ifdef Q_OS_WIN
        clearWindowsToastHistoryForApp();
#endif
        return;
    }

    LOG_INFO() << "Toast activation requested but no cached update; forcing update check";
    if (updateManager) updateManager->checkForUpdatesForce();
}

#ifdef Q_OS_WIN
std::wstring TrayManager::toastActivationEventNameWide() {
    return ::toastActivationEventNameWide();
}

void TrayManager::setupToastActivationBridge() {
    const std::wstring eventName = toastActivationEventNameWide();
    m_toastActivationEvent = CreateEventW(nullptr, TRUE, FALSE, eventName.c_str());
    if (!m_toastActivationEvent) {
        LOG_WARNING() << "Failed to create/open toast activation event. GetLastError="
                      << static_cast<qulonglong>(GetLastError());
        return;
    }

    m_toastActivationPollTimer = new QTimer(this);
    m_toastActivationPollTimer->setInterval(250);
    connect(m_toastActivationPollTimer, &QTimer::timeout, this, &TrayManager::pollToastActivationSignal);
    m_toastActivationPollTimer->start();
}

void TrayManager::pollToastActivationSignal() {
    if (!m_toastActivationEvent) return;

    if (const DWORD waitResult = WaitForSingleObject(m_toastActivationEvent, 0); waitResult != WAIT_OBJECT_0) {
        return;
    }

    ResetEvent(m_toastActivationEvent);
    LOG_INFO() << "Toast activation signal received from external launch";
    handleToastActivationRequest();
}
#endif

void TrayManager::animateToggleButton() {
    auto *effect = new QGraphicsOpacityEffect(ui.toggle_btn);
    ui.toggle_btn->setGraphicsEffect(effect);

    auto *fadeOutBtn = new QPropertyAnimation(effect, "opacity");
    fadeOutBtn->setDuration(160);
    fadeOutBtn->setStartValue(1.0);
    fadeOutBtn->setEndValue(0.0);

    auto *fadeInBtn = new QPropertyAnimation(effect, "opacity");
    fadeInBtn->setDuration(200);
    fadeInBtn->setStartValue(0.0);
    fadeInBtn->setEndValue(1.0);

    connect(fadeOutBtn, &QPropertyAnimation::finished, this, [this, fadeInBtn]() {
        updateInfo();
        fadeInBtn->start(QAbstractAnimation::DeleteWhenStopped);
    });

    connect(fadeInBtn, &QPropertyAnimation::finished, this, [this]() {
        ui.toggle_btn->setGraphicsEffect(nullptr);
    });

    fadeOutBtn->start(QAbstractAnimation::DeleteWhenStopped);
}

void TrayManager::setupUiBehavior() {
    connect(ui.settings_btn, &QPushButton::clicked, this, &TrayManager::openSettings);

    if (settingsWindow) {
        connect(settingsWindow, &SettingsWindow::settingsSaved, this, [this]() {
            updateInfo();
            updateTrayIcon();
            if (m_currentGlobalNotif) m_currentGlobalNotif->refreshTranslations();
        });
    }

    connect(ui.exit_btn, &QToolButton::clicked, this, &TrayManager::exitRequested);

    connect(ui.toggle_btn, &QToolButton::clicked, this, [this]() {
        enabled = !enabled;
        soundManager::instance().playEffect(enabled ? audioEffectOn : audioEffectOff);
        emit keyboardToggled(enabled);
        animateToggleButton();
        updateTrayIcon();
    });

    TrayHoverHelper::initializeHover(this);
}

bool TrayManager::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui.info_frame) {
        if (event->type() == QEvent::Enter) TrayHoverHelper::animateHover(ui.info_frame, true);
        else if (event->type() == QEvent::Leave) TrayHoverHelper::animateHover(ui.info_frame, false);
    }
    return QWidget::eventFilter(obj, event);
}

void TrayManager::focusOutEvent(QFocusEvent *event) {
    hideAnimated();
    QWidget::focusOutEvent(event);
}

void TrayManager::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    AcrylicHelper::updateRegion(this);
}
