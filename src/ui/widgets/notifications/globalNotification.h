#pragma once
#include "ui_EasyLangSwitcher_notification.h"
#include "../../helpers/closeButton.h"
#include <QWidget>
#include <QPointer>
#include <QString>

namespace Ui {
    class notification_main_widget;
}


class QNetworkReply;
class QFile;

class GlobalNotification final : public QWidget {
    Q_OBJECT

public:
    enum class UiState { Buttons, Progress, Hidden };

    enum Mode { UpdateAvailable, UpToDate, Error };

    explicit GlobalNotification(Mode mode, const QString &version, const QString &url = "", QWidget *parent = nullptr);

    ~GlobalNotification() override;

    void toggleInterface(UiState state);

    void startShowAnimation();

    void startExitAnimation();

    void refreshTranslations();

protected:
    void mousePressEvent(QMouseEvent *event) override;

    bool event(QEvent *event) override;

    void changeEvent(QEvent *event) override;

    void moveEvent(QMoveEvent *event) override;

    void showEvent(QShowEvent *event) override;

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
    QNetworkReply *m_reply = nullptr;
    QFile *m_file = nullptr;
    QPointer<NotificationCloseButton> m_externalCloseBtn;
    bool m_isExiting = false;
};
