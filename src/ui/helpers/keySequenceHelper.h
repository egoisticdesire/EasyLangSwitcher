#pragma once
#include "../../core/config/logger.h"
#include "../helpers/vkMapper.h"
#include <QObject>
#include <QKeySequenceEdit>
#include <QToolButton>
#include <QLineEdit>
#include <QKeyEvent>

/*
KeySequenceHelper:
— блокирует модификаторы в отображаемой последовательности (оставляет только основную клавишу)
— эмитирует signal hotkeySelected(mainVk, modifiersMask, name)
— при очистке эмитит (0,0,"")
— защищён от рекурсивных setKeySequence и дублей
— восстанавливает кастомный placeholder даже при смене фокуса
*/

class KeySequenceHelper final : public QObject {
    Q_OBJECT

public:
    KeySequenceHelper(
        const QWidget *root,
        const QString &objectName,
        const QIcon &icon,
        const QString &placeholder = "Key...",
        QObject *parent = nullptr
    ) : QObject(parent), m_placeholder(placeholder) {
        LOG_DEBUG() << "KeySequenceHelper initialized";

        m_edit = root->findChild<QKeySequenceEdit *>(objectName);
        m_edit->setStyleSheet("color: rgba(255, 255, 255, 225);");
        if (!m_edit) return;

        m_edit->setClearButtonEnabled(false);

        m_lineEdit = m_edit->findChild<QLineEdit *>();
        if (m_lineEdit) {
            m_lineEdit->setAlignment(Qt::AlignCenter);
            m_lineEdit->setPlaceholderText(m_placeholder);
        }

        m_btn = new QToolButton(m_edit);
        m_btn->setIcon(icon);
        m_btn->setCursor(Qt::ArrowCursor);
        m_btn->setAutoRaise(true);
        m_btn->setVisible(!m_edit->keySequence().isEmpty());
        m_btn->setStyleSheet(R"(
            QToolButton { margin: 1px 0 0 0;
                        padding: 2px 1px 2px 2px;
                        border: none;
                        border-radius: 6px;
                        background: transparent; }
            QToolButton:hover { background: rgba(255, 255, 255, 25); }
            QToolButton:pressed { background: rgba(255, 255, 255, 7); }
        )");

        connect(m_btn, &QToolButton::clicked, this, [this]() {
            if (!m_edit) return;
            m_internalUpdate = true;
            m_edit->setKeySequence(QKeySequence());
            if (m_lineEdit) m_lineEdit->setPlaceholderText(m_placeholder);
            m_btn->setVisible(false);
            m_internalUpdate = false;

            LOG_DEBUG() << "Clear button pressed";

            if (m_lastEmittedVk != 0 || !m_lastEmittedName.isEmpty()) {
                m_lastEmittedVk = 0;
                m_lastEmittedName.clear();
                emit hotkeySelected(0, 0, QString());
            }
        });

        connect(m_edit, &QKeySequenceEdit::keySequenceChanged, this, [this]() {
            if (!m_edit) return;
            if (m_internalUpdate) return;

            const QKeySequence seq = m_edit->keySequence();

            if (seq.isEmpty()) {
                m_btn->setVisible(false);
                if (m_lineEdit) m_lineEdit->setPlaceholderText(m_placeholder);

                if (m_lastEmittedVk != 0 || !m_lastEmittedName.isEmpty()) {
                    m_lastEmittedVk = 0;
                    m_lastEmittedName.clear();
                    emit hotkeySelected(0, 0, QString());
                }
                return;
            }

            // обрезать модификаторы и извлечь первый элемент
            const QKeyCombination raw = seq[0];
            const int baseKey = raw.key() & ~(
                                    Qt::ShiftModifier
                                    | Qt::ControlModifier
                                    | Qt::AltModifier
                                    | Qt::MetaModifier
                                );

            if (blockedKeys.contains(baseKey)) {
                const int vk = VkMapper::sequenceToVk(QKeySequence(baseKey));
                const QString keyName = vk
                                            ? VkMapper::vkToName(vk)
                                            : QKeySequence(baseKey).toString(QKeySequence::NativeText);

                LOG_DEBUG() << "Blocked key pressed: " << keyName;

                m_internalUpdate = true;
                m_edit->setKeySequence(QKeySequence());
                if (m_lineEdit) m_lineEdit->setPlaceholderText(m_placeholder);
                m_btn->setVisible(false);
                m_internalUpdate = false;

                if (m_lastEmittedVk != 0 || !m_lastEmittedName.isEmpty()) {
                    m_lastEmittedVk = 0;
                    m_lastEmittedName.clear();
                    emit hotkeySelected(0, 0, QString());
                }
                return;
            }

            int vk = 0;
            QString name;
            if (baseKey != 0 && baseKey != Qt::Key_unknown) {
                const QKeySequence cleanSeq(baseKey);

                // устанавливаем очищенную последовательность, если она отличается
                if (m_edit->keySequence() != cleanSeq) {
                    m_internalUpdate = true;
                    m_edit->setKeySequence(cleanSeq);
                    if (m_lineEdit && m_lineEdit->placeholderText().isEmpty())
                        m_lineEdit->setPlaceholderText(m_placeholder);
                    m_btn->setVisible(true);
                    m_internalUpdate = false;
                } else {
                    m_btn->setVisible(true);
                }

                vk = VkMapper::sequenceToVk(cleanSeq);
                name = vk ? VkMapper::vkToName(vk) : cleanSeq.toString(QKeySequence::NativeText);
            } else {
                const QString fallback = seq.toString(QKeySequence::NativeText);
                m_internalUpdate = true;
                m_edit->setKeySequence(QKeySequence());
                if (m_lineEdit) m_lineEdit->setPlaceholderText(m_placeholder);
                m_btn->setVisible(false);
                m_internalUpdate = false;
                vk = 0;
                name = fallback;

                LOG_DEBUG() << "Fallback sequence detected, raw seq=" << fallback;
            }

            if (vk != m_lastEmittedVk || name != m_lastEmittedName) {
                m_lastEmittedVk = vk;
                m_lastEmittedName = name;
                emit hotkeySelected(vk, 0, name);

                LOG_DEBUG() << QString("Selected cleaned: vk=%1; name='%2'").arg(vk).arg(name);
            }
        });

        // переопределение плейсхолдера "Press shortcut"
        m_edit->installEventFilter(this);
        if (m_lineEdit) m_lineEdit->installEventFilter(this);

        updatePosition();
    }

signals:
    void hotkeySelected(int mainVk, int modifiersMask, const QString &name);

protected:
    bool eventFilter(QObject *w, QEvent *ev) override {
        if (w == m_edit || w == m_lineEdit) {
            if (ev->type() == QEvent::KeyPress) {
                const auto *ke = static_cast<QKeyEvent *>(ev);

                // ESC — снять фокус
                if (ke->key() == Qt::Key_Escape) {
                    m_edit->clearFocus();
                    if (m_lineEdit) m_lineEdit->clearFocus();
                    if (m_lineEdit) m_lineEdit->setPlaceholderText(m_placeholder);
                    return true;
                }

                // BACKSPACE — очистка (симулируем нажатие кнопки очистки)
                if (ke->key() == Qt::Key_Backspace) {
                    if (m_btn && m_btn->isVisible()) {
                        emit m_btn->clicked();
                    }
                    return true;
                }
            }

            if (ev->type() == QEvent::Resize || ev->type() == QEvent::FontChange) {
                updatePosition();
            }

            // переопределение плейсхолдера "Press shortcut"
            if (ev->type() == QEvent::FocusIn) {
                if (m_lineEdit) {
                    QTimer::singleShot(0, [this]() {
                        if (m_lineEdit) m_lineEdit->setPlaceholderText(m_placeholder);
                    });
                }
            } else if (ev->type() == QEvent::FocusOut) {
                if (m_lineEdit) m_lineEdit->setPlaceholderText(m_placeholder);
            }
        }

        return QObject::eventFilter(w, ev);
    }

private:
    void updatePosition() const {
        if (!m_edit || !m_btn) return;

        const int h = m_edit->height();
        const int btnW = m_btn->sizeHint().width();
        const int btnH = m_btn->sizeHint().height() - 1;
        constexpr int margin = 4;
        m_btn->setGeometry(m_edit->width() - btnW - margin, (h - btnH) / 2, btnW, btnH);
    }

    QKeySequenceEdit *m_edit = nullptr;
    QLineEdit *m_lineEdit = nullptr;
    QToolButton *m_btn = nullptr;
    QString m_placeholder;

    bool m_internalUpdate = false;
    int m_lastEmittedVk = -1;
    QString m_lastEmittedName;

    inline static const QSet<int> blockedKeys = {
        Qt::Key_CapsLock,
        Qt::Key_Backspace,
        Qt::Key_Delete,
        Qt::Key_Insert,
        Qt::Key_Home,
        Qt::Key_End,
        Qt::Key_PageUp,
        Qt::Key_PageDown,
        Qt::Key_Escape,
    };
};
