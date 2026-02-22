#pragma once
#include <QList>
#include <QObject>
#include <QPointer>
#include <QSoundEffect>

class soundManager final : public QObject {
    Q_OBJECT
public:
    static soundManager& instance();

    // регистрируется каждый QSoundEffect
    void registerEffect(QSoundEffect *effect);
    void playEffect(QSoundEffect *effect) const;

private:
    explicit soundManager(QObject *parent = nullptr);

    QList<QPointer<QSoundEffect>> effects;
    [[nodiscard]] bool shouldMuteBySystemState() const;

    void reinitAll();
};
