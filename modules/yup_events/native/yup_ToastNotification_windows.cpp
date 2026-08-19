/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

/*
    This file is a port of the WinToast library
    (https://github.com/mohabouje/WinToast, Copyright (C) 2016-2023 Mohammed
    Boujemaoui), adapted to the YUP APIs and conventions.
*/

#if YUP_WINDOWS

YUP_BEGIN_IGNORE_WARNINGS_MSVC (4471 4710 4711 4514 4820 4668 4505)

#include <sdkddkver.h>
#include <ShObjIdl.h>
#include <wrl/implements.h>
#include <wrl/event.h>
#include <windows.ui.notifications.h>
#include <roapi.h>
#include <propvarutil.h>
#include <functiondiscoverykeys.h>
#include <winstring.h>
#include <strsafe.h>

YUP_END_IGNORE_WARNINGS_MSVC

#include <map>
#include <string>
#include <cwchar>

#if ! YUP_DONT_AUTOLINK_TO_WIN32_LIBRARIES
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "user32")
#endif

namespace yup
{
namespace detail
{

using namespace Microsoft::WRL;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::UI::Notifications;

namespace
{

//==============================================================================
// Dynamically loaded WinRT functions. Loading them lazily at runtime keeps the
// module linkable on systems without the WinRT API surface. The libraries are
// kept open for the process lifetime.
namespace DllImporter
{
using FnSetCurrentProcessExplicitAppUserModelID = HRESULT (FAR STDAPICALLTYPE*) (PCWSTR);
using FnPropVariantToString = HRESULT (FAR STDAPICALLTYPE*) (REFPROPVARIANT, PWSTR, UINT);
using FnRoGetActivationFactory = HRESULT (FAR STDAPICALLTYPE*) (HSTRING, REFIID, void**);
using FnWindowsCreateStringReference = HRESULT (FAR STDAPICALLTYPE*) (PCWSTR, UINT32, HSTRING_HEADER*, HSTRING*);
using FnWindowsGetStringRawBuffer = PCWSTR (FAR STDAPICALLTYPE*) (HSTRING, UINT32*);
using FnWindowsDeleteString = HRESULT (FAR STDAPICALLTYPE*) (HSTRING);

DynamicLibrary shell32Library;
DynamicLibrary propsysLibrary;
DynamicLibrary combaseLibrary;

FnSetCurrentProcessExplicitAppUserModelID setCurrentProcessExplicitAppUserModelID = nullptr;
FnPropVariantToString propVariantToString = nullptr;
FnRoGetActivationFactory roGetActivationFactory = nullptr;
FnWindowsCreateStringReference windowsCreateStringReference = nullptr;
FnWindowsGetStringRawBuffer windowsGetStringRawBuffer = nullptr;
FnWindowsDeleteString windowsDeleteString = nullptr;

template <typename Function>
bool loadFunctionFromLibrary (DynamicLibrary& library, const char* name, Function& function)
{
    function = reinterpret_cast<Function> (library.getFunction (name));
    return function != nullptr;
}

bool initialize()
{
    if (setCurrentProcessExplicitAppUserModelID != nullptr)
        return true;

    if (! shell32Library.open ("SHELL32.DLL"))
        return false;

    if (! loadFunctionFromLibrary (shell32Library, "SetCurrentProcessExplicitAppUserModelID", setCurrentProcessExplicitAppUserModelID))
        return false;

    if (! propsysLibrary.open ("PROPSYS.DLL"))
        return false;

    if (! loadFunctionFromLibrary (propsysLibrary, "PropVariantToString", propVariantToString))
        return false;

    if (! combaseLibrary.open ("COMBASE.DLL"))
        return false;

    return loadFunctionFromLibrary (combaseLibrary, "RoGetActivationFactory", roGetActivationFactory)
        && loadFunctionFromLibrary (combaseLibrary, "WindowsCreateStringReference", windowsCreateStringReference)
        && loadFunctionFromLibrary (combaseLibrary, "WindowsGetStringRawBuffer", windowsGetStringRawBuffer)
        && loadFunctionFromLibrary (combaseLibrary, "WindowsDeleteString", windowsDeleteString);
}

template <typename T>
HRESULT getActivationFactory (HSTRING activatableClassId, T** factory)
{
    return roGetActivationFactory (activatableClassId, IID_INS_ARGS (factory));
}

template <typename T>
HRESULT wrapGetActivationFactory (HSTRING activatableClassId, Details::ComPtrRef<T> factory) noexcept
{
    return getActivationFactory (activatableClassId, factory.ReleaseAndGetAddressOf());
}
} // namespace DllImporter

//==============================================================================
// A scoped HSTRING built from a string without copying (when possible).
class WinToastStringWrapper
{
public:
    WinToastStringWrapper (const wchar_t* stringRef, UINT32 length) noexcept
    {
        if (FAILED (DllImporter::windowsCreateStringReference (stringRef, length, &header, &hstring)))
            RaiseException (static_cast<DWORD> (STATUS_INVALID_PARAMETER), EXCEPTION_NONCONTINUABLE, 0, nullptr);
    }

    explicit WinToastStringWrapper (const std::wstring& stringRef) noexcept
        : WinToastStringWrapper (stringRef.c_str(), static_cast<UINT32> (stringRef.length()))
    {
    }

    ~WinToastStringWrapper()
    {
        DllImporter::windowsDeleteString (hstring);
    }

    HSTRING get() const noexcept { return hstring; }

private:
    HSTRING hstring {};
    HSTRING_HEADER header {};
};

//==============================================================================
// A custom IReference<DateTime> used to set the expiration time of a toast.
class InternalDateTime final : public IReference<DateTime>
{
public:
    static INT64 now()
    {
        FILETIME fileTime;
        ::GetSystemTimeAsFileTime (&fileTime);
        return ((static_cast<INT64> (fileTime.dwHighDateTime) << 32) | fileTime.dwLowDateTime);
    }

    explicit InternalDateTime (DateTime dateTime)
        : dateTime (dateTime)
    {
    }

