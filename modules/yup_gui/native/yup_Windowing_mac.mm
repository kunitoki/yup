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

Rectangle<int> getNativeWindowPosition (void* nativeWindow)
{
    NSView* view = reinterpret_cast<NSView*> (nativeWindow);
    NSWindow* window = [view window];

    // Convert view bounds to window coordinates
    NSRect viewRectInWindow = [view convertRect:[view bounds] toView:nil];

    // Convert window coordinates to global macOS screen coordinates
    NSRect windowRect = [window convertRectToScreen:viewRectInWindow];

    // Get main screen (primary) and current screen (window's screen)
    NSScreen* mainScreen = [NSScreen screens].firstObject;
    NSScreen* screen = [window screen] ?: mainScreen;

    // Calculate vertical offset between current screen and main screen
    CGFloat adjustedY = NSMaxY ([screen frame]) - NSMaxY ([mainScreen frame]);

    // Correctly flip Y coordinate relative to main screen's top-left origin
    windowRect.origin.y = NSMaxY ([screen frame]) - NSMaxY (windowRect) - adjustedY;

    return {
        static_cast<int> (windowRect.origin.x),
        static_cast<int> (windowRect.origin.y),
        static_cast<int> (NSWidth (windowRect)),
        static_cast<int> (NSHeight (windowRect))
    };
}

} // namespace yup
