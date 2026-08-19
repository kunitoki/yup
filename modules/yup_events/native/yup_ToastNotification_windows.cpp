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
#include <windows.ui.notifications.h>
#include <roapi.h>
#include <propvarutil.h>
#include <functiondiscoverykeys.h>
#include <winstring.h>

YUP_END_IGNORE_WARNINGS_MSVC

#include <cwchar>
#include <map>

#if ! YUP_DONT_AUTOLINK_TO_WIN32_LIBRARIES
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "user32")
#endif

namespace yup
{
namespace detail
{

using namespace ABI::Windows::Data::Xml::Dom;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::UI::Notifications;

using WinRTToastNotification = ABI::Windows::UI::Notifications::ToastNotification;

namespace
{

//==============================================================================
// Dynamically loaded WinRT entry points. Resolving them lazily keeps the module
// linkable on systems without the WinRT API surface; the libraries stay open
// for the lifetime of the process.
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

template <typename FunctionType>
bool loadFunction (DynamicLibrary& library, const char* name, FunctionType& function)
{
    function = reinterpret_cast<FunctionType> (library.getFunction (name));
    return function != nullptr;
}

bool initialize()
{
    if (windowsDeleteString != nullptr)
        return true;

    return shell32Library.open ("SHELL32.DLL")
        && loadFunction (shell32Library, "SetCurrentProcessExplicitAppUserModelID", setCurrentProcessExplicitAppUserModelID)
        && propsysLibrary.open ("PROPSYS.DLL")
        && loadFunction (propsysLibrary, "PropVariantToString", propVariantToString)
        && combaseLibrary.open ("COMBASE.DLL")
        && loadFunction (combaseLibrary, "RoGetActivationFactory", roGetActivationFactory)
        && loadFunction (combaseLibrary, "WindowsCreateStringReference", windowsCreateStringReference)
        && loadFunction (combaseLibrary, "WindowsGetStringRawBuffer", windowsGetStringRawBuffer)
        && loadFunction (combaseLibrary, "WindowsDeleteString", windowsDeleteString);
}

template <typename Type>
HRESULT getActivationFactory (HSTRING activatableClassId, ComSmartPtr<Type>& factory)
{
    return roGetActivationFactory (activatableClassId,
                                   __uuidof (Type),
                                   reinterpret_cast<void**> (factory.resetAndGetPointerAddress()));
}
} // namespace DllImporter

//==============================================================================
/*  A scoped HSTRING referencing a String's own UTF-16 buffer, without copying.

    The String is held by value on purpose: toWideCharPointer() returns a
    pointer into the string's own storage, so it stays valid for exactly as long
    as this object does. The HSTRING must not outlive it either, which is why
    every call site passes a temporary directly into a WinRT call.
*/
class ScopedHString
{
public:
    explicit ScopedHString (String stringToUse)
        : text (std::move (stringToUse))
    {
        const auto* buffer = text.toWideCharPointer();
        const auto length = static_cast<UINT32> (std::wcslen (buffer));

        if (FAILED (DllImporter::windowsCreateStringReference (buffer, length, &header, &handle)))
        {
            jassertfalse;
            handle = nullptr;
        }
    }

    ~ScopedHString()
    {
        DllImporter::windowsDeleteString (handle);
    }

    operator HSTRING() const noexcept { return handle; }

private:
    String text;
    HSTRING handle = nullptr;
    HSTRING_HEADER header {};

    YUP_DECLARE_NON_COPYABLE (ScopedHString)
};

//==============================================================================
// Reads an HSTRING returned by a WinRT call and releases it.
String consumeHString (HSTRING handle)
{
    const auto* buffer = DllImporter::windowsGetStringRawBuffer (handle, nullptr);
    String result (buffer != nullptr ? buffer : L"");
    DllImporter::windowsDeleteString (handle);
    return result;
}

//==============================================================================
// A WinRT DateTime counts 100-nanosecond ticks since 1601-01-01, while Time
// counts milliseconds since 1970-01-01.
DateTime toDateTime (Time time) noexcept
{
    constexpr int64 ticksPerMillisecond = 10000;
    constexpr int64 ticksBetweenEpochs = 116444736000000000LL;

    DateTime dateTime;
    dateTime.UniversalTime = time.toMilliseconds() * ticksPerMillisecond + ticksBetweenEpochs;
    return dateTime;
}

/*  The IReference<DateTime> handed to IToastNotification::put_ExpirationTime.

    The notification retains it for as long as it lives, so it is reference
    counted rather than owned by the caller.
*/
class DateTimeReference final : public ComBaseClassHelper<IReference<DateTime>>
{
public:
    explicit DateTimeReference (Time time)
        : dateTime (toDateTime (time))
    {
    }

    YUP_COMRESULT get_Value (DateTime* value) override
    {
        if (value == nullptr)
            return E_POINTER;

        *value = dateTime;
        return S_OK;
    }

    YUP_COMRESULT GetIids (ULONG*, IID**) override { return E_NOTIMPL; }

    YUP_COMRESULT GetRuntimeClassName (HSTRING*) override { return E_NOTIMPL; }

