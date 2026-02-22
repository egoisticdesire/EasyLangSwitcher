#include "settingsWindow.h"
#include "../../../core/config/logger.h"

SettingsWindow::SettingsWindow(QWidget *parent) : QWidget(parent) {
    ui.setupUi(this);

    // Визуальное оформление
    initVisuals();

    // Логика обновлений
    initUpdateLogic();

    // Язык и автозагрузка
    initLanguageAndStartup();

    // Хоткеи
    buildPresetMap();
    initHotkeyLogic();

    // Автосохранение
    initAutosaveLogic();

    // Системные настройки окна
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::ClickFocus);

    refreshTranslations();
    LOG_DEBUG() << "SettingsWindow initialized";
}

SettingsWindow::~SettingsWindow() = default;
