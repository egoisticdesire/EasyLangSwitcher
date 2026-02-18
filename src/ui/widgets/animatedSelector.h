#pragma once
#include <QButtonGroup>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include <QLineEdit>
#include <QWidget>
#include <QFrame>
#include <QPointer>

class AnimatedSelector final : public QObject {
    Q_OBJECT

public:
    explicit AnimatedSelector(QWidget *parent);

    void bindToFrame(QFrame *frame, const QString &extraStyle = QString());

    void initPosition();

    void stopAndResetAnimation();

    void animateToCurrentState();

    [[nodiscard]] QFrame *boundFrame() const;

    [[nodiscard]] QFrame *indicator() const { return m_indicator; }

private:
    QFrame *m_indicator = nullptr;

    QFrame *m_frame = nullptr;

    QButtonGroup *m_group = nullptr;

    QPointer<QLineEdit> m_customEdit;

    QWidget *m_parent = nullptr;

    QGraphicsDropShadowEffect *m_shadow = nullptr;

    QRect m_indicatorGeometry;

    QPointer<QVariantAnimation> m_runningAnim;

    QString m_extraStyle;

    bool m_animating = false;

    bool m_inTextHandler = false;

    bool m_forceCustomStartFromFrame = false;

    bool m_customHasText = false;

private slots:
    void animateToButton(QAbstractButton *btn);

    void animateToCustomEdit();

    void onCustomEditChanged(const QString &text);

    void updateButtonColors() const;

    void updateEditStyle() const;
};
