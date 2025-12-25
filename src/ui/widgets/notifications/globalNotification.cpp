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
#include <QStyle>

GlobalNotification::GlobalNotification(const Mode mode, const QString &version, const QString &url, QWidget *parent)
    : QWidget(nullptr),
      ui(new Ui::notification_main_widget),
      m_mode(mode),
      m_version(version),
      m_downloadUrl(url) {
    ui->setupUi(this);
    this->setFixedWidth(400);

    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_DeleteOnClose);
    setMouseTracking(true);
    ui->background_frame->setMouseTracking(true);
    ui->background_frame->setAttribute(Qt::WA_Hover);
    ui->info_title_label->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui->info_desc_label->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui->info_icon->setAttribute(Qt::WA_TransparentForMouseEvents);

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
    connect(ui->btn_cancel_process, &QPushButton::clicked, this, &GlobalNotification::cancelDownload);
    connect(ui->btn_releases, &QPushButton::clicked, this, [this]() {
        if (!m_downloadUrl.isEmpty()) QDesktopServices::openUrl(QUrl(m_downloadUrl));
    });
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

// Плавное переключение между кнопками и прогрессом
void GlobalNotification::toggleInterface(const bool downloading) {
    this->setUpdatesEnabled(false);

    // Если включаем загрузку, обнуляем бар сразу
    if (downloading) ui->progress_bar->setValue(0);

    ui->btn_frame->hide();
    ui->progress_frame->hide();

    this->layout()->invalidate();
    this->layout()->activate();

    if (downloading) ui->progress_frame->show();
    else ui->btn_frame->show();

    this->adjustSize();

    if (const QScreen *screen = QGuiApplication::primaryScreen()) {
        const QRect desktop = screen->availableGeometry();
        constexpr int margin = 20;
        const int newY = desktop.bottom() - this->height() - margin;
        this->move(this->x(), newY);
    }

    this->setUpdatesEnabled(true);
    this->style()->unpolish(this);
    this->style()->polish(this);
    ui->background_frame->update();

    QWidget *activeFrame = downloading ? ui->progress_frame : ui->btn_frame;
    auto *fadeAnim = new QPropertyAnimation(activeFrame, "windowOpacity");
    fadeAnim->setDuration(250);
    fadeAnim->setStartValue(0.0);
    fadeAnim->setEndValue(1.0);
    fadeAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void GlobalNotification::refreshTranslations() {
    // Сохраняем текущую нижнюю точку, чтобы виджет не прыгал вниз
    const int anchorY = this->frameGeometry().bottom();

    if (m_mode == UpToDate) {
        ui->info_title_label->setText(Lang::tr("NOTIFICATION_UPD_NOT_AVAILABLE_TITLE"));
        ui->info_desc_label->setText(Lang::tr("NOTIFICATION_UPD_NOT_AVAILABLE_DESC").arg(m_version));
        ui->btn_frame->hide();
        ui->progress_frame->hide();
    } else if (m_mode == UpdateAvailable) {
        ui->info_title_label->setText(Lang::tr("NOTIFICATION_UPD_AVAILABLE_TITLE"));
        ui->info_desc_label->setText(Lang::tr("NOTIFICATION_UPD_AVAILABLE_DESC").arg(m_version));
        ui->btn_download->setText(Lang::tr("NOTIFICATION_UPD_BTN_DOWNLOAD"));
        ui->btn_releases->setText(Lang::tr("NOTIFICATION_UPD_BTN_RELEASES"));

        toggleInterface(m_reply != nullptr);
    }

    ui->info_icon->setIcon(IconHelper::loadIcon(":/icons/icons/FlashSparkleFilled2.png", QColor(), QSize(42, 42)));
    ui->btn_download->setIcon(
        IconHelper::loadIcon(":/icons/icons/DownloadFilled.svg", QColor(175, 175, 175), QSize(20, 20)));
    ui->btn_cancel_process->setIcon(
        IconHelper::loadIcon(":/icons/icons/DownloadOffFilled.svg", QColor(175, 175, 175), QSize(20, 20)));
    ui->btn_save_as->setIcon(
        IconHelper::loadIcon(":/icons/icons/MoreFilled.svg", QColor(175, 175, 175), QSize(20, 20)));
    ui->btn_releases->setIcon(
        IconHelper::loadIcon(":/icons/icons/OpenFilled.svg", QColor(175, 175, 175), QSize(20, 20)));

    this->adjustSize();

    // Фикс дрейфа: вычисляем новый Y так, чтобы НИЗ (anchorY) остался на той же линии
    if (this->isVisible()) {
        const int newY = anchorY - (this->height() - 1);
        this->move(this->x(), newY);
        AcrylicHelper::updateRegion(this);
    }
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
        this, Lang::tr("NOTIFICATION_UPD_SAVE_FILE_TITLE"), fileName,
        "Executable (*.exe)"); !path.isEmpty())
        executeDownload(path);
}

