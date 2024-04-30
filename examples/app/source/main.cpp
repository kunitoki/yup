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

#include <yup_gui/yup_gui.h>

class Application : public juce::JUCEApplication, public juce::Timer
{
public:
    Application() = default;

    const juce::String getApplicationName() override
    {
        return "yup app!";
    }

    const juce::String getApplicationVersion() override
    {
        return "1.0";
    }

    void initialise (const juce::String& commandLineParameters) override
    {
        juce::Logger::outputDebugString ("Starting app " + commandLineParameters);

        startTimer (1000);
    }

    void shutdown() override
    {
        juce::Logger::outputDebugString ("Shutting down");
    }

    void timerCallback() override
    {
        stopTimer();

        juce::MessageManager::callAsync([this] { systemRequestedQuit(); });
    }
};

START_JUCE_APPLICATION(Application)