    explicit InternalDateTime (INT64 millisecondsFromNow)
    {
        dateTime.UniversalTime = now() + millisecondsFromNow * 10000;
    }

    ~InternalDateTime() = default;

    operator INT64() const { return dateTime.UniversalTime; }

    HRESULT STDMETHODCALLTYPE get_Value (DateTime* value) override
    {
        if (value == nullptr)
            return E_POINTER;

        *value = dateTime;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface (const IID& riid, void** ppvObject) override
    {
        if (ppvObject == nullptr)
            return E_POINTER;

        if (riid == __uuidof (IUnknown) || riid == __uuidof (IReference<DateTime>))
        {
            *ppvObject = static_cast<IReference<DateTime>*> (this);
            AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return ++refCount; }

    ULONG STDMETHODCALLTYPE Release() override { return --refCount; }

    HRESULT STDMETHODCALLTYPE GetIids (ULONG*, IID**) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE GetRuntimeClassName (HSTRING*) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE GetTrustLevel (TrustLevel*) override { return E_NOTIMPL; }

private:
    DateTime dateTime {};
    ULONG refCount { 1 };
};

//==============================================================================
// A copy of the event callbacks of a ToastTemplate, retained until the
// notification is gone.
struct ToastCallbacks
{
    std::function<void()> onActivated;
    std::function<void (int)> onActivatedWithAction;
    std::function<void (ToastTemplate::DismissalReason)> onDismissed;
    std::function<void()> onFailed;
};

//==============================================================================
// A displayed notification plus the tokens of its event handlers.
class NotifyData
{
public:
    NotifyData (ComPtr<IToastNotification> notify, EventRegistrationToken activatedToken, EventRegistrationToken dismissedToken, EventRegistrationToken failedToken)
        : notify (std::move (notify))
        , activatedToken (activatedToken)
        , dismissedToken (dismissedToken)
        , failedToken (failedToken)
    {
    }

    ~NotifyData()
    {
        removeTokens();
    }

    NotifyData (NotifyData&&) noexcept = default;
    NotifyData& operator= (NotifyData&&) noexcept = default;

    NotifyData (const NotifyData&) = delete;
    NotifyData& operator= (const NotifyData&) = delete;

    void removeTokens()
    {
        if (previouslyTokenRemoved || ! notify.Get())
            return;

        notify->remove_Activated (activatedToken);
        notify->remove_Dismissed (dismissedToken);
        notify->remove_Failed (failedToken);
        previouslyTokenRemoved = true;
    }

    void markAsReadyForDeletion() { readyForDeletion = true; }

    bool isReadyForDeletion() const { return readyForDeletion; }

    IToastNotification* getNotification() { return notify.Get(); }

private:
    ComPtr<IToastNotification> notify { nullptr };
    EventRegistrationToken activatedToken {};
    EventRegistrationToken dismissedToken {};
    EventRegistrationToken failedToken {};
    bool readyForDeletion { false };
    bool previouslyTokenRemoved { false };
};

//==============================================================================
// The per-process state of the toast backend.
struct ToastState
{
    bool isInitialized { false };
    bool hasCoInitialized { false };
    ToastNotification::ShortcutPolicy shortcutPolicy { ToastNotification::ShortcutPolicy::requireCreate };
    std::wstring appName;
    std::wstring aumi;
    std::map<int64, NotifyData> buffer;
    CriticalSection bufferLock;
    yup::Atomic<int64> nextId { 1 };

    bool isCompatible() const { return DllImporter::initialize(); }
};

ToastState& getToastState()
{
    static ToastState state;
    return state;
}

//==============================================================================
// Utility functions.
namespace Util
{

HRESULT getRealOSVersion (RTL_OSVERSIONINFOW& versionInfo)
{
    const HMODULE hMod = ::GetModuleHandleW (L"ntdll.dll");
    if (hMod == nullptr)
        return E_FAIL;

    using RtlGetVersionPtr = LONG (WINAPI*) (PRTL_OSVERSIONINFOW);
    if (const auto function = reinterpret_cast<RtlGetVersionPtr> (::GetProcAddress (hMod, "RtlGetVersion")))
    {
        versionInfo = {};
        versionInfo.dwOSVersionInfoSize = sizeof (versionInfo);
        return function (&versionInfo) == 0 ? S_OK : E_FAIL;
    }

    return E_FAIL;
}

HRESULT defaultExecutablePath (wchar_t* path, DWORD nSize = MAX_PATH)
{
    const HMODULE module = ::GetModuleHandleW (nullptr);
    return module != nullptr && ::GetModuleFileNameW (module, path, nSize) > 0 ? S_OK : E_FAIL;
}

HRESULT defaultShellLinksDirectory (wchar_t* path, DWORD nSize = MAX_PATH)
{
    const DWORD written = ::GetEnvironmentVariableW (L"APPDATA", path, nSize);
    HRESULT hr = written > 0 ? S_OK : E_INVALIDARG;

    if (SUCCEEDED (hr))
    {
        const errno_t result = wcscat_s (path, nSize, L"\\Microsoft\\Windows\\Start Menu\\Programs\\");
        hr = result == 0 ? S_OK : E_INVALIDARG;
    }

    return hr;
}

HRESULT defaultShellLinkPath (const std::wstring& appName, wchar_t* path, DWORD nSize = MAX_PATH)
{
    HRESULT hr = defaultShellLinksDirectory (path, nSize);

    if (SUCCEEDED (hr))
    {
        const std::wstring appLink (appName + L".lnk");
        const errno_t result = wcscat_s (path, nSize, appLink.c_str());
        hr = result == 0 ? S_OK : E_INVALIDARG;
    }

    return hr;
}

HRESULT setNodeStringValue (const std::wstring& string, IXmlNode* node, IXmlDocument* xml)
{
    ComPtr<IXmlText> textNode;
    HRESULT hr = xml->CreateTextNode (WinToastStringWrapper (string).get(), &textNode);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> stringNode;
    hr = textNode.As (&stringNode);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> appendedChild;
    return node->AppendChild (stringNode.Get(), &appendedChild);
}

template <typename MarkAsReadyForDeletionFunc>
HRESULT setEventHandlers (IToastNotification* notification, ToastCallbacks callbacks, INT64 expirationTime, EventRegistrationToken& activatedToken, EventRegistrationToken& dismissedToken, EventRegistrationToken& failedToken, MarkAsReadyForDeletionFunc&& markAsReadyForDeletionFunc)
{
    HRESULT hr = notification->add_Activated (
        Callback<Implements<RuntimeClassFlags<ClassicCom>, ITypedEventHandler<::ABI::Windows::UI::Notifications::ToastNotification*, IInspectable*>>> (
            [callbacks, markAsReadyForDeletionFunc] (IToastNotification*, IInspectable* inspectable)
    {
        const auto handlePlainActivation = [&]()
        {
            if (callbacks.onActivated)
                callbacks.onActivated();

            markAsReadyForDeletionFunc();
            return S_OK;
        };

        if (inspectable == nullptr)
            return handlePlainActivation();

        ComPtr<IToastActivatedEventArgs> activatedEventArgs;
        if (FAILED (inspectable->QueryInterface (activatedEventArgs.GetAddressOf())))
            return handlePlainActivation();

        HSTRING argumentsHandle;
        if (FAILED (activatedEventArgs->get_Arguments (&argumentsHandle)))
            return handlePlainActivation();

        const PCWSTR arguments = DllImporter::windowsGetStringRawBuffer (argumentsHandle, nullptr);

        if (arguments != nullptr && *arguments != 0)
        {
            if (callbacks.onActivatedWithAction)
                callbacks.onActivatedWithAction (static_cast<int> (wcstol (arguments, nullptr, 10)));

            DllImporter::windowsDeleteString (argumentsHandle);
            markAsReadyForDeletionFunc();
            return S_OK;
        }

        DllImporter::windowsDeleteString (argumentsHandle);
        return handlePlainActivation();
    })
            .Get(),
        &activatedToken);

    if (FAILED (hr))
        return hr;

    hr = notification->add_Dismissed (
        Callback<Implements<RuntimeClassFlags<ClassicCom>, ITypedEventHandler<::ABI::Windows::UI::Notifications::ToastNotification*, ToastDismissedEventArgs*>>> (
            [callbacks, expirationTime, markAsReadyForDeletionFunc] (IToastNotification*, IToastDismissedEventArgs* eventArgs)
    {
        ToastDismissalReason reason;
        if (FAILED (eventArgs->get_Reason (&reason)))
        {
            markAsReadyForDeletionFunc();
            return S_OK;
        }

        if (reason == ToastDismissalReason_UserCanceled && expirationTime != 0 && InternalDateTime::now() >= expirationTime)
            reason = ToastDismissalReason_TimedOut;

        if (callbacks.onDismissed)
            callbacks.onDismissed (static_cast<ToastTemplate::DismissalReason> (reason));

        markAsReadyForDeletionFunc();
        return S_OK;
    })
            .Get(),
        &dismissedToken);

    if (FAILED (hr))
        return hr;

    return notification->add_Failed (
        Callback<Implements<RuntimeClassFlags<ClassicCom>, ITypedEventHandler<::ABI::Windows::UI::Notifications::ToastNotification*, ToastFailedEventArgs*>>> (
            [callbacks, markAsReadyForDeletionFunc] (IToastNotification*, IToastFailedEventArgs*)
    {
        if (callbacks.onFailed)
            callbacks.onFailed();

        markAsReadyForDeletionFunc();
        return S_OK;
    })
            .Get(),
        &failedToken);
}

HRESULT addAttribute (IXmlDocument* xml, const std::wstring& name, IXmlNamedNodeMap* attributeMap)
{
    ComPtr<IXmlAttribute> srcAttribute;
    HRESULT hr = xml->CreateAttribute (WinToastStringWrapper (name).get(), &srcAttribute);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> node;
    hr = srcAttribute.As (&node);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> pNode;
    return attributeMap->SetNamedItem (node.Get(), &pNode);
}

HRESULT createElement (IXmlDocument* xml, const std::wstring& rootNode, const std::wstring& elementName, const std::vector<std::wstring>& attributeNames)
{
    ComPtr<IXmlNodeList> rootList;
    HRESULT hr = xml->GetElementsByTagName (WinToastStringWrapper (rootNode).get(), &rootList);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> root;
    hr = rootList->Item (0, &root);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlElement> element;
    hr = xml->CreateElement (WinToastStringWrapper (elementName).get(), &element);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> elementNode;
    hr = element.As (&elementNode);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> appendedNode;
    hr = root->AppendChild (elementNode.Get(), &appendedNode);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNamedNodeMap> attributes;
    hr = appendedNode->get_Attributes (&attributes);
    if (FAILED (hr))
        return hr;

    for (const auto& attribute : attributeNames)
    {
        hr = addAttribute (xml, attribute, attributes.Get());
        if (FAILED (hr))
            return hr;
    }

    return S_OK;
}

} // namespace Util

//==============================================================================
std::wstring toWideString (const String& text)
{
    return { text.toWideCharPointer(), static_cast<std::size_t> (text.length()) };
}

std::wstring toWideString (const File& file)
{
    return toWideString (file.getFullPathName());
}

std::wstring scenarioToWideString (ToastTemplate::Scenario scenario)
{
    switch (scenario)
    {
        case ToastTemplate::Scenario::alarm:
            return L"Alarm";
        case ToastTemplate::Scenario::incomingCall:
            return L"IncomingCall";
        case ToastTemplate::Scenario::reminder:
            return L"Reminder";
        case ToastTemplate::Scenario::default_:
            break;
    }

    return L"Default";
}

std::wstring audioSystemFileToWideString (ToastTemplate::AudioSystemFile file)
{
    switch (file)
    {
        case ToastTemplate::AudioSystemFile::defaultSound:
            return L"ms-winsoundevent:Notification.Default";
        case ToastTemplate::AudioSystemFile::im:
            return L"ms-winsoundevent:Notification.IM";
        case ToastTemplate::AudioSystemFile::mail:
            return L"ms-winsoundevent:Notification.Mail";
        case ToastTemplate::AudioSystemFile::reminder:
            return L"ms-winsoundevent:Notification.Reminder";
        case ToastTemplate::AudioSystemFile::sms:
            return L"ms-winsoundevent:Notification.SMS";
        case ToastTemplate::AudioSystemFile::alarm:
            return L"ms-winsoundevent:Notification.Looping.Alarm";
        case ToastTemplate::AudioSystemFile::alarm2:
            return L"ms-winsoundevent:Notification.Looping.Alarm2";
        case ToastTemplate::AudioSystemFile::alarm3:
            return L"ms-winsoundevent:Notification.Looping.Alarm3";
        case ToastTemplate::AudioSystemFile::alarm4:
            return L"ms-winsoundevent:Notification.Looping.Alarm4";
        case ToastTemplate::AudioSystemFile::alarm5:
            return L"ms-winsoundevent:Notification.Looping.Alarm5";
        case ToastTemplate::AudioSystemFile::alarm6:
            return L"ms-winsoundevent:Notification.Looping.Alarm6";
        case ToastTemplate::AudioSystemFile::alarm7:
            return L"ms-winsoundevent:Notification.Looping.Alarm7";
        case ToastTemplate::AudioSystemFile::alarm8:
            return L"ms-winsoundevent:Notification.Looping.Alarm8";
        case ToastTemplate::AudioSystemFile::alarm9:
            return L"ms-winsoundevent:Notification.Looping.Alarm9";
        case ToastTemplate::AudioSystemFile::alarm10:
            return L"ms-winsoundevent:Notification.Looping.Alarm10";
        case ToastTemplate::AudioSystemFile::call:
            return L"ms-winsoundevent:Notification.Looping.Call";
        case ToastTemplate::AudioSystemFile::call1:
            return L"ms-winsoundevent:Notification.Looping.Call1";
        case ToastTemplate::AudioSystemFile::call2:
            return L"ms-winsoundevent:Notification.Looping.Call2";
        case ToastTemplate::AudioSystemFile::call3:
            return L"ms-winsoundevent:Notification.Looping.Call3";
        case ToastTemplate::AudioSystemFile::call4:
            return L"ms-winsoundevent:Notification.Looping.Call4";
        case ToastTemplate::AudioSystemFile::call5:
            return L"ms-winsoundevent:Notification.Looping.Call5";
        case ToastTemplate::AudioSystemFile::call6:
            return L"ms-winsoundevent:Notification.Looping.Call6";
        case ToastTemplate::AudioSystemFile::call7:
            return L"ms-winsoundevent:Notification.Looping.Call7";
        case ToastTemplate::AudioSystemFile::call8:
            return L"ms-winsoundevent:Notification.Looping.Call8";
        case ToastTemplate::AudioSystemFile::call9:
            return L"ms-winsoundevent:Notification.Looping.Call9";
        case ToastTemplate::AudioSystemFile::call10:
            return L"ms-winsoundevent:Notification.Looping.Call10";
    }

    jassertfalse;
    return L"";
}

//==============================================================================
HRESULT validateShellLinkHelper (ToastState& state, bool& wasChanged)
{
    wchar_t path[MAX_PATH] = { L'\0' };
    if (FAILED (Util::defaultShellLinkPath (state.appName, path)))
        return E_FAIL;

    if (::GetFileAttributesW (path) == INVALID_FILE_ATTRIBUTES)
        return E_FAIL;

    ComPtr<IShellLink> shellLink;
    HRESULT hr = ::CoCreateInstance (CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS (&shellLink));
    if (FAILED (hr))
        return hr;

    ComPtr<IPersistFile> persistFile;
    hr = shellLink.As (&persistFile);
    if (FAILED (hr))
        return hr;

    hr = persistFile->Load (path, STGM_READWRITE);
    if (FAILED (hr))
        return hr;

    ComPtr<IPropertyStore> propertyStore;
    hr = shellLink.As (&propertyStore);
    if (FAILED (hr))
        return hr;

    PROPVARIANT appIdPropVar;
    hr = propertyStore->GetValue (PKEY_AppUserModel_ID, &appIdPropVar);
    if (FAILED (hr))
        return hr;

    wchar_t aumi[MAX_PATH];
    hr = DllImporter::propVariantToString (appIdPropVar, aumi, MAX_PATH);
    wasChanged = false;

    if (SUCCEEDED (hr) && state.aumi == aumi)
    {
        ::PropVariantClear (&appIdPropVar);
        return S_OK;
    }

    // The AUMI differs (or couldn't be read): update it when the policy allows.
    if (state.shortcutPolicy != ToastNotification::ShortcutPolicy::requireCreate)
    {
        ::PropVariantClear (&appIdPropVar);
        return E_FAIL;
    }

    wasChanged = true;
    ::PropVariantClear (&appIdPropVar);

    hr = ::InitPropVariantFromString (state.aumi.c_str(), &appIdPropVar);
    if (FAILED (hr))
        return hr;

    hr = propertyStore->SetValue (PKEY_AppUserModel_ID, appIdPropVar);
    if (SUCCEEDED (hr))
        hr = propertyStore->Commit();

    if (SUCCEEDED (hr) && SUCCEEDED (persistFile->IsDirty()))
        hr = persistFile->Save (path, TRUE);

    ::PropVariantClear (&appIdPropVar);
    return hr;
}

HRESULT createShellLinkHelper (ToastState& state)
{
    if (state.shortcutPolicy != ToastNotification::ShortcutPolicy::requireCreate)
        return E_FAIL;

    wchar_t exePath[MAX_PATH] = { L'\0' };
    wchar_t slPath[MAX_PATH] = { L'\0' };
    if (FAILED (Util::defaultShellLinkPath (state.appName, slPath)) || FAILED (Util::defaultExecutablePath (exePath)))
        return E_FAIL;

    ComPtr<IShellLinkW> shellLink;
    HRESULT hr = ::CoCreateInstance (CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS (&shellLink));
    if (FAILED (hr))
        return hr;

    hr = shellLink->SetPath (exePath);
    if (FAILED (hr))
        return hr;

    hr = shellLink->SetArguments (L"");
    if (FAILED (hr))
        return hr;

    hr = shellLink->SetWorkingDirectory (exePath);
    if (FAILED (hr))
        return hr;

    ComPtr<IPropertyStore> propertyStore;
    hr = shellLink.As (&propertyStore);
    if (FAILED (hr))
        return hr;

    PROPVARIANT appIdPropVar;
    hr = ::InitPropVariantFromString (state.aumi.c_str(), &appIdPropVar);
    if (FAILED (hr))
        return hr;

    hr = propertyStore->SetValue (PKEY_AppUserModel_ID, appIdPropVar);
    if (SUCCEEDED (hr))
        hr = propertyStore->Commit();

    if (SUCCEEDED (hr))
    {
        ComPtr<IPersistFile> persistFile;
        hr = shellLink.As (&persistFile);

        if (SUCCEEDED (hr))
            hr = persistFile->Save (slPath, TRUE);
    }

    ::PropVariantClear (&appIdPropVar);
    return hr;
}

bool isSupportingModernFeatures()
{
    RTL_OSVERSIONINFOW versionInfo;
    return SUCCEEDED (Util::getRealOSVersion (versionInfo)) && versionInfo.dwMajorVersion > 6;
}

// The upstream createShortcut() result codes, kept for parity.
enum ShortcutResult
{
    shortcutUnchanged = 0,
    shortcutWasChanged = 1,
    shortcutWasCreated = 2,
    shortcutMissingParameters = -1,
    shortcutIncompatibleOS = -2,
    shortcutComInitFailure = -3,
    shortcutCreateFailed = -4
};

int createShortcut (ToastState& state)
{
    if (state.aumi.empty() || state.appName.empty())
        return shortcutMissingParameters;

    if (! state.isCompatible())
        return shortcutIncompatibleOS;

    if (! state.hasCoInitialized)
    {
        const HRESULT initHr = ::CoInitializeEx (nullptr, COINIT_MULTITHREADED);
        if (initHr != RPC_E_CHANGED_MODE)
        {
            if (FAILED (initHr) && initHr != S_FALSE)
                return shortcutComInitFailure;

            state.hasCoInitialized = true;
        }
    }

    bool wasChanged = false;
    if (SUCCEEDED (validateShellLinkHelper (state, wasChanged)))
        return wasChanged ? shortcutWasChanged : shortcutUnchanged;

    return SUCCEEDED (createShellLinkHelper (state)) ? shortcutWasCreated : shortcutCreateFailed;
}

//==============================================================================
// XML template helpers.
HRESULT setTextFieldHelper (IXmlDocument* xml, const std::wstring& text, UINT32 position)
{
    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = xml->GetElementsByTagName (WinToastStringWrapper (L"text").get(), &nodeList);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> node;
    hr = nodeList->Item (position, &node);
    if (FAILED (hr))
        return hr;

    return Util::setNodeStringValue (text, node.Get(), xml);
}

HRESULT setAttributionTextFieldHelper (IXmlDocument* xml, const std::wstring& text)
{
    Util::createElement (xml, L"binding", L"text", { L"placement" });

    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = xml->GetElementsByTagName (WinToastStringWrapper (L"text").get(), &nodeList);
    if (FAILED (hr))
        return hr;

    UINT32 nodeListLength = 0;
    hr = nodeList->get_Length (&nodeListLength);
    if (FAILED (hr))
        return hr;

    for (UINT32 i = 0; i < nodeListLength; ++i)
    {
        ComPtr<IXmlNode> textNode;
        ComPtr<IXmlNamedNodeMap> attributes;
        ComPtr<IXmlNode> editedNode;

        if (FAILED (nodeList->Item (i, &textNode))
            || FAILED (textNode->get_Attributes (&attributes))
            || FAILED (attributes->GetNamedItem (WinToastStringWrapper (L"placement").get(), &editedNode))
            || ! editedNode)
        {
            continue;
        }

        hr = Util::setNodeStringValue (L"attribution", editedNode.Get(), xml);
        if (SUCCEEDED (hr))
            hr = setTextFieldHelper (xml, text, i);

        return hr;
    }

    return hr;
}

HRESULT setBindToastGenericHelper (IXmlDocument* xml)
{
    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = xml->GetElementsByTagName (WinToastStringWrapper (L"binding").get(), &nodeList);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> bindingNode;
    hr = nodeList->Item (0, &bindingNode);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlElement> bindingElement;
    hr = bindingNode.As (&bindingElement);
    if (FAILED (hr))
        return hr;

    return bindingElement->SetAttribute (WinToastStringWrapper (L"template").get(),
                                         WinToastStringWrapper (L"ToastGeneric").get());
}

HRESULT setImageFieldHelper (IXmlDocument* xml, const std::wstring& path, bool isToastGeneric, bool isCropHintCircle)
{
    std::wstring imagePath (L"file:///");
    imagePath += path;

    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = xml->GetElementsByTagName (WinToastStringWrapper (L"image").get(), &nodeList);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> node;
    hr = nodeList->Item (0, &node);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlElement> imageElement;
    const HRESULT hrImage = node.As (&imageElement);

    if (SUCCEEDED (hrImage) && isToastGeneric)
    {
        hr = imageElement->SetAttribute (WinToastStringWrapper (L"placement").get(),
                                         WinToastStringWrapper (L"appLogoOverride").get());
        if (FAILED (hr))
            return hr;

        if (isCropHintCircle)
        {
            hr = imageElement->SetAttribute (WinToastStringWrapper (L"hint-crop").get(),
                                             WinToastStringWrapper (L"circle").get());
            if (FAILED (hr))
                return hr;
        }
    }

    ComPtr<IXmlNamedNodeMap> attributes;
    hr = node->get_Attributes (&attributes);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> editedNode;
    hr = attributes->GetNamedItem (WinToastStringWrapper (L"src").get(), &editedNode);
    if (FAILED (hr))
        return hr;

    return Util::setNodeStringValue (imagePath, editedNode.Get(), xml);
}

HRESULT setHeroImageHelper (IXmlDocument* xml, const std::wstring& path, bool isInlineImage)
{
    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = xml->GetElementsByTagName (WinToastStringWrapper (L"binding").get(), &nodeList);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> bindingNode;
    hr = nodeList->Item (0, &bindingNode);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlElement> imageElement;
    hr = xml->CreateElement (WinToastStringWrapper (L"image").get(), &imageElement);
    if (FAILED (hr))
        return hr;

    if (! isInlineImage)
    {
        hr = imageElement->SetAttribute (WinToastStringWrapper (L"placement").get(),
                                         WinToastStringWrapper (L"hero").get());
        if (FAILED (hr))
            return hr;
    }

    hr = imageElement->SetAttribute (WinToastStringWrapper (L"src").get(), WinToastStringWrapper (path).get());
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> imageNode;
    hr = imageElement.As (&imageNode);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> appendedChild;
    return bindingNode->AppendChild (imageNode.Get(), &appendedChild);
}

HRESULT setAudioFieldHelper (IXmlDocument* xml, const std::wstring& path, ToastTemplate::AudioOption option)
{
    std::vector<std::wstring> attributes;
    if (! path.empty())
        attributes.push_back (L"src");

    if (option == ToastTemplate::AudioOption::loop)
        attributes.push_back (L"loop");

    if (option == ToastTemplate::AudioOption::silent)
        attributes.push_back (L"silent");

    Util::createElement (xml, L"toast", L"audio", attributes);

    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = xml->GetElementsByTagName (WinToastStringWrapper (L"audio").get(), &nodeList);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> node;
    hr = nodeList->Item (0, &node);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNamedNodeMap> nodeAttributes;
    hr = node->get_Attributes (&nodeAttributes);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> editedNode;

    if (! path.empty())
    {
        hr = nodeAttributes->GetNamedItem (WinToastStringWrapper (L"src").get(), &editedNode);
        if (FAILED (hr))
            return hr;

        hr = Util::setNodeStringValue (path, editedNode.Get(), xml);
        if (FAILED (hr))
            return hr;
    }

    if (option == ToastTemplate::AudioOption::loop)
    {
        hr = nodeAttributes->GetNamedItem (WinToastStringWrapper (L"loop").get(), &editedNode);
        if (FAILED (hr))
            return hr;

        return Util::setNodeStringValue (L"true", editedNode.Get(), xml);
    }

    if (option == ToastTemplate::AudioOption::silent)
    {
        hr = nodeAttributes->GetNamedItem (WinToastStringWrapper (L"silent").get(), &editedNode);
        if (FAILED (hr))
            return hr;

        return Util::setNodeStringValue (L"true", editedNode.Get(), xml);
    }

    return hr;
}

// Creates an <actions> element under the toast, switching the template to
// ToastGeneric with a long duration, and returns it via actionsNode.
HRESULT createActionsElement (IXmlDocument* xml, ComPtr<IXmlNode>& actionsNode)
{
    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = xml->GetElementsByTagName (WinToastStringWrapper (L"toast").get(), &nodeList);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> toastNode;
    hr = nodeList->Item (0, &toastNode);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlElement> toastElement;
    hr = toastNode.As (&toastElement);
    if (FAILED (hr))
        return hr;

    hr = toastElement->SetAttribute (WinToastStringWrapper (L"template").get(),
                                     WinToastStringWrapper (L"ToastGeneric").get());
    if (FAILED (hr))
        return hr;

    hr = toastElement->SetAttribute (WinToastStringWrapper (L"duration").get(),
                                     WinToastStringWrapper (L"long").get());
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlElement> actionsElement;
    hr = xml->CreateElement (WinToastStringWrapper (L"actions").get(), &actionsElement);
    if (FAILED (hr))
        return hr;

    hr = actionsElement.As (&actionsNode);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> appendedChild;
    return toastNode->AppendChild (actionsNode.Get(), &appendedChild);
}

HRESULT addActionHelper (IXmlDocument* xml, const std::wstring& content, const std::wstring& arguments)
{
    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = xml->GetElementsByTagName (WinToastStringWrapper (L"actions").get(), &nodeList);
    if (FAILED (hr))
        return hr;

    UINT32 length = 0;
    hr = nodeList->get_Length (&length);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> actionsNode;

    if (length > 0)
    {
        hr = nodeList->Item (0, &actionsNode);
        if (FAILED (hr))
            return hr;
    }
    else
    {
        hr = createActionsElement (xml, actionsNode);
        if (FAILED (hr))
            return hr;
    }

    ComPtr<IXmlElement> actionElement;
    hr = xml->CreateElement (WinToastStringWrapper (L"action").get(), &actionElement);
    if (FAILED (hr))
        return hr;

    hr = actionElement->SetAttribute (WinToastStringWrapper (L"content").get(), WinToastStringWrapper (content).get());
    if (FAILED (hr))
        return hr;

    hr = actionElement->SetAttribute (WinToastStringWrapper (L"arguments").get(), WinToastStringWrapper (arguments).get());
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> actionNode;
    hr = actionElement.As (&actionNode);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> appendedChild;
    return actionsNode->AppendChild (actionNode.Get(), &appendedChild);
}

HRESULT addDurationHelper (IXmlDocument* xml, const std::wstring& duration)
{
    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = xml->GetElementsByTagName (WinToastStringWrapper (L"toast").get(), &nodeList);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> toastNode;
    hr = nodeList->Item (0, &toastNode);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlElement> toastElement;
    hr = toastNode.As (&toastElement);
    if (FAILED (hr))
        return hr;

    return toastElement->SetAttribute (WinToastStringWrapper (L"duration").get(), WinToastStringWrapper (duration).get());
}

HRESULT addScenarioHelper (IXmlDocument* xml, const std::wstring& scenario)
{
    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = xml->GetElementsByTagName (WinToastStringWrapper (L"toast").get(), &nodeList);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlNode> toastNode;
    hr = nodeList->Item (0, &toastNode);
    if (FAILED (hr))
        return hr;

    ComPtr<IXmlElement> toastElement;
    hr = toastNode.As (&toastElement);
    if (FAILED (hr))
        return hr;

    return toastElement->SetAttribute (WinToastStringWrapper (L"scenario").get(), WinToastStringWrapper (scenario).get());
}

//==============================================================================
ComPtr<IToastNotifier> notifier (const std::wstring& aumi, bool& succeeded)
{
    succeeded = false;
    ComPtr<IToastNotificationManagerStatics> notificationManager;
    ComPtr<IToastNotifier> toastNotifier;

    HRESULT hr = DllImporter::wrapGetActivationFactory (
        WinToastStringWrapper (RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).get(), &notificationManager);

    if (SUCCEEDED (hr))
        hr = notificationManager->CreateToastNotifierWithId (WinToastStringWrapper (aumi).get(), &toastNotifier);

    succeeded = SUCCEEDED (hr);
    return toastNotifier;
}

void markAsReadyForDeletion (ToastState& state, int64 id)
{
    ScopedLock lock (state.bufferLock);

    // Flush the buffer, removing the toasts that are ready for deletion.
    for (auto it = state.buffer.begin(); it != state.buffer.end();)
    {
        if (it->second.isReadyForDeletion())
        {
            it->second.removeTokens();
            it = state.buffer.erase (it);
        }
        else
        {
            ++it;
        }
    }

    // Mark the toast as ready for deletion, so it is removed on the next flush.
    const auto iter = state.buffer.find (id);
    if (iter != state.buffer.end())
        iter->second.markAsReadyForDeletion();
}

bool isToastGeneric (const ToastTemplate& toast)
{
    return toast.hasHeroImage() || toast.getCropHint() == ToastTemplate::CropHint::circle;
}

} // namespace

//==============================================================================
Result toastNotificationInitialize (const ToastNotificationSettings& settings)
{
    auto& state = getToastState();

    if (! state.isCompatible())
        return Result::fail (ToastNotification::getErrorDescription (ToastNotification::Error::systemNotSupported));

    state.appName = toWideString (settings.appName);
    state.aumi = toWideString (settings.appUserModelId);
    state.shortcutPolicy = settings.shortcutPolicy;

    if (state.aumi.empty() || state.appName.empty())
        return Result::fail (ToastNotification::getErrorDescription (ToastNotification::Error::invalidParameters));

    if (state.shortcutPolicy != ToastNotification::ShortcutPolicy::ignore)
    {
        if (createShortcut (state) < 0)
            return Result::fail (ToastNotification::getErrorDescription (ToastNotification::Error::shellLinkNotCreated));
    }

    if (FAILED (DllImporter::setCurrentProcessExplicitAppUserModelID (state.aumi.c_str())))
        return Result::fail (ToastNotification::getErrorDescription (ToastNotification::Error::invalidAppUserModelID));

    state.isInitialized = true;
    return Result::ok();
}

//==============================================================================
ResultValue<int64> showToastImpl (const ToastTemplate& toast, const ToastNotificationSettings& settings)
{
    auto& state = getToastState();

    if (! state.isInitialized)
        return makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::notInitialized));

