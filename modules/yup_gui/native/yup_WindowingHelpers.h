/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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
/**
    Get native window position from a native window handle.

    @param nativeWindow The native window handle.

    @return The position of the native window.

    @see setNativeParent
 */
Rectangle<int> getNativeWindowPosition (void* nativeWindow);

/**
    Gives focus to a native window handle when supported by the current platform.

    @param nativeWindow The native window handle.
 */
void focusNativeWindow (void* nativeWindow);

#if YUP_MAC
/**
    Brings the first top-level standalone window to the front and makes it key, once, the first
    time it is called.

    The application itself is activated earlier through the AppKit launch sequence (-finishLaunching,
    run from the mac AppDelegate); this only promotes the main window, as a normal app does when it
    shows its main window on launch.

    @param nativeWindow The native window handle.
 */
void activateAppWindowOnLaunch (void* nativeWindow);
#endif

} // namespace yup
