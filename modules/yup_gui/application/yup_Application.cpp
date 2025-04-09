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

YUPApplication::YUPApplication()
{
    initialiseYup_Windowing();

#if JUCE_MAC
    NSMenu* menuBar = [[NSMenu alloc] init];
    NSMenuItem* menuBarItem = [[NSMenuItem alloc] init];
    [menuBar addItem:menuBarItem];

    NSMenu* appMenu = [[NSMenu alloc] init];
    NSMenuItem* quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit"
                                                          action:@selector(terminate:)
                                                   keyEquivalent:@"q"];
    [appMenu addItem:quitMenuItem];
    [menuBarItem setSubmenu:appMenu];

    [NSApp setMainMenu:menuBar];
#endif
}

YUPApplication::~YUPApplication()
{
    shutdownYup_Windowing();
}

bool YUPApplication::moreThanOneInstanceAllowed()
{
    return true;
}

void YUPApplication::anotherInstanceStarted (const String& commandLine)
{
    ignoreUnused (commandLine);
}

void YUPApplication::systemRequestedQuit()
{
    quit();
}

void YUPApplication::suspended()
{
}

void YUPApplication::resumed()
{
}

void YUPApplication::unhandledException (const std::exception* ex,
                                         const String& sourceFilename,
                                         int lineNumber)
{
    ignoreUnused (ex, sourceFilename, lineNumber);
}

} // namespace yup
