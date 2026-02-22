#pragma once

#include <QString>
#include <functional>

namespace WindowsToastNotification {
    bool showToastNotification(
        const QString &title,
        const QString &body,
        const std::function<void()> &onActivated,
        bool suppressPopup
    );

    void clearToastHistoryForApp();
}
