#pragma once
#include "ui_EasyLangSwitcher_settings_upd_check.h"
#include "../../core/config/appSettings.h"
#include <QWidget>

class UpdFrequencyPopup final : public QWidget {
    Q_OBJECT

public:
    explicit UpdFrequencyPopup(QWidget *parent = nullptr);

    void setCurrent(AppSettings::UpdateFrequency value) const;

    int currentButtonYOffset() const;

    void refreshTranslations();

signals:
    void selected(AppSettings::UpdateFrequency value);

protected:
    void showEvent(QShowEvent *event) override;

private:
    Ui::upd_freq_main_widget ui;

    void setupConnections();
};
