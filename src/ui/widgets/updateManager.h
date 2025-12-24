#pragma once
#include <QTimer>
#include <QNetworkReply>

class UpdateManager final : public QObject {
    Q_OBJECT

public:
    explicit UpdateManager(QObject *parent = nullptr);

    void start();

    void stop();

    // Публичный слот, чтобы его можно было вызывать из SettingsWindow
    void checkForUpdatesIfDue();

    // Принудительная проверка
    void checkForUpdatesForce();

signals:
    // Сигнал, что обновление найдено (передаем ссылку на скачивание или версию)
    void updateAvailable(const QString &tagName, const QString &url);

    void noUpdateAvailable();

    // Сигнал об ошибке (опционально)
    void updateError(const QString &errorText);

private slots:
    void onGithubResponse(QNetworkReply *reply);

private:
    QTimer timer;
    QNetworkAccessManager *networkManager;

    static bool isUpdateDue();

    // Вспомогательная функция для сравнения версий
    static bool isNewerVersion(const QString &currentVer, const QString &remoteVer);

    void performCheck();
};
