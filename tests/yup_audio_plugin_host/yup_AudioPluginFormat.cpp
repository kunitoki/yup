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

using namespace yup;

class AudioPluginFormatTests : public ::testing::Test
{
protected:
    class OpenedInstance : public AudioPluginInstance
    {
    public:
        explicit OpenedInstance (const AudioPluginDescription& description)
            : AudioPluginInstance (description,
                                   AudioBusLayout ({ AudioBus ("Input", AudioBus::Type::Audio, AudioBus::Direction::Input, 1),
                                                     AudioBus ("Sidechain", AudioBus::Type::Audio, AudioBus::Direction::Input, 2, AudioBus::Role::Auxiliary),
                                                     AudioBus ("MIDI Input", AudioBus::Type::Midi, AudioBus::Direction::Input, 1) },
                                                   { AudioBus ("Output", AudioBus::Type::Audio, AudioBus::Direction::Output, 6) }))
        {
        }

        void prepareToPlay (const AudioSpec&) override {}

        void releaseResources() override {}

        void processBlock (AudioProcessContext<float>&) override {}

        int getCurrentPreset() const noexcept override { return 0; }

        void setCurrentPreset (int) noexcept override {}

        int getNumPresets() const override { return 0; }

        String getPresetName (int) const override { return {}; }

        void setPresetName (int, StringRef) override {}

        Result loadStateFromMemory (const MemoryBlock&) override { return Result::ok(); }

        Result saveStateIntoMemory (MemoryBlock&) override { return Result::ok(); }

        bool hasEditor() const override { return false; }
    };

    class EstimatingFormat : public AudioPluginFormat
    {
    public:
        AudioPluginFormatType getFormatType() const override { return AudioPluginFormatType::clap; }

        String getFormatName() const override { return "Estimating"; }

        StringArray getFileExtensions() const override { return { ".clap" }; }

        FileSearchPath getDefaultSearchPaths() const override { return {}; }

        ResultValue<std::vector<AudioPluginDescription>> scanFile (const File&) override
        {
            if (failScan)
                return makeResultValueFail ("unreadable file");

            std::vector<AudioPluginDescription> descriptions;

            for (const auto& name : names)
            {
                AudioPluginDescription description;
                description.formatType = AudioPluginFormatType::clap;
                description.name = name;
                description.identifier = "test." + name;
                description.numInputChannels = 2;
                description.numOutputChannels = 2;
                descriptions.push_back (description);
            }

            return makeResultValueOk (std::move (descriptions));
        }

        ResultValue<std::unique_ptr<AudioPluginInstance>> loadPlugin (const AudioPluginDescription& description,
                                                                      const AudioPluginHostContext&) override
        {
            ++numLoads;

            if (description.name.startsWith ("broken"))
                return makeResultValueFail ("cannot open " + description.name);

            return makeResultValueOk (std::unique_ptr<AudioPluginInstance> (std::make_unique<OpenedInstance> (description)));
        }

        StringArray names;
        int numLoads = 0;
        bool failScan = false;
    };

    EstimatingFormat format;
};

TEST_F (AudioPluginFormatTests, ScanFileDoesNotOpenPlugins)
{
    format.names = { "synth" };

    auto result = format.scanFile (File {});

    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (0, format.numLoads);
    EXPECT_EQ (2, result.getValue()[0].numInputChannels);
}

TEST_F (AudioPluginFormatTests, ScanFileFullyReplacesEstimatesWithOpenedInstanceCounts)
{
    format.names = { "synth" };

    auto result = format.scanFileFully (File {});

    ASSERT_TRUE (result.wasOk());
    ASSERT_EQ (1u, result.getValue().size());

    const auto description = result.getValue()[0];
    EXPECT_EQ (1, format.numLoads);
    EXPECT_EQ ("synth", description.name);
    EXPECT_EQ (3, description.numInputChannels);
    EXPECT_EQ (6, description.numOutputChannels);
    EXPECT_EQ (1, description.numMidiInputPorts);
    EXPECT_EQ (0, description.numMidiOutputPorts);
}

TEST_F (AudioPluginFormatTests, ScanFileFullyDropsPluginsThatFailToOpen)
{
    format.names = { "broken1", "fx" };

    auto result = format.scanFileFully (File {});

    ASSERT_TRUE (result.wasOk());
    ASSERT_EQ (1u, result.getValue().size());
    EXPECT_EQ ("fx", result.getValue()[0].name);
    EXPECT_EQ (2, format.numLoads);
}

TEST_F (AudioPluginFormatTests, ScanFileFullyFailsWhenNothingOpens)
{
    format.names = { "broken1" };

    auto result = format.scanFileFully (File {});

    ASSERT_TRUE (result.failed());
    EXPECT_EQ ("cannot open broken1", result.getErrorMessage());
}

TEST_F (AudioPluginFormatTests, ScanFileFullyFailsWhenScanFindsNothing)
{
    auto result = format.scanFileFully (File {});

    ASSERT_TRUE (result.failed());
    EXPECT_EQ (0, format.numLoads);
}

TEST_F (AudioPluginFormatTests, ScanFileFullyForwardsScanFailures)
{
    format.names = { "synth" };
    format.failScan = true;

    auto result = format.scanFileFully (File {});

    ASSERT_TRUE (result.failed());
    EXPECT_EQ ("unreadable file", result.getErrorMessage());
    EXPECT_EQ (0, format.numLoads);
}
