#pragma once
#include <QObject>

class QThread;
class KeyboardHookWorker;

class KeyboardHandler final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(KeyboardHandler)

public:
    explicit KeyboardHandler(QObject* parent = nullptr);

    ~KeyboardHandler() override;

    void start();

    void stop();

    void setActive(bool value);

private:
    bool isActive = true;
    QThread* hookThread = nullptr;
    KeyboardHookWorker* worker = nullptr;
};
