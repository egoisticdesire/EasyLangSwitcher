#include "updFrequencyPopup.h"

#include <QTimer>

#include "../../core/i18n/lang.h"
#include "../helpers/acrylicHelper.h"

UpdFrequencyPopup::UpdFrequencyPopup(QWidget* parent) : QWidget(parent)
{
    ui.setupUi(this);

    setWindowFlags(Qt::Popup | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    setupConnections();
}

void UpdFrequencyPopup::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    QTimer::singleShot(0, this, [this]() {
        AcrylicHelper::enableAcrylic(this);
        AcrylicHelper::updateRegion(this);
    });
}

void UpdFrequencyPopup::setupConnections()
{
    connect(ui.btn_upd_never, &QPushButton::clicked, this, [this] {
        emit selected(AppSettings::UpdateFrequency::Never);
        close();
    });

    connect(ui.btn_upd_daily, &QPushButton::clicked, this, [this] {
        emit selected(AppSettings::UpdateFrequency::Daily);
        close();
    });

    connect(ui.btn_upd_weekly, &QPushButton::clicked, this, [this] {
        emit selected(AppSettings::UpdateFrequency::Weekly);
        close();
    });

    connect(ui.btn_upd_monthly, &QPushButton::clicked, this, [this] {
        emit selected(AppSettings::UpdateFrequency::Monthly);
        close();
    });
}

int UpdFrequencyPopup::currentButtonYOffset() const
{
    const QPushButton* btn = nullptr;

    switch (AppSettings::updateFrequency) {
        case AppSettings::UpdateFrequency::Never:
            btn = ui.btn_upd_never;
            break;
        case AppSettings::UpdateFrequency::Daily:
            btn = ui.btn_upd_daily;
            break;
        case AppSettings::UpdateFrequency::Weekly:
            btn = ui.btn_upd_weekly;
            break;
        case AppSettings::UpdateFrequency::Monthly:
            btn = ui.btn_upd_monthly;
            break;
    }

    if (btn == nullptr) {
        return 0;
    }

    // позиция кнопки относительно pop-up
    return btn->mapTo(this, QPoint(0, 0)).y();
}

void UpdFrequencyPopup::setCurrent(const AppSettings::UpdateFrequency value) const
{
    ui.btn_upd_never->setChecked(value == AppSettings::UpdateFrequency::Never);
    ui.btn_upd_daily->setChecked(value == AppSettings::UpdateFrequency::Daily);
    ui.btn_upd_weekly->setChecked(value == AppSettings::UpdateFrequency::Weekly);
    ui.btn_upd_monthly->setChecked(value == AppSettings::UpdateFrequency::Monthly);
}

void UpdFrequencyPopup::refreshTranslations()
{
    ui.btn_upd_never->setText(Lang::tr("SETTINGS_APP_UPD_CHECK_NEVER"));
    ui.btn_upd_daily->setText(Lang::tr("SETTINGS_APP_UPD_CHECK_DAILY"));
    ui.btn_upd_weekly->setText(Lang::tr("SETTINGS_APP_UPD_CHECK_WEEKLY"));
    ui.btn_upd_monthly->setText(Lang::tr("SETTINGS_APP_UPD_CHECK_MONTHLY"));

    if (this->isVisible()) {
        this->adjustSize();
        AcrylicHelper::updateRegion(this);
    }
}
