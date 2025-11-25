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

    void initPosition();

    QFrame *indicator() const { return m_indicator; }

private:
    QFrame *m_indicator = nullptr;

    QFrame *m_frame = nullptr;

    QButtonGroup *m_group = nullptr;

    QLineEdit *m_customEdit = nullptr;

    QWidget *m_parent = nullptr;

    QGraphicsDropShadowEffect *m_shadow = nullptr;

    QRect m_indicatorGeometry;

    QVariantAnimation *m_runningAnim = nullptr;

    QGraphicsOpacityEffect *m_opacity = nullptr;

    QString m_extraStyle;

    bool m_animating = false;

private slots:
    void animateToButton(QAbstractButton *btn);

    void animateToCustomEdit();

    void onCustomEditChanged(const QString &text);

    void updateButtonColors() const;

    void updateEditStyle() const;
};
