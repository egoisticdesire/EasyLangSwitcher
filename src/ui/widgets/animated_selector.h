#pragma once
#include <QButtonGroup>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QLineEdit>
#include <QWidget>
#include <QFrame>

class AnimatedSelector final : public QObject {
    Q_OBJECT

public:
    explicit AnimatedSelector(QWidget *parent);

    void bindToFrame(QFrame *frame, const QString &extraStyle = QString());

    void initPosition() const;

    QFrame *indicator() const { return m_indicator; }

private:
    QWidget *m_parent = nullptr;

    QFrame *m_indicator = nullptr;

    QFrame *m_frame = nullptr;

    QButtonGroup *m_group = nullptr;

    QLineEdit *m_customEdit = nullptr;

    QGraphicsOpacityEffect *m_opacity = nullptr;

private slots:
    void animateToButton(const QAbstractButton *btn);

    void animateToCustomEdit();

    void onCustomEditChanged(const QString &text);

    void updateButtonColors() const;

    void updateEditStyle() const;
};
