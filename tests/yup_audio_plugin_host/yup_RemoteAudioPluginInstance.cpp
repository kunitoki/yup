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


#include <gtest/gtest.h>

#include <yup_audio_plugin_host/yup_audio_plugin_host.h>

#if YUP_MAC || YUP_WINDOWS || YUP_LINUX

using namespace yup;

namespace yup
{
std::function<bool (const String&)>& getGoogleTestChildProcessEntryPoint();
} // namespace yup

class RemoteAudioPluginInstanceTests : public ::testing::Test
{
public:
    static constexpr const char* serverID = "yupBridgeServerTests";

    /** A stereo gain with MIDI echo, 7 samples of latency and a preset setting the gain to 0.5.
        The "crashy" variant kills its process when it receives MIDI, the "faultyeditor" one
        reports an editor it fails to create.
    */
    class GainInstance : public AudioPluginInstance
    {
    public:
        explicit GainInstance (const AudioPluginDescription& description)
            : AudioPluginInstance (description,
                                   AudioBusLayout ({ AudioBus ("Input", AudioBus::Type::Audio, AudioBus::Direction::Input, 2) },
                                                   { AudioBus ("Output", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) }))
            , crashOnMidi (description.name == "crashy")
            , faultyEditor (description.name == "faultyeditor")
        {
            gain = AudioParameterBuilder()
                       .withID ("gain")
                       .withName ("Gain")
                       .withRange (0.0f, 2.0f)
                       .withDefault (1.0f)
                       .withValueToString ([] (float value)
            {
                return String (value, 2) + "x";
            })
                       .build();

            addParameter (gain);
            setLatencySamples (7);

            if (description.name == "bypassable")
            {
                for (const auto* id : { "bypass", "secondBypass" })
                {
                    addParameter (AudioParameterBuilder()
                                      .withID (id)
                                      .withName (id)
                                      .withRange (0.0f, 1.0f)
                                      .withDefault (0.0f)
                                      .withBypass (true)
                                      .build());
                }
            }
        }

        void prepareToPlay (const AudioSpec&) override {}

        void releaseResources() override {}

        void processBlock (AudioProcessContext<float>& context) override
        {
            if (crashOnMidi && ! context.midi.isEmpty())
                Process::terminate();

            for (const auto& change : context.params)
            {
                if (change.parameterIndex == 0)
                    gain->setNormalizedValue (change.normalizedValue);
            }

            context.audio.applyGain (gain->getValue());
        }

        bool acceptsMidi() const noexcept override { return true; }

        bool producesMidi() const noexcept override { return true; }

        int getCurrentPreset() const noexcept override { return currentPreset; }

        void setCurrentPreset (int index) noexcept override
        {
            currentPreset = index;
            gain->setValueNotifyingHost (index == 1 ? 0.5f : 1.0f);
        }

        int getNumPresets() const override { return 2; }

        String getPresetName (int index) const override { return index == 0 ? "Unity" : "Half"; }

        void setPresetName (int, StringRef) override {}

        Result loadStateFromMemory (const MemoryBlock& memoryBlock) override
        {
            if (memoryBlock.getSize() != sizeof (float))
                return Result::fail ("Invalid state");

            float value = 0.0f;
            memoryBlock.copyTo (&value, 0, sizeof (float));
            gain->setValueNotifyingHost (value);
            return Result::ok();
        }

        Result saveStateIntoMemory (MemoryBlock& memoryBlock) override
        {
            const auto value = gain->getValue();
            memoryBlock.replaceAll (&value, sizeof (float));
            return Result::ok();
        }

        bool hasEditor() const override { return faultyEditor; }

        AudioProcessorEditor* createEditor() override { return nullptr; }

    private:
        AudioParameter::Ptr gain;
        const bool crashOnMidi;
        const bool faultyEditor;
        int currentPreset = 0;
    };

    class GainFormat : public AudioPluginFormat
    {
    public:
        AudioPluginFormatType getFormatType() const override { return AudioPluginFormatType::clap; }

        String getFormatName() const override { return "Gain"; }

        StringArray getFileExtensions() const override { return { ".gainplugin" }; }

        FileSearchPath getDefaultSearchPaths() const override { return {}; }