    YUP_COMRESULT GetTrustLevel (TrustLevel*) override { return E_NOTIMPL; }

private:
    DateTime dateTime {};
};

//==============================================================================
using ActivatedHandler = ITypedEventHandler<WinRTToastNotification*, IInspectable*>;
using DismissedHandler = ITypedEventHandler<WinRTToastNotification*, ToastDismissedEventArgs*>;
using FailedHandler = ITypedEventHandler<WinRTToastNotification*, ToastFailedEventArgs*>;

/*  Adapts a callable onto one of the toast's event handler interfaces. All
    three of them report the notification as their first argument, which is of
    no use here as the caller already knows which toast it subscribed to.
*/
template <typename HandlerType, typename ArgsType>
class ToastEventHandler final : public ComBaseClassHelper<HandlerType>
{
public:
    explicit ToastEventHandler (std::function<void (ArgsType*)> callbackToUse)
        : callback (std::move (callbackToUse))
    {
    }

    YUP_COMRESULT Invoke (IToastNotification*, ArgsType* args) override
    {
        callback (args);
        return S_OK;
    }

private:
    std::function<void (ArgsType*)> callback;
};

template <typename HandlerType, typename ArgsType, typename CallbackType>
ComSmartPtr<HandlerType> makeToastEventHandler (CallbackType&& callback)
{
    auto* handler = new ToastEventHandler<HandlerType, ArgsType> (std::forward<CallbackType> (callback));
    return becomeComSmartPtrOwner (static_cast<HandlerType*> (handler));
}

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

struct EventTokens
{
    EventRegistrationToken activated {};
    EventRegistrationToken dismissed {};
    EventRegistrationToken failed {};
};

// A displayed notification plus the tokens of its event handlers.
class NotifyData
{
public:
    NotifyData (ComSmartPtr<IToastNotification> notificationToUse, EventTokens tokensToUse)
        : notification (std::move (notificationToUse))
        , tokens (tokensToUse)
    {
    }

    ~NotifyData()
    {
        removeTokens();
    }

    void removeTokens()
    {
        if (tokensRemoved || notification == nullptr)
            return;

        notification->remove_Activated (tokens.activated);
        notification->remove_Dismissed (tokens.dismissed);
        notification->remove_Failed (tokens.failed);
        tokensRemoved = true;
    }

    void markAsReadyForDeletion() { readyForDeletion = true; }

    bool isReadyForDeletion() const { return readyForDeletion; }

    IToastNotification* getNotification() const { return notification; }

private:
    ComSmartPtr<IToastNotification> notification;
    EventTokens tokens;
    bool readyForDeletion = false;
    bool tokensRemoved = false;

    YUP_DECLARE_NON_COPYABLE (NotifyData)
};

//==============================================================================
// The per-process state of the toast backend.
struct ToastState
{
    bool isInitialized = false;
    bool hasCoInitialized = false;
    ToastNotification::ShortcutPolicy shortcutPolicy = ToastNotification::ShortcutPolicy::requireCreate;
    String appName;
    String aumi;
    std::map<int64, NotifyData> buffer;
    CriticalSection bufferLock;
    Atomic<int64> nextId { 1 };

