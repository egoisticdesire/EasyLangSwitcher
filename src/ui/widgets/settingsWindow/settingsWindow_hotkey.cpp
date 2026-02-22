#include "settingsWindow.h"
#include "../../../core/i18n/lang.h"
#include "../../../core/config/logger.h"
#include "../../../core/config/appSettings.h"
#include "../../helpers/iconHelper.h"
#include "../../helpers/keySequenceHelper.h"
#include "../../helpers/vkMapper.h"
#include <QPushButton>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <algorithm>

void SettingsWindow::initHotkeyLogic() {
    const auto KEY_PLACEHOLDER = Lang::tr("SETTINGS_KEY_SEQUENCE");
    m_keyHelper = new KeySequenceHelper(
        this, "btn_sequence",
        IconHelper::loadIcon(":/icons/icons/ClearFilled.svg", QColor(175, 175, 175), QSize(12, 12)),
        KEY_PLACEHOLDER, this);

    // Коннект для кастомного ввода
    connect(m_keyHelper, &KeySequenceHelper::hotkeySelected, this,
            [this](const int mainVk, int /*mods*/, const QString &name) {
                if (mainVk == 0) {
                    LOG_DEBUG() << "Sequence cleared by user";
                    restorePreviousPresetIfNeeded();
                    return;
                }

                if (const auto it = std::find(presetMap.constBegin(), presetMap.constEnd(), AppSettings::hotkeyMainVk);
                    it != presetMap.constEnd()) {
                    AppSettings::previousHotkeyMainVk = AppSettings::hotkeyMainVk;
                    AppSettings::previousHotkeyName = AppSettings::hotkeyName;
                    LOG_DEBUG() << "Saved previous preset: vk="
                                << AppSettings::previousHotkeyMainVk << "; name='"
                                << AppSettings::previousHotkeyName << "'";
                } else {
                    LOG_DEBUG() << "Current hotkey is custom - not saving previous preset";
                }

                LOG_DEBUG() << QString("Custom hotkey selected: vk=%1; name='%2'").arg(mainVk).arg(name);
                applyHotkeyIfChanged(mainVk, name);
            });

    // Обработка кнопок-пресетов
    const QList<QPushButton *> presetButtons = ui.key_select_frame->findChildren<QPushButton *>();
    bool matchedPreset = false;

    for (auto *btn: presetButtons) {
        if (const int vk = presetMap.value(btn->objectName(), 0); vk != 0 && vk == AppSettings::hotkeyMainVk) {
            btn->setChecked(true);
            matchedPreset = true;
            LOG_DEBUG() << QString("Matched preset button '%1' for hotkey '%2'")
                            .arg(btn->objectName(), AppSettings::hotkeyName);
        } else {
            btn->setChecked(false);
        }

        connect(btn, &QPushButton::clicked, this, [this, btn]() {
            if (!btn) return;
            const int vk_ = presetMap.value(btn->objectName(), 0);
            if (vk_ == 0) return;
            const QString name = nameFromVk(vk_);

            LOG_DEBUG() << QString("Preset '%1' selected: vk=%2; name='%3'").arg(btn->objectName()).arg(vk_).arg(name);
            applyHotkeyIfChanged(vk_, name);

            AppSettings::previousHotkeyMainVk = vk_;
            AppSettings::previousHotkeyName = name;

            if (auto *seq = findChild<QKeySequenceEdit *>("btn_sequence")) {
                seq->setKeySequence(QKeySequence());
                seq->setStyleSheet("color: rgba(255, 255, 255, 225);");
                seq->clearFocus();
                this->setFocus(Qt::OtherFocusReason);
                if (auto *le = seq->findChild<QLineEdit *>())
                    le->setPlaceholderText(Lang::tr("SETTINGS_KEY_SEQUENCE"));
            }
        });
    }

    if (!matchedPreset) {
        if (auto *seq = findChild<QKeySequenceEdit *>("btn_sequence")) {
            if (const QString name = AppSettings::hotkeyName; !name.isEmpty()) {
                seq->setKeySequence(QKeySequence(name));
                LOG_DEBUG() << "Populated sequence edit with saved custom key: '" << name << "'";
            }
        }
    }
}

