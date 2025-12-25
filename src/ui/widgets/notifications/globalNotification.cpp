#include "globalNotification.h"
#include "../../helpers/acrylicHelper.h"
#include "../../../core/config/appSettings.h"
#include "../../../core/i18n/lang.h"

#include <QScreen>
#include <QGuiApplication>
#include <QNetworkAccessManager>
#include <QSettings>
#include <QNetworkReply>
#include <QFile>
#include <QStandardPaths>
#include <QFileDialog>
#include <QFileInfo>
#include <QTimer>
#include <QDesktopServices>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

GlobalNotification::GlobalNotification(const Mode mode, const QString &version, const QString &url, QWidget *parent)
    : QWidget(nullptr),
      ui(new Ui::notification_main_widget),
      m_mode(mode),
      m_version(version),
      m_downloadUrl(url) {
    ui->setupUi(this);
    this->setFixedWidth(380);

    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_DeleteOnClose);
    setMouseTracking(true);
    ui->background_frame->setMouseTracking(true);
    ui->info_title_label->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui->info_desc_label->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui->info_icon->setAttribute(Qt::WA_TransparentForMouseEvents);

    // Повторяем твой стиль инициализации Акрила
    QTimer::singleShot(0, this, [this]() {
        AcrylicHelper::enableAcrylic(this);
        AcrylicHelper::updateRegion(this);
    });

    refreshTranslations();
    applySystemAccentColor();

    m_externalCloseBtn = new NotificationCloseButton(this);

    // Коннекты
    connect(ui->btn_download, &QPushButton::clicked, this, &GlobalNotification::startFastDownload);
    connect(ui->btn_save_as, &QPushButton::clicked, this, &GlobalNotification::startCustomDownload);
    connect(ui->cancel_process, &QPushButton::clicked, this, &GlobalNotification::cancelDownload);
    connect(ui->btn_releases, &QPushButton::clicked, this, [this]() {
        if (!m_downloadUrl.isEmpty()) QDesktopServices::openUrl(QUrl(m_downloadUrl));
    });
    connect(ui->cancel_process, &QPushButton::clicked, this, &GlobalNotification::startExitAnimation);
    connect(m_externalCloseBtn, &QPushButton::clicked, this, &GlobalNotification::startExitAnimation);

    // if (m_mode == UpToDate) {
    //     // Закрыть через 5 секунд, если это просто инфо-сообщение
    //     QTimer::singleShot(5000, this, [this]() {
    //         // Проверяем, не скачиваем ли мы что-то в этот момент (на всякий случай)
    //         if (!m_reply) {
    //             if (m_externalCloseBtn) m_externalCloseBtn->close();
    //             this->close();
    //         }
    //     });
    // }

    moveToBottomRight();
    LOG_DEBUG() << "UpdateNotification CREATED:" << this;
}

GlobalNotification::~GlobalNotification() {
    LOG_DEBUG() << "UpdateNotification DESTROYED:" << this;
    delete ui;
}

