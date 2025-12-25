#pragma once
#include "ui_EasyLangSwitcher_notification.h"
#include "../helpers/acrylicHelper.h"
#include "../helpers/closeButton.h"
#include "../../core/i18n/lang.h"
#include <QWidget>
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

class UpdateNotification final : public QWidget {
    Q_OBJECT

public:
    enum Mode { UpdateAvailable, UpToDate, Error };

    ~UpdateNotification() override {
        LOG_DEBUG() << "UpdateNotification DESTROYED:" << this;
    }

    explicit UpdateNotification(const Mode mode, const QString &version, const QString &url = "",
                                QWidget *parent = nullptr)
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

        // Повторяем твой стиль инициализации Акрила
        QTimer::singleShot(0, this, [this]() {
            AcrylicHelper::enableAcrylic(this);
            AcrylicHelper::updateRegion(this);
        });

        refreshTranslations();
        applySystemAccentColor();

        m_externalCloseBtn = new NotificationCloseButton(this);

        // Коннекты
        connect(ui->btn_download, &QPushButton::clicked, this, &UpdateNotification::startFastDownload);
        connect(ui->btn_save_as, &QPushButton::clicked, this, &UpdateNotification::startCustomDownload);
        connect(ui->cancel_process, &QPushButton::clicked, this, &UpdateNotification::cancelDownload);
        connect(ui->btn_releases, &QPushButton::clicked, this, [this]() {
            if (!m_downloadUrl.isEmpty()) QDesktopServices::openUrl(QUrl(m_downloadUrl));
        });
        connect(ui->cancel_process, &QPushButton::clicked, this, &UpdateNotification::startExitAnimation);
        connect(m_externalCloseBtn, &QPushButton::clicked, this, &UpdateNotification::startExitAnimation);

        setMouseTracking(true);
        ui->background_frame->setMouseTracking(true);
        ui->info_title_label->setAttribute(Qt::WA_TransparentForMouseEvents);
        ui->info_desc_label->setAttribute(Qt::WA_TransparentForMouseEvents);
        ui->info_icon->setAttribute(Qt::WA_TransparentForMouseEvents);

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

    void startShowAnimation() {
        this->setWindowOpacity(0.0);

        const QScreen *screen = QGuiApplication::primaryScreen();
        const QRect desktop = screen->availableGeometry();

        const QPoint endPos(desktop.right() - this->width() - 20, desktop.bottom() - this->height() - 20);
        const QPoint startPos(desktop.right() + 20, endPos.y());

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

    void startExitAnimation() {
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

        connect(group, &QParallelAnimationGroup::finished, this, &UpdateNotification::close);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void refreshTranslations() {
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

        adjustSize();

        // Обновляем регион только если окно уже полностью показано и не в процессе скрытия
        if (this->isVisible() && !m_isExiting) {
            AcrylicHelper::updateRegion(this);
        }
    }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::MiddleButton) startExitAnimation();
        QWidget::mousePressEvent(event);
    }