void GlobalNotification::executeDownload(const QString &filePath) {
    ui->progress_bar->setValue(0);

    toggleInterface(true);

    if (m_file) {
        m_file->close();
        delete m_file;
    }

    m_file = new QFile(filePath);
    if (!m_file->open(QIODevice::WriteOnly)) {
        toggleInterface(false);
        delete m_file;
        m_file = nullptr;
        return;
    }

    auto *manager = new QNetworkAccessManager(this);
    m_reply = manager->get(QNetworkRequest(QUrl(m_downloadUrl)));

    connect(m_reply, &QNetworkReply::readyRead, this, [this]() {
        if (m_file && m_reply && m_reply->error() == QNetworkReply::NoError)
            m_file->write(m_reply->readAll());
    });

    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](const qint64 rec, const qint64 total) {
        if (total > 0) {
            const int percent = static_cast<int>((rec * 100) / total);
            ui->progress_bar->setValue(percent);
        }
    });

    connect(m_reply, &QNetworkReply::finished, this, &GlobalNotification::onDownloadFinished);
}

void GlobalNotification::onDownloadFinished() {
    // Проверяем наличие reply (защита от повторных вызовов)
    if (!m_reply) return;

    const bool isCanceled = (m_reply->error() == QNetworkReply::OperationCanceledError);
    const bool hasError = (m_reply->error() != QNetworkReply::NoError && !isCanceled);

    // Закрываем и удаляем файл безопасно
    if (m_file) {
        m_file->close();
        if (hasError || isCanceled) m_file->remove(); // Удаляем недокачанный мусор
        delete m_file;
        m_file = nullptr;
    }

    // Обработка текста
    if (hasError) {
        ui->info_desc_label->setText(Lang::tr("NOTIFICATION_UPD_DOWNLOAD_ERROR"));
    } else if (!isCanceled) {
        ui->info_desc_label->setText(Lang::tr("NOTIFICATION_UPD_DOWNLOAD_COMPLETE"));
        QTimer::singleShot(5000, this, &GlobalNotification::startExitAnimation);
    }

    // Очистка сетевого ответа
    m_reply->deleteLater();
    m_reply = nullptr;

    // Если это была отмена — возвращаем кнопки
    if (isCanceled) toggleInterface(false);
}

void GlobalNotification::cancelDownload() {
    if (m_reply && m_reply->isRunning()) {
        // Просто прерываем. Это вызовет сигнал finished(),
        // который перейдет в onDownloadFinished, где всё корректно почистится.
        m_reply->abort();
    } else {
        // Если загрузка даже не началась, но мы в режиме прогресса — просто вернем кнопки
        toggleInterface(false);
    }
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

        const QString style = QString("QProgressBar::chunk { background-color: %1; border-radius: 2px; }")
                .arg(accent.name());

        ui->progress_bar->setStyleSheet(style);
    }
}

void GlobalNotification::moveToBottomRight() {
    const QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    const QRect desktopRect = screen->availableGeometry();

    this->ensurePolished();
    this->adjustSize();

    constexpr int margin = 20;
    const int x = desktopRect.right() - this->width() - margin;
    const int y = desktopRect.bottom() - this->height() - margin;

    this->move(x, y);
}
