#pragma once
#include <QWidget>
#include <QSet>

class WindowDragger final : public QObject {
    Q_OBJECT

public:
    explicit WindowDragger(QWidget *target);

    void addIgnoredWidget(QWidget *w);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    bool dragging = false;
    QPoint dragOffset;

    QWidget *win = nullptr;
    QSet<QWidget *> ignore;
};
