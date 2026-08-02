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

#include <yup_audio_processors/yup_audio_processors.h>
#include <yup_audio_plugin_client/yup_audio_plugin_client.h>

using namespace yup;

namespace
{

//==============================================================================
/** A minimal test processor with a known parameter set and bus layout. */
class UtilitiesTestProcessor final : public AudioProcessor
{
public:
    UtilitiesTestProcessor()
        : AudioProcessor ("UtilitiesTest",
                          AudioBusLayout ({ AudioBus ("Input", AudioBus::Type::Audio, AudioBus::Direction::Input, 2) },
                                          { AudioBus ("Output", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) }))
    {
        // Float parameter, automatable, modulatable — host ID 100
        auto floatMeta = AudioParameter::Metadata {};
        floatMeta.name = "Gain";
        floatMeta.hostParameterID = 100;
        floatMeta.valueRange = { 0.0f, 1.0f };
        floatMeta.defaultValue = 0.5f;
        floatMeta.setModulatable (true);
        addParameter (new AudioParameter ("gain", floatMeta));

        // Stepped integer parameter, enumerated — host ID 200
        auto steppedMeta = AudioParameter::Metadata {};
        steppedMeta.name = "Mode";
        steppedMeta.hostParameterID = 200;
        steppedMeta.valueRange = { 0.0f, 4.0f, 1.0f };
        steppedMeta.defaultValue = 0.0f;
        steppedMeta.setStepped (true);
        steppedMeta.setEnum (true);
        addParameter (new AudioParameter ("mode", steppedMeta));

        // Read-only parameter (meter) — host ID 300
        auto readOnlyMeta = AudioParameter::Metadata {};
        readOnlyMeta.name = "Meter";
        readOnlyMeta.hostParameterID = 300;
        readOnlyMeta.valueRange = { -60.0f, 0.0f };
        readOnlyMeta.defaultValue = -60.0f;
        readOnlyMeta.setReadOnly (true);
        readOnlyMeta.setAutomatable (false);
        addParameter (new AudioParameter ("meter", readOnlyMeta));

        // Non-automatable parameter — host ID 400
        auto nonAutoMeta = AudioParameter::Metadata {};
        nonAutoMeta.name = "Internal";
        nonAutoMeta.hostParameterID = 400;
        nonAutoMeta.valueRange = { 0.0f, 100.0f };
        nonAutoMeta.defaultValue = 50.0f;
        nonAutoMeta.setAutomatable (false);
        addParameter (new AudioParameter ("internal", nonAutoMeta));
    }

    void prepareToPlay (const AudioSpec&) override { prepared = true; }

    void releaseResources() override { prepared = false; }

    void processBlock (AudioProcessContext<float>&) override
    {
        ++floatProcessCallCount;
    }

    void processBlock (AudioProcessContext<double>&) override
    {
        ++doubleProcessCallCount;
    }

    void processBlockBypassed (AudioProcessContext<float>&) override
    {
        ++floatBypassCallCount;
    }

    void processBlockBypassed (AudioProcessContext<double>&) override
    {
        ++doubleBypassCallCount;
    }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    Result loadStateFromMemory (const MemoryBlock& block) override
    {
        lastLoadedState = block;
        return Result::ok();
    }

    Result saveStateIntoMemory (MemoryBlock& block) override
    {
        block = lastSavedState;
        return Result::ok();
    }

    bool hasEditor() const override { return false; }

