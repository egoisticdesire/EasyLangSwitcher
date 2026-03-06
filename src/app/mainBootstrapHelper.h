#pragma once

#include <QStringList>

namespace MainBootstrapHelper
{
bool ensureToastShortcut(const wchar_t* appUserModelId);

void ensureProtocolRegistration();

QStringList startupArgsFromArgv(int argc, char* const* argv);

QString instanceLockPath();

bool isToastActivationLaunch(const QStringList& startupArgs);
} // namespace MainBootstrapHelper
