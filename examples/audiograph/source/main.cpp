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

#include <yup_core/yup_core.h>
#include <yup_audio_basics/yup_audio_basics.h>
#include <yup_audio_devices/yup_audio_devices.h>
#include <yup_audio_formats/yup_audio_formats.h>
#include <yup_audio_processors/yup_audio_processors.h>
#include <yup_audio_graph/yup_audio_graph.h>
#include <yup_events/yup_events.h>
#include <yup_graphics/yup_graphics.h>
#include <yup_gui/yup_gui.h>
#include <yup_audio_gui/yup_audio_gui.h>

#if YUP_DESKTOP
#include <yup_audio_plugin_host/yup_audio_plugin_host.h>
#endif

#include "AudioGraphApp.h"

//==============================================================================

class AudioGraphWindow final : public yup::DocumentWindow
{
public:
    AudioGraphWindow()
        : yup::DocumentWindow (yup::ComponentNative::Options().withAllowedHighDensityDisplay (true),
                               yup::Color (0xff0d1117))
    {
        setTitle ("YUP Audio Graph");

        app = std::make_unique<AudioGraphApp>();
        addAndMakeVisible (app.get());
    }

    void resized() override
    {
        app->setBounds (getLocalBounds());
    }

    void keyDown (const yup::KeyPress& keys, const yup::Point<float>&) override
    {
        if (keys.getKey() == yup::KeyPress::escapeKey)
            userTriedToCloseWindow();
    }

    void userTriedToCloseWindow() override
    {
        yup::YUPApplication::getInstance()->systemRequestedQuit();
    }

private:
    std::unique_ptr<AudioGraphApp> app;
};

//==============================================================================

struct Application final : yup::YUPApplication
{
    Application() = default;

    yup::String getApplicationName() override
    {
        return "YUP Audio Graph";
    }

    yup::String getApplicationVersion() override
    {
        return "1.0";
    }

    void initialise (const yup::String&) override
    {
        yup::MessageManager::callAsync ([this]
        {
            yup::Process::makeForegroundProcess();

            window = std::make_unique<AudioGraphWindow>();
            window->centreWithSize ({ 1200, 800 });
            window->setVisible (true);
        });
    }

    void shutdown() override
    {
        window.reset();
    }

private:
    std::unique_ptr<AudioGraphWindow> window;
};

START_YUP_APPLICATION (Application)
