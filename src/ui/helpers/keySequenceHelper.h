#pragma once
#include <QEvent>
#include <QObject>
#include <QKeySequenceEdit>
#include <QToolButton>
#include <QLineEdit>

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
        m_edit = root->findChild<QKeySequenceEdit *>(objectName);
        if (!m_edit) return;

        // отключаем стандартную кнопку очистки
        m_edit->setClearButtonEnabled(false);

        // находим внутренний QLineEdit
        m_lineEdit = m_edit->findChild<QLineEdit *>();
        if (m_lineEdit) {
            m_lineEdit->setAlignment(Qt::AlignCenter);
            m_lineEdit->setPlaceholderText(m_placeholder);
        }

        // создаем внешнюю кнопку очистки
        m_btn = new QToolButton(m_edit);
        m_btn->setIcon(icon);
        m_btn->setCursor(Qt::ArrowCursor);
        m_btn->setAutoRaise(true);
        m_btn->setVisible(!m_edit->keySequence().isEmpty());
        m_btn->setStyleSheet(R"(
            QToolButton {
                margin: 1px 0 0 0;
                padding: 3px 1px 1px 2px;
                border: none;
                border-radius: 6px;
                background: transparent;
            }
            QToolButton:hover {
                background: rgba(255, 255, 255, 25);
            }
            QToolButton:pressed {
                background: rgba(255, 255, 255, 7);
            }
        )");

        // очистка последовательности по кнопке
        connect(m_btn, &QToolButton::clicked, this, [this]() {
            if (!m_edit) return;
            m_edit->setKeySequence(QKeySequence());
            if (m_lineEdit)
                m_lineEdit->setPlaceholderText(m_placeholder);
        });

        // обновление кнопки и placeholder при изменении последовательности
        connect(m_edit, &QKeySequenceEdit::keySequenceChanged, this, [this]() {
            if (!m_edit) return;
            m_btn->setVisible(!m_edit->keySequence().isEmpty());
            if (m_edit->keySequence().isEmpty() && m_lineEdit)
                m_lineEdit->setPlaceholderText(m_placeholder);
        });

        // установка фильтра событий для кнопки и edit
        m_edit->installEventFilter(this);
        if (m_lineEdit) m_lineEdit->installEventFilter(this);

        updatePosition();
    }

protected:
    bool eventFilter(QObject *w, QEvent *ev) override {
        if (w == m_edit || w == m_lineEdit) {
            if (ev->type() == QEvent::Resize || ev->type() == QEvent::FontChange) {
                updatePosition();
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

        m_btn->setGeometry(
            m_edit->width() - btnW - margin,
            (h - btnH) / 2,
            btnW,
            btnH
        );
    }

    QKeySequenceEdit *m_edit = nullptr;
    QLineEdit *m_lineEdit = nullptr;
    QToolButton *m_btn = nullptr;
    QString m_placeholder;
};
