#pragma once
#include <QTimer>
#include <QNetworkReply>
#include <QHash>

class QNetworkAccessManager;

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
    void updateAvailable(const QString &tagName, const QString &url, bool isManualCheck);

    void noUpdateAvailable(const QString &version, bool isManualCheck);

    // Сигнал об ошибке (опционально)
    void updateError(const QString &errorText, bool isManualCheck);

private slots:
    void onGithubResponse(QNetworkReply *reply);

private:
    QTimer timer;
    QNetworkAccessManager *networkManager = nullptr;
    QHash<QNetworkReply *, bool> m_replyIsManual;

    static bool isUpdateDue();

    // Вспомогательная функция для сравнения версий
    static bool isNewerVersion(const QString &currentVer, const QString &remoteVer);

    void performCheck(bool isManualCheck);
};