    if (state.aumi.empty())
        state.aumi = toWideString (settings.appUserModelId);

    if (state.aumi.empty())
        return makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::invalidParameters));

    const auto failWith = [] (ToastNotification::Error error)
    {
        return makeResultValueFail (ToastNotification::getErrorDescription (error));
    };

    ComPtr<IToastNotificationManagerStatics> notificationManager;
    HRESULT hr = DllImporter::wrapGetActivationFactory (
        WinToastStringWrapper (RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).get(), &notificationManager);
    if (FAILED (hr))
        return failWith (ToastNotification::Error::notDisplayed);

    ComPtr<IToastNotifier> toastNotifier;
    hr = notificationManager->CreateToastNotifierWithId (WinToastStringWrapper (state.aumi).get(), &toastNotifier);
    if (FAILED (hr))
        return failWith (ToastNotification::Error::notDisplayed);

    ComPtr<IToastNotificationFactory> notificationFactory;
    hr = DllImporter::wrapGetActivationFactory (
        WinToastStringWrapper (RuntimeClass_Windows_UI_Notifications_ToastNotification).get(), &notificationFactory);
    if (FAILED (hr))
        return failWith (ToastNotification::Error::notDisplayed);

    ComPtr<IXmlDocument> xmlDocument;
    hr = notificationManager->GetTemplateContent (static_cast<ToastTemplateType> (toast.getType()), &xmlDocument);
    if (FAILED (hr))
        return failWith (ToastNotification::Error::notDisplayed);