    bool isCompatible() const { return DllImporter::initialize(); }
};

ToastState& getToastState()
{
    static ToastState state;
    return state;
}

void markAsReadyForDeletion (int64 id)
{
    auto& state = getToastState();
    const ScopedLock lock (state.bufferLock);

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

    // Mark this toast as ready for deletion, so it is removed on the next flush.
    const auto iter = state.buffer.find (id);
    if (iter != state.buffer.end())
        iter->second.markAsReadyForDeletion();
}

//==============================================================================
// SystemStats reports the Windows family, which is what gates the adaptive
// toast features as a whole.
bool isSupportingModernFeatures()
{
    return SystemStats::getOperatingSystemType() >= SystemStats::Windows10;
}

// Individual features are gated on the build number instead, which SystemStats
// does not expose.
int getWindowsBuildNumber()
{
    if (const auto ntdll = ::GetModuleHandleW (L"ntdll.dll"))
    {
        using RtlGetVersionFn = LONG (WINAPI*) (PRTL_OSVERSIONINFOW);

        if (const auto rtlGetVersion = reinterpret_cast<RtlGetVersionFn> (::GetProcAddress (ntdll, "RtlGetVersion")))
        {
            RTL_OSVERSIONINFOW versionInfo {};
            versionInfo.dwOSVersionInfoSize = sizeof (versionInfo);

            if (rtlGetVersion (&versionInfo) == 0)
                return static_cast<int> (versionInfo.dwBuildNumber);
        }
    }

    return 0;
}

//==============================================================================
// Thin wrappers over the WinRT XML DOM, which is several calls deep for every
// read and write. A null return means "absent or unreadable"; the callers map
// every such failure onto the same user-facing error.

ComSmartPtr<IXmlNode> getNodeByTagName (IXmlDocument* xml, const String& tagName, uint32 index = 0)
{
    ComSmartPtr<IXmlNodeList> nodeList;
    if (FAILED (xml->GetElementsByTagName (ScopedHString (tagName), nodeList.resetAndGetPointerAddress())))
        return {};

    ComSmartPtr<IXmlNode> node;
    if (FAILED (nodeList->Item (index, node.resetAndGetPointerAddress())))
        return {};

    return node;
}

ComSmartPtr<IXmlElement> getElementByTagName (IXmlDocument* xml, const String& tagName, uint32 index = 0)
{
    const auto node = getNodeByTagName (xml, tagName, index);

    if (node == nullptr)
        return {};

    return node.getInterface<IXmlElement>();
}

uint32 getNodeCountByTagName (IXmlDocument* xml, const String& tagName)
{
    ComSmartPtr<IXmlNodeList> nodeList;
    if (FAILED (xml->GetElementsByTagName (ScopedHString (tagName), nodeList.resetAndGetPointerAddress())))
        return 0;

    UINT32 length = 0;
    return SUCCEEDED (nodeList->get_Length (&length)) ? length : 0;
}

bool setElementAttribute (IXmlDocument* xml, const String& tagName, const String& name, const String& value)
{
    const auto element = getElementByTagName (xml, tagName);

    return element != nullptr
        && SUCCEEDED (element->SetAttribute (ScopedHString (name), ScopedHString (value)));
}

// The DOM holds the value of both elements and attributes in a child text node,
// so a value is assigned by appending one.
bool setNodeValue (IXmlDocument* xml, IXmlNode* node, const String& value)
{
    if (node == nullptr)
        return false;

    ComSmartPtr<IXmlText> textNode;
    if (FAILED (xml->CreateTextNode (ScopedHString (value), textNode.resetAndGetPointerAddress())))
        return false;

    const auto valueNode = textNode.getInterface<IXmlNode>();
    if (valueNode == nullptr)
        return false;

    ComSmartPtr<IXmlNode> appendedChild;
    return SUCCEEDED (node->AppendChild (valueNode, appendedChild.resetAndGetPointerAddress()));
}

ComSmartPtr<IXmlNamedNodeMap> getAttributes (IXmlNode* node)
{
    ComSmartPtr<IXmlNamedNodeMap> attributes;

    if (node == nullptr || FAILED (node->get_Attributes (attributes.resetAndGetPointerAddress())))
        return {};

    return attributes;
}

ComSmartPtr<IXmlNode> getAttributeNode (IXmlNode* node, const String& name)
{
    const auto attributes = getAttributes (node);
    if (attributes == nullptr)
        return {};

    ComSmartPtr<IXmlNode> attribute;
    if (FAILED (attributes->GetNamedItem (ScopedHString (name), attribute.resetAndGetPointerAddress())))
        return {};

    return attribute;
}

ComSmartPtr<IXmlNode> addAttributeNode (IXmlDocument* xml, IXmlNode* node, const String& name)
{
    const auto attributes = getAttributes (node);
    if (attributes == nullptr)
        return {};

    ComSmartPtr<IXmlAttribute> attribute;
    if (FAILED (xml->CreateAttribute (ScopedHString (name), attribute.resetAndGetPointerAddress())))
        return {};

    const auto attributeNode = attribute.getInterface<IXmlNode>();
    if (attributeNode == nullptr)
        return {};

    ComSmartPtr<IXmlNode> replacedNode;
    if (FAILED (attributes->SetNamedItem (attributeNode, replacedNode.resetAndGetPointerAddress())))
        return {};

    return attributeNode;
}

// Returns the named attribute, creating an empty one when the template that the
// node came from does not declare it.
ComSmartPtr<IXmlNode> getOrAddAttributeNode (IXmlDocument* xml, IXmlNode* node, const String& name)
{
    if (const auto attribute = getAttributeNode (node, name))
        return attribute;

    return addAttributeNode (xml, node, name);
}

bool removeAttributeNode (IXmlNode* node, const String& name)
{
    const auto attributes = getAttributes (node);
    if (attributes == nullptr)
        return false;

    ComSmartPtr<IXmlNode> removedNode;
    return SUCCEEDED (attributes->RemoveNamedItem (ScopedHString (name), removedNode.resetAndGetPointerAddress()));
}

ComSmartPtr<IXmlNode> appendElement (IXmlDocument* xml, IXmlNode* parent, const String& tagName)
{
    ComSmartPtr<IXmlElement> element;

    if (parent == nullptr || FAILED (xml->CreateElement (ScopedHString (tagName), element.resetAndGetPointerAddress())))
        return {};

    const auto elementNode = element.getInterface<IXmlNode>();
    if (elementNode == nullptr)
        return {};

    ComSmartPtr<IXmlNode> appendedNode;
    if (FAILED (parent->AppendChild (elementNode, appendedNode.resetAndGetPointerAddress())))
        return {};

    return appendedNode;
}

// Appends an element, carrying the given empty attributes, to the first node
// with the root tag name.
ComSmartPtr<IXmlNode> appendElementTo (IXmlDocument* xml, const String& rootTagName, const String& tagName, const StringArray& attributeNames = {})
{
    const auto node = appendElement (xml, getNodeByTagName (xml, rootTagName), tagName);
    if (node == nullptr)
        return {};

    for (const auto& name : attributeNames)
    {
        if (addAttributeNode (xml, node, name) == nullptr)
            return {};
    }

    return node;
}

String getXmlString (const ComSmartPtr<IXmlDocument>& xml)
{
    const auto serializer = xml.getInterface<IXmlNodeSerializer>();
    if (serializer == nullptr)
        return {};

    HSTRING xmlString = nullptr;
    if (FAILED (serializer->GetXml (&xmlString)))
        return {};

    return consumeHString (xmlString);
}

//==============================================================================
// The toast schema declares "scenario" as an enumeration of exactly
// "reminder" | "alarm" | "incomingCall" | "urgent". The default scenario is
// expressed by omitting the attribute, so an empty string is returned for it.
// https://learn.microsoft.com/en-us/uwp/schemas/tiles/toastschema/element-toast
String getScenarioName (ToastTemplate::Scenario scenario)
{
    switch (scenario)
    {
        case ToastTemplate::Scenario::alarm:
            return "alarm";
        case ToastTemplate::Scenario::incomingCall:
            return "incomingCall";
        case ToastTemplate::Scenario::reminder:
            return "reminder";
        case ToastTemplate::Scenario::default_:
            break;
    }

    return {};
}

String getAudioSystemFileUri (ToastTemplate::AudioSystemFile file)
{
    switch (file)
    {
        case ToastTemplate::AudioSystemFile::defaultSound:
            return "ms-winsoundevent:Notification.Default";
        case ToastTemplate::AudioSystemFile::im:
            return "ms-winsoundevent:Notification.IM";
        case ToastTemplate::AudioSystemFile::mail:
            return "ms-winsoundevent:Notification.Mail";
        case ToastTemplate::AudioSystemFile::reminder:
            return "ms-winsoundevent:Notification.Reminder";
        case ToastTemplate::AudioSystemFile::sms:
            return "ms-winsoundevent:Notification.SMS";
        case ToastTemplate::AudioSystemFile::alarm:
            return "ms-winsoundevent:Notification.Looping.Alarm";
        case ToastTemplate::AudioSystemFile::alarm2:
            return "ms-winsoundevent:Notification.Looping.Alarm2";
        case ToastTemplate::AudioSystemFile::alarm3:
            return "ms-winsoundevent:Notification.Looping.Alarm3";
        case ToastTemplate::AudioSystemFile::alarm4:
            return "ms-winsoundevent:Notification.Looping.Alarm4";
        case ToastTemplate::AudioSystemFile::alarm5:
            return "ms-winsoundevent:Notification.Looping.Alarm5";
        case ToastTemplate::AudioSystemFile::alarm6:
            return "ms-winsoundevent:Notification.Looping.Alarm6";
        case ToastTemplate::AudioSystemFile::alarm7:
            return "ms-winsoundevent:Notification.Looping.Alarm7";
        case ToastTemplate::AudioSystemFile::alarm8:
            return "ms-winsoundevent:Notification.Looping.Alarm8";
        case ToastTemplate::AudioSystemFile::alarm9:
            return "ms-winsoundevent:Notification.Looping.Alarm9";
        case ToastTemplate::AudioSystemFile::alarm10:
            return "ms-winsoundevent:Notification.Looping.Alarm10";
        case ToastTemplate::AudioSystemFile::call:
            return "ms-winsoundevent:Notification.Looping.Call";
        case ToastTemplate::AudioSystemFile::call1:
            return "ms-winsoundevent:Notification.Looping.Call1";
        case ToastTemplate::AudioSystemFile::call2:
            return "ms-winsoundevent:Notification.Looping.Call2";
        case ToastTemplate::AudioSystemFile::call3:
            return "ms-winsoundevent:Notification.Looping.Call3";
        case ToastTemplate::AudioSystemFile::call4:
            return "ms-winsoundevent:Notification.Looping.Call4";
        case ToastTemplate::AudioSystemFile::call5:
            return "ms-winsoundevent:Notification.Looping.Call5";
        case ToastTemplate::AudioSystemFile::call6:
            return "ms-winsoundevent:Notification.Looping.Call6";
        case ToastTemplate::AudioSystemFile::call7:
            return "ms-winsoundevent:Notification.Looping.Call7";
        case ToastTemplate::AudioSystemFile::call8:
            return "ms-winsoundevent:Notification.Looping.Call8";
        case ToastTemplate::AudioSystemFile::call9:
            return "ms-winsoundevent:Notification.Looping.Call9";
        case ToastTemplate::AudioSystemFile::call10:
            return "ms-winsoundevent:Notification.Looping.Call10";
    }

    jassertfalse;
    return {};
}

bool isToastGeneric (const ToastTemplate& toast)
{
    return toast.hasHeroImage() || toast.getCropHint() == ToastTemplate::CropHint::circle;
}

//==============================================================================
// The start menu shortcut carrying the app user model id, without which Windows
// refuses to display notifications for an unpackaged app.
File getShellLinkFile (const String& appName)
{
    return File::getSpecialLocation (File::userApplicationDataDirectory)
        .getChildFile ("Microsoft/Windows/Start Menu/Programs")
        .getChildFile (appName + ".lnk");
}

File getExecutableFile()
{
    return File::getSpecialLocation (File::hostApplicationPath);
}

Result validateShellLink (const ToastState& state)
{
    const auto linkFile = getShellLinkFile (state.appName);
    if (! linkFile.existsAsFile())
        return Result::fail ("The shell link does not exist");

    ComSmartPtr<IShellLinkW> shellLink;
    if (FAILED (shellLink.CoCreateInstance (CLSID_ShellLink)))
        return Result::fail ("Could not create a shell link");

    const auto persistFile = shellLink.getInterface<IPersistFile>();
    if (persistFile == nullptr || FAILED (persistFile->Load (linkFile.getFullPathName().toWideCharPointer(), STGM_READ)))
        return Result::fail ("Could not load the shell link");

    const auto propertyStore = shellLink.getInterface<IPropertyStore>();
    if (propertyStore == nullptr)
        return Result::fail ("Could not access the shell link properties");

    PROPVARIANT appIdPropVar;
    if (FAILED (propertyStore->GetValue (PKEY_AppUserModel_ID, &appIdPropVar)))
        return Result::fail ("Could not read the shell link app user model id");

    wchar_t aumi[MAX_PATH] = { L'\0' };
    const auto readAumi = SUCCEEDED (DllImporter::propVariantToString (appIdPropVar, aumi, MAX_PATH));
    ::PropVariantClear (&appIdPropVar);

    if (! readAumi || state.aumi != String (aumi))
        return Result::fail ("The shell link carries a different app user model id");

    wchar_t targetPath[MAX_PATH] = { L'\0' };
    if (FAILED (shellLink->GetPath (targetPath, MAX_PATH, nullptr, SLGP_RAWPATH))
        || File (String (targetPath)) != getExecutableFile())
        return Result::fail ("The shell link points at a different executable");

    wchar_t workingDirectory[MAX_PATH] = { L'\0' };
    if (FAILED (shellLink->GetWorkingDirectory (workingDirectory, MAX_PATH))
        || File (String (workingDirectory)) != getExecutableFile().getParentDirectory())
        return Result::fail ("The shell link has a different working directory");

    return Result::ok();
}

Result createShellLink (const ToastState& state)
{
    if (state.shortcutPolicy != ToastNotification::ShortcutPolicy::requireCreate)
        return Result::fail ("The shortcut policy does not allow creating a shell link");

    const auto executable = getExecutableFile();
    const auto executablePath = executable.getFullPathName();

    ComSmartPtr<IShellLinkW> shellLink;
    if (FAILED (shellLink.CoCreateInstance (CLSID_ShellLink)))
        return Result::fail ("Could not create a shell link");

    if (FAILED (shellLink->SetPath (executablePath.toWideCharPointer()))
        || FAILED (shellLink->SetIconLocation (executablePath.toWideCharPointer(), 0))
        || FAILED (shellLink->SetArguments (L""))
        || FAILED (shellLink->SetWorkingDirectory (executable.getParentDirectory().getFullPathName().toWideCharPointer())))
        return Result::fail ("Could not populate the shell link");

    const auto propertyStore = shellLink.getInterface<IPropertyStore>();
    if (propertyStore == nullptr)
        return Result::fail ("Could not access the shell link properties");

    PROPVARIANT appIdPropVar;
    if (FAILED (::InitPropVariantFromString (state.aumi.toWideCharPointer(), &appIdPropVar)))
        return Result::fail ("Could not create the app user model id property");

    const auto hr = propertyStore->SetValue (PKEY_AppUserModel_ID, appIdPropVar);
    ::PropVariantClear (&appIdPropVar);

    if (FAILED (hr) || FAILED (propertyStore->Commit()))
        return Result::fail ("Could not store the app user model id in the shell link");

    const auto persistFile = shellLink.getInterface<IPersistFile>();
    if (persistFile == nullptr)
        return Result::fail ("Could not access the shell link file");

    if (FAILED (persistFile->Save (getShellLinkFile (state.appName).getFullPathName().toWideCharPointer(), TRUE)))
        return Result::fail ("Could not save the shell link");

    return Result::ok();
}

Result createShortcut (ToastState& state)
{
    if (state.aumi.isEmpty() || state.appName.isEmpty())
        return Result::fail (ToastNotification::getErrorDescription (ToastNotification::Error::invalidParameters));

    if (! state.isCompatible())
        return Result::fail (ToastNotification::getErrorDescription (ToastNotification::Error::systemNotSupported));

    if (! state.hasCoInitialized)
    {
        const auto hr = ::CoInitializeEx (nullptr, COINIT_MULTITHREADED);

        if (hr != RPC_E_CHANGED_MODE)
        {
            if (FAILED (hr) && hr != S_FALSE)
                return Result::fail ("Could not initialize COM");

            state.hasCoInitialized = true;
        }
    }

    // A shell link that no longer matches the executable is rebuilt from
    // scratch rather than patched in place.
    if (validateShellLink (state).wasOk())
        return Result::ok();

    return createShellLink (state);
}

//==============================================================================
// Toast payload builders. Each one edits the XML document that
// GetTemplateContent() produced for the template type of the toast.

Result setTextField (IXmlDocument* xml, const String& text, uint32 position)
{
    if (! setNodeValue (xml, getNodeByTagName (xml, "text", position), text))
        return Result::fail ("Could not set the text field at position " + String (position));

    return Result::ok();
}

// Attribution text is an adaptive toast feature, expressed as an extra
// <text placement="attribution"> appended to the binding.
Result setAttributionTextField (IXmlDocument* xml, const String& text)
{
    const auto node = appendElementTo (xml, "binding", "text", { "placement" });
    if (node == nullptr)
        return Result::fail ("Could not create the attribution text element");

    if (! setNodeValue (xml, getAttributeNode (node, "placement"), "attribution"))
        return Result::fail ("Could not mark the attribution text element");

    // The element was appended last, so it is also the last <text> in the
    // document order that GetElementsByTagName() reports.
    const auto textCount = getNodeCountByTagName (xml, "text");
    if (textCount == 0)
        return Result::fail ("Could not locate the attribution text element");

    return setTextField (xml, text, textCount - 1);
}

Result setImageField (IXmlDocument* xml, const File& imageFile, bool useAppLogoPlacement, bool useCircleCropHint)
{
    // A toast whose image cannot be resolved is dropped by the platform without
    // reporting an error, so an unreachable file is caught here instead.
    if (! imageFile.existsAsFile())
        return Result::fail ("The toast image does not exist: " + imageFile.getFullPathName());

    const auto node = getNodeByTagName (xml, "image");
    if (node == nullptr)
        return Result::fail ("The template carries no image element");

    // A template that does not expose the image as an element is left with its
    // default placement, exactly as the source it was ported from does.
    if (useAppLogoPlacement)
    {
        if (const auto element = node.getInterface<IXmlElement>())
        {
            if (FAILED (element->SetAttribute (ScopedHString ("placement"), ScopedHString ("appLogoOverride"))))
                return Result::fail ("Could not set the image placement");

            if (useCircleCropHint
                && FAILED (element->SetAttribute (ScopedHString ("hint-crop"), ScopedHString ("circle"))))
                return Result::fail ("Could not set the image crop hint");
        }
    }

    // A file:// URI uses forward slashes, a Windows path uses backslashes.
    const auto imageUri = "file:///" + imageFile.getFullPathName().replaceCharacter ('\\', '/');

    if (! setNodeValue (xml, getOrAddAttributeNode (xml, node, "src"), imageUri))
        return Result::fail ("Could not set the image source");

    return Result::ok();
}

Result setHeroImageField (IXmlDocument* xml, const File& imageFile, bool isInlineImage)
{
    const auto node = appendElementTo (xml, "binding", "image");
    if (node == nullptr)
        return Result::fail ("Could not create the hero image element");

    const auto element = node.getInterface<IXmlElement>();
    if (element == nullptr)
        return Result::fail ("Could not create the hero image element");

    if (! isInlineImage
        && FAILED (element->SetAttribute (ScopedHString ("placement"), ScopedHString ("hero"))))
        return Result::fail ("Could not set the hero image placement");

    if (FAILED (element->SetAttribute (ScopedHString ("src"), ScopedHString (imageFile.getFullPathName()))))
        return Result::fail ("Could not set the hero image source");

    return Result::ok();
}

Result setAudioField (IXmlDocument* xml, const String& path, ToastTemplate::AudioOption option)
{
    StringArray attributeNames;

    if (path.isNotEmpty())
        attributeNames.add ("src");

    if (option == ToastTemplate::AudioOption::loop)
        attributeNames.add ("loop");
    else if (option == ToastTemplate::AudioOption::silent)
        attributeNames.add ("silent");

    const auto node = appendElementTo (xml, "toast", "audio", attributeNames);
    if (node == nullptr)
        return Result::fail ("Could not create the audio element");

    if (path.isNotEmpty() && ! setNodeValue (xml, getAttributeNode (node, "src"), path))
        return Result::fail ("Could not set the audio source");

    if (option == ToastTemplate::AudioOption::loop
        && ! setNodeValue (xml, getAttributeNode (node, "loop"), "true"))
        return Result::fail ("Could not set the audio loop flag");

    if (option == ToastTemplate::AudioOption::silent
        && ! setNodeValue (xml, getAttributeNode (node, "silent"), "true"))
        return Result::fail ("Could not set the audio silent flag");

    return Result::ok();
}

Result addAction (IXmlDocument* xml, const String& content, const String& arguments)
{
    ComSmartPtr<IXmlNode> actionsNode;

    if (getNodeCountByTagName (xml, "actions") > 0)
    {
        actionsNode = getNodeByTagName (xml, "actions");
    }
    else
    {
        // Buttons are rendered by the adaptive template only, which also wants
        // the longer display duration.
        if (! setElementAttribute (xml, "toast", "template", "ToastGeneric")
            || ! setElementAttribute (xml, "toast", "duration", "long"))
            return Result::fail ("Could not switch the toast to the adaptive template");

        actionsNode = appendElementTo (xml, "toast", "actions");
    }

    const auto node = appendElement (xml, actionsNode, "action");
    if (node == nullptr)
        return Result::fail ("Could not create the action element");

    const auto element = node.getInterface<IXmlElement>();

    if (element == nullptr
        || FAILED (element->SetAttribute (ScopedHString ("content"), ScopedHString (content)))
        || FAILED (element->SetAttribute (ScopedHString ("arguments"), ScopedHString (arguments))))
        return Result::fail ("Could not create the action element");

    return Result::ok();
}

// Turns the legacy template that the toast was created from into an adaptive
// (ToastGeneric) one, which is what Windows 10 and above render.
Result convertToAdaptiveToast (IXmlDocument* xml)
{
    if (! setElementAttribute (xml, "binding", "template", "ToastGeneric"))
        return Result::fail ("Could not switch the binding to the adaptive template");

    // The legacy image templates carry a placement the adaptive one rejects.
    const auto imageCount = getNodeCountByTagName (xml, "image");

    for (uint32 i = 0; i < imageCount; ++i)
    {
        const auto node = getNodeByTagName (xml, "image", i);

        if (node != nullptr && getAttributeNode (node, "placement") != nullptr
            && ! removeAttributeNode (node, "placement"))
            return Result::fail ("Could not remove the image placement attribute");
    }

    if (! setElementAttribute (xml, "toast", "template", "ToastGeneric"))
        return Result::fail ("Could not switch the toast to the adaptive template");

    return Result::ok();
}

//==============================================================================
// The activation argument carries the index of the action button that was
// pressed; it is empty when the toast body itself was clicked.
String getActivationArguments (IInspectable* inspectable)
{
    if (inspectable == nullptr)
        return {};

    ComSmartPtr<IToastActivatedEventArgs> activatedEventArgs;
    if (FAILED (inspectable->QueryInterface (__uuidof (IToastActivatedEventArgs),
                                             reinterpret_cast<void**> (activatedEventArgs.resetAndGetPointerAddress()))))
        return {};

    HSTRING argumentsHandle = nullptr;
    if (FAILED (activatedEventArgs->get_Arguments (&argumentsHandle)))
        return {};

    return consumeHString (argumentsHandle);
}

bool addEventHandlers (IToastNotification* notification, ToastCallbacks callbacks, Time expirationTime, int64 id, EventTokens& tokens)
{
    const auto activated = makeToastEventHandler<ActivatedHandler, IInspectable> (
        [callbacks, id] (IInspectable* inspectable)
    {
        if (const auto arguments = getActivationArguments (inspectable); arguments.isNotEmpty())
        {
            if (callbacks.onActivatedWithAction)
                callbacks.onActivatedWithAction (arguments.getIntValue());
        }
        else if (callbacks.onActivated)
        {
            callbacks.onActivated();
        }

        markAsReadyForDeletion (id);
    });

    if (FAILED (notification->add_Activated (activated, &tokens.activated)))
        return false;

    const auto dismissed = makeToastEventHandler<DismissedHandler, IToastDismissedEventArgs> (
        [callbacks, expirationTime, id] (IToastDismissedEventArgs* eventArgs)
    {
        auto reason = ToastDismissalReason_UserCanceled;

        if (eventArgs != nullptr && SUCCEEDED (eventArgs->get_Reason (&reason)))
        {
            // Windows reports an expired toast as if the user had cancelled it.
            if (reason == ToastDismissalReason_UserCanceled
                && expirationTime.toMilliseconds() != 0
                && Time::getCurrentTime() >= expirationTime)
                reason = ToastDismissalReason_TimedOut;

            if (callbacks.onDismissed)
                callbacks.onDismissed (static_cast<ToastTemplate::DismissalReason> (reason));
        }

        markAsReadyForDeletion (id);
    });

    if (FAILED (notification->add_Dismissed (dismissed, &tokens.dismissed)))
        return false;

    const auto failed = makeToastEventHandler<FailedHandler, IToastFailedEventArgs> (
        [callbacks, id] (IToastFailedEventArgs*)
    {
        if (callbacks.onFailed)
            callbacks.onFailed();

        markAsReadyForDeletion (id);
    });

    return SUCCEEDED (notification->add_Failed (failed, &tokens.failed));
}

//==============================================================================
ComSmartPtr<IToastNotificationManagerStatics> getNotificationManager()
{
    ComSmartPtr<IToastNotificationManagerStatics> notificationManager;

    if (! DllImporter::initialize()
        || FAILED (DllImporter::getActivationFactory (ScopedHString (RuntimeClass_Windows_UI_Notifications_ToastNotificationManager),
                                                      notificationManager)))
        return {};

    return notificationManager;
}

ComSmartPtr<IToastNotifier> createNotifier (const String& aumi)
{
    const auto notificationManager = getNotificationManager();
    if (notificationManager == nullptr)
        return {};

    ComSmartPtr<IToastNotifier> notifier;
    if (FAILED (notificationManager->CreateToastNotifierWithId (ScopedHString (aumi), notifier.resetAndGetPointerAddress())))
        return {};

    return notifier;
}

//==============================================================================
ResultValue<int64> showToastImpl (const ToastTemplate& toast, const ToastNotificationSettings& settings)
{
    auto& state = getToastState();

    if (! state.isInitialized)
        return makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::notInitialized));

