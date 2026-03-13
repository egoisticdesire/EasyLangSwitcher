#include "windowsToastNotification.h"

#ifdef Q_OS_WIN
#include "../../core/config/logger.h"
#include "windowsToastIdentity.h"

#include <QApplication>
#include <QMetaObject>
#include <roapi.h>
#include <string>
#include <windows.data.xml.dom.h>
#include <windows.foundation.h>
#include <windows.ui.notifications.h>
#include <wrl.h>
#include <wrl/event.h>
#include <wrl/wrappers/corewrappers.h>

#pragma comment(lib, "runtimeobject.lib")

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace ABI::Windows::UI::Notifications;
using ToastActivatedHandler = __FITypedEventHandler_2_Windows__CUI__CNotifications__CToastNotification_IInspectable;

namespace
{
constexpr HRESULT kToastHistoryNotFoundHr = static_cast<HRESULT>(0x80070490);

ComPtr<IToastNotification>& activeToast()
{
    static ComPtr<IToastNotification> value;
    return value;
}

ComPtr<ToastActivatedHandler>& activeToastActivatedHandler()
{
    static ComPtr<ToastActivatedHandler> value;
    return value;
}

EventRegistrationToken& activeToastActivatedToken()
{
    static EventRegistrationToken value{};
    return value;
}

bool ensureToastRuntimeInitialized()
{
    static bool initialized = false;
    static HRESULT initResult = E_FAIL;

    if (!initialized) {
        initResult = RoInitialize(RO_INIT_MULTITHREADED);
        if (initResult == RPC_E_CHANGED_MODE) {
            initResult = RoInitialize(RO_INIT_SINGLETHREADED);
        }
        initialized = true;
    }

    if (FAILED(initResult)) {
        LOG_WARNING() << "RoInitialize failed for toast. HRESULT=0x"
                      << QString::number(static_cast<qulonglong>(initResult), 16);
        return false;
    }
    return true;
}

void resetActiveToastHandlers()
{
    if (activeToast() && activeToastActivatedHandler()) {
        if (const HRESULT hr = activeToast()->remove_Activated(activeToastActivatedToken()); FAILED(hr)) {
            LOG_WARNING() << "Toast activation callback unregistration failed. HRESULT=0x"
                          << QString::number(static_cast<qulonglong>(hr), 16);
        }
    }
    activeToast().Reset();
    activeToastActivatedHandler().Reset();
    activeToastActivatedToken() = {};
}

bool appendToastTextNode(const ComPtr<IXmlDocument>& xml, const int index, const QString& text)
{
    ComPtr<IXmlNodeList> textNodes;
    HRESULT hr = xml->GetElementsByTagName(HStringReference(L"text").Get(), &textNodes);
    if (FAILED(hr) || !textNodes)
        return false;

    ComPtr<IXmlNode> textNode;
    hr = textNodes->Item(index, &textNode);
    if (FAILED(hr) || !textNode)
        return false;

    ComPtr<IXmlText> textValue;
    const std::wstring wideText = text.toStdWString();
    hr = xml->CreateTextNode(HStringReference(wideText.c_str()).Get(), &textValue);
    if (FAILED(hr) || !textValue)
        return false;

    ComPtr<IXmlNode> textValueNode;
    hr = textValue.As(&textValueNode);
    if (FAILED(hr) || !textValueNode)
        return false;

    ComPtr<IXmlNode> appended;
    hr = textNode->AppendChild(textValueNode.Get(), &appended);
    return SUCCEEDED(hr);
}

bool setToastLaunchAttributes(const ComPtr<IXmlDocument>& xml, const QString& launchArgument, const bool suppressPopup)
{
    ComPtr<IXmlElement> root;
    HRESULT hr = xml->get_DocumentElement(&root);
    if (FAILED(hr) || !root)
        return false;

    const std::wstring launch = launchArgument.toStdWString();
    hr = root->SetAttribute(HStringReference(L"launch").Get(), HStringReference(launch.c_str()).Get());
    if (FAILED(hr))
        return false;

    hr = root->SetAttribute(HStringReference(L"activationType").Get(), HStringReference(L"protocol").Get());
    if (FAILED(hr))
        return false;

    if (suppressPopup) {
        hr = root->SetAttribute(HStringReference(L"suppressPopup").Get(), HStringReference(L"true").Get());
        if (FAILED(hr))
            return false;
    }

    return true;
}
} // namespace