        ResultValue<std::vector<AudioPluginDescription>> scanFile (const File&) override
        {
            return makeResultValueFail ("not scannable");
        }

        ResultValue<std::unique_ptr<AudioPluginInstance>> loadPlugin (const AudioPluginDescription& description,
                                                                      const AudioPluginHostContext&) override
        {
            if (description.name == "broken")
                return makeResultValueFail ("cannot open " + description.name);

            return makeResultValueOk (std::unique_ptr<AudioPluginInstance> (std::make_unique<GainInstance> (description)));
        }
    };

    static bool runServer (const String& commandLine)
    {
        static std::unique_ptr<AudioPluginBridgeServer> server;

        auto newServer = std::make_unique<AudioPluginBridgeServer>();
        newServer->addFormat (std::make_unique<GainFormat>());

        if (! newServer->initialiseFromCommandLine (commandLine, serverID))
            return false;

        server = std::move (newServer);
        return true;
    }

protected:
    static constexpr int blockSize = 64;

    static AudioPluginDescription makeDescription (const String& name)
    {
        AudioPluginDescription description;
        description.formatType = AudioPluginFormatType::clap;
        description.name = name;
        description.identifier = "test." + name;
        return description;
    }

    static RemoteAudioPluginInstance::Options makeOptions (const File& executable = File::getSpecialLocation (File::currentExecutableFile))
    {
        RemoteAudioPluginInstance::Options options;
        options.executable = executable;
        options.commandLineUniqueID = serverID;
        options.timeoutMs = 5000;
        return options;
    }

    static std::unique_ptr<RemoteAudioPluginInstance> load (const String& name)
    {
        auto result = RemoteAudioPluginInstance::load (makeDescription (name), {}, makeOptions());
        EXPECT_TRUE (result.wasOk()) << (result.failed() ? result.getErrorMessage() : String());
        return result.wasOk() ? std::move (result).getValue() : nullptr;
    }

    static void prepare (RemoteAudioPluginInstance& instance)
    {
        instance.setNonRealtime (true);
        instance.setPlaybackConfiguration (44100.0f, blockSize);
        instance.prepareToPlay (AudioSpec (44100.0f, blockSize, 2));
    }

    /** Processes a block of 0.25 on both channels, returning the first output sample. */
    float process (RemoteAudioPluginInstance& instance, bool withNote = false, std::optional<float> normalizedGain = {})
    {
        audio.setSize (2, blockSize, false, false, true);
        for (int channel = 0; channel < 2; ++channel)
            FloatVectorOperations::fill (audio.getWritePointer (channel), 0.25f, blockSize);

        midi.clear();
        if (withNote)
            midi.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 3);

        params.clear();
        params.reserve (16);
        if (normalizedGain.has_value())
            params.addChange (0, *normalizedGain, 0);

        AudioProcessContext<float> context { audio, midi, params, nullptr };
        instance.processBlock (context);
        return audio.getSample (1, blockSize - 1);
    }

    static bool waitUntil (const std::function<bool()>& condition)
    {
        for (int i = 0; i < 200 && ! condition(); ++i)
            MessageManager::getInstance()->runDispatchLoopUntil (20);

        return condition();
    }

    AudioBuffer<float> audio;
    MidiBuffer midi;
    ParameterChangeBuffer params;
};

namespace
{
const bool bridgeServerEntryPointRegistered = []
{
    auto& entryPoint = yup::getGoogleTestChildProcessEntryPoint();

    entryPoint = [previous = entryPoint] (const String& commandLine)
    {
        return RemoteAudioPluginInstanceTests::runServer (commandLine) || (previous && previous (commandLine));
    };

    return true;
}();
} // namespace

TEST_F (RemoteAudioPluginInstanceTests, LoadsPluginInfoAndParameters)
{
    auto instance = load ("gain");
    ASSERT_NE (nullptr, instance);

    ASSERT_EQ (1u, instance->getParameters().size());
    const auto& gain = instance->getParameters()[0];
    EXPECT_EQ ("gain", gain->getID());
    EXPECT_EQ ("Gain", gain->getName());
    EXPECT_FLOAT_EQ (1.0f, gain->getValue());
    EXPECT_FLOAT_EQ (2.0f, gain->getMaximumValue());
    EXPECT_EQ ("0.50x", gain->convertToString (0.5f));

    EXPECT_EQ (7, instance->getLatencySamples());
    EXPECT_TRUE (instance->acceptsMidi());
    EXPECT_TRUE (instance->producesMidi());
    EXPECT_FALSE (instance->hasEditor());
    EXPECT_EQ (2, instance->getBusLayout().getOutputBuses()[0].getNumChannels());
    ASSERT_EQ (2, instance->getNumPresets());
    EXPECT_EQ ("Half", instance->getPresetName (1));
    EXPECT_FALSE (instance->hasCrashed());
}

