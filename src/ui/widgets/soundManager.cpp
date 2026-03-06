#include "soundManager.h"

#include "../helpers/windowsNotificationState.h"

#include <QMediaDevices>
#include <algorithm>

soundManager& soundManager::instance()
{
    static soundManager inst;
    return inst;
}

soundManager::soundManager(QObject* parent) : QObject(parent)
{
    // создаём объект устройств
    const auto* devices = new QMediaDevices(this);

    connect(devices, &QMediaDevices::audioOutputsChanged, this, &soundManager::reinitAll);
}

void soundManager::registerEffect(QSoundEffect* effect)
{
    if (effect == nullptr) {
        return;
    }

    const auto exists = std::ranges::any_of(
            std::as_const(effects), [effect](const QPointer<QSoundEffect>& ptr) { return ptr.data() == effect; });
    if (exists) {
        return;
    }

    effects.append(QPointer<QSoundEffect>(effect));
    connect(effect, &QObject::destroyed, this, [this]() {
        effects.erase(
                std::ranges::remove_if(effects, [](const QPointer<QSoundEffect>& ptr) { return ptr.isNull(); }).begin(),
                effects.end());
    });
}

void soundManager::playEffect(QSoundEffect* effect)
{
    if (effect == nullptr) {
        return;
    }
    if (shouldMuteBySystemState()) {
        return;
    }
    effect->play();
}

bool soundManager::shouldMuteBySystemState()
{
#ifdef Q_OS_WIN
    return WindowsNotificationState::evaluatePopupDeferral().shouldDefer;
#else
    return false;
#endif
}

void soundManager::reinitAll()
{
    effects.erase(
            std::ranges::remove_if(effects, [](const QPointer<QSoundEffect>& ptr) { return ptr.isNull(); }).begin(),
            effects.end());

    for (const QPointer<QSoundEffect>& effectPtr : effects) {
        QSoundEffect* e = effectPtr.data();
        if (e == nullptr) {
            continue;
        }

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