    if (isToastGeneric (toast))
    {
        hr = setBindToastGenericHelper (xmlDocument.Get());
        if (FAILED (hr))
            return failWith (ToastNotification::Error::notDisplayed);
    }

    for (UINT32 i = 0; i < toast.getTextFieldsCount(); ++i)
    {
        hr = setTextFieldHelper (xmlDocument.Get(), toWideString (toast.getTextField (static_cast<ToastTemplate::TextField> (i))), i);
        if (FAILED (hr))
            return failWith (ToastNotification::Error::notDisplayed);
    }

    // Modern features are supported on Windows 10 and above.
    if (isSupportingModernFeatures())
    {
        if (! toast.getAttributionText().isEmpty())
        {
            hr = setAttributionTextFieldHelper (xmlDocument.Get(), toWideString (toast.getAttributionText()));
            if (FAILED (hr))
                return failWith (ToastNotification::Error::notDisplayed);
        }

        for (size_t i = 0; i < toast.getActionsCount(); ++i)
        {
            hr = addActionHelper (xmlDocument.Get(), toWideString (toast.getActionLabel (i)), std::to_wstring (i));
            if (FAILED (hr))
                return failWith (ToastNotification::Error::notDisplayed);
        }

        std::wstring audioPath;
        if (toast.getAudioSystemFile().has_value())
            audioPath = audioSystemFileToWideString (*toast.getAudioSystemFile());
        else
            audioPath = toWideString (toast.getAudioPath());

        if (! (audioPath.empty() && toast.getAudioOption() == ToastTemplate::AudioOption::default_))
        {
            hr = setAudioFieldHelper (xmlDocument.Get(), audioPath, toast.getAudioOption());
            if (FAILED (hr))
                return failWith (ToastNotification::Error::notDisplayed);
        }

        if (toast.getDuration() != ToastTemplate::Duration::system)
        {
            hr = addDurationHelper (xmlDocument.Get(), toast.getDuration() == ToastTemplate::Duration::short_ ? L"short" : L"long");
            if (FAILED (hr))
                return failWith (ToastNotification::Error::notDisplayed);
        }

        hr = addScenarioHelper (xmlDocument.Get(), scenarioToWideString (toast.getScenario()));
        if (FAILED (hr))
            return failWith (ToastNotification::Error::notDisplayed);
    }

