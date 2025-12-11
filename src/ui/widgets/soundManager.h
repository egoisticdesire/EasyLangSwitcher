#pragma once
#include <QSoundEffect>

class soundManager final : public QObject {
    Q_OBJECT
public:
    static soundManager& instance();

    // регистрируется каждый QSoundEffect
    void registerEffect(QSoundEffect *effect);

private:
    explicit soundManager(QObject *parent = nullptr);

    QList<QSoundEffect*> effects;

    void reinitAll();
};
