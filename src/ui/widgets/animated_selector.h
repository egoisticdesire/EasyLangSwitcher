#pragma once
#include <QButtonGroup>
#include <QPropertyAnimation>
#include <QLineEdit>
#include <QWidget>

class AnimatedSelector final : public QObject {
    Q_OBJECT

public:
    explicit AnimatedSelector(QWidget *parent);

    // подключение к любому контейнеру
    void bindToFrame(QFrame *frame, const QString &extraStyle = QString());

    // опционально — старт с выбранной кнопки
    void initPosition() const;

    // доступ к индикатору
    QFrame *indicator() const { return m_indicator; }

private:
    QWidget *m_parent = nullptr;
    QFrame *m_indicator = nullptr;
    QFrame *m_frame = nullptr;
    QButtonGroup *m_group = nullptr;
    QLineEdit *m_customEdit = nullptr;

private slots:
    void animateToButton(const QAbstractButton *btn);

    void onCustomEditChanged(const QString &text) const;

    void updateButtonColors() const;

    void updateEditStyle() const;
};