    if (state.aumi.isEmpty())
        state.aumi = settings.appUserModelId;

    if (state.aumi.isEmpty())
        return makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::invalidParameters));

    const auto notificationManager = getNotificationManager();

    ComSmartPtr<IToastNotifier> notifier;
    ComSmartPtr<IToastNotificationFactory> notificationFactory;
    ComSmartPtr<IXmlDocument> xmlDocument;

    if (notificationManager == nullptr
        || FAILED (notificationManager->CreateToastNotifierWithId (ScopedHString (state.aumi), notifier.resetAndGetPointerAddress()))
        || FAILED (DllImporter::getActivationFactory (ScopedHString (RuntimeClass_Windows_UI_Notifications_ToastNotification), notificationFactory))
        || FAILED (notificationManager->GetTemplateContent (static_cast<ToastTemplateType> (toast.getType()),
                                                            xmlDocument.resetAndGetPointerAddress())))
        return makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::notDisplayed));

    // Every failure below is reported with the payload built so far, as the
    // platform gives no indication of what it rejected.
    const auto failWith = [&xmlDocument] (const String& message)
    {
        return makeResultValueFail (message + "\nXML: " + getXmlString (xmlDocument));
    };

    if (isSupportingModernFeatures())
    {
        if (const auto result = convertToAdaptiveToast (xmlDocument); result.failed())
            return failWith (result.getErrorMessage());
    }

    for (uint32 i = 0; i < static_cast<uint32> (toast.getTextFieldsCount()); ++i)
    {
        const auto text = toast.getTextField (static_cast<ToastTemplate::TextField> (i));

        if (const auto result = setTextField (xmlDocument, text, i); result.failed())
            return failWith (result.getErrorMessage());
    }

    if (isSupportingModernFeatures())
    {
        // Note that this runs after the template's own text fields have been
        // filled, as attribution text adds one more of them.
        if (toast.getAttributionText().isNotEmpty())
        {
            if (const auto result = setAttributionTextField (xmlDocument, toast.getAttributionText()); result.failed())
                return failWith (result.getErrorMessage());
        }

        for (size_t i = 0; i < toast.getActionsCount(); ++i)
        {
            if (const auto result = addAction (xmlDocument, toast.getActionLabel (i), String (static_cast<int> (i))); result.failed())
                return failWith (result.getErrorMessage());
        }

        const auto audioPath = toast.getAudioSystemFile().has_value()
                                 ? getAudioSystemFileUri (*toast.getAudioSystemFile())
                                 : toast.getAudioPath();

        if (audioPath.isNotEmpty() || toast.getAudioOption() != ToastTemplate::AudioOption::default_)
        {
            if (const auto result = setAudioField (xmlDocument, audioPath, toast.getAudioOption()); result.failed())
                return failWith (result.getErrorMessage());
        }

        if (toast.getDuration() != ToastTemplate::Duration::system)
        {
            const auto duration = toast.getDuration() == ToastTemplate::Duration::short_ ? "short" : "long";

            if (! setElementAttribute (xmlDocument, "toast", "duration", duration))
                return failWith ("Could not set the toast duration");
        }

        if (const auto scenario = getScenarioName (toast.getScenario()); scenario.isNotEmpty())
        {
            if (! setElementAttribute (xmlDocument, "toast", "scenario", scenario))
                return failWith ("Could not set the toast scenario");
        }
    }

    if (toast.hasImage())
    {
        // The circular crop hint arrived with the Windows 10 Anniversary Update.
        const auto useCircleCropHint = toast.getCropHint() == ToastTemplate::CropHint::circle
                                    && getWindowsBuildNumber() >= 14393;

        if (const auto result = setImageField (xmlDocument, toast.getImagePath(), isToastGeneric (toast), useCircleCropHint); result.failed())
            return failWith (result.getErrorMessage());
    }

    if (toast.hasHeroImage())
    {
        if (const auto result = setHeroImageField (xmlDocument, toast.getHeroImagePath(), toast.isInlineHeroImage()); result.failed())
            return failWith (result.getErrorMessage());
    }

    ComSmartPtr<IToastNotification> notification;
    if (FAILED (notificationFactory->CreateToastNotification (xmlDocument, notification.resetAndGetPointerAddress())))
        return failWith ("Could not create the toast notification");

    Time expirationTime;

    if (toast.getExpiration() > 0)
    {
        expirationTime = Time::getCurrentTime() + RelativeTime::milliseconds (toast.getExpiration());

        const auto expiration = becomeComSmartPtrOwner (new DateTimeReference (expirationTime));
        IReference<DateTime>* expirationValue = expiration;

        if (FAILED (notification->put_ExpirationTime (expirationValue)))
            return failWith ("Could not set the toast expiration time");
    }

    ToastCallbacks callbacks;
    callbacks.onActivated = toast.onActivated;
    callbacks.onActivatedWithAction = toast.onActivatedWithAction;
    callbacks.onDismissed = toast.onDismissed;
    callbacks.onFailed = toast.onFailed;

    const auto id = (state.nextId += 1) - 1;

    EventTokens tokens;
    if (! addEventHandlers (notification, std::move (callbacks), expirationTime, id, tokens))
        return makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::invalidHandler));

    {
        const ScopedLock lock (state.bufferLock);
        state.buffer.try_emplace (id, notification, tokens);
    }

    // The payload is logged as well, since the platform will happily accept a
    // notification it then decides not to show.
    YUP_DBG ("[yup toast] payload: " << getXmlString (xmlDocument));

    if (FAILED (notifier->Show (notification)))
    {
        const ScopedLock lock (state.bufferLock);
        state.buffer.erase (id);

        return failWith (ToastNotification::getErrorDescription (ToastNotification::Error::notDisplayed));
    }

    return makeResultValueOk (id);
}

} // namespace

