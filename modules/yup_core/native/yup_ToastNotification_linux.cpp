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
    The Linux backend shells out to the system notification tools: notify-send
    (from libnotify) when available, falling back to zenity. Command
    availability is probed once during initialize(). Action buttons are not
    delivered (notify-send actions require DBus callback plumbing that is not
    available here), so onActivatedWithAction is never invoked on Linux.
*/

#if YUP_LINUX || YUP_BSD

namespace yup
{
namespace detail
{

namespace
{

//==============================================================================
// The per-process state of the toast backend.
struct LinuxToastState
{
    enum class Command
    {
        none = 0,
        notifySend = 1,
        zenity = 2
    };

    Command command { Command::none };
    bool initialized { false };
};

LinuxToastState& getToastState()
{
    static LinuxToastState state;
    return state;
}

//==============================================================================
bool isCommandAvailable (const String& command)
{
    ChildProcess process;
    process.start (StringArray { command, "--version" }, ChildProcess::wantStdOut | ChildProcess::wantStdErr);

    if (! process.isRunning())
        return false;

    process.waitForProcessToFinish (10000);
    return ! process.isRunning() && process.getExitCode() == 0;
}

String urgencyForScenario (ToastTemplate::Scenario scenario)
{
    switch (scenario)
    {
        case ToastTemplate::Scenario::alarm:
        case ToastTemplate::Scenario::incomingCall:
        case ToastTemplate::Scenario::reminder:
            return "critical";

        case ToastTemplate::Scenario::default_:
            break;
    }

    return "normal";
}

void addArguments (StringArray& target, std::initializer_list<String> arguments)
{
    for (const auto& argument : arguments)
        target.add (argument);
}

} // namespace

//==============================================================================
Result toastNotificationInitialize (const ToastNotificationSettings&)
{
    auto& state = getToastState();

    if (state.initialized)
        return Result::ok();

    if (isCommandAvailable ("notify-send"))
        state.command = LinuxToastState::Command::notifySend;
    else if (isCommandAvailable ("zenity"))
        state.command = LinuxToastState::Command::zenity;
    else
        return Result::fail (ToastNotification::getErrorDescription (ToastNotification::Error::systemNotSupported));

    state.initialized = true;
    return Result::ok();
}

//==============================================================================
ResultValue<int64> toastNotificationShow (const ToastTemplate& toast, const ToastNotificationSettings& settings)
{
    auto& state = getToastState();

    if (! state.initialized)
        return makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::notInitialized));

    const String title = toast.getTextField (ToastTemplate::TextField::firstLine);
    const String message = toast.getTextField (ToastTemplate::TextField::secondLine);

    StringArray arguments;

    if (state.command == LinuxToastState::Command::notifySend)
    {
        addArguments (arguments, { "notify-send", "-a", settings.appName });

        if (toast.getExpiration() > 0)
            addArguments (arguments, { "-t", String (toast.getExpiration()) });

        File image = toast.getImagePath();
        if (! image.existsAsFile() && settings.fallbackImage.has_value())
            image = *settings.fallbackImage;

        if (image.existsAsFile())
            addArguments (arguments, { "-i", image.getFullPathName() });

        if (toast.getScenario() != ToastTemplate::Scenario::default_)
            addArguments (arguments, { "-u", urgencyForScenario (toast.getScenario()) });

        addArguments (arguments, { title, message });
    }
    else if (state.command == LinuxToastState::Command::zenity)
    {
        addArguments (arguments, { "zenity", "--notification", "--text", message.isEmpty() ? title : message });
    }
    else
    {
        return makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::systemNotSupported));
    }

    ChildProcess process;
    process.start (arguments, ChildProcess::wantStdOut | ChildProcess::wantStdErr);

    if (! process.isRunning())
        return makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::notDisplayed));

    process.waitForProcessToFinish (10000);

    if (process.isRunning() || process.getExitCode() != 0)
        return makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::notDisplayed));

    return makeResultValueOk (0);
}

//==============================================================================
bool toastNotificationHide (int64)
{
    // The Linux notification tools don't support hiding a single notification.
    return false;
}

//==============================================================================
void toastNotificationClear()
{
}

} // namespace detail
} // namespace yup

#endif // YUP_LINUX || YUP_BSD
