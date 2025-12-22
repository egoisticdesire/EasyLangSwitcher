#include "updateManager.h"
#include "../../core/config/appSettings.h"
#include "../../core/config/logger.h"
#include <QCoreApplication>
#include <QJsonObject>
#include <QRegularExpression>
#include <QVersionNumber>

UpdateManager::UpdateManager(QObject *parent)
    : QObject(parent) {
    networkManager = new QNetworkAccessManager(this);

    // Таймер: проверяем каждый час
    timer.setInterval(60 * 60 * 1000);
    connect(&timer, &QTimer::timeout, this, &UpdateManager::checkForUpdatesIfDue);
}

void UpdateManager::start() {
    timer.start();
    LOG_DEBUG() << "UpdateManager started";
    // Проверяем сразу при запуске (с небольшой задержкой, чтобы не тормозить старт UI)
    QTimer::singleShot(5000, this, &UpdateManager::checkForUpdatesIfDue);
}

void UpdateManager::stop() {
    timer.stop();
    LOG_DEBUG() << "UpdateManager stopped";
}

void UpdateManager::checkForUpdatesIfDue() {
    if (!isUpdateDue()) {
        // LOG_DEBUG() << "Update check skipped: not due yet.";
        return;
    }

    performCheck();
}

void UpdateManager::checkForUpdatesForce() {
    LOG_DEBUG() << "UpdateManager: Force checking updates...";
    performCheck();
}

void UpdateManager::performCheck() {
    LOG_DEBUG() << "UpdateManager: Requesting GitHub API...";

    // Формируем URL для получения последнего релиза
    // См: https://docs.github.com/en/rest/releases/releases
    const QString apiUrl = QString("https://api.github.com/repos/%1/releases/latest")
            .arg(AppSettings::GITHUB_REPO);

    QNetworkRequest request((QUrl(apiUrl)));

    // GitHub API требует User-Agent
    request.setRawHeader("User-Agent", AppSettings::APP_NAME);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onGithubResponse(reply);
    });
}

void UpdateManager::onGithubResponse(QNetworkReply *reply) {
    reply->deleteLater(); // Удаляем объект после выхода из функции

    if (reply->error() != QNetworkReply::NoError) {
        LOG_DEBUG() << "Update check network error: " << reply->errorString();
        emit updateError(reply->errorString());
        return;
    }

    const QByteArray data = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    const QJsonObject obj = doc.object();

    // Получаем версию с GitHub
    const QString remoteTag = obj.value("tag_name").toString();
    const QString htmlUrl = obj.value("html_url").toString(); // Ссылка на страницу релиза

    if (remoteTag.isEmpty()) {
        LOG_DEBUG() << "Update check failed: tag_name is empty in response.";
        return;
    }

    // Текущая версия приложения
    const QString currentVersion = QCoreApplication::applicationVersion();

    LOG_DEBUG() << "Update check: Current=" << currentVersion << "; Remote=" << remoteTag;

    // Обновляем дату последней успешной проверки
    AppSettings::lastUpdateCheckDate = QDate::currentDate();
    AppSettings::save();

    // Сравниваем версии
    if (isNewerVersion(currentVersion, remoteTag)) {
        LOG_DEBUG() << "New update found!";
        emit updateAvailable(remoteTag, htmlUrl);
    } else {
        LOG_DEBUG() << "No updates available.";
        emit noUpdateAvailable();
    }
}

bool UpdateManager::isUpdateDue() {
    using UF = AppSettings::UpdateFrequency;

    if (AppSettings::updateFrequency == UF::Never)
        return false;

    // Если дата никогда не сохранялась (invalid), считаем, что проверка нужна прямо сейчас
    if (!AppSettings::lastUpdateCheckDate.isValid())
        return true;

    const QDate today = QDate::currentDate();
    const QDate lastCheck = AppSettings::lastUpdateCheckDate;

    // Если дата последней проверки в будущем (пользователь менял часы), сбрасываем
    if (lastCheck > today)
        return true;

    switch (AppSettings::updateFrequency) {
        case UF::Daily:
            // Прошло больше 0 дней с последней проверки? (т.е. хотя бы вчера)
            return lastCheck.daysTo(today) >= 1;

        case UF::Weekly:
            // Используем addDays для точного расчета
            // Если (Дата последней + 7 дней) <= Сегодня, значит пора
            return lastCheck.addDays(7) <= today;

        case UF::Monthly:
            // Если (Дата последней + 1 месяц) <= Сегодня
            return lastCheck.addMonths(1) <= today;

        default:
            return false;
    }
}

bool UpdateManager::isNewerVersion(const QString &currentVer, const QString &remoteVer) {
    // Удаляем префикс 'v', если он есть (v1.0.1 -> 1.0.1)
    QString c = currentVer;
    if (c.startsWith('v', Qt::CaseInsensitive)) c.remove(0, 1);

    QString r = remoteVer;
    if (r.startsWith('v', Qt::CaseInsensitive)) r.remove(0, 1);

    // Простой способ через QVersionNumber (требует Qt 5.6+)
    const QVersionNumber vCurrent = QVersionNumber::fromString(c);
    const QVersionNumber vRemote = QVersionNumber::fromString(r);

    return vRemote > vCurrent;
}