//==============================================================================
Result toastNotificationInitialize (const ToastNotificationSettings& settings)
{
    auto& state = getToastState();

    if (! state.isCompatible())
        return Result::fail (ToastNotification::getErrorDescription (ToastNotification::Error::systemNotSupported));

    state.appName = settings.appName;
    state.aumi = settings.appUserModelId;
    state.shortcutPolicy = settings.shortcutPolicy;

    if (state.aumi.isEmpty() || state.appName.isEmpty())
        return Result::fail (ToastNotification::getErrorDescription (ToastNotification::Error::invalidParameters));

    if (state.shortcutPolicy != ToastNotification::ShortcutPolicy::ignore)
    {
        if (const auto result = createShortcut (state); result.failed())
            return Result::fail (ToastNotification::getErrorDescription (ToastNotification::Error::shellLinkNotCreated)
                                 + " (" + result.getErrorMessage() + ")");
    }

    if (FAILED (DllImporter::setCurrentProcessExplicitAppUserModelID (state.aumi.toWideCharPointer())))
        return Result::fail (ToastNotification::getErrorDescription (ToastNotification::Error::invalidAppUserModelID));

    state.isInitialized = true;
    return Result::ok();
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

    const auto notifier = createNotifier (state.aumi);
    if (notifier == nullptr)
        return false;

    ComSmartPtr<IToastNotification> notification;

    {
        const ScopedLock lock (state.bufferLock);

        const auto iter = state.buffer.find (id);
        if (iter == state.buffer.end())
            return false;

        notification = addComSmartPtrOwner (iter->second.getNotification());
    }

    if (FAILED (notifier->Hide (notification)))
        return false;

    const ScopedLock lock (state.bufferLock);

    const auto iter = state.buffer.find (id);
    if (iter == state.buffer.end())
        return false;

    iter->second.removeTokens();
    state.buffer.erase (iter);
    return true;
}

//==============================================================================
void toastNotificationClear()
{
    auto& state = getToastState();

    const auto notifier = createNotifier (state.aumi);

    const ScopedLock lock (state.bufferLock);

    for (auto& entry : state.buffer)
    {
        entry.second.removeTokens();

        if (notifier != nullptr)
            notifier->Hide (entry.second.getNotification());
    }

    state.buffer.clear();
}

} // namespace detail
} // namespace yup

#endif // YUP_WINDOWS
