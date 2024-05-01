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

namespace juce
{

//==============================================================================

JUCEApplication::JUCEApplication()
{
    staticInitialisation();
}

JUCEApplication::~JUCEApplication()
{
    staticFinalisation();
}

bool JUCEApplication::moreThanOneInstanceAllowed()
{
    return true;
}

void JUCEApplication::anotherInstanceStarted (const String& commandLine)
{
    ignoreUnused (commandLine);
}

void JUCEApplication::systemRequestedQuit()
{
    quit();
}

void JUCEApplication::suspended()
{
}

void JUCEApplication::resumed()
{
}

void JUCEApplication::unhandledException (const std::exception* ex,
                                          const String& sourceFilename,
                                          int lineNumber)
{
    ignoreUnused (ex, sourceFilename, lineNumber);
}

} // namespace juce
