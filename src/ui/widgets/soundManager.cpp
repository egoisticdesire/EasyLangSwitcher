#include "soundManager.h"
#include <QMediaDevices>

soundManager& soundManager::instance() {
    static soundManager inst;
    return inst;
}

soundManager::soundManager(QObject *parent)
    : QObject(parent)
{
    // создаём объект устройств
    const auto *devices = new QMediaDevices(this);

    connect(devices, &QMediaDevices::audioOutputsChanged,
            this, &soundManager::reinitAll);
}

void soundManager::registerEffect(QSoundEffect *effect) {
    if (!effects.contains(effect))
        effects.append(effect);
}

void soundManager::reinitAll() {
    for (QSoundEffect *e : effects) {
        if (!e) continue;

        const QUrl src = e->source();
        const float vol = e->volume();
        const int loop = e->loopCount();

        e->stop();
        e->setSource(QUrl());
        e->setSource(src);
        e->setVolume(vol);
        e->setLoopCount(loop);
    }
}