    if (toast.hasImage())
    {
        RTL_OSVERSIONINFOW versionInfo;
        const bool isWin10AnniversaryOrAbove = SUCCEEDED (Util::getRealOSVersion (versionInfo))
                                            && versionInfo.dwBuildNumber >= 14393;

        hr = setImageFieldHelper (xmlDocument.Get(), toWideString (toast.getImagePath()), isToastGeneric (toast), isWin10AnniversaryOrAbove && toast.getCropHint() == ToastTemplate::CropHint::circle);
        if (FAILED (hr))
            return failWith (ToastNotification::Error::notDisplayed);
    }

    if (toast.hasHeroImage())
    {
        hr = setHeroImageHelper (xmlDocument.Get(), toWideString (toast.getHeroImagePath()), toast.isInlineHeroImage());
        if (FAILED (hr))
            return failWith (ToastNotification::Error::notDisplayed);
    }

    ComPtr<IToastNotification> notification;
    hr = notificationFactory->CreateToastNotification (xmlDocument.Get(), &notification);
    if (FAILED (hr))
        return failWith (ToastNotification::Error::notDisplayed);

    INT64 expiration = 0;
    const INT64 relativeExpiration = toast.getExpiration();

    if (relativeExpiration > 0)
    {
        InternalDateTime expirationDateTime (relativeExpiration);
        expiration = expirationDateTime;
        hr = notification->put_ExpirationTime (&expirationDateTime);
        if (FAILED (hr))
            return failWith (ToastNotification::Error::notDisplayed);
    }

