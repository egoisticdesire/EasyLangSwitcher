#pragma once
#include "ui_EasyLangSwitcher_settings.h"
#include "animatedSelector.h"
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

class SettingsWindow final : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = nullptr);

    ~SettingsWindow() override;

    void openCentered();

signals:
    void settingsChanged();

protected:
    void showEvent(QShowEvent *event) override;

    bool event(QEvent *ev) override;

private:
    Ui::settings_main_widget ui{};

    QVector<AnimatedSelector *> selectors;

    WindowDragger *dragger = nullptr;

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
};
