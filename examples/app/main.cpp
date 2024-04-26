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

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

struct Application : juce::JUCEApplicationBase, juce::Timer
{
    Application()
    {}

    const juce::String getApplicationName() override
    {
        return "yup!";
    }

    const juce::String getApplicationVersion() override
    {
        return "1.0";
    }

    bool moreThanOneInstanceAllowed() override
    {
        return false;
    }

    void timerCallback() override
    {
        stopTimer();

        juce::MessageManager::callAsync([this] { systemRequestedQuit(); });
    }

    void initialise (const juce::String& commandLineParameters) override
    {
        DBG("Starting app " << commandLineParameters);
        startTimer (1000);
    }

    void shutdown() override
    {
        DBG("Shutting down");
    }

    void anotherInstanceStarted (const juce::String& commandLine) override
    {
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void suspended() override
    {
    }

    void resumed() override
    {
    }

    void unhandledException (const std::exception*,
                             const juce::String& sourceFilename,
                             int lineNumber) override
    {
    }
};

START_JUCE_APPLICATION(Application)