    EventRegistrationToken activatedToken, dismissedToken, failedToken;
    const int64 id = (state.nextId += 1) - 1;

    ToastCallbacks callbacks;
    callbacks.onActivated = toast.onActivated;
    callbacks.onActivatedWithAction = toast.onActivatedWithAction;
    callbacks.onDismissed = toast.onDismissed;
    callbacks.onFailed = toast.onFailed;

    hr = Util::setEventHandlers (notification.Get(), std::move (callbacks), expiration, activatedToken, dismissedToken, failedToken, [&state, id]()
    {
        markAsReadyForDeletion (state, id);
    });

    if (FAILED (hr))
        return failWith (ToastNotification::Error::invalidHandler);

    {
        ScopedLock lock (state.bufferLock);
        state.buffer.emplace (id, NotifyData (notification, activatedToken, dismissedToken, failedToken));
    }

    hr = toastNotifier->Show (notification.Get());
    if (FAILED (hr))
    {
        ScopedLock lock (state.bufferLock);
        state.buffer.erase (id);

        return failWith (ToastNotification::Error::notDisplayed);
    }

    return makeResultValueOk (id);
}

//==============================================================================
ResultValue<int64> toastNotificationShow (const ToastTemplate& toast, const ToastNotificationSettings& settings, std::function<void (const ResultValue<int64>&)> completion)
{
    const auto result = showToastImpl (toast, settings);

    if (completion)
        completion (result);

    return result;
}

