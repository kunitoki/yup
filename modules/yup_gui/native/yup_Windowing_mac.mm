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

//==============================================================================

void focusNativeWindow (void* nativeWindow)
{
    if (nativeWindow == nullptr)
        return;

    id nativeObject = (__bridge id) nativeWindow;
    NSWindow* window = [nativeObject isKindOfClass:[NSWindow class]]
                         ? (NSWindow*) nativeObject
                         : [nativeObject window];

    [NSApp activateIgnoringOtherApps:YES];
    [window makeKeyAndOrderFront:nil];
}

void activateAppWindowOnLaunch (void* nativeWindow)
{
    static std::atomic_flag hasActivatedOnLaunch;
    if (hasActivatedOnLaunch.test_and_set())
        return;

    if (nativeWindow == nullptr)
        return;

    id nativeObject = (__bridge id) nativeWindow;
    NSWindow* window = [nativeObject isKindOfClass:[NSWindow class]]
                         ? (NSWindow*) nativeObject
                         : [nativeObject window];

    YUP_MODULE_DBG (GUI_WINDOWING, "MAC: activateAppWindowOnLaunch: isActive=" << String ([NSApp isActive] ? "true" : "false")
                                     << ", isKeyWindow=" << String ([window isKeyWindow] ? "true" : "false"));

    const auto bringToFront = ^{
        if (@available (macOS 14.0, *))
            [NSApp activate];
        else
            [NSApp activateIgnoringOtherApps:YES];

        [window makeKeyAndOrderFront:nil];
    };

    bringToFront();

    dispatch_after (dispatch_time (DISPATCH_TIME_NOW, (int64_t) (0.2 * NSEC_PER_SEC)), dispatch_get_main_queue(),
    ^{
        if ([NSApp isActive])
            return;

        YUP_MODULE_DBG (GUI_WINDOWING, "MAC: launch activation was dropped, re-injecting activation event");

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

        bringToFront();
    });
}

} // namespace yup
