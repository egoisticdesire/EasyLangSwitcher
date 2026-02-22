#pragma once
#include <QPointer>
#include <QSoundEffect>

class soundManager final : public QObject {
    Q_OBJECT

public:
    static soundManager &instance();

    // регистрируется каждый QSoundEffect
    void registerEffect(QSoundEffect *effect);

    static void playEffect(QSoundEffect *effect);

private:
    explicit soundManager(QObject *parent = nullptr);

    QList<QPointer<QSoundEffect> > effects;

    [[nodiscard]] static bool shouldMuteBySystemState();

    void reinitAll();
};
