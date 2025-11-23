#pragma once
#include "ui_EasyLangSwitcher_warning.h"
#include "iconHelper.h"
#include "acrylicHelper.h"
#include "../widgets/window_dragger.h"
#include <QDialog>

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

        QTimer::singleShot(0, this, [this] {
            AcrylicHelper::enableAcrylic(this);
        });


        auto *closeAction = new QAction(this);
        closeAction->setShortcut(Qt::Key_Escape);
        connect(closeAction, &QAction::triggered, this, &WarningDialog::close);
        addAction(closeAction);

        connect(ui.btn_close, &QPushButton::clicked, closeAction, &QAction::trigger);

        dragger = new WindowDragger(this);
        dragger->addIgnoredWidget(ui.btn_close);
    }

    void setText(const QString &text) const {
        ui.text_label->setText(text);
    }

private:
    Ui::warning_main_widget ui{};

    WindowDragger *dragger = nullptr;
};
