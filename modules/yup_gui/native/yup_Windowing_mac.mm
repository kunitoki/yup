/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

namespace
{

NSWindow* getNSWindowFromNativeHandle (void* nativeWindow) noexcept
{
    if (nativeWindow == nullptr)
        return nil;

    id nativeObject = (__bridge id) nativeWindow;
    return [nativeObject isKindOfClass:[NSWindow class]]
             ? (NSWindow*) nativeObject
             : [nativeObject window];
}

void activateAndOrderWindowFront (NSWindow* window)
{
    if (window == nil)
        return;

    YUP_BEGIN_IGNORE_DEPRECATION_WARNINGS
    [NSApp activateIgnoringOtherApps:YES];
    YUP_END_IGNORE_DEPRECATION_WARNINGS

    [window makeKeyAndOrderFront:nil];
}

bool isSystemFrontmostApplication()
{
    NSRunningApplication* frontmost = [[NSWorkspace sharedWorkspace] frontmostApplication];
    return frontmost != nil && [frontmost processIdentifier] == [[NSProcessInfo processInfo] processIdentifier];
}

void replayApplicationActivatedEvent()
{
    NSEvent* activationEvent = [NSEvent otherEventWithType:NSEventTypeAppKitDefined
                                                  location:NSZeroPoint
                                             modifierFlags:0
                                                 timestamp:0.0
                                              windowNumber:0
                                                   context:nil
                                                   subtype:NSEventSubtypeApplicationActivated
                                                     data1:0
                                                     data2:0];

    [NSApp postEvent:activationEvent atStart:YES];
}

} // namespace

//==============================================================================

void focusNativeWindow (void* nativeWindow)
{
    activateAndOrderWindowFront (getNSWindowFromNativeHandle (nativeWindow));
}

void activateAppWindowOnLaunch (void* nativeWindow)
{
    NSWindow* window = getNSWindowFromNativeHandle (nativeWindow);
    if (window == nil || [NSApp activationPolicy] != NSApplicationActivationPolicyRegular)
        return;

    static std::atomic_flag hasActivatedOnLaunch;
    if (hasActivatedOnLaunch.test_and_set())
        return;

    YUP_MODULE_DBG (GUI_WINDOWING, "MAC: activateAppWindowOnLaunch: isActive=" << String ([NSApp isActive] ? "true" : "false")
                                     << ", isKeyWindow=" << String ([window isKeyWindow] ? "true" : "false"));

    activateAndOrderWindowFront (window);

    __weak NSWindow* weakWindow = window;
    const int64 delayNanoseconds = static_cast<int64> (200) * static_cast<int64> (NSEC_PER_MSEC);

    dispatch_after (dispatch_time (DISPATCH_TIME_NOW, delayNanoseconds), dispatch_get_main_queue(),
    ^{
        NSWindow* strongWindow = weakWindow;
        if (strongWindow == nil || ! [strongWindow isVisible] || [strongWindow isMiniaturized])
            return;

        if ([NSApp isActive] && [NSApp keyWindow] != nil)
            return;

        if (! [NSApp isActive] && isSystemFrontmostApplication())
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "MAC: launch activation was missed by AppKit, replaying it");

            replayApplicationActivatedEvent();
            [strongWindow makeKeyAndOrderFront:nil];
            return;
        }

        YUP_MODULE_DBG (GUI_WINDOWING, "MAC: launch activation was dropped, retrying window activation");

        activateAndOrderWindowFront (strongWindow);
    });
}

} // namespace yup