    bool prepared = false;
    int floatProcessCallCount = 0;
    int doubleProcessCallCount = 0;
    int floatBypassCallCount = 0;
    int doubleBypassCallCount = 0;
    MemoryBlock lastLoadedState;
    MemoryBlock lastSavedState;
};

//==============================================================================
/** A processor with colliding host parameter IDs for collision tests. */
class CollidingIDProcessor final : public AudioProcessor
{
public:
    CollidingIDProcessor()
        : AudioProcessor ("CollidingID",
                          AudioBusLayout ({}, {}))
    {
        // Fill IDs 0, 1, 2
        for (uint32 i = 0; i < 3; ++i)
        {
            auto meta = AudioParameter::Metadata {};
            meta.name = "Param" + String (static_cast<int> (i));
            meta.hostParameterID = i;
            addParameter (new AudioParameter ("p" + String (static_cast<int> (i)), meta));
        }
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

//==============================================================================
/** A processor with a multi-bus layout for channel-count tests. */
class MultiBusProcessor final : public AudioProcessor
{
public:
    MultiBusProcessor()
        : AudioProcessor ("MultiBus",
                          AudioBusLayout ({ AudioBus ("Main In", AudioBus::Type::Audio, AudioBus::Direction::Input, 2),
                                            AudioBus ("Sidechain In", AudioBus::Type::Audio, AudioBus::Direction::Input, 1),
                                            AudioBus ("MIDI In", AudioBus::Type::Midi, AudioBus::Direction::Input, 1) },
                                          { AudioBus ("Main Out", AudioBus::Type::Audio, AudioBus::Direction::Output, 2),
                                            AudioBus ("Aux Out", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) }))
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

} // namespace

//==============================================================================
// writeWrapperBypassState / readWrapperBypassState
//==============================================================================

class WrapperBypassStateTests : public ::testing::Test
{
protected:
    static constexpr int kMagic = 0x12345678;
    static constexpr int kVersion = 3;
};

TEST_F (WrapperBypassStateTests, RoundTripWithPayload)
{
    MemoryBlock payload (128);
    payload.fillWith (0xAB);

    auto written = writeWrapperBypassState (kMagic, kVersion, true, payload, true);
    auto result = readWrapperBypassState (written, kMagic, kVersion);

    EXPECT_TRUE (result.hasWrapperState);
    EXPECT_TRUE (result.isBypassed);
    EXPECT_TRUE (result.hasProcessorState);
    EXPECT_EQ (128u, result.processorState.getSize());
    EXPECT_EQ (0, std::memcmp (payload.getData(), result.processorState.getData(), 128));
}

TEST_F (WrapperBypassStateTests, RoundTripNotBypassed)
{
    MemoryBlock payload (64);
    payload.fillWith (0xCD);

    auto written = writeWrapperBypassState (kMagic, kVersion, false, payload, true);
    auto result = readWrapperBypassState (written, kMagic, kVersion);

    EXPECT_TRUE (result.hasWrapperState);
    EXPECT_FALSE (result.isBypassed);
    EXPECT_TRUE (result.hasProcessorState);
    EXPECT_EQ (64u, result.processorState.getSize());
    EXPECT_EQ (0, std::memcmp (payload.getData(), result.processorState.getData(), 64));
}

TEST_F (WrapperBypassStateTests, RoundTripEmptyPayload)
{
    MemoryBlock emptyPayload;

    auto written = writeWrapperBypassState (kMagic, kVersion, false, emptyPayload, false);
    auto result = readWrapperBypassState (written, kMagic, kVersion);

    EXPECT_TRUE (result.hasWrapperState);
    EXPECT_FALSE (result.isBypassed);
    EXPECT_FALSE (result.hasProcessorState);
    EXPECT_EQ (0u, result.processorState.getSize());
}

TEST_F (WrapperBypassStateTests, FallsBackOnMagicMismatch)
{
    MemoryBlock rawData (32);
    rawData.fillWith (0xEF);

    auto result = readWrapperBypassState (rawData, kMagic, kVersion);

    EXPECT_FALSE (result.hasWrapperState);
    EXPECT_FALSE (result.isBypassed);
    EXPECT_EQ (32u, result.processorState.getSize());
    EXPECT_EQ (0, std::memcmp (rawData.getData(), result.processorState.getData(), 32));
}

TEST_F (WrapperBypassStateTests, FallsBackOnVersionMismatch)
{
    MemoryBlock payload (8);
    payload.fillWith (0x11);

    auto written = writeWrapperBypassState (kMagic, kVersion, true, payload, true);

    // Read with wrong version
    auto result = readWrapperBypassState (written, kMagic, kVersion + 1);

    EXPECT_FALSE (result.hasWrapperState);
    // Fallback returns the whole blob as processor state
    EXPECT_EQ (written.getSize(), result.processorState.getSize());
}

TEST_F (WrapperBypassStateTests, HandlesCorruptedNegativeSize)
{
    // Manually craft a block with a negative size field
    MemoryBlock data;
    MemoryOutputStream output (data, false);
    output.writeInt (kMagic);
    output.writeInt (kVersion);
    output.writeBool (false);
    output.writeBool (true);
    output.writeInt64 (-1); // Negative size
    output.flush();

    auto result = readWrapperBypassState (data, kMagic, kVersion);

    EXPECT_FALSE (result.hasWrapperState);
    EXPECT_FALSE (result.hasProcessorState);
    EXPECT_EQ (data.getSize(), result.processorState.getSize());
}

TEST_F (WrapperBypassStateTests, HandlesCorruptedSizeExceedingRemaining)
{
    MemoryBlock data;
    MemoryOutputStream output (data, false);
    output.writeInt (kMagic);
    output.writeInt (kVersion);
    output.writeBool (false);
    output.writeBool (true);
    output.writeInt64 (999999); // Size exceeds available data
    output.flush();

    auto result = readWrapperBypassState (data, kMagic, kVersion);

    EXPECT_FALSE (result.hasWrapperState);
    EXPECT_FALSE (result.hasProcessorState);
    EXPECT_EQ (data.getSize(), result.processorState.getSize());
}

//==============================================================================
// findFirstUnusedHostParameterID / getBypassHostParameterID
//==============================================================================

class HostParameterIDTests : public ::testing::Test
{
protected:
    CollidingIDProcessor processor;
};

TEST_F (HostParameterIDTests, ReturnsPreferredWhenNoCollision)
{
    EXPECT_EQ (10u, findFirstUnusedHostParameterID (processor, 10));
}

TEST_F (HostParameterIDTests, SkipsCollision)
{
    // ID 0 is occupied, so preferred 0 should return 3 (first free after 0,1,2)
    EXPECT_EQ (3u, findFirstUnusedHostParameterID (processor, 0));
}

TEST_F (HostParameterIDTests, SkipsMultipleCollisions)
{
    // ID 1 is occupied, preferred 1 → skips to 3
    EXPECT_EQ (3u, findFirstUnusedHostParameterID (processor, 1));
}

TEST_F (HostParameterIDTests, BypassIDStartsAtParameterCount)
{
    // 3 parameters, so bypass scan starts at 3
    EXPECT_EQ (3u, getBypassHostParameterID (processor));
}

//==============================================================================
// getTotalAudioOutputChannels / getTotalAudioInputChannels
//==============================================================================

class ChannelCountTests : public ::testing::Test
{
protected:
    MultiBusProcessor processor;
};

TEST_F (ChannelCountTests, CountsAudioInputChannels)
{
    // 2 (Main In) + 1 (Sidechain In) = 3, MIDI In excluded
    EXPECT_EQ (3, getTotalAudioInputChannels (processor));
}

TEST_F (ChannelCountTests, CountsAudioOutputChannels)
{
    // 2 (Main Out) + 2 (Aux Out) = 4
    EXPECT_EQ (4, getTotalAudioOutputChannels (processor));
}

/** A processor with explicitly-role-tagged sidechain input buses. */
class RoleTaggedSidechainProcessor final : public AudioProcessor
{
public:
    RoleTaggedSidechainProcessor()
        : AudioProcessor ("RoleTaggedSidechain",
                          AudioBusLayout ({ AudioBus ("Main", AudioBus::Type::Audio, AudioBus::Direction::Input, 2),
                                            AudioBus ("SC", AudioBus::Type::Audio, AudioBus::Direction::Input, 1, AudioBus::Role::Auxiliary),
                                            AudioBus ("MIDI", AudioBus::Type::Midi, AudioBus::Direction::Input, 1) },
                                          { AudioBus ("Out", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) }))
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

TEST (AudioInputChannelCountTests, CountsAudioChannelsInRoleTaggedSidechainLayout)
{
    RoleTaggedSidechainProcessor processor;

    // 2 (Main) + 1 (Sidechain) = 3 audio input channels, MIDI excluded
    EXPECT_EQ (3, getTotalAudioInputChannels (processor));
    // 2 (Out) = 2 audio output channels
    EXPECT_EQ (2, getTotalAudioOutputChannels (processor));
}

/** A processor with no audio inputs (e.g. a pure tone generator). */
class NoAudioInputProcessor final : public AudioProcessor
{
public:
    NoAudioInputProcessor()
        : AudioProcessor ("NoAudioInput",
                          AudioBusLayout ({},
                                          { AudioBus ("Out", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) }))
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

TEST (AudioInputChannelCountTests, CountsZeroWhenNoAudioInputs)
{
    NoAudioInputProcessor processor;

    EXPECT_EQ (0, getTotalAudioInputChannels (processor));
    EXPECT_EQ (2, getTotalAudioOutputChannels (processor));
}

//==============================================================================
// getDefaultParameterChangeCapacity
//==============================================================================

TEST (DefaultCapacityTests, ReturnsExpectedFormula)
{
    UtilitiesTestProcessor processor;
    // 4 parameters → 4 * 4 + 32 = 48
    EXPECT_EQ (48, getDefaultParameterChangeCapacity (processor));
}

//==============================================================================
// addParameterChangeByHostParameterID
//==============================================================================

class ParameterChangeByHostIDTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        changes.reserve (getDefaultParameterChangeCapacity (processor));
    }

    UtilitiesTestProcessor processor;
    ParameterChangeBuffer changes;
};

TEST_F (ParameterChangeByHostIDTests, AddsChangeForValidHostID)
{
    // host ID 100 maps to "Gain" (index 0)
    EXPECT_TRUE (addParameterChangeByHostParameterID (processor, changes, 100, 0.75f, 0));

    ASSERT_EQ (1, changes.getNumChanges());
    EXPECT_EQ (0, changes.begin()->parameterIndex);
    EXPECT_FLOAT_EQ (0.75f, changes.begin()->normalizedValue);
    EXPECT_EQ (0, changes.begin()->sampleOffset);
}

TEST_F (ParameterChangeByHostIDTests, AddsChangeForSteppedParameter)
{
    // host ID 200 maps to "Mode" (index 1)
    EXPECT_TRUE (addParameterChangeByHostParameterID (processor, changes, 200, 1.0f, 64));

    ASSERT_EQ (1, changes.getNumChanges());
    EXPECT_EQ (1, changes.begin()->parameterIndex);
    EXPECT_FLOAT_EQ (1.0f, changes.begin()->normalizedValue);
    EXPECT_EQ (64, changes.begin()->sampleOffset);
}

TEST_F (ParameterChangeByHostIDTests, ReturnsFalseForInvalidHostID)
{
    EXPECT_FALSE (addParameterChangeByHostParameterID (processor, changes, 999, 0.5f, 0));
    EXPECT_EQ (0, changes.getNumChanges());
}

TEST_F (ParameterChangeByHostIDTests, ReturnsFalseForOutOfRangeIndex)
{
    // host ID 400 maps to "Internal" (index 3), which is valid
    EXPECT_TRUE (addParameterChangeByHostParameterID (processor, changes, 400, 0.5f, 0));

    // host ID beyond any known param
    EXPECT_FALSE (addParameterChangeByHostParameterID (processor, changes, 99999, 0.5f, 0));
}

//==============================================================================
// applyParameterChangesToProcessor
//==============================================================================

class ApplyParameterChangesTests : public ::testing::Test
{
protected:
    UtilitiesTestProcessor processor;
};

TEST_F (ApplyParameterChangesTests, AppliesChangesToParameters)
{
    ParameterChangeBuffer changes;
    changes.reserve (getDefaultParameterChangeCapacity (processor));
    changes.addChange (0, 0.25f, 0); // Gain
    changes.addChange (1, 0.75f, 0); // Mode

    applyParameterChangesToProcessor (processor, changes);

    // Normalized values are applied through setNormalizedValue
    // AudioParameter stores denormalized values
    auto gainParam = processor.getParameterByHostID (100);
    auto modeParam = processor.getParameterByHostID (200);
    ASSERT_NE (nullptr, gainParam);
    ASSERT_NE (nullptr, modeParam);
    EXPECT_FLOAT_EQ (0.25f, gainParam->getValue());
    EXPECT_FLOAT_EQ (3.0f, modeParam->getValue()); // 0.75 * 4 = 3.0
}

TEST_F (ApplyParameterChangesTests, SkipsOutOfRangeIndices)
{
    ParameterChangeBuffer changes;
    changes.reserve (getDefaultParameterChangeCapacity (processor));
    changes.addChange (0, 0.5f, 0);
    changes.addChange (999, 0.5f, 0); // Out of range

    applyParameterChangesToProcessor (processor, changes);

    auto gainParam = processor.getParameterByHostID (100);
    ASSERT_NE (nullptr, gainParam);
    EXPECT_FLOAT_EQ (0.5f, gainParam->getValue());
}

//==============================================================================
// endActiveParameterGestures
//==============================================================================

class EndGestureTests : public ::testing::Test
{
protected:
    UtilitiesTestProcessor processor;
};

TEST_F (EndGestureTests, EndsInProgressGestures)
{
    auto gainParam = processor.getParameterByHostID (100);
    ASSERT_NE (nullptr, gainParam);

    gainParam->beginChangeGesture();
    EXPECT_TRUE (gainParam->isPerformingChangeGesture());

    endActiveParameterGestures (&processor);

    EXPECT_FALSE (gainParam->isPerformingChangeGesture());
}

TEST_F (EndGestureTests, HandlesNestedGestures)
{
    auto gainParam = processor.getParameterByHostID (100);
    ASSERT_NE (nullptr, gainParam);

    gainParam->beginChangeGesture();
    gainParam->beginChangeGesture();
    EXPECT_TRUE (gainParam->isPerformingChangeGesture());

    endActiveParameterGestures (&processor);

    EXPECT_FALSE (gainParam->isPerformingChangeGesture());
}

TEST_F (EndGestureTests, NullProcessorIsSafe)
{
    EXPECT_NO_FATAL_FAILURE (endActiveParameterGestures (nullptr));
}

//==============================================================================
// processAudioBlock
//==============================================================================

class ProcessAudioBlockTests : public ::testing::Test
{
protected:
    UtilitiesTestProcessor processor;
};

TEST_F (ProcessAudioBlockTests, RoutesToProcessBlockWhenNotBypassed)
{
    AudioBuffer<float> audio (2, 64);
    MidiBuffer midi;
    ParameterChangeBuffer params;
    AudioProcessContext<float> ctx { audio, midi, params };

    processAudioBlock (processor, ctx, false);

    EXPECT_EQ (1, processor.floatProcessCallCount);
    EXPECT_EQ (0, processor.floatBypassCallCount);
}

TEST_F (ProcessAudioBlockTests, RoutesToBypassedWhenBypassed)
{
    AudioBuffer<float> audio (2, 64);
    MidiBuffer midi;
    ParameterChangeBuffer params;
    AudioProcessContext<float> ctx { audio, midi, params };

    processAudioBlock (processor, ctx, true);

    EXPECT_EQ (0, processor.floatProcessCallCount);
    EXPECT_EQ (1, processor.floatBypassCallCount);
}
