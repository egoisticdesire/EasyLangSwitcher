#include "autoStartup.h"
#include "../../core/config/appSettings.h"
#include "../../core/config/logger.h"
#include <QCoreApplication>
#include <QDir>
#include <QSettings>

QString AutoStartupManager::getExePath() {
    return QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
}

// Включение/выключение автозапуска
bool AutoStartupManager::setAutoStartup(const bool enable) {
    QSettings reg(R"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run)",
                  QSettings::NativeFormat);
    const QString name = AppSettings::APP_NAME;

    if (enable) {
        reg.setValue(name, getExePath());
        LOG_DEBUG() << "Auto-start enabled via registry: " << getExePath();
    } else {
        reg.remove(name);
        LOG_DEBUG() << "Auto-start disabled via registry";
    }
    return true;
}

// Проверка, включен ли автозапуск
bool AutoStartupManager::isAutoStartupEnabled() {
    const QSettings reg(R"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run)",
                        QSettings::NativeFormat);
    const QString name = AppSettings::APP_NAME;
    return reg.contains(name);
}
