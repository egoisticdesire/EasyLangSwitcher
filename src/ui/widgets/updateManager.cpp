#include "updateManager.h"
#include "../../core/config/appSettings.h"
#include "../../core/config/logger.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QRegularExpression>
#include <QVersionNumber>

namespace {
    QString extractVersionCore(QString value) {
        value = value.trimmed();
        static const QRegularExpression versionPattern(
            R"((?:^|[^0-9])v?(\d+(?:\.\d+)+))",
            QRegularExpression::CaseInsensitiveOption
        );

        QString lastMatch;
        QRegularExpressionMatchIterator it = versionPattern.globalMatch(value);
        while (it.hasNext()) {
            if (const QRegularExpressionMatch match = it.next(); match.hasMatch()) {
                lastMatch = match.captured(1);
            }
        }

        if (!lastMatch.isEmpty()) return lastMatch;

        if (value.startsWith('v', Qt::CaseInsensitive)) {
            value.remove(0, 1);
        }
        return value;
    }
}

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
        LOG_DEBUG() << "Update check skipped: not due yet";
        return;
    }

    performCheck(false);
}

void UpdateManager::checkForUpdatesForce() {
    LOG_DEBUG() << "UpdateManager: Force checking updates...";
    performCheck(true);
}

void UpdateManager::performCheck(const bool isManualCheck) {
    LOG_DEBUG() << "UpdateManager: Requesting GitHub API...";

    // Формируем URL для получения последнего релиза
    // См: https://docs.github.com/en/rest/releases/releases
    const QString apiUrl = QString("https://api.github.com/repos/%1/releases/latest")
            .arg(AppSettings::GITHUB_REPO);

    QNetworkRequest request((QUrl(apiUrl)));
    request.setRawHeader("User-Agent", AppSettings::APP_NAME);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);

    QNetworkReply *reply = networkManager->get(request);
    m_replyIsManual.insert(reply, isManualCheck);

    // Создаем таймер, который «убьет» запрос, если он затянется
    const auto timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);

    connect(timeoutTimer, &QTimer::timeout, reply, [reply]() {
        if (reply->isRunning()) {
            LOG_DEBUG() << "UpdateManager: Request timed out (15s). Aborting...";
            reply->abort();
        }
    });

    timeoutTimer->start(15000);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onGithubResponse(reply); });
}

void UpdateManager::onGithubResponse(QNetworkReply *reply) {
    const bool isManualCheck = m_replyIsManual.take(reply);
    reply->deleteLater();

    // Проверка сетевых ошибок (тайм-аут, нет интернета)
    if (reply->error() != QNetworkReply::NoError) {
        LOG_ERROR() << "Update check failed: " << reply->errorString();
        emit updateError(reply->errorString(), isManualCheck);
        return;
    }

    // Проверка HTTP кодов (403 - лимит, 404 - репо не найден)
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode == 403) {
        emit updateError("GitHub API rate limit exceeded. Try again later.", isManualCheck);
        return;
    }
    if (statusCode != 200) {
        emit updateError(QString("Server returned error code: %1").arg(statusCode), isManualCheck);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        emit updateError("Failed to parse update information", isManualCheck);
        return;
    }

    const QJsonObject obj = doc.object();
    const QString remoteTag = obj.value("tag_name").toString();

    if (remoteTag.isEmpty()) {
        emit updateError("No version information found in response", isManualCheck);
        return;
    }

    // Ищем прямую ссылку на EXE в ассетах
    QString downloadUrl = "";
    for (QJsonArray assets = obj.value("assets").toArray(); const QJsonValue &assetValue: assets) {
        QJsonObject assetObj = assetValue.toObject();
        if (QString fileName = assetObj.value("name").toString(); fileName.endsWith(".exe", Qt::CaseInsensitive)) {
            downloadUrl = assetObj.value("browser_download_url").toString();
            break;
        }
    }

    // Если EXE не нашли, откатываемся на ссылку страницы релиза
    if (downloadUrl.isEmpty()) downloadUrl = obj.value("html_url").toString();

    const QString currentVersion = QCoreApplication::applicationVersion();
    LOG_DEBUG() << "Version check. current=" << currentVersion << "; remote=" << remoteTag;

    AppSettings::lastUpdateCheckDate = QDate::currentDate();
    AppSettings::lastUpdateCheckDateTime = QDateTime::currentDateTime();
    AppSettings::save();

    if (isNewerVersion(currentVersion, remoteTag)) {
        LOG_DEBUG() << "Update found:" << remoteTag;
        emit updateAvailable(remoteTag, downloadUrl, isManualCheck);
    } else {
        LOG_DEBUG() << "No updates available";
        emit noUpdateAvailable(currentVersion, isManualCheck);
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
    const QString currentNormalized = extractVersionCore(currentVer);
    const QString remoteNormalized = extractVersionCore(remoteVer);
    const QVersionNumber currentVersionNumber = QVersionNumber::fromString(currentNormalized);
    const QVersionNumber remoteVersionNumber = QVersionNumber::fromString(remoteNormalized);

    LOG_DEBUG() << "Parsed versions. currentRaw=" << currentVer
                << "; currentNorm=" << currentNormalized
                << "; remoteRaw=" << remoteVer
                << "; remoteNorm=" << remoteNormalized;

    if (currentVersionNumber.isNull() || remoteVersionNumber.isNull()) {
        LOG_WARNING() << "Version parsing failed. currentRaw=" << currentVer
                << "; currentNorm=" << currentNormalized
                << "; remoteRaw=" << remoteVer
                << "; remoteNorm=" << remoteNormalized;
        return false;
    }

    return remoteVersionNumber > currentVersionNumber;
}
