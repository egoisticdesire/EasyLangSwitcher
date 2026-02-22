#pragma once
#include "ui_EasyLangSwitcher_settings.h"
#include "../hoverWarning.h"
#include "../animatedSelector.h"
#include "../customToolTip.h"
#include "../updateManager.h"
#include "../updFrequencyPopup.h"
#include "../windowDragger.h"
#include <QVector>
#include <QTimer>
#include <QGraphicsDropShadowEffect>

class KeySequenceHelper;

class SettingsWindow final : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = nullptr);

    ~SettingsWindow() override;

    void openCentered();

    void setUpdateManager(UpdateManager *manager);

    void setPendingUpdateHint(bool hasPending, QString version = {});

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
    // UI и компоненты
    Ui::settings_main_widget ui{};
    KeySequenceHelper *m_keyHelper = nullptr;
    QVector<AnimatedSelector *> selectors;
    WindowDragger *dragger = nullptr;
    KeyHoverWarning *keyHoverWarning = nullptr;
    UpdFrequencyPopup *updPopup = nullptr;
    CustomToolTip *updateBtnToolTip = nullptr;
    UpdateManager *updateManager = nullptr;
    QGraphicsDropShadowEffect *m_shadow = nullptr;
    QVariantAnimation *m_syncRotationAnim = nullptr;

    void updateSyncIconRotation(int angle) const;

    void stopSyncAnimation() const;

    void updateManualCheckButtonIcon() const;

    void refreshUpdateButtonTooltipLive() const;

    [[nodiscard]] static QString lastUpdateCheckDisplay();

    // Логика и состояние
    QTimer autosaveTimer;
    static constexpr int autosaveIntervalMs = 1000;
    bool hasPendingChanges = false;
    bool m_hasPendingUpdate = false;
    QString m_pendingUpdateVersion;
    QHash<QString, int> presetMap;

    // Методы инициализации
    void initVisuals(); // Иконки, тени, драггер, скрытие лишнего
    void initHotkeyLogic(); // PresetMap, KeySequenceHelper и кнопки клавиш
    void initUpdateLogic(); // Тултипы, попапы частоты обновлений
    void initLanguageAndStartup(); // Кнопки языка и автозагрузки
    void initAutosaveLogic(); // Коннекты таймера и логика сохранения

    // Вспомогательные методы
    void addSelectorForFrame(QFrame *frame, const QString &extraStyle = QString());

    [[nodiscard]] bool isManualCheckActive() const {
        return m_syncRotationAnim && m_syncRotationAnim->state() == QAbstractAnimation::Running;
    }

    void buildPresetMap();

    [[nodiscard]] int vkFromPresetObjectName(const QString &obj) const;

    static QString nameFromVk(int vk);

    void applyHotkeyIfChanged(int newVk, const QString &newName);

    void markChanged();

    void restorePreviousPresetIfNeeded();

    void restoreDefaultsForCurrentPage();

    void restoreDefaults_General();

    void refreshTranslations() const;
};
