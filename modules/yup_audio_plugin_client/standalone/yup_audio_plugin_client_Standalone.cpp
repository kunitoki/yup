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

#include <yup_audio_devices/yup_audio_devices.h>

#if ! defined(YUP_AUDIO_PLUGIN_ENABLE_STANDALONE)
#error "YUP_AUDIO_PLUGIN_ENABLE_STANDALONE must be defined"
#endif

extern "C" yup::AudioProcessor* createPluginProcessor();

namespace yup
{

//==============================================================================

class AudioProcessorEditorWindow
    : public DocumentWindow
{
public:
    AudioProcessorEditorWindow (StringRef windowTitle, AudioProcessorEditor* editor)
        : yup::DocumentWindow (ComponentNative::Options(), {})
        , editor (editor)
    {
        setTitle (windowTitle);

        addAndMakeVisible (*editor);

        takeKeyboardFocus();
    }

    void resized() override
    {
        editor->setBounds (getLocalBounds());
    }

    void userTriedToCloseWindow() override
    {
        YUPApplication::getInstance()->systemRequestedQuit();
    }

private:
    std::unique_ptr<AudioProcessorEditor> editor;
};

//==============================================================================

class AudioProcessorApplication
    : public YUPApplication
    , public AudioIODeviceCallback
{
public:
    AudioProcessorApplication()
        : processor (::createPluginProcessor())
    {
    }

    String getApplicationName() override
    {
        return processor->getName();
    }

    String getApplicationVersion() override
    {
        return "1.0"; // processor->getVersion();
    }

    void initialise (const String& commandLineParameters) override
    {
        YUP_PROFILE_START();

        yup::Logger::outputDebugString ("Starting app " + commandLineParameters);

        deviceManager.initialiseWithDefaultDevices (0, 2);
        deviceManager.addAudioCallback (this);

        auto* editor = processor->createEditor();
        window = std::make_unique<AudioProcessorEditorWindow> (getApplicationName(), editor);
        window->centreWithSize (editor->getPreferredSize());
        window->setVisible (true);
        window->toFront (true);
    }

    void shutdown() override
    {
        yup::Logger::outputDebugString ("Shutting down");

        window.reset();

        deviceManager.removeAudioCallback (this);
        deviceManager.closeAudioDevice();

        YUP_PROFILE_STOP();
    }

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const AudioIODeviceCallbackContext& context) override
    {
        if (numInputChannels > 0)
        {
            for (int inputIndex = 0; inputIndex < numInputChannels; ++inputIndex)
                audioBuffer.copyFrom (inputIndex, 0, inputChannelData[inputIndex], numSamples);
        }
        else
        {
            audioBuffer.clear();
        }

        MidiBuffer midiBuffer;
        processor->processBlock (audioBuffer, midiBuffer);

        AudioBuffer<float> outputBuffer { outputChannelData, numOutputChannels, numSamples };
        for (int outputIndex = 0; outputIndex < numOutputChannels; ++outputIndex)
            outputBuffer.copyFrom (outputIndex, 0, audioBuffer, outputIndex, 0, numSamples);
    }

    void audioDeviceAboutToStart (AudioIODevice* device) override
    {
        processor->prepareToPlay (device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());

        audioBuffer.setSize (
            jmax (
                processor->getNumAudioInputs(),
                processor->getNumAudioOutputs(),
                device->getActiveInputChannels().countNumberOfSetBits(),
                device->getActiveOutputChannels().countNumberOfSetBits()),
            device->getCurrentBufferSizeSamples());
    }

    void audioDeviceStopped() override
    {
        processor->releaseResources();
    }

private:
    AudioDeviceManager deviceManager;
    AudioBuffer<float> audioBuffer;
    std::unique_ptr<AudioProcessor> processor;
    std::unique_ptr<AudioProcessorEditorWindow> window;
};

} // namespace yup

START_YUP_APPLICATION (yup::AudioProcessorApplication)
