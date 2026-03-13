#pragma once

#include "../widgets/notifications/globalNotification.h"
#include "../widgets/settingsWindow/settingsWindow.h"
#include "../widgets/updateManager.h"
#include "ui_EasyLangSwitcher_tray.h"

#include <QParallelAnimationGroup>
#include <QPointer>
#include <QPropertyAnimation>
#include <QSoundEffect>
#include <QSystemTrayIcon>
#include <QWidget>
#ifdef Q_OS_WIN
#include <Windows.h>
#endif

class TrayManager final : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TrayManager)
    Q_PROPERTY(QPoint pos READ pos WRITE move)

public:
    explicit TrayManager(QWidget* parent = nullptr);

    ~TrayManager() override;

    void showAtCursor();

    void handleToastActivationRequest();

signals:
    void exitRequested();

    void keyboardToggled(bool enabled);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

    void focusOutEvent(QFocusEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;

private:
    void setupAnimations();

    void setupTrayIcon();

    void setupUiBehavior();

    void updateTrayIcon();

    void updateInfo() const;

    void openSettings();

    void ensureSettingsWindow();

    void hideAnimated() const;

    void animateToggleButton();

    [[nodiscard]] static bool shouldDeferPopupNotification();

    void handleUpdateAvailable(const QString& version, const QString& url, bool isManualCheck);

    void handleNoUpdateAvailable(const QString& version, bool isManualCheck);

    void showGlobalNotification(GlobalNotification::Mode mode, const QString& version, const QString& url = {});

    void setAvailableUpdate(const QString& version, const QString& url);

    void clearAvailableUpdate();

    void setPendingUpdate(const QString& version, const QString& url);

    void clearPendingUpdate();

    [[nodiscard]] QIcon buildTrayIcon() const;

    void updateTrayToolTip();

    void showSystemUpdateMessage(const QString& version, bool isManualCheck);

    void playUpdateAvailableAlert() const;

#ifdef Q_OS_WIN
    static std::wstring toastActivationEventNameWide();

    void setupToastActivationBridge();

    void pollToastActivationSignal();
#endif

    Ui::tray_main_widget ui{};
    QSystemTrayIcon trayIcon;
    SettingsWindow* settingsWindow = nullptr;
    UpdateManager* updateManager = nullptr;

    bool enabled = true;
    mutable bool m_isClosing = false;

    QParallelAnimationGroup* showGroup = nullptr;
    QPropertyAnimation* fadeIn = nullptr;
    QPropertyAnimation* posAnim = nullptr;
    QPropertyAnimation* fadeOut = nullptr;

    QSoundEffect* audioEffectOn = nullptr;
    QSoundEffect* audioEffectOff = nullptr;
    QSoundEffect* audioEffectUpdateAlert = nullptr;
    QTimer* clickTimer = nullptr;
    QPointer<GlobalNotification> m_currentGlobalNotif;
    QString m_availableUpdateVersion;
    QString m_availableUpdateUrl;
    bool m_hasAvailableUpdate = false;
    QString m_pendingUpdateVersion;
    QString m_pendingUpdateUrl;
    bool m_hasPendingUpdate = false;
    qint64 m_lastToastActivationMs = 0;
#ifdef Q_OS_WIN
    HANDLE m_toastActivationEvent = nullptr;
    QTimer* m_toastActivationPollTimer = nullptr;
#endif
};
