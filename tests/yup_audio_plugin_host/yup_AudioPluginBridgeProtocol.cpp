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

#include <atomic>
#include <cstring>
#include <tuple>

#include <yup_audio_plugin_host/host/yup_AudioPluginParameterMailbox.h>
#include <yup_audio_plugin_host/host/yup_AudioPluginBridgeProtocol.h>

using namespace yup;

class AudioPluginBridgeProtocolTests : public ::testing::Test
{
protected:
    using Protocol = AudioPluginBridgeProtocol;

    class PresetProcessor : public AudioPluginInstance
    {
    public:
        PresetProcessor()
            : AudioPluginInstance (AudioPluginDescription(),
                                   AudioBusLayout ({}, { AudioBus ("Output", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) }))
        {
        }

        void prepareToPlay (const AudioSpec&) override {}

        void releaseResources() override {}

        void processBlock (AudioProcessContext<float>&) override {}

        int getCurrentPreset() const noexcept override { return 1; }

        void setCurrentPreset (int) noexcept override {}

        int getNumPresets() const override { return 3; }

        String getPresetName (int index) const override { return "Preset " + String (index); }

        void setPresetName (int, StringRef) override {}

        Result loadStateFromMemory (const MemoryBlock&) override { return Result::ok(); }

        Result saveStateIntoMemory (MemoryBlock&) override { return Result::ok(); }

        bool hasEditor() const override { return false; }
    };

    static MemoryInputStream makeInput (const MemoryOutputStream& output)
    {
        return MemoryInputStream (output.getData(), output.getDataSize(), true);
    }
};

TEST_F (AudioPluginBridgeProtocolTests, HeaderRoundTrips)
{
    MemoryOutputStream output;
    Protocol::writeHeader (output, Protocol::MessageType::setState, 42);

    auto input = makeInput (output);
    const auto [type, requestId] = Protocol::readHeader (input);

    EXPECT_EQ (Protocol::MessageType::setState, type);
    EXPECT_EQ (42, requestId);
}

TEST_F (AudioPluginBridgeProtocolTests, HostContextRoundTrips)
{
    AudioPluginHostContext context;
    context.sampleRate = 96000.0f;
    context.maxBlockSize = 128;
    context.preferDoublePrecision = true;
    context.isNonRealtime = true;
    context.hostName = "Host";
    context.hostVendor = "Vendor";
    context.hostVersion = "2.1";

    MemoryOutputStream output;
    Protocol::writeHostContext (output, context);

    auto input = makeInput (output);
    const auto read = Protocol::readHostContext (input);

    EXPECT_FLOAT_EQ (96000.0f, read.sampleRate);
    EXPECT_EQ (128, read.maxBlockSize);
    EXPECT_TRUE (read.preferDoublePrecision);
    EXPECT_TRUE (read.isNonRealtime);
    EXPECT_EQ ("Host", read.hostName);
    EXPECT_EQ ("Vendor", read.hostVendor);
    EXPECT_EQ ("2.1", read.hostVersion);
}

TEST_F (AudioPluginBridgeProtocolTests, BusLayoutRoundTrips)
{
    const AudioBusLayout layout ({ AudioBus ("Input", AudioBus::Type::Audio, AudioBus::Direction::Input, 2),
                                   AudioBus ("Sidechain", AudioBus::Type::Audio, AudioBus::Direction::Input, 1, AudioBus::Role::Auxiliary, false),
                                   AudioBus ("MIDI In", AudioBus::Type::Midi, AudioBus::Direction::Input, 1) },
                                 { AudioBus ("Output", AudioBus::Type::Audio, AudioBus::Direction::Output, 6) });

    MemoryOutputStream output;
    Protocol::writeBusLayout (output, layout);

    auto input = makeInput (output);
    const auto read = Protocol::readBusLayout (input);

    const auto inputs = read.getInputBuses();
    const auto outputs = read.getOutputBuses();
    ASSERT_EQ (3u, inputs.size());
    ASSERT_EQ (1u, outputs.size());

    EXPECT_EQ ("Input", inputs[0].getName());
    EXPECT_EQ (2, inputs[0].getNumChannels());
    EXPECT_TRUE (inputs[0].isDefaultActive());

    EXPECT_EQ ("Sidechain", inputs[1].getName());
    EXPECT_EQ (AudioBus::Role::Auxiliary, inputs[1].getRole());
    EXPECT_FALSE (inputs[1].isDefaultActive());

    EXPECT_EQ (AudioBus::Type::Midi, inputs[2].getType());
    EXPECT_EQ (AudioBus::Direction::Input, inputs[2].getDirection());

    EXPECT_EQ ("Output", outputs[0].getName());
    EXPECT_EQ (AudioBus::Direction::Output, outputs[0].getDirection());
    EXPECT_EQ (6, outputs[0].getNumChannels());
}

TEST_F (AudioPluginBridgeProtocolTests, TruncatedBusLayoutStopsAtEndOfStream)
{
    MemoryOutputStream output;
    output.writeInt (3);

    auto input = makeInput (output);
    const auto read = Protocol::readBusLayout (input);

    EXPECT_EQ (0u, read.getInputBuses().size());
    EXPECT_EQ (0u, read.getOutputBuses().size());
}

