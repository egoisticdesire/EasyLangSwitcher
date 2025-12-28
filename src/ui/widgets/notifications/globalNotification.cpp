#include "globalNotification.h"
#include "../../helpers/acrylicHelper.h"
#include "../../../core/config/appSettings.h"
#include "../../../core/i18n/lang.h"
#include <QScreen>
#include <QGuiApplication>
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
#include <QSequentialAnimationGroup>
#include <QStyle>

GlobalNotification::GlobalNotification(const Mode mode, const QString &version, const QString &url, QWidget *parent)
    : QWidget(nullptr),
      ui(new Ui::notification_main_widget),
      m_mode(mode),
      m_version(version),
      m_downloadUrl(url) {
    ui->setupUi(this);
    this->setFixedWidth(420);

    ui->btn_stack->setCurrentIndex(0);
    ui->btn_stack->layout()->activate();

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

    m_externalCloseBtn = new NotificationCloseButton(this);
    m_stackOpacityEffect = new QGraphicsOpacityEffect(ui->btn_stack);
    ui->btn_stack->setGraphicsEffect(m_stackOpacityEffect);

    if (m_mode == UpToDate) {
        m_currentState = UiState::Hidden;
        ui->btn_stack->hide();
        ui->vlayout_background_frame->setSpacing(0);
    } else {
        m_currentState = UiState::Buttons;
        ui->btn_stack->show();
        ui->vlayout_background_frame->setSpacing(20);
    }

    refreshTranslations();
    applySystemAccentColor();

    // Коннекты
    connect(ui->btn_download, &QPushButton::clicked, this, &GlobalNotification::startFastDownload);
    connect(ui->btn_save_as, &QPushButton::clicked, this, &GlobalNotification::startCustomDownload);
    connect(ui->btn_cancel_process, &QPushButton::clicked, this, &GlobalNotification::cancelDownload);
    connect(ui->btn_releases, &QPushButton::clicked, this, [this]() {
        if (m_downloadUrl.isEmpty()) return;
        QString releasePage = m_downloadUrl;

        if (const int downloadIdx = releasePage.indexOf("/download/"); downloadIdx != -1)
            releasePage = releasePage.left(downloadIdx) + "/latest";

        LOG_DEBUG() << "Opening release page: " << releasePage;
        QDesktopServices::openUrl(QUrl(releasePage));
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

    this->adjustSize();
    QTimer::singleShot(0, this, [this]() {
        AcrylicHelper::enableAcrylic(this);
        AcrylicHelper::updateRegion(this);
    });

    moveToBottomRight();
    LOG_DEBUG() << "GlobalNotification CREATED: " << this;
}

GlobalNotification::~GlobalNotification() {
    LOG_DEBUG() << "GlobalNotification DESTROYED: " << this;
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

    auto *posAnim = new QPropertyAnimation(this, "pos");
    posAnim->setDuration(500);
    posAnim->setStartValue(startPos);
    posAnim->setEndValue(endPos);
    posAnim->setEasingCurve(QEasingCurve::OutBack);

    auto *opacityAnim = new QPropertyAnimation(this, "windowOpacity");
    opacityAnim->setDuration(400);
    opacityAnim->setStartValue(0.0);
    opacityAnim->setEndValue(1.0);

    auto *group = new QParallelAnimationGroup(this);
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
    if (m_reply && m_reply->isRunning()) m_reply->abort();

    m_isExiting = true;

    if (m_externalCloseBtn) m_externalCloseBtn->setFade(false);

    auto *posAnim = new QPropertyAnimation(this, "pos");
    posAnim->setDuration(350);
    posAnim->setEndValue(this->pos() + QPoint(0, 100));
    posAnim->setEasingCurve(QEasingCurve::InBack);

    auto *opacityAnim = new QPropertyAnimation(this, "windowOpacity");
    opacityAnim->setDuration(250);
    opacityAnim->setEndValue(0.0);

    auto *group = new QParallelAnimationGroup(this);
    group->addAnimation(posAnim);
    group->addAnimation(opacityAnim);

    connect(group, &QParallelAnimationGroup::finished, this, &GlobalNotification::close);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void GlobalNotification::toggleInterface(const UiState state) {
    if (m_currentState == state) return;
    const UiState oldState = m_currentState;
    m_currentState = state;

    // Обновляем текст сразу, чтобы sizeHint в анимации высоты был верным
    updateContentOnly();

    // Если это переход между Buttons <-> Progress — крутим слайд
    if (oldState != UiState::Hidden && state != UiState::Hidden) {
        const int nextIdx = (state == UiState::Buttons) ? 0 : (state == UiState::Progress ? 1 : 2);
        animateStackTransition(nextIdx);
    }

    // Всегда запускаем анимацию высоты
    animateHeightChange();
}

void GlobalNotification::animateStackTransition(int nextIndex) {
    const int currentIndex = ui->btn_stack->currentIndex();
    if (currentIndex == nextIndex) return;

    QWidget *currentWrap = (currentIndex == 0) ? ui->btn_wrap : ui->progress_wrap;
    QWidget *nextWrap = (nextIndex == 0) ? ui->btn_wrap : ui->progress_wrap;

    const int dir = (nextIndex > currentIndex) ? 1 : -1;
    const int offset = 80 * dir;
    constexpr int buffer = 15;

    auto *seq = new QSequentialAnimationGroup(this);

    // EXIT
    auto *exitGroup = new QParallelAnimationGroup(this);
    auto *slideOut = new QPropertyAnimation(currentWrap, "pos");
    slideOut->setDuration(350);
    slideOut->setStartValue(QPoint(buffer, 0));
    slideOut->setEndValue(QPoint(buffer - offset, 0));
    slideOut->setEasingCurve(QEasingCurve::InBack);

    auto *fadeOut = new QPropertyAnimation(m_stackOpacityEffect, "opacity");
    fadeOut->setDuration(300);
    fadeOut->setEndValue(0.0);

    exitGroup->addAnimation(slideOut);
    exitGroup->addAnimation(fadeOut);

    connect(exitGroup, &QParallelAnimationGroup::finished, this, [this, nextIndex, nextWrap, offset]() {
        ui->btn_stack->setCurrentIndex(nextIndex);
        nextWrap->move(buffer + offset, 0);
    });

    // ENTER
    auto *enterGroup = new QParallelAnimationGroup(this);
    auto *slideIn = new QPropertyAnimation(nextWrap, "pos");
    slideIn->setDuration(350);
    slideIn->setStartValue(QPoint(buffer + offset, 0));
    slideIn->setEndValue(QPoint(buffer, 0));
    slideIn->setEasingCurve(QEasingCurve::OutBack);

    auto *fadeIn = new QPropertyAnimation(m_stackOpacityEffect, "opacity");
    fadeIn->setDuration(300);
    fadeIn->setEndValue(1.0);

    enterGroup->addAnimation(slideIn);
    enterGroup->addAnimation(fadeIn);

    seq->addAnimation(exitGroup);
    seq->addAnimation(enterGroup);
    seq->start(QAbstractAnimation::DeleteWhenStopped);
}

void GlobalNotification::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) startExitAnimation();
    QWidget::mousePressEvent(event);
}

bool GlobalNotification::event(QEvent *event) {
    if (event->type() == QEvent::WindowActivate)
        if (m_externalCloseBtn) m_externalCloseBtn->raise();
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
    if (m_externalCloseBtn) m_externalCloseBtn->hide();
    QWidget::hideEvent(event);
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
    if (!fileName.endsWith(".exe", Qt::CaseInsensitive))
        fileName = QString("%1.exe").arg(AppSettings::APP_NAME);

    const QString path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/" + fileName;
    executeDownload(path);
}

void GlobalNotification::startCustomDownload() {
    QString fileName = QFileInfo(m_downloadUrl).fileName();
    if (!fileName.endsWith(".exe", Qt::CaseInsensitive))
        fileName = QString("%1.exe").arg(AppSettings::APP_NAME);

    if (const QString path = QFileDialog::getSaveFileName(
        this, Lang::tr("NOTIFICATION_UPD_SAVE_FILE_TITLE"), fileName,
        "Executable (*.exe)"); !path.isEmpty())
        executeDownload(path);
}

void GlobalNotification::executeDownload(const QString &filePath) {
    ui->progress_bar->setValue(0);

    toggleInterface(UiState::Progress);
    ui->info_desc_label->setText(Lang::tr("NOTIFICATION_UPD_DOWNLOAD_PROGRESS"));

    if (m_file) {
        m_file->close();
        delete m_file;
    }

    m_file = new QFile(filePath);
    if (!m_file->open(QIODevice::WriteOnly)) {
        ui->info_desc_label->setText(Lang::tr("NOTIFICATION_UPD_DOWNLOAD_ERROR"));
        toggleInterface(UiState::Buttons);
        delete m_file;
        m_file = nullptr;
        return;
    }

    auto *manager = new QNetworkAccessManager(this);
    m_reply = manager->get(QNetworkRequest(QUrl(m_downloadUrl))); //"https://api.github.coms/repos/%1/releases/latest"

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
    if (!m_reply) return;

    const bool isCanceled = (m_reply->error() == QNetworkReply::OperationCanceledError);
    const bool hasError = (m_reply->error() != QNetworkReply::NoError && !isCanceled);

    if (m_file) {
        m_file->close();
        if (hasError || isCanceled) m_file->remove();
        delete m_file;
        m_file = nullptr;
    }

    if (hasError) {
        ui->info_desc_label->setText(Lang::tr("NOTIFICATION_UPD_DOWNLOAD_ERROR") + ": " + m_reply->errorString());
        toggleInterface(UiState::Buttons);
    } else if (!isCanceled) {
        ui->info_desc_label->setText(Lang::tr("NOTIFICATION_UPD_DOWNLOAD_COMPLETE"));
        toggleInterface(UiState::Hidden);
        // QTimer::singleShot(5000, this, &GlobalNotification::startExitAnimation);
    }

    m_reply->deleteLater();
    m_reply = nullptr;

    if (isCanceled) toggleInterface(UiState::Buttons);
}

void GlobalNotification::cancelDownload() {
    if (m_reply && m_reply->isRunning()) m_reply->abort();
    toggleInterface(UiState::Buttons);
    animateHeightChange();
}

void GlobalNotification::applySystemAccentColor() const {
    const QSettings dwmSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\DWM", QSettings::NativeFormat);
    bool ok;
    const unsigned int rgba = dwmSettings.value("AccentColor").toUInt(&ok);
    if (ok) {
        const QColor accent(rgba & 0xFF, (rgba >> 8) & 0xFF, (rgba >> 16) & 0xFF);
        ui->progress_bar->setStyleSheet(
            QString("QProgressBar::chunk { background-color: %1; border-radius: 2px; }").arg(accent.name()));
    }
}

void GlobalNotification::moveToBottomRight() {
    const QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    constexpr int margin = 20;
    const QRect desktopRect = screen->availableGeometry();
    const int x = desktopRect.right() - this->width() - margin;
    const int y = desktopRect.bottom() - this->height() - margin;

    this->move(x, y);
}

void GlobalNotification::refreshTranslations() {
    // Просто обновляем все данные (текст, иконки)
    updateContentOnly();

    // Логика изменения размера
    if (!this->isVisible()) {
        // Если окно еще не показано (в момент создания), просто подгоняем размер без анимаций
        this->setMinimumHeight(0);
        this->setMaximumHeight(16777215);
        ui->background_frame->layout()->activate();
        this->adjustSize();
    } else {
        // Если окно уже на экране и язык сменился — запускаем пересчет
        animateHeightChange();
    }
}

void GlobalNotification::updateContentOnly() const {
    // Обновляем заголовки
    if (m_mode == UpToDate) ui->info_title_label->setText(Lang::tr("NOTIFICATION_UPD_NOT_AVAILABLE_TITLE"));
    else ui->info_title_label->setText(Lang::tr("NOTIFICATION_UPD_AVAILABLE_TITLE"));

    // Если мы уже скачали файл, не даем сбросить текст на исходный
    if (m_currentState == UiState::Progress) {
        ui->info_desc_label->setText(Lang::tr("NOTIFICATION_UPD_DOWNLOAD_PROGRESS"));
    } else if (m_currentState == UiState::Hidden) {
        // Если мы перешли в Hidden после загрузки
        ui->info_desc_label->setText(Lang::tr("NOTIFICATION_UPD_DOWNLOAD_COMPLETE"));
    } else {
        // Исходное состояние (Buttons)
        ui->info_desc_label->setText(Lang::tr("NOTIFICATION_UPD_AVAILABLE_DESC").arg(m_version));
    }

    // Обновляем кнопки
    ui->btn_download->setText(Lang::tr("NOTIFICATION_UPD_BTN_DOWNLOAD"));
    ui->btn_releases->setText(Lang::tr("NOTIFICATION_UPD_BTN_RELEASES"));

    // Иконки
    ui->info_icon->setIcon(
        IconHelper::loadIcon(":/icons/icons/FlashSparkleFilled2.png", QColor(), QSize(42, 42)));
    ui->btn_download->setIcon(
        IconHelper::loadIcon(":/icons/icons/DownloadFilled.svg", QColor(175, 175, 175), QSize(20, 20)));
    ui->btn_cancel_process->setIcon(
        IconHelper::loadIcon(":/icons/icons/DownloadOffFilled.svg", QColor(175, 175, 175), QSize(20, 20)));
    ui->btn_save_as->setIcon(
        IconHelper::loadIcon(":/icons/icons/MoreFilled.svg", QColor(175, 175, 175), QSize(20, 20)));
    ui->btn_releases->setIcon(
        IconHelper::loadIcon(":/icons/icons/OpenFilled.svg", QColor(175, 175, 175), QSize(20, 20)));
}

void GlobalNotification::animateHeightChange() {
    this->setMinimumHeight(0);
    this->setMaximumHeight(16777215);

    if (m_currentState == UiState::Hidden) {
        ui->btn_stack->hide();
        ui->vlayout_background_frame->setSpacing(0);
    } else {
        ui->btn_stack->show();
        ui->vlayout_background_frame->setSpacing(20);

        if (!ui->btn_stack->isVisible()) {
            const int pageIndex = (m_currentState == UiState::Buttons)
                                      ? 0
                                      : (m_currentState == UiState::Progress ? 1 : 2);
            ui->btn_stack->setCurrentIndex(pageIndex);
        }
    }

    // Просчитываем новый размер
    ui->info_title_label->updateGeometry();
    ui->info_desc_label->updateGeometry();
    ui->background_frame->layout()->invalidate();
    ui->background_frame->layout()->activate();
    QCoreApplication::processEvents();

    const int startHeight = this->height();
    const int targetHeight = this->sizeHint().height();

    const int anchorY = this->geometry().bottom();
    const int windowX = this->x();
    const int windowWidth = this->width();

    if (qAbs(startHeight - targetHeight) < 2) {
        this->setFixedHeight(targetHeight);
        return;
    }

    auto *geoAnim = new QVariantAnimation(this);
    geoAnim->setDuration(500);
    geoAnim->setStartValue(startHeight);
    geoAnim->setEndValue(targetHeight);
    geoAnim->setEasingCurve(QEasingCurve::OutBack);

    connect(geoAnim, &QVariantAnimation::valueChanged, this,
            [this, anchorY, windowX, windowWidth](const QVariant &value) {
                const int h = value.toInt();
                // Временно разрешаем окну быть любого размера в процессе анимации
                this->setMinimumHeight(qMin(h, this->minimumHeight()));
                this->setGeometry(windowX, anchorY - h + 1, windowWidth, h);
                AcrylicHelper::updateRegion(this);
            });

    connect(geoAnim, &QVariantAnimation::finished, this, [this, targetHeight]() {
        this->setFixedHeight(targetHeight);
        if (m_externalCloseBtn) m_externalCloseBtn->updatePosition();
    });

    geoAnim->start(QAbstractAnimation::DeleteWhenStopped);
}