    void enterEvent(QEnterEvent *event) override {
        if (m_externalCloseBtn && !m_isExiting) m_externalCloseBtn->setFade(true);
        QWidget::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override {
        if (m_externalCloseBtn.isNull() || m_isExiting) return;

        const QPoint globalPos = QCursor::pos();
        const bool overMe = this->geometry().contains(globalPos);

        if (const bool overBtn = m_externalCloseBtn->geometry().contains(globalPos); !overMe && !overBtn) {
            m_externalCloseBtn->setFade(false);
        }
        QWidget::leaveEvent(event);
    }

    bool event(QEvent *event) override {
        if (event->type() == QEvent::WindowActivate) {
            if (m_externalCloseBtn) m_externalCloseBtn->raise();
        }
        return QWidget::event(event);
    }

    // Поддержка системного LanguageChange
    void changeEvent(QEvent *event) override {
        if (event->type() == QEvent::LanguageChange) refreshTranslations();
        QWidget::changeEvent(event);
    }

    void moveEvent(QMoveEvent *event) override {
        if (m_externalCloseBtn) m_externalCloseBtn->updatePosition();
        QWidget::moveEvent(event);
    }

    void showEvent(QShowEvent *event) override {
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

    void hideEvent(QHideEvent *event) override {
        QWidget::hideEvent(event);
        if (m_externalCloseBtn) m_externalCloseBtn->hide();
    }

    void closeEvent(QCloseEvent *event) override {
        if (m_externalCloseBtn) {
            m_externalCloseBtn->hide();
            m_externalCloseBtn->deleteLater();
        }
        QWidget::closeEvent(event);
    }

private slots:
    void startFastDownload() {
        QString fileName = QFileInfo(m_downloadUrl).fileName();
        if (!fileName.endsWith(".exe", Qt::CaseInsensitive)) fileName = "EasyLangSwitcher.exe";

        const QString path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/" + fileName;
        executeDownload(path);
    }

    void startCustomDownload() {
        QString fileName = QFileInfo(m_downloadUrl).fileName();
        if (!fileName.endsWith(".exe", Qt::CaseInsensitive)) fileName = "EasyLangSwitcher.exe";

        if (const QString path = QFileDialog::getSaveFileName(
            this, Lang::tr("SAVE_FILE_TITLE"), fileName,
            "Executable (*.exe)"); !path.isEmpty())
            executeDownload(path);
    }

    void executeDownload(const QString &filePath) {
        ui->btn_frame->hide();
        ui->progress_frame->show();
        adjustSize(); // Пересчитываем геометрию под прогресс-бар

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

        connect(m_reply, &QNetworkReply::finished, this, &UpdateNotification::onDownloadFinished);
    }

    void onDownloadFinished() {
        if (m_file) m_file->close();
        if (m_reply->error() == QNetworkReply::NoError) {
            ui->info_desc_label->setText(Lang::tr("DOWNLOAD_COMPLETE"));
            // Плавно уходим после успеха
            QTimer::singleShot(5000, this, &UpdateNotification::startExitAnimation);
        } else if (m_reply->error() != QNetworkReply::OperationCanceledError) {
            ui->info_desc_label->setText(Lang::tr("DOWNLOAD_ERROR"));
            if (m_file) m_file->remove();
        }
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    void cancelDownload() {
        if (m_reply) m_reply->abort();
        ui->progress_frame->hide();
        ui->btn_frame->show();
        adjustSize();
    }

    void applySystemAccentColor() const {
        // Читаем цвет акцента из реестра Windows
        const QSettings dwmSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\DWM", QSettings::NativeFormat);
        bool ok;
        const unsigned int rgba = dwmSettings.value("AccentColor").toUInt(&ok);

        if (ok) {
            // Формат в реестре: 0xAABBGGRR. Нам нужно пересобрать в RGB для QColor
            const int r = rgba & 0xFF;
            const int g = (rgba >> 8) & 0xFF;
            const int b = (rgba >> 16) & 0xFF;
            const QColor accent(r, g, b);

            // Применяем к прогресс-бару (используем твой стиль, но с динамическим цветом)
            const QString style = QString(
                "QProgressBar::chunk {"
                "   background-color: %1;"
                "   border-radius: 2px;"
                "}"
            ).arg(accent.name());

            ui->progress_bar->setStyleSheet(style);
        }
    }

    void moveToBottomRight() {
        const QScreen *screen = QGuiApplication::primaryScreen();
        if (!screen) return;

        // Получаем геометрию без учета панели задач
        const QRect desktopRect = screen->availableGeometry();

        // Подгоняем размер окна под контент (важно после скрытия/показа фреймов)
        this->adjustSize();

        // Считаем координаты: край экрана минус размер окна минус отступы
        constexpr int margin = 20;
        const int x = desktopRect.right() - this->width() - margin;
        const int y = desktopRect.bottom() - this->height() - margin;

        this->move(x, y);
    }

private:
    Ui::notification_main_widget *ui;
    Mode m_mode;
    QString m_version;
    QString m_downloadUrl;
    QNetworkReply *m_reply = nullptr;
    QFile *m_file = nullptr;
    QPointer<NotificationCloseButton> m_externalCloseBtn;
    bool m_isExiting = false;
};
