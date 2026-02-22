#pragma once
#include "ui_EasyLangSwitcher_notification.h"
#include "../../helpers/closeButton.h"
#include <QWidget>
#include <QGraphicsOpacityEffect>
#include <QPointer>
#include <QString>

namespace Ui {
    class notification_main_widget;
}


class QNetworkReply;
class QFile;
class QNetworkAccessManager;

class GlobalNotification final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double progress READ progress WRITE setProgress)

public:
    enum class UiState { Buttons, Progress, Finish, Hidden };

    enum Mode { UpdateAvailable, UpToDate, Error };

    explicit GlobalNotification(Mode mode, QString version, QString url = {}, QWidget *parent = nullptr);

    ~GlobalNotification() override;

    void toggleInterface(UiState state);

    void startShowAnimation();

    void startExitAnimation();

    void refreshTranslations();

protected:
    void mousePressEvent(QMouseEvent *event) override;

    void paintEvent(QPaintEvent *event) override;

    bool event(QEvent *event) override;

    void changeEvent(QEvent *event) override;

    void moveEvent(QMoveEvent *event) override;

    void showEvent(QShowEvent *event) override;

    void enterEvent(QEnterEvent *event) override;

    void leaveEvent(QEvent *event) override;

    void hideEvent(QHideEvent *event) override;

    void closeEvent(QCloseEvent *event) override;

private slots:
    void startFastDownload();

    void startCustomDownload();

    void executeDownload(const QString &filePath);

    void onDownloadFinished();

    void cancelDownload();

    void applySystemAccentColor() const;

    void moveToBottomRight();

private:
    Ui::notification_main_widget *ui;
    Mode m_mode;
    QString m_version;
    QString m_downloadUrl;
    QString m_downloadPath;
    QNetworkAccessManager *m_networkManager = nullptr;
    QNetworkReply *m_reply = nullptr;
    QFile *m_file = nullptr;
    QPointer<NotificationCloseButton> m_externalCloseBtn;
    bool m_isExiting = false;
    UiState m_currentState = UiState::Buttons;
    QGraphicsOpacityEffect *m_stackOpacityEffect = nullptr;
    QTimer *m_hideTimer;
    const int AUTOHIDE_DELAY = 5000;
    QPropertyAnimation *m_progressAnim = nullptr;
    double m_progress = 0.0;

    void startAutohideTimer();

    [[nodiscard]] double progress() const {
        return m_progress;
    }

    void setProgress(const double p) {
        m_progress = p;
        update();
    }

    void animateStackTransition(int nextIndex);

    void updateContentOnly() const;

    void animateHeightChange();

    void cleanupDownloadResources(bool removePartialFile);
};