void GlobalNotification::startShowAnimation() {
    this->setWindowOpacity(0.0);

    const QScreen *screen = QGuiApplication::primaryScreen();
    const QRect desktop = screen->availableGeometry();

    constexpr int margin = 20;
    const QPoint endPos(desktop.right() - this->width() - margin, desktop.bottom() - this->height() - margin);
    const QPoint startPos(desktop.right() + margin, endPos.y());

    this->move(startPos);

    const auto posAnim = new QPropertyAnimation(this, "pos");
    posAnim->setDuration(500);
    posAnim->setStartValue(startPos);
    posAnim->setEndValue(endPos);
    posAnim->setEasingCurve(QEasingCurve::OutBack);

    const auto opacityAnim = new QPropertyAnimation(this, "windowOpacity");
    opacityAnim->setDuration(400);
    opacityAnim->setStartValue(0.0);
    opacityAnim->setEndValue(1.0);

    const auto group = new QParallelAnimationGroup(this);
    group->addAnimation(posAnim);
    group->addAnimation(opacityAnim);

    connect(group, &QParallelAnimationGroup::finished, this, [this]() {
        AcrylicHelper::enableAcrylic(this);
        AcrylicHelper::updateRegion(this);
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void GlobalNotification::startExitAnimation() {
    if (m_isExiting) return;
    m_isExiting = true;

    if (m_externalCloseBtn) m_externalCloseBtn->setFade(false);

    const auto posAnim = new QPropertyAnimation(this, "pos");
    posAnim->setDuration(350);
    posAnim->setEndValue(this->pos() + QPoint(0, 100));
    posAnim->setEasingCurve(QEasingCurve::InBack);

    const auto opacityAnim = new QPropertyAnimation(this, "windowOpacity");
    opacityAnim->setDuration(250);
    opacityAnim->setEndValue(0.0);

    const auto group = new QParallelAnimationGroup(this);
    group->addAnimation(posAnim);
    group->addAnimation(opacityAnim);

    connect(group, &QParallelAnimationGroup::finished, this, &GlobalNotification::close);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void GlobalNotification::refreshTranslations() {
    if (m_mode == UpToDate) {
        ui->info_title_label->setText(Lang::tr("NOTIFICATION_UPD_NOT_AVAILABLE_TITLE"));
        ui->info_desc_label->setText(Lang::tr("NOTIFICATION_UPD_NOT_AVAILABLE_DESC").arg(m_version));
        ui->btn_frame->hide();
        ui->progress_frame->hide();
    } else if (m_mode == UpdateAvailable) {
        ui->info_title_label->setText(Lang::tr("NOTIFICATION_UPD_AVAILABLE_TITLE"));
        ui->info_desc_label->setText(Lang::tr("NOTIFICATION_UPD_AVAILABLE_DESC").arg(m_version));
        ui->btn_download->setText(Lang::tr("SETTINGS_DOWNLOAD_LABEL"));
        ui->btn_releases->setText(Lang::tr("SETTINGS_RELEASE_NOTES_LABEL"));
        ui->btn_frame->show();
        ui->progress_frame->hide();
    }

    ui->info_icon->setIcon(IconHelper::loadIcon(":/icons/icons/FlashSparkleFilled2.png", QColor(), QSize(42, 42)));

    adjustSize();

    if (this->isVisible() && !m_isExiting) AcrylicHelper::updateRegion(this);
}

void GlobalNotification::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) startExitAnimation();
    QWidget::mousePressEvent(event);
}

void GlobalNotification::enterEvent(QEnterEvent *event) {
    if (m_externalCloseBtn && !m_isExiting) m_externalCloseBtn->setFade(true);
    QWidget::enterEvent(event);
}

void GlobalNotification::leaveEvent(QEvent *event) {
    if (m_externalCloseBtn.isNull() || m_isExiting) return;

    const QPoint globalPos = QCursor::pos();
    const bool overMe = this->geometry().contains(globalPos);

    if (const bool overBtn = m_externalCloseBtn->geometry().contains(globalPos); !overMe && !overBtn) {
        m_externalCloseBtn->setFade(false);
    }
    QWidget::leaveEvent(event);
}

bool GlobalNotification::event(QEvent *event) {
    if (event->type() == QEvent::WindowActivate) {
        if (m_externalCloseBtn) m_externalCloseBtn->raise();
    }
    return QWidget::event(event);
}

void GlobalNotification::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) refreshTranslations();
    QWidget::changeEvent(event);
}

void GlobalNotification::moveEvent(QMoveEvent *event) {
    if (m_externalCloseBtn) m_externalCloseBtn->updatePosition();
    QWidget::moveEvent(event);
}

void GlobalNotification::showEvent(QShowEvent *event) {
    if (!this->property("shown").toBool()) {
        this->setProperty("shown", true);
        this->adjustSize();

        startShowAnimation();
    }

    if (m_externalCloseBtn) {
        m_externalCloseBtn->show();
        m_externalCloseBtn->updatePosition();
        m_externalCloseBtn->raise();
    }
    QWidget::showEvent(event);
}