TEST_F (RemoteAudioPluginInstanceTests, ProcessesAudioMidiAndParameterChanges)
{
    auto instance = load ("gain");
    ASSERT_NE (nullptr, instance);
    prepare (*instance);

    EXPECT_FLOAT_EQ (0.25f, process (*instance, true));

    ASSERT_EQ (1, midi.getNumEvents());
    const auto event = (*midi.begin()).getMessage();
    EXPECT_TRUE (event.isNoteOn());
    EXPECT_EQ (60, event.getNoteNumber());
    EXPECT_EQ (3, (*midi.begin()).samplePosition);

    EXPECT_FLOAT_EQ (0.125f, process (*instance, false, 0.25f));
    EXPECT_FLOAT_EQ (0.5f, instance->getParameters()[0]->getValue());

    instance->getParameters()[0]->setValue (2.0f);
    EXPECT_FLOAT_EQ (0.5f, process (*instance));
}

TEST_F (RemoteAudioPluginInstanceTests, StateAndPresetsReachThePlugin)
{
    auto instance = load ("gain");
    ASSERT_NE (nullptr, instance);

    const float half = 0.5f;
    ASSERT_TRUE (instance->loadStateFromMemory (MemoryBlock (&half, sizeof (float))).wasOk());
    EXPECT_TRUE (waitUntil ([&] { return instance->getParameters()[0]->getValue() == 0.5f; }));

    MemoryBlock state;
    ASSERT_TRUE (instance->saveStateIntoMemory (state).wasOk());
    EXPECT_EQ (MemoryBlock (&half, sizeof (float)), state);

    EXPECT_TRUE (instance->loadStateFromMemory (MemoryBlock (3)).failed());

    instance->setCurrentPreset (0);
    EXPECT_TRUE (waitUntil ([&] { return instance->getParameters()[0]->getValue() == 1.0f; }));
    EXPECT_EQ (0, instance->getCurrentPreset());
}

TEST_F (RemoteAudioPluginInstanceTests, ShowEditorWithoutEditorDoesNothing)
{
    auto instance = load ("gain");
    ASSERT_NE (nullptr, instance);

    instance->showEditor();
    EXPECT_FALSE (instance->isEditorShowing());
}

TEST_F (RemoteAudioPluginInstanceTests, EditorFailingToOpenReportsClosed)
{
    auto instance = load ("faultyeditor");
    ASSERT_NE (nullptr, instance);
    ASSERT_TRUE (instance->hasEditor());

    bool closed = false;
    instance->onEditorClosed = [&]
    {
        closed = true;
    };

    instance->showEditor();
    EXPECT_TRUE (instance->isEditorShowing());

    EXPECT_TRUE (waitUntil ([&] { return closed; }));
    EXPECT_FALSE (instance->isEditorShowing());

    instance->hideEditor();
    EXPECT_FALSE (instance->isEditorShowing());
}

TEST_F (RemoteAudioPluginInstanceTests, CrashOutputsSilenceAndReloadRestoresState)
{
    auto instance = load ("crashy");
    ASSERT_NE (nullptr, instance);

    bool crashReported = false;
    instance->onCrashed = [&]
    {
        crashReported = true;
    };

    const float half = 0.5f;
    ASSERT_TRUE (instance->loadStateFromMemory (MemoryBlock (&half, sizeof (float))).wasOk());
    ASSERT_TRUE (waitUntil ([&] { return instance->getParameters()[0]->getValue() == 0.5f; }));

    prepare (*instance);
    EXPECT_FLOAT_EQ (0.125f, process (*instance));

    EXPECT_FLOAT_EQ (0.0f, process (*instance, true));
    EXPECT_EQ (0, midi.getNumEvents());
    EXPECT_TRUE (instance->hasCrashed());
    EXPECT_TRUE (waitUntil ([&] { return crashReported; }));

    EXPECT_FLOAT_EQ (0.0f, process (*instance));

    MemoryBlock state;
    ASSERT_TRUE (instance->saveStateIntoMemory (state).wasOk());
    EXPECT_EQ (MemoryBlock (&half, sizeof (float)), state);

    ASSERT_TRUE (instance->reload().wasOk());
    EXPECT_FALSE (instance->hasCrashed());
    EXPECT_FLOAT_EQ (0.125f, process (*instance));
}