TEST_F (AudioPluginBridgeProtocolTests, ParameterRoundTrips)
{
    AudioParameter::Metadata metadata;
    metadata.name = "Cutoff";
    metadata.hostParameterID = 7;
    metadata.valueRange = NormalisableRange<float> (-10.0f, 10.0f, 0.5f, 0.75f, true);
    metadata.defaultValue = 2.0f;
    metadata.smoothingTimeMs = 5.0f;
    metadata.modulePath = "Filter/Main";
    metadata.unit = AudioParameter::ParameterUnit::Custom;
    metadata.unitName = "dBFS";
    metadata.setAutomatable (false);
    metadata.setReadOnly (true);
    metadata.setStepped (true);
    metadata.setEnum (true);
    metadata.setModulatable (true);
    metadata.setPerNoteModulatable (true);
    metadata.setSmoothingEnabled (true);
    metadata.setBypass (true);

    const AudioParameter::Ptr parameter = new AudioParameter ("cutoff", metadata);

    MemoryOutputStream output;
    Protocol::writeParameter (output, *parameter);

    auto input = makeInput (output);
    const auto [id, read, value] = Protocol::readParameter (input);

    EXPECT_EQ ("cutoff", id);
    EXPECT_EQ ("Cutoff", read.name);
    EXPECT_EQ (7u, read.hostParameterID);
    EXPECT_FLOAT_EQ (-10.0f, read.valueRange.start);
    EXPECT_FLOAT_EQ (10.0f, read.valueRange.end);
    EXPECT_FLOAT_EQ (0.5f, read.valueRange.interval);
    EXPECT_FLOAT_EQ (0.75f, read.valueRange.skew);
    EXPECT_TRUE (read.valueRange.symmetricSkew);
    EXPECT_FLOAT_EQ (2.0f, read.defaultValue);
    EXPECT_FLOAT_EQ (5.0f, read.smoothingTimeMs);
    EXPECT_EQ ("Filter/Main", read.modulePath);
    EXPECT_EQ (AudioParameter::ParameterUnit::Custom, read.unit);
    EXPECT_EQ ("dBFS", read.unitName);
    EXPECT_FALSE (read.isAutomatable());
    EXPECT_TRUE (read.isReadOnly());
    EXPECT_TRUE (read.isStepped());
    EXPECT_TRUE (read.isEnum());
    EXPECT_TRUE (read.isModulatable());
    EXPECT_TRUE (read.isPerNoteModulatable());
    EXPECT_TRUE (read.isSmoothingEnabled());
    EXPECT_TRUE (read.isBypass());
    EXPECT_FLOAT_EQ (parameter->getValue(), value);
}

TEST_F (AudioPluginBridgeProtocolTests, PresetsRoundTrip)
{
    PresetProcessor processor;

    MemoryOutputStream output;
    Protocol::writePresets (output, processor);

    auto input = makeInput (output);
    const auto [currentPreset, names] = Protocol::readPresets (input);

    EXPECT_EQ (1, currentPreset);
    ASSERT_EQ (3, names.size());
    EXPECT_EQ ("Preset 0", names[0]);
    EXPECT_EQ ("Preset 2", names[2]);
}

TEST_F (AudioPluginBridgeProtocolTests, MidiRoundTripsAndDropsEventsThatDoNotFit)
{
    MidiBuffer midi;
    midi.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 5);
    midi.addEvent (MidiMessage::controllerEvent (1, 7, 100), 10);

    uint8 packed[64] = {};
    const auto written = Protocol::writeMidi (midi, packed, static_cast<int> (sizeof (packed)));
    EXPECT_EQ (18, written);

    MidiBuffer read;
    Protocol::readMidi (packed, written, read);
    ASSERT_EQ (2, read.getNumEvents());

    const auto first = (*read.begin()).getMessage();
    EXPECT_TRUE (first.isNoteOn());
    EXPECT_EQ (60, first.getNoteNumber());
    EXPECT_EQ (5, (*read.begin()).samplePosition);

    EXPECT_EQ (9, Protocol::writeMidi (midi, packed, 12));
}

TEST_F (AudioPluginBridgeProtocolTests, ReadMidiIgnoresTruncatedEvents)
{
    MidiBuffer midi;
    midi.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);

    uint8 packed[16] = {};
    ASSERT_EQ (9, Protocol::writeMidi (midi, packed, static_cast<int> (sizeof (packed))));

    MidiBuffer read;
    Protocol::readMidi (packed, 8, read);

    EXPECT_TRUE (read.isEmpty());
}

TEST (AudioPluginParameterMailboxTests, IgnoresOutOfRangeIndicesAndCoalescesPendingSlots)
{
    AudioPluginParameterMailbox mailbox;
    mailbox.resize (2);

    mailbox.set (-1, 1.0);
    mailbox.set (2, 1.0);
    mailbox.post (5, AudioPluginParameterMailbox::gestureBegan);

    int numCalls = 0;
    mailbox.flush ([&] (int, double, uint8)
    {
        ++numCalls;
    });
    EXPECT_EQ (0, numCalls);

    mailbox.set (1, 0.25);
    mailbox.set (1, 0.5);
    mailbox.post (1, AudioPluginParameterMailbox::gestureBegan);

    mailbox.flush ([&] (int index, double value, uint8 flags)
    {
        ++numCalls;
        EXPECT_EQ (1, index);
        EXPECT_DOUBLE_EQ (0.5, value);
        EXPECT_EQ (AudioPluginParameterMailbox::valueChanged | AudioPluginParameterMailbox::gestureBegan, flags);
    });
    EXPECT_EQ (1, numCalls);

    mailbox.flush ([&] (int, double, uint8)
    {
        ++numCalls;
    });
    EXPECT_EQ (1, numCalls);
}
