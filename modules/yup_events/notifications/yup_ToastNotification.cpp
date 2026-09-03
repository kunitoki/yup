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

namespace yup
{

//==============================================================================
YUP_IMPLEMENT_SINGLETON (ToastNotification)

//==============================================================================
ToastTemplate::ToastTemplate (TemplateType type)
    : type (type)
{
    static constexpr std::array<size_t, 8> textFieldsPerType { 1, 2, 2, 3, 1, 2, 2, 3 };
    textFields.assign (textFieldsPerType[static_cast<size_t> (type)], String());
}

void ToastTemplate::setTextField (const String& text, TextField position)
{
    const auto index = static_cast<size_t> (position);
    if (index >= textFields.size())
        return;

    textFields[index] = text;
}

String ToastTemplate::getTextField (TextField position) const
{
    const auto index = static_cast<size_t> (position);
    return index < textFields.size() ? textFields[index] : String();
}

size_t ToastTemplate::getTextFieldsCount() const noexcept
{
    return textFields.size();
}

void ToastTemplate::setAttributionText (String newAttributionText)
{
    attributionText = std::move (newAttributionText);
}

void ToastTemplate::setImagePath (File newImagePath, CropHint newCropHint)
{
    imagePath = std::move (newImagePath);
    cropHint = newCropHint;
}

void ToastTemplate::setHeroImagePath (File newHeroImagePath, bool inlineImage)
{
    heroImagePath = std::move (newHeroImagePath);
    inlineHeroImage = inlineImage;
}

void ToastTemplate::setAudioPath (AudioSystemFile file)
{
    audioSystemFile = file;
    audioPath.clear();
}

void ToastTemplate::setAudioPath (String newAudioPath)
{
    audioPath = std::move (newAudioPath);
    audioSystemFile.reset();
}

void ToastTemplate::addAction (String label)
{
    actions.push_back (std::move (label));
}

const String& ToastTemplate::getActionLabel (size_t index) const
{
    jassert (index < actions.size());

    static const String emptyString;
    return index < actions.size() ? actions[index] : emptyString;
}

//==============================================================================
ToastNotification::ToastNotification()
{
    appName = File::getSpecialLocation (File::currentExecutableFile).getFileNameWithoutExtension();
}

ToastNotification::~ToastNotification()
{
    detail::toastNotificationClear();
    clearSingletonInstance();
}

//==============================================================================
Result ToastNotification::initialize()
{
    auto expected = State::created;
    if (state.compare_exchange_strong (expected, State::initializing))
    {
        detail::ToastNotificationSettings settings;
        settings.appName = appName;
        settings.appUserModelId = appUserModelId.isEmpty() ? appName : appUserModelId;
        settings.shortcutPolicy = shortcutPolicy;
        settings.fallbackImage = fallbackImage;

        state = detail::toastNotificationInitialize (settings).wasOk() ? State::initialized
                                                                       : State::failed;
    }

    while (true)
    {
        if (state.load (std::memory_order_relaxed) == State::initialized)
            return Result::ok();

        if (state.load (std::memory_order_relaxed) == State::failed)
            return Result::fail ("Initialization of toast notification failed");

        Thread::sleep (1);
    }
}

//==============================================================================
void ToastNotification::clear()
{
    detail::toastNotificationClear();
}

void ToastNotification::setAppName (String newAppName)
{
    appName = std::move (newAppName);
}

void ToastNotification::setAppUserModelId (String newAppUserModelId)
{
    appUserModelId = std::move (newAppUserModelId);
}

void ToastNotification::setFallbackImage (std::optional<File> newFallbackImage)
{
    fallbackImage = std::move (newFallbackImage);
}

//==============================================================================
String ToastNotification::configureAUMI (StringRef companyName, StringRef productName, StringRef subProduct, StringRef versionInformation)
{
    String aumi (companyName);
    aumi += ".";
    aumi += productName;

    if (subProduct.isNotEmpty())
    {
        aumi += ".";
        aumi += subProduct;

        if (versionInformation.isNotEmpty())
        {
            aumi += ".";
            aumi += versionInformation;
        }
    }

    return aumi;
}

String ToastNotification::getErrorDescription (Error error)
{
    switch (error)
    {
        case Error::noError:
            return "No error. The process was executed correctly";

        case Error::notInitialized:
            return "The library has not been initialized";

        case Error::systemNotSupported:
            return "The OS does not support toast notifications";

        case Error::shellLinkNotCreated:
            return "The library was not able to create a Shell Link for the app";

        case Error::invalidAppUserModelID:
            return "The AUMI is not a valid one";

        case Error::invalidParameters:
            return "Invalid parameters, please double-check the AUMI or App Name";

        case Error::invalidHandler:
            return "Invalid handler";

        case Error::notDisplayed:
            return "The toast was created correctly but could not be displayed";

        case Error::permissionDenied:
            return "The user denied or revoked notification permission";

        case Error::unknownError:
            return "Unknown error";
    }

    jassertfalse;
    return "Unknown error";
}

//==============================================================================
void ToastNotification::getPermissionState (std::function<void (PermissionState)> callback)
{
    detail::toastNotificationGetPermissionState (std::move (callback));
}

void ToastNotification::requestPermission (std::function<void (PermissionState)> callback)
{
    detail::toastNotificationRequestPermission (std::move (callback));
}

void ToastNotification::setPermissionStateChangedCallback (std::function<void (PermissionState)> callback)
{
    detail::toastNotificationSetPermissionStateChangedCallback (std::move (callback));
}

//==============================================================================
ResultValue<int64> ToastNotification::showToast (const ToastTemplate& toast, std::function<void (const ResultValue<int64>&)> completion)
{
    if (state.load (std::memory_order_relaxed) != State::initialized)
        return makeResultValueFail (getErrorDescription (Error::notInitialized));

    detail::ToastNotificationSettings settings;
    settings.appName = appName;
    settings.appUserModelId = appUserModelId.isEmpty() ? appName : appUserModelId;
    settings.shortcutPolicy = shortcutPolicy;
    settings.fallbackImage = fallbackImage;

    return detail::toastNotificationShow (toast, settings, std::move (completion));
}

bool ToastNotification::hideToast (int64 id)
{
    return detail::toastNotificationHide (id);
}

//==============================================================================
void ToastNotification::sendNotification (StringRef title, StringRef message, std::function<void (const Result&)> resultCallback, std::optional<int> expirationMilliseconds)
{
    if (auto result = initialize(); result.failed())
    {
        if (resultCallback)
            resultCallback (result);

        return;
    }

    ToastTemplate toast (ToastTemplate::TemplateType::text02);
    toast.setFirstLine (title);
    toast.setSecondLine (message);

    if (expirationMilliseconds.has_value())
        toast.setExpiration (*expirationMilliseconds);

    showToast (toast, [resultCallback = std::move (resultCallback)] (const ResultValue<int64>& result)
    {
        if (resultCallback)
            resultCallback (result.wasOk() ? Result::ok() : Result::fail (result.getErrorMessage()));
    });
}

//==============================================================================
#if ! (YUP_WINDOWS || YUP_MAC || YUP_IOS || YUP_ANDROID || YUP_LINUX || YUP_BSD || YUP_WASM)
namespace detail
{
Result toastNotificationInitialize (const ToastNotificationSettings&)
{
    return Result::fail ("Toast notifications are not supported on this platform");
}

void toastNotificationGetPermissionState (std::function<void (ToastNotification::PermissionState)> callback)
{
    if (callback)
        callback (ToastNotification::PermissionState::notDetermined);
}

void toastNotificationRequestPermission (std::function<void (ToastNotification::PermissionState)> callback)
{
    if (callback)
        callback (ToastNotification::PermissionState::notDetermined);
}

void toastNotificationSetPermissionStateChangedCallback (std::function<void (ToastNotification::PermissionState)>)
{
}

ResultValue<int64> toastNotificationShow (const ToastTemplate&, const ToastNotificationSettings&, std::function<void (const ResultValue<int64>&)> completion)
{
    const auto result = makeResultValueFail ("Toast notifications are not supported on this platform");
    if (completion)
        completion (result);

    return result;
}

bool toastNotificationHide (int64)
{
    return false;
}

void toastNotificationClear()
{
}
} // namespace detail
#endif

} // namespace yup
