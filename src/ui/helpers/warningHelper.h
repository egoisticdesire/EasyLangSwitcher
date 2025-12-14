#pragma once
#include "ui_EasyLangSwitcher_warning.h"
#include "../widgets/soundManager.h"
#include "../widgets/windowDragger.h"
#include "iconHelper.h"
#include "acrylicHelper.h"
#include <QDialog>
#include <QScreen>
#include <QSoundEffect>

class WarningDialog final : public QDialog {
    Q_OBJECT

public:
    explicit WarningDialog(QWidget *parent = nullptr) : QDialog(parent) {
        ui.setupUi(this);
        this->setWindowIcon(IconHelper::loadIcon(":/icons/icons/FlashSparkleFilled2.png"));

        ui.icon_label->setIcon(
            IconHelper::loadIcon(":/icons/icons/WarningFilled.svg", QColor(234, 191, 0), QSize(48, 48)));
        ui.icon_label->setAttribute(Qt::WA_TransparentForMouseEvents);

        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground);

        QTimer::singleShot(0, this, [this] { AcrylicHelper::enableAcrylic(this); });


        auto *closeAction = new QAction(this);
        closeAction->setShortcut(Qt::Key_Escape);
        connect(closeAction, &QAction::triggered, this, &WarningDialog::close);
        addAction(closeAction);

        connect(ui.btn_close, &QPushButton::clicked, closeAction, &QAction::trigger);

        dragger = new WindowDragger(this);
        dragger->addIgnoredWidget(ui.btn_close);

        audioEffect = new QSoundEffect(this);
        audioEffect->setSource(QUrl("qrc:/sounds/sounds/error.wav"));
        audioEffect->setVolume(0.5f);

        soundManager::instance().registerEffect(audioEffect);
    }

    void setText(const QString &text) const {
        ui.text_label->setText(text);
    }

    void openCentered() {
        const QScreen *screen = QGuiApplication::primaryScreen();
        if (!screen) screen = QGuiApplication::screens().first();
        const QRect geom = screen->availableGeometry();

        adjustSize();
        const QSize s = size();

        const int x = geom.center().x() - s.width() / 2;
        const int y = geom.center().y() - s.height() / 2;

        move(x, y);
        show();
        raise();
        activateWindow();

        if (audioEffect) audioEffect->play();
    }

    void setTranslations(const QString &text) const {
        ui.btn_close->setText(text);
    }

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override {
        if (eventType == "windows_generic_MSG") {
            // Третий параметр - цвет фона, который будет на превью вместо прозрачности.
            if (AcrylicHelper::handleIconicMessages(this, message, QColor(32, 32, 32))) {
                *result = 0;
                return true;
            }
        }
        return QWidget::nativeEvent(eventType, message, result);
    }

private:
    Ui::warning_main_widget ui{};

    WindowDragger *dragger = nullptr;

    QSoundEffect *audioEffect = nullptr;
};