void GlobalNotification::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    if (m_externalCloseBtn) m_externalCloseBtn->hide();
}

void GlobalNotification::closeEvent(QCloseEvent *event) {
    if (m_externalCloseBtn) {
        m_externalCloseBtn->hide();
        m_externalCloseBtn->deleteLater();
    }
    QWidget::closeEvent(event);
}

void GlobalNotification::startFastDownload() {
    QString fileName = QFileInfo(m_downloadUrl).fileName();
    if (!fileName.endsWith(".exe", Qt::CaseInsensitive)) fileName = QString("%1.exe").arg(AppSettings::APP_NAME);

    const QString path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/" + fileName;
    executeDownload(path);
}

void GlobalNotification::startCustomDownload() {
    QString fileName = QFileInfo(m_downloadUrl).fileName();
    if (!fileName.endsWith(".exe", Qt::CaseInsensitive)) fileName = QString("%1.exe").arg(AppSettings::APP_NAME);

    if (const QString path = QFileDialog::getSaveFileName(
        this, Lang::tr("SAVE_FILE_TITLE"), fileName,
        "Executable (*.exe)"); !path.isEmpty())
        executeDownload(path);
}

void GlobalNotification::executeDownload(const QString &filePath) {
    ui->btn_frame->hide();
    ui->progress_frame->show();
    adjustSize();

    m_file = new QFile(filePath);
    if (!m_file->open(QIODevice::WriteOnly)) return;

    auto *manager = new QNetworkAccessManager(this);
    m_reply = manager->get(QNetworkRequest(QUrl(m_downloadUrl)));

    connect(m_reply, &QNetworkReply::readyRead, this, [this]() {
        if (m_file) m_file->write(m_reply->readAll());
    });

    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](const qint64 rec, const qint64 total) {
        if (total > 0) ui->progress_bar->setValue(static_cast<int>((rec * 100) / total));
    });

    connect(m_reply, &QNetworkReply::finished, this, &GlobalNotification::onDownloadFinished);
}

void GlobalNotification::onDownloadFinished() {
    if (m_file) m_file->close();
    if (m_reply->error() == QNetworkReply::NoError) {
        ui->info_desc_label->setText(Lang::tr("DOWNLOAD_COMPLETE"));
        QTimer::singleShot(5000, this, &GlobalNotification::startExitAnimation);
    } else if (m_reply->error() != QNetworkReply::OperationCanceledError) {
        ui->info_desc_label->setText(Lang::tr("DOWNLOAD_ERROR"));
        if (m_file) m_file->remove();
    }
    m_reply->deleteLater();
    m_reply = nullptr;
}

void GlobalNotification::cancelDownload() {
    if (m_reply) m_reply->abort();
    ui->progress_frame->hide();
    ui->btn_frame->show();
    adjustSize();
}

void GlobalNotification::applySystemAccentColor() const {
    const QSettings dwmSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\DWM", QSettings::NativeFormat);
    bool ok;
    const unsigned int rgba = dwmSettings.value("AccentColor").toUInt(&ok);

    if (ok) {
        const int r = rgba & 0xFF;
        const int g = (rgba >> 8) & 0xFF;
        const int b = (rgba >> 16) & 0xFF;
        const QColor accent(r, g, b);

        const QString style = QString(
            "QProgressBar::chunk {"
            "   background-color: %1;"
            "   border-radius: 2px;"
            "}"
        ).arg(accent.name());

        ui->progress_bar->setStyleSheet(style);
    }
}

void GlobalNotification::moveToBottomRight() {
    const QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    const QRect desktopRect = screen->availableGeometry();
    this->adjustSize();

    constexpr int margin = 20;
    const int x = desktopRect.right() - this->width() - margin;
    const int y = desktopRect.bottom() - this->height() - margin;

    this->move(x, y);
}