bool WindowsToastNotification::showToastNotification(const QString& title,
                                                     const QString& body,
                                                     const std::function<void()>& onActivated,
                                                     const bool suppressPopup)
{
    if (!ensureToastRuntimeInitialized())
        return false;
    resetActiveToastHandlers();

    const std::wstring appId = WindowsToastIdentity::appUserModelIdWide();
    if (appId.empty()) {
        return false;
    }

    ComPtr<IToastNotificationManagerStatics> toastManager;
    HRESULT hr = RoGetActivationFactory(
            HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),
            IID_PPV_ARGS(&toastManager));
    if (FAILED(hr) || !toastManager) {
        LOG_WARNING() << "Toast manager activation failed. HRESULT=0x"
                      << QString::number(static_cast<qulonglong>(hr), 16);
        return false;
    }

    ComPtr<IXmlDocument> toastXml;
    hr = toastManager->GetTemplateContent(ToastTemplateType_ToastText02, &toastXml);
    if (FAILED(hr) || !toastXml) {
        LOG_WARNING() << "Toast XML template failed. HRESULT=0x" << QString::number(static_cast<qulonglong>(hr), 16);
        return false;
    }

    if (!appendToastTextNode(toastXml, 0, title) || !appendToastTextNode(toastXml, 1, body)) {
        LOG_WARNING() << "Toast XML text setup failed";
        return false;
    }

    if (!setToastLaunchAttributes(toastXml, WindowsToastIdentity::toastProtocolUri(), suppressPopup)) {
        LOG_WARNING() << "Failed to set toast launch attribute";
    }

    ComPtr<IToastNotificationFactory> toastFactory;
    hr = RoGetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(),
                                IID_PPV_ARGS(&toastFactory));
    if (FAILED(hr) || !toastFactory) {
        LOG_WARNING() << "Toast factory activation failed. HRESULT=0x"
                      << QString::number(static_cast<qulonglong>(hr), 16);
        return false;
    }

    ComPtr<IToastNotification> toast;
    hr = toastFactory->CreateToastNotification(toastXml.Get(), &toast);
    if (FAILED(hr) || !toast) {
        LOG_WARNING() << "Toast creation failed. HRESULT=0x" << QString::number(static_cast<qulonglong>(hr), 16);
        return false;
    }

    if (onActivated) {
        activeToastActivatedHandler() = Callback<ToastActivatedHandler>(
                [onActivated](ABI::Windows::UI::Notifications::IToastNotification*, IInspectable*) -> HRESULT {
                    if (auto* const app = dynamic_cast<QApplication*>(QCoreApplication::instance()); app != nullptr) {
                        QMetaObject::invokeMethod(app, [onActivated]() { onActivated(); }, Qt::QueuedConnection);
                    }
                    else {
                        onActivated();
                    }
                    return S_OK;
                });

        if (!activeToastActivatedHandler().Get()) {
            LOG_WARNING() << "Toast activation callback creation failed";
            return false;
        }

        hr = toast->add_Activated(activeToastActivatedHandler().Get(), &activeToastActivatedToken());
        if (FAILED(hr)) {
            LOG_WARNING() << "Toast activation callback registration failed. HRESULT=0x"
                          << QString::number(static_cast<qulonglong>(hr), 16);
            activeToastActivatedHandler().Reset();
            activeToastActivatedToken() = {};
        }
    }

    ComPtr<IToastNotifier> notifier;
    hr = toastManager->CreateToastNotifierWithId(HStringReference(appId.c_str()).Get(), &notifier);
    if (FAILED(hr) || !notifier) {
        LOG_WARNING() << "Toast notifier creation failed. HRESULT=0x"
                      << QString::number(static_cast<qulonglong>(hr), 16);
        return false;
    }

    hr = notifier->Show(toast.Get());
    if (FAILED(hr)) {
        LOG_WARNING() << "Toast show failed. HRESULT=0x" << QString::number(static_cast<qulonglong>(hr), 16);
        resetActiveToastHandlers();
        return false;
    }

    activeToast() = toast;
    return true;
}

void WindowsToastNotification::clearToastHistoryForApp()
{
    if (!ensureToastRuntimeInitialized())
        return;

    ComPtr<IToastNotificationManagerStatics> toastManager;
    HRESULT hr = RoGetActivationFactory(
            HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),
            IID_PPV_ARGS(&toastManager));
    if (FAILED(hr) || !toastManager)
        return;

    ComPtr<IToastNotificationManagerStatics2> toastManager2;
    hr = toastManager.As(&toastManager2);
    if (FAILED(hr) || !toastManager2)
        return;

    ComPtr<IToastNotificationHistory> history;
    hr = toastManager2->get_History(&history);
    if (FAILED(hr) || !history)
        return;

    const std::wstring appId = WindowsToastIdentity::appUserModelIdWide();
    if (!appId.empty()) {
        const HRESULT clearWithIdHr = history->ClearWithId(HStringReference(appId.c_str()).Get());
        if (SUCCEEDED(clearWithIdHr) || clearWithIdHr == kToastHistoryNotFoundHr) {
            resetActiveToastHandlers();
            return;
        }
        LOG_WARNING() << "Toast history ClearWithId failed. HRESULT=0x"
                      << QString::number(static_cast<qulonglong>(clearWithIdHr), 16);
    }

    if (const HRESULT clearHr = history->Clear(); FAILED(clearHr) && clearHr != kToastHistoryNotFoundHr) {
        LOG_WARNING() << "Toast history Clear failed. HRESULT=0x"
                      << QString::number(static_cast<qulonglong>(clearHr), 16);
    }
    resetActiveToastHandlers();
}

#else

bool WindowsToastNotification::showToastNotification(const QString& title,
                                                     const QString& body,
                                                     const std::function<void()>& onActivated,
                                                     const bool suppressPopup)
{
    Q_UNUSED(title);
    Q_UNUSED(body);
    Q_UNUSED(onActivated);
    Q_UNUSED(suppressPopup);
    return false;
}

void WindowsToastNotification::clearToastHistoryForApp() {}

#endif
