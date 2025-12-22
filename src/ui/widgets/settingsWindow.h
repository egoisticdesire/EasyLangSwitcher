#pragma once
#include "ui_EasyLangSwitcher_settings.h"
#include "hoverWarning.h"
#include "animatedSelector.h"
#include "customToolTip.h"
#include "updateManager.h"
#include "updFrequencyPopup.h"
#include "windowDragger.h"
#include <QVector>
#include <QTimer>

/*
SettingsWindow
— управление выбором клавишей-триггером
— автосохранение (1s) при реальных изменениях
— восстановление предыдущего preset при очистке input-поля
— единый маппинг preset кнопок
*/
class KeySequenceHelper;
class SaveNotification;

class SettingsWindow final : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = nullptr);

    ~SettingsWindow() override;

    void openCentered();

    void setUpdateManager(UpdateManager *manager);

signals:
    void settingsChanged();

    void settingsSaved();

protected:
    void showEvent(QShowEvent *event) override;

    bool event(QEvent *ev) override;

    bool eventFilter(QObject *watched, QEvent *event) override;

    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

    void hideEvent(QHideEvent *event) override;

private:
    Ui::settings_main_widget ui{};

    KeySequenceHelper *m_keyHelper = nullptr;

    SaveNotification *saveNotif = nullptr;

    QVector<AnimatedSelector *> selectors;

    WindowDragger *dragger = nullptr;

    KeyHoverWarning *keyHoverWarning = nullptr;

    UpdFrequencyPopup *updPopup = nullptr;

    void initUpdateFrequency();

    UpdateManager *updateManager = nullptr;

    CustomToolTip *updateBtnToolTip = nullptr;

    QTimer autosaveTimer;

    QGraphicsDropShadowEffect *m_shadow;

    static constexpr int autosaveIntervalMs = 1000;

    bool hasPendingChanges = false;

    int previousPresetVk = 0;

    QString previousPresetName;

    QHash<QString, int> presetMap;

    void addSelectorForFrame(QFrame *frame, const QString &extraStyle = QString());

    void buildPresetMap();

    int vkFromPresetObjectName(const QString &obj) const;

    static QString nameFromVk(int vk);

    void applyHotkeyIfChanged(int newVk, const QString &newName);

    void markChanged();

    void restorePreviousPresetIfNeeded();

    void restoreDefaultsForCurrentPage();

    void restoreDefaults_General();

    void refreshTranslations() const;
};
