#pragma once
#include "ui_EasyLangSwitcher_tray.h"
#include "../widgets/settingsWindow.h"
#include "../widgets/updateManager.h"
#include <QWidget>
#include <QSystemTrayIcon>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QSoundEffect>
#include <QTimer>

class TrayManager final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QPoint pos READ pos WRITE move)

public:
    explicit TrayManager(QWidget *parent = nullptr);

    ~TrayManager() override;

    void showAtCursor();

signals:
    void exitRequested();

    void keyboardToggled(bool enabled);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

    void focusOutEvent(QFocusEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;

private:
    void setupAnimations();

    void setupTrayIcon();

    void setupUiBehavior();

    void updateTrayIcon();

    void updateInfo() const;

    void openSettings() const;

    void hideAnimated() const;

    void animateToggleButton();

    Ui::tray_main_widget ui{};
    QSystemTrayIcon trayIcon;
    SettingsWindow *settingsWindow = nullptr;
    UpdateManager *updateManager = nullptr;

    bool enabled = true;
    mutable bool m_isClosing = false;

    QParallelAnimationGroup *showGroup = nullptr;
    QPropertyAnimation *fadeIn = nullptr;
    QPropertyAnimation *posAnim = nullptr;
    QPropertyAnimation *fadeOut = nullptr;

    QSoundEffect *audioEffectOn = nullptr;
    QSoundEffect *audioEffectOff = nullptr;
    QTimer *clickTimer = nullptr;
};