void SettingsWindow::buildPresetMap() {
    presetMap.clear();
    for (const auto presetButtons = ui.key_select_frame->findChildren<QPushButton *>();
         const auto *btn: presetButtons) {
        const QString obj = btn->objectName().toLower();
        int vk = 0;
        if (obj.contains("lctrl")) vk = VK_LCONTROL;
        else if (obj.contains("rctrl")) vk = VK_RCONTROL;
        else if (obj.contains("lalt")) vk = VK_LMENU;
        else if (obj.contains("ralt")) vk = VK_RMENU;
        else if (obj.contains("lshift")) vk = VK_LSHIFT;
        else if (obj.contains("rshift")) vk = VK_RSHIFT;
        else if (obj.contains("caps")) vk = VK_CAPITAL;

        if (vk != 0) presetMap.insert(btn->objectName(), vk);
    }
    LOG_DEBUG() << "Preset map built: size=" << presetMap.size();
}

void SettingsWindow::applyHotkeyIfChanged(const int newVk, const QString &newName) {
    if (newVk == AppSettings::hotkeyMainVk && newName == AppSettings::hotkeyName) {
        LOG_DEBUG() << "Hotkey not changed";
        return;
    }
    AppSettings::hotkeyMainVk = newVk;
    AppSettings::hotkeyModifiers = 0;
    AppSettings::hotkeyName = newName;

    for (const auto presetButtons = ui.key_select_frame->findChildren<QPushButton *>(); auto *btn: presetButtons) {
        const int vk = vkFromPresetObjectName(btn->objectName());
        btn->setChecked(vk != 0 && vk == newVk);
    }
    markChanged();
}

int SettingsWindow::vkFromPresetObjectName(const QString &obj) const { return presetMap.value(obj, 0); }

QString SettingsWindow::nameFromVk(const int vk) { return VkMapper::vkToName(vk); }

void SettingsWindow::restorePreviousPresetIfNeeded() {
    if (AppSettings::previousHotkeyMainVk != 0) {
        if (AppSettings::hotkeyMainVk != AppSettings::previousHotkeyMainVk) {
            AppSettings::hotkeyMainVk = AppSettings::previousHotkeyMainVk;
            AppSettings::hotkeyModifiers = 0;
            AppSettings::hotkeyName = AppSettings::previousHotkeyName;
            for (auto *btn: ui.key_select_frame->findChildren<QPushButton *>()) {
                const int vk = vkFromPresetObjectName(btn->objectName());
                btn->setChecked(vk != 0 && vk == AppSettings::previousHotkeyMainVk);
            }
            LOG_DEBUG() << QString("Previous preset restored: vk=%1; name='%2'")
                            .arg(AppSettings::previousHotkeyMainVk).arg(AppSettings::previousHotkeyName);
            AppSettings::previousHotkeyMainVk = 0;
            AppSettings::previousHotkeyName.clear();
            markChanged();
        } else {
            AppSettings::previousHotkeyMainVk = 0;
            AppSettings::previousHotkeyName.clear();
        }
    }
}

void SettingsWindow::restoreDefaultsForCurrentPage() {
    if (ui.content_container->currentIndex() == 0) restoreDefaults_General();
    markChanged();
}

void SettingsWindow::restoreDefaults_General() {
    AppSettings::reset();
    for (auto *btn: ui.key_select_frame->findChildren<QPushButton *>()) {
        const int vk = vkFromPresetObjectName(btn->objectName());
        btn->blockSignals(true);
        btn->setChecked(vk != 0 && vk == AppSettings::hotkeyMainVk);
        btn->blockSignals(false);
    }

    if (auto *seq = findChild<QKeySequenceEdit *>("btn_sequence")) {
        const auto it = std::find(presetMap.constBegin(), presetMap.constEnd(), AppSettings::defaultHotkeyMainVk);

        if (const bool isDefaultInPresets = (it != presetMap.constEnd());
            AppSettings::hotkeyMainVk == AppSettings::defaultHotkeyMainVk && isDefaultInPresets) {
            seq->setKeySequence(QKeySequence());
            if (auto *le = seq->findChild<QLineEdit *>()) le->setPlaceholderText(Lang::tr("SETTINGS_KEY_SEQUENCE"));
        } else {
            seq->setKeySequence(QKeySequence(AppSettings::hotkeyName));
        }
        seq->clearFocus();
    }

    ui.key_delay_slider->setValue(AppSettings::defaultSwitchDelayMs);
    ui.key_delay_spinbox->setValue(AppSettings::defaultSwitchDelayMs);

    for (auto *sel: selectors) {
        if (sel && sel->boundFrame() == ui.key_select_frame) {
            sel->stopAndResetAnimation();
            QTimer::singleShot(0, sel, &AnimatedSelector::animateToCurrentState);
            break;
        }
    }
}