//==============================================================================
void toastNotificationGetPermissionState (std::function<void (ToastNotification::PermissionState)> callback)
{
    if (callback)
        callback (ToastNotification::PermissionState::granted);
}

void toastNotificationRequestPermission (std::function<void (ToastNotification::PermissionState)> callback)
{
    if (callback)
        callback (ToastNotification::PermissionState::granted);
}

void toastNotificationSetPermissionStateChangedCallback (std::function<void (ToastNotification::PermissionState)>)
{
    // Windows has no user-facing notification permission.
}

//==============================================================================
bool toastNotificationHide (int64 id)
{
    auto& state = getToastState();

    if (! state.isInitialized)
        return false;

    bool succeeded = false;
    ComPtr<IToastNotifier> toastNotifier = notifier (state.aumi, succeeded);
    if (! succeeded)
        return false;

    ComPtr<IToastNotification> notification;

    {
        ScopedLock lock (state.bufferLock);
        const auto iter = state.buffer.find (id);
        if (iter == state.buffer.end())
            return false;

        notification = iter->second.getNotification();
    }

    if (FAILED (toastNotifier->Hide (notification.Get())))
        return false;

    {
        ScopedLock lock (state.bufferLock);
        const auto iter = state.buffer.find (id);
        if (iter == state.buffer.end())
            return false;

        iter->second.markAsReadyForDeletion();
        iter->second.removeTokens();
        state.buffer.erase (iter);
    }

    return true;
}

//==============================================================================
void toastNotificationClear()
{
    auto& state = getToastState();

    bool succeeded = false;
    ComPtr<IToastNotifier> toastNotifier = notifier (state.aumi, succeeded);
    if (! succeeded)
        return;

    {
        ScopedLock lock (state.bufferLock);
        for (auto& entry : state.buffer)
        {
            entry.second.removeTokens();
            toastNotifier->Hide (entry.second.getNotification());
        }

        state.buffer.clear();
    }
}

} // namespace detail
} // namespace yup

#endif // YUP_WINDOWS