TEST_F (RemoteAudioPluginInstanceTests, ReportsFixedCapabilitiesAndConvertsTextThroughTheServer)
{
    auto instance = load ("gain");
    ASSERT_NE (nullptr, instance);

    EXPECT_FALSE (instance->supportsNoteExpression());
    EXPECT_FALSE (instance->supportsMpe());
    EXPECT_EQ (0, instance->getTailSamples());
    EXPECT_EQ (nullptr, instance->createEditor());

    instance->setPresetName (1, "Renamed");
    EXPECT_EQ ("Half", instance->getPresetName (1));

    EXPECT_FLOAT_EQ (1.5f, instance->getParameters()[0]->convertFromString ("1.5"));
}

TEST_F (RemoteAudioPluginInstanceTests, BypassWithoutBypassParameterPassesAudioThrough)
{
    auto instance = load ("gain");
    ASSERT_NE (nullptr, instance);
    prepare (*instance);

    instance->getParameters()[0]->setValue (2.0f);
    EXPECT_FLOAT_EQ (0.5f, process (*instance));

    instance->setBypassed (true);
    EXPECT_FLOAT_EQ (0.25f, process (*instance));

    instance->setBypassed (false);
    EXPECT_FLOAT_EQ (0.5f, process (*instance));
}

TEST_F (RemoteAudioPluginInstanceTests, OnlyTheFirstBypassParameterDrivesBypass)
{
    auto instance = load ("bypassable");
    ASSERT_NE (nullptr, instance);

    const auto parameters = instance->getParameters();
    ASSERT_EQ (3u, parameters.size());
    EXPECT_TRUE (parameters[1]->isBypass());
    EXPECT_FALSE (parameters[2]->isBypass());

    instance->setBypassed (true);
    EXPECT_FLOAT_EQ (1.0f, parameters[1]->getNormalizedValue());

    instance->setBypassed (false);
    EXPECT_FLOAT_EQ (0.0f, parameters[1]->getNormalizedValue());
}

TEST_F (RemoteAudioPluginInstanceTests, CrashedInstanceFallsBackToLocalConversionsAndState)
{
    auto instance = load ("crashy");
    ASSERT_NE (nullptr, instance);
    prepare (*instance);

    EXPECT_FLOAT_EQ (0.0f, process (*instance, true));
    ASSERT_TRUE (instance->hasCrashed());

    const auto& gain = instance->getParameters()[0];
    EXPECT_EQ ("0.50", gain->convertToString (0.5f));
    EXPECT_FLOAT_EQ (0.75f, gain->convertFromString ("0.75"));
    EXPECT_EQ (nullptr, instance->createEditor());

    MemoryBlock state;
    EXPECT_TRUE (instance->saveStateIntoMemory (state).failed());
}

TEST_F (RemoteAudioPluginInstanceTests, PluginFailingToLoadReportsError)
{
    auto result = RemoteAudioPluginInstance::load (makeDescription ("broken"), {}, makeOptions());

    ASSERT_TRUE (result.failed());
    EXPECT_EQ ("cannot open broken", result.getErrorMessage());
}

TEST_F (RemoteAudioPluginInstanceTests, MissingExecutableFails)
{
    auto result = RemoteAudioPluginInstance::load (makeDescription ("gain"), {}, makeOptions (File::getSpecialLocation (File::tempDirectory).getChildFile ("missing_bridge_server")));

    EXPECT_TRUE (result.failed());
}

TEST_F (RemoteAudioPluginInstanceTests, NonServerCommandLineIsIgnored)
{
    AudioPluginBridgeServer server;

    EXPECT_FALSE (server.initialiseFromCommandLine ("--gtest_filter=*", serverID));
}

#endif
