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

#include <yup_gui/yup_gui.h>

#include "AudioGraphApp.h"

//==============================================================================

class AudioGraphWindow final : public yup::DocumentWindow
{
public:
    AudioGraphWindow()
        : yup::DocumentWindow (yup::ComponentNative::Options(), yup::Color (0xff0d1117))
    {
        setTitle ("YUP Audio Graph");

        app = std::make_unique<AudioGraphApp>();
        addAndMakeVisible (app.get());
    }

    void resized() override
    {
        app->setBounds (getLocalBounds());
    }

    bool keyDown (const yup::KeyPress& keys, const yup::Point<float>&) override
    {
        if (keys.getKey() != yup::KeyPress::escapeKey)
            return false;

        userTriedToCloseWindow();
        return true;
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

    void initialise (const yup::String& commandLine) override
    {
#if YUP_DESKTOP
        // Relaunched by the plugin scanner to open plugins out of process
        auto worker = std::make_unique<yup::AudioPluginScannerWorker>();
        addPluginFormats (*worker);
        if (worker->initialiseFromCommandLine (commandLine))
        {
            scannerWorker = std::move (worker);
            return;
        }

        // Relaunched by a sandboxed plugin node to run the plugin out of process
        auto server = std::make_unique<yup::AudioPluginBridgeServer>();
        addPluginFormats (*server);
        if (server->initialiseFromCommandLine (commandLine))
        {
            bridgeServer = std::move (server);
            return;
        }
#endif

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
#if YUP_DESKTOP
    template <class Target>
    static void addPluginFormats (Target& target)
    {
#if YUP_AUDIO_PLUGIN_HOST_ENABLE_VST3
        target.addFormat (std::make_unique<yup::VST3Format>());
#endif
#if YUP_AUDIO_PLUGIN_HOST_ENABLE_CLAP
        target.addFormat (std::make_unique<yup::CLAPFormat>());
#endif
#if YUP_AUDIO_PLUGIN_HOST_ENABLE_AU && YUP_MAC
        target.addFormat (std::make_unique<yup::AUFormat>());
#endif
    }
#endif

    std::unique_ptr<AudioGraphWindow> window;
#if YUP_DESKTOP
    std::unique_ptr<yup::AudioPluginScannerWorker> scannerWorker;
    std::unique_ptr<yup::AudioPluginBridgeServer> bridgeServer;
#endif
};

START_YUP_APPLICATION (Application)
