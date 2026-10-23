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

#include "../yup_audio_plugin_client.h"

#include "../common/yup_AudioPluginUtilities.h"

#include <yup_audio_devices/yup_audio_devices.h>
#include <yup_audio_gui/yup_audio_gui.h>

#if YUP_EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include "yup_StandaloneProcessorPlayer.h"
#include "yup_StandaloneStateStore.h"
#include "yup_StandaloneContent.h"

#if ! defined(YUP_AUDIO_PLUGIN_ENABLE_STANDALONE)
#error "YUP_AUDIO_PLUGIN_ENABLE_STANDALONE must be defined"
#endif

extern "C" yup::AudioProcessor* YUP_AUDIO_PLUGIN_CREATE_FUNCTION();

namespace yup
{

//==============================================================================

class StandaloneWindow
    : public DocumentWindow
{
public:
    StandaloneWindow (StringRef windowTitle, std::unique_ptr<StandaloneContent> windowContent)
        : yup::DocumentWindow (ComponentNative::Options(), {})
        , content (std::move (windowContent))
    {
        setTitle (windowTitle);

        addAndMakeVisible (*content);

        takeKeyboardFocus();
    }

    void resized() override
    {
        content->setBounds (getLocalBounds());
    }

    void userTriedToCloseWindow() override
    {
        YUPApplication::getInstance()->systemRequestedQuit();
    }

private:
    std::unique_ptr<StandaloneContent> content;
};

//==============================================================================

class AudioProcessorApplication
    : public YUPApplication
    , public StandalonePluginHost
{
public:
    AudioProcessorApplication()
        : processor (::YUP_AUDIO_PLUGIN_CREATE_FUNCTION())
        , player (*processor, &deviceManager)
        , stateStore (StandaloneStateStore::createDefault (YupPlugin_Vendor, YupPlugin_Name, YupPlugin_Id))
    {
    }

    String getApplicationName() override
    {
        return processor->getName();
    }

    String getApplicationVersion() override
    {
        return YupPlugin_Version;
    }

    void initialise (const String& commandLineParameters) override
    {
        YUP_PROFILE_START();

        yup::Logger::outputDebugString ("Starting app " + commandLineParameters);

        const auto state = stateStore.load();

        if (const auto result = StandaloneStateStore::restoreProcessorState (*processor, state.processorState); result.failed())
            yup::Logger::outputDebugString ("Unable to restore the plugin state: " + result.getErrorMessage());

        const auto deviceError = deviceManager.initialise (getTotalAudioInputChannels (*processor),
                                                           getTotalAudioOutputChannels (*processor),
                                                           state.deviceState.get(),
                                                           true);
        if (deviceError.isNotEmpty())
            yup::Logger::outputDebugString ("Unable to open the audio device: " + deviceError);

        deviceManager.addAudioCallback (&player);
        deviceManager.addMidiInputDeviceCallback ({}, &player.getMidiMessageCollector());

        auto windowContent = std::make_unique<StandaloneContent> (*processor,
                                                                  deviceManager,
                                                                  player.getMidiMessageCollector(),
                                                                  processor->acceptsMidi() || YupPlugin_IsSynth);
        content = windowContent.get();
        content->setToolbarVisible (toolbarVisible);
        content->onSettingsClosed = [this]
        {
            saveState();
        };
        content->onPreferredSizeChanged = [this]
        {
            if (window == nullptr)
                return;

            window->setWindowAspectRatio (content->getPreferredAspectRatio());
            window->setSize (content->getPreferredSize().to<float>());
        };

        window = std::make_unique<StandaloneWindow> (getApplicationName(), std::move (windowContent));
        window->setWindowAspectRatio (content->getPreferredAspectRatio());
        window->centreWithSize (content->getPreferredSize());
        window->setVisible (true);
        window->toFront (true);

        if (settingsRequested)
            content->showSettings();

#if YUP_EMSCRIPTEN
        // Closing a browser tab never reaches shutdown(), so the state is also saved whenever the page is hidden.
        emscripten_set_visibilitychange_callback (this, false, visibilityChangeCallback);
#endif
    }

    void shutdown() override
    {
        yup::Logger::outputDebugString ("Shutting down");

#if YUP_EMSCRIPTEN
        emscripten_set_visibilitychange_callback (nullptr, false, nullptr);
#endif

        saveState();

        content = nullptr;
        window.reset();

        deviceManager.removeMidiInputDeviceCallback ({}, &player.getMidiMessageCollector());
        deviceManager.removeAudioCallback (&player);
        deviceManager.closeAudioDevice();

        YUP_PROFILE_STOP();
    }

    //==============================================================================
    void showAudioSettings() override
    {
        settingsRequested = content == nullptr;

        if (content != nullptr)
            content->showSettings();
    }

    void setToolbarVisible (bool shouldBeVisible) override
    {
        toolbarVisible = shouldBeVisible;

        if (content != nullptr)
            content->setToolbarVisible (shouldBeVisible);
    }

private:
    void saveState()
    {
        const auto deviceState = deviceManager.createStateXml();

        if (const auto result = stateStore.save (deviceState.get(), *processor); result.failed())
            yup::Logger::outputDebugString ("Unable to save the standalone state: " + result.getErrorMessage());
    }

#if YUP_EMSCRIPTEN
    static EM_BOOL visibilityChangeCallback (int, const EmscriptenVisibilityChangeEvent* event, void* userData)
    {
        if (event != nullptr && event->hidden)
            static_cast<AudioProcessorApplication*> (userData)->saveState();

        return EM_FALSE;
    }
#endif

    std::unique_ptr<AudioProcessor> processor;
    AudioDeviceManager deviceManager;
    StandaloneProcessorPlayer player;
    StandaloneStateStore stateStore;

    std::unique_ptr<StandaloneWindow> window;
    StandaloneContent* content = nullptr;

    bool toolbarVisible = true;
    bool settingsRequested = false;
};

} // namespace yup

START_YUP_APPLICATION (yup::AudioProcessorApplication)
