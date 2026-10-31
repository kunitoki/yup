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

#include <yup_audio_plugin_client/yup_audio_plugin_client.h>
#include <yup_audio_devices/yup_audio_devices.h>
#include <yup_audio_gui/yup_audio_gui.h>

#include "yup_TestPluginProcessor.h"

#include <yup_audio_plugin_client/standalone/yup_StandaloneProcessorPlayer.h>
#include <yup_audio_plugin_client/standalone/yup_StandaloneStateStore.h>
#include <yup_audio_plugin_client/standalone/yup_StandaloneContent.h>

using namespace yup;

//==============================================================================
class StandaloneProcessorPlayerTests : public ::testing::Test
{
protected:
    /** Device-side channel storage with the pointer array a device callback receives. */
    struct DeviceChannels
    {
        DeviceChannels (int numChannels, int numSamples, float value)
            : data (static_cast<size_t> (numChannels), std::vector<float> (static_cast<size_t> (numSamples), value))
        {
            for (auto& channel : data)
                pointers.push_back (channel.data());
        }

        float* const* get() noexcept { return pointers.data(); }

        int size() const noexcept { return static_cast<int> (pointers.size()); }

        std::vector<std::vector<float>> data;
        std::vector<float*> pointers;
    };

    static AudioBusLayout makeLayout (int numIns, int numOuts)
    {
        std::vector<AudioBus> inputs;
        if (numIns > 0)
            inputs.emplace_back ("Input", AudioBus::Type::Audio, AudioBus::Direction::Input, numIns);

        return AudioBusLayout (std::move (inputs),
                               { AudioBus ("Output", AudioBus::Type::Audio, AudioBus::Direction::Output, numOuts) });
    }

    static void process (StandaloneProcessorPlayer& player, DeviceChannels* inputs, DeviceChannels& outputs, int numSamples, const MidiBuffer& midi = {})
    {
        player.process (inputs != nullptr ? inputs->get() : nullptr,
                        inputs != nullptr ? inputs->size() : 0,
                        outputs.get(),
                        outputs.size(),
                        numSamples,
                        midi);
    }
};

TEST_F (StandaloneProcessorPlayerTests, PrepareUsesBusLayoutForSpec)
{
    TestPluginProcessor processor (testPluginBusLayoutWithSidechain());
    StandaloneProcessorPlayer player (processor);

    player.prepare (48000.0, 256);

    ASSERT_TRUE (processor.lastSpec.has_value());
    EXPECT_FLOAT_EQ (48000.0f, processor.lastSpec->sampleRate);
    EXPECT_EQ (256, processor.lastSpec->maxBlockSize);
    EXPECT_EQ (3, processor.lastSpec->numChannels);
    EXPECT_FLOAT_EQ (48000.0f, processor.getSampleRate());
    EXPECT_EQ (256, processor.getSamplesPerBlock());
    EXPECT_TRUE (processor.prepared);

    player.release();
    EXPECT_FALSE (processor.prepared);
}

TEST_F (StandaloneProcessorPlayerTests, ExtraOutputsAreCleared)
{
    TestPluginProcessor processor (testPluginBusLayoutStereo());
    processor.outputFillValue = 0.5f;

    StandaloneProcessorPlayer player (processor);
    player.prepare (44100.0, 64);

    DeviceChannels outputs (4, 64, 1.0f);
    process (player, nullptr, outputs, 64);

    EXPECT_FLOAT_EQ (0.5f, outputs.data[0][10]);
    EXPECT_FLOAT_EQ (0.5f, outputs.data[1][10]);
    EXPECT_FLOAT_EQ (0.0f, outputs.data[2][10]);
    EXPECT_FLOAT_EQ (0.0f, outputs.data[3][10]);
}

TEST_F (StandaloneProcessorPlayerTests, MissingInputsAreZeroed)
{
    TestPluginProcessor processor (testPluginBusLayoutStereo());
    StandaloneProcessorPlayer player (processor);
    player.prepare (44100.0, 64);

    DeviceChannels inputs (1, 64, 0.25f);
    DeviceChannels outputs (2, 64, 1.0f);
    process (player, &inputs, outputs, 64);

    ASSERT_EQ (2u, processor.lastFirstSamplesIn.size());
    EXPECT_FLOAT_EQ (0.25f, processor.lastFirstSamplesIn[0]);
    EXPECT_FLOAT_EQ (0.0f, processor.lastFirstSamplesIn[1]);

    ASSERT_EQ (1u, processor.lastInputBusChannels.size());
    ASSERT_EQ (2u, processor.lastInputBusChannels[0].size());
    EXPECT_NE (nullptr, processor.lastInputBusChannels[0][0]);
    EXPECT_NE (nullptr, processor.lastInputBusChannels[0][1]);
}

TEST_F (StandaloneProcessorPlayerTests, InputDoesNotLeakOntoOutputsTheProcessorLacks)
{
    TestPluginProcessor processor (makeLayout (2, 1));
    StandaloneProcessorPlayer player (processor);
    player.prepare (44100.0, 64);

    DeviceChannels inputs (2, 64, 0.7f);
    DeviceChannels outputs (2, 64, 1.0f);
    process (player, &inputs, outputs, 64);

    EXPECT_FLOAT_EQ (0.7f, outputs.data[0][5]);
    EXPECT_FLOAT_EQ (0.0f, outputs.data[1][5]);
}

TEST_F (StandaloneProcessorPlayerTests, OversizedBlocksAreSplit)
{
    TestPluginProcessor processor (testPluginBusLayoutStereo());
    StandaloneProcessorPlayer player (processor);
    player.prepare (44100.0, 64);

    DeviceChannels outputs (2, 150, 0.0f);
    process (player, nullptr, outputs, 150);

    EXPECT_EQ ((std::vector<int> { 64, 64, 22 }), processor.processedBlockSizes);
}

TEST_F (StandaloneProcessorPlayerTests, MidiIsRebasedOntoEachSubBlock)
{
    TestPluginProcessor processor (testPluginBusLayoutStereo());
    StandaloneProcessorPlayer player (processor);
    player.prepare (44100.0, 64);

    MidiBuffer midi;
    midi.addEvent (MidiMessage::noteOn (1, 60, 0.5f), 10);
    midi.addEvent (MidiMessage::noteOn (1, 62, 0.5f), 70);
    midi.addEvent (MidiMessage::noteOn (1, 64, 0.5f), 130);

    DeviceChannels outputs (2, 150, 0.0f);
    process (player, nullptr, outputs, 150, midi);

    const std::vector<std::vector<int>> expected { { 10 }, { 6 }, { 2 } };
    EXPECT_EQ (expected, processor.midiSamplePositionsPerBlock);
}

TEST_F (StandaloneProcessorPlayerTests, CollectedMidiReachesTheProcessor)
{
    TestPluginProcessor processor (testPluginBusLayoutStereo());
    StandaloneProcessorPlayer player (processor);
    player.prepare (44100.0, 128);

    auto message = MidiMessage::noteOn (1, 60, 0.5f);
    message.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
    player.getMidiMessageCollector().addMessageToQueue (message);

    DeviceChannels outputs (2, 128, 0.0f);
    player.audioDeviceIOCallbackWithContext (nullptr, 0, outputs.get(), outputs.size(), 128, {});

    ASSERT_EQ (1u, processor.midiSamplePositionsPerBlock.size());
    ASSERT_EQ (1u, processor.midiSamplePositionsPerBlock[0].size());
    EXPECT_GE (processor.midiSamplePositionsPerBlock[0][0], 0);
    EXPECT_LT (processor.midiSamplePositionsPerBlock[0][0], 128);
}

TEST_F (StandaloneProcessorPlayerTests, SuspendedProcessorOutputsSilence)
{
    TestPluginProcessor processor (testPluginBusLayoutStereo());
    processor.outputFillValue = 0.5f;

    StandaloneProcessorPlayer player (processor);
    player.prepare (44100.0, 64);

    {
        AudioProcessorBase::ScopedProcessSuspension suspension (processor);

        DeviceChannels outputs (2, 64, 1.0f);
        process (player, nullptr, outputs, 64);

        EXPECT_EQ (0, processor.processCallCount);
        EXPECT_FLOAT_EQ (0.0f, outputs.data[0][0]);
        EXPECT_FLOAT_EQ (0.0f, outputs.data[1][63]);
    }

    DeviceChannels outputs (2, 64, 1.0f);
    process (player, nullptr, outputs, 64);

    EXPECT_EQ (1, processor.processCallCount);
    EXPECT_FLOAT_EQ (0.5f, outputs.data[0][0]);
}

TEST_F (StandaloneProcessorPlayerTests, UnpreparedPlayerOutputsSilence)
{
    TestPluginProcessor processor (testPluginBusLayoutStereo());
    StandaloneProcessorPlayer player (processor);

    DeviceChannels outputs (2, 64, 1.0f);
    process (player, nullptr, outputs, 64);

    EXPECT_EQ (0, processor.processCallCount);
    EXPECT_FLOAT_EQ (0.0f, outputs.data[0][0]);
}

//==============================================================================
class StandaloneStateStoreTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        folder = File::getSpecialLocation (File::tempDirectory).getNonexistentChildFile ("yup_standalone_state", {}, false);
        stateFile = folder.getChildFile ("nested").getChildFile ("standalone.xml");
    }

    void TearDown() override
    {
        folder.deleteRecursively();
    }

    static MemoryBlock makeProcessorState()
    {
        const char bytes[] = { 1, 2, 3, 0, 42 };
        return MemoryBlock (bytes, sizeof (bytes));
    }

    File folder;
    File stateFile;
};

TEST_F (StandaloneStateStoreTests, RoundTripsDeviceAndProcessorState)
{
    TestPluginProcessor processor (testPluginBusLayoutStereo());
    processor.lastSavedState = makeProcessorState();

    XmlElement deviceState ("DEVICESETUP");
    deviceState.setAttribute ("deviceType", "Test");

    StandaloneStateStore store (stateFile);
    ASSERT_TRUE (store.save (&deviceState, processor).wasOk());
    EXPECT_TRUE (stateFile.existsAsFile());

    const auto state = StandaloneStateStore (stateFile).load();

    ASSERT_NE (nullptr, state.deviceState);
    EXPECT_TRUE (state.deviceState->hasTagName ("DEVICESETUP"));
    EXPECT_EQ (String ("Test"), state.deviceState->getStringAttribute ("deviceType"));
    EXPECT_EQ (makeProcessorState(), state.processorState);

    ASSERT_TRUE (StandaloneStateStore::restoreProcessorState (processor, state.processorState).wasOk());
    EXPECT_EQ (makeProcessorState(), processor.lastLoadedState);
    EXPECT_FALSE (processor.isSuspended());
}

TEST_F (StandaloneStateStoreTests, NullDeviceStateIsHandled)
{
    TestPluginProcessor processor (testPluginBusLayoutStereo());
    processor.lastSavedState = makeProcessorState();

    StandaloneStateStore store (stateFile);
    ASSERT_TRUE (store.save (nullptr, processor).wasOk());

    const auto state = store.load();

    EXPECT_EQ (nullptr, state.deviceState);
    EXPECT_EQ (makeProcessorState(), state.processorState);
}

TEST_F (StandaloneStateStoreTests, EmptyProcessorStateIsNotRestored)
{
    TestPluginProcessor processor (testPluginBusLayoutStereo());

    StandaloneStateStore store (stateFile);
    ASSERT_TRUE (store.save (nullptr, processor).wasOk());

    const auto state = store.load();
    EXPECT_TRUE (state.processorState.isEmpty());

    processor.lastLoadedState = makeProcessorState();
    EXPECT_TRUE (StandaloneStateStore::restoreProcessorState (processor, state.processorState).wasOk());
    EXPECT_EQ (makeProcessorState(), processor.lastLoadedState);
}

#if ! YUP_EMSCRIPTEN
TEST_F (StandaloneStateStoreTests, DefaultStoreLivesInTheUserPreferencesFolder)
{
    const auto expected = File::getSpecialLocation (File::userPreferencesDirectory)
                              .getChildFile ("Vendor")
                              .getChildFile ("Product")
                              .getChildFile ("standalone.xml");

    EXPECT_EQ (expected, StandaloneStateStore::createDefault ("Vendor", "Product", "vendor.product").getFile());
}
#endif

TEST_F (StandaloneStateStoreTests, MissingFileLoadsEmptyState)
{
    const auto state = StandaloneStateStore (stateFile).load();

    EXPECT_EQ (nullptr, state.deviceState);
    EXPECT_TRUE (state.processorState.isEmpty());
}

TEST_F (StandaloneStateStoreTests, CorruptFileLoadsEmptyState)
{
    ASSERT_TRUE (stateFile.getParentDirectory().createDirectory().wasOk());
    ASSERT_TRUE (stateFile.replaceWithText ("not xml at all"));

    const auto state = StandaloneStateStore (stateFile).load();

    EXPECT_EQ (nullptr, state.deviceState);
    EXPECT_TRUE (state.processorState.isEmpty());
}

//==============================================================================
class StandaloneContentTests : public ::testing::Test
{
protected:
    class FixedAspectEditor final : public AudioProcessorEditor
    {
    public:
        explicit FixedAspectEditor (bool shouldKeepAspect)
            : keepsAspect (shouldKeepAspect)
        {
        }

        bool isResizable() const override { return true; }

        Size<int> getPreferredSize() const override { return { 400, 200 }; }

        bool shouldPreserveAspectRatio() const override { return keepsAspect; }

    private:
        bool keepsAspect;
    };

    AudioDeviceManager deviceManager;
    MidiMessageCollector collector;
    TestPluginProcessor processor { testPluginBusLayoutStereo() };
};

TEST_F (StandaloneContentTests, ShowsPlaceholderWithoutEditor)
{
    StandaloneContent content (processor, deviceManager, collector, false);

    EXPECT_EQ (nullptr, content.getEditor());
    EXPECT_EQ (400, content.getPreferredSize().getWidth());
    EXPECT_EQ (300 + 28, content.getPreferredSize().getHeight());
}

TEST_F (StandaloneContentTests, ShowsAndHidesSettingsOverlay)
{
    StandaloneContent content (processor, deviceManager, collector, false);
    content.setBounds (0, 0, 600, 800);

    int closedCount = 0;
    content.onSettingsClosed = [&]
    {
        ++closedCount;
    };

    const auto childrenBefore = content.getNumChildComponents();

    content.showSettings();
    EXPECT_TRUE (content.isShowingSettings());
    EXPECT_EQ (childrenBefore + 1, content.getNumChildComponents());

    content.showSettings();
    EXPECT_EQ (childrenBefore + 1, content.getNumChildComponents());

    content.hideSettings();
    EXPECT_FALSE (content.isShowingSettings());
    EXPECT_EQ (childrenBefore, content.getNumChildComponents());
    EXPECT_EQ (1, closedCount);

    content.hideSettings();
    EXPECT_EQ (1, closedCount);
}

TEST_F (StandaloneContentTests, HidingToolbarShrinksPreferredSize)
{
    StandaloneContent content (processor, deviceManager, collector, false);

    int changes = 0;
    content.onPreferredSizeChanged = [&]
    {
        ++changes;
    };

    content.setToolbarVisible (false);

    EXPECT_FALSE (content.isToolbarVisible());
    EXPECT_EQ (300, content.getPreferredSize().getHeight());
    EXPECT_EQ (1, changes);

    content.setToolbarVisible (false);
    EXPECT_EQ (1, changes);
}

TEST_F (StandaloneContentTests, HasNoAspectRatioWithoutAnEditor)
{
    StandaloneContent content (processor, deviceManager, collector, false);

    EXPECT_FALSE (content.getPreferredAspectRatio().has_value());
}

TEST_F (StandaloneContentTests, AspectRatioFollowsAnEditorThatKeepsIt)
{
    processor.editorFactory = [] { return new FixedAspectEditor (true); };
    StandaloneContent keeping (processor, deviceManager, collector, false);

    ASSERT_TRUE (keeping.getPreferredAspectRatio().has_value());
    EXPECT_FLOAT_EQ (400.0f / (200.0f + 28.0f), *keeping.getPreferredAspectRatio());

    keeping.setToolbarVisible (false);
    EXPECT_FLOAT_EQ (400.0f / 200.0f, *keeping.getPreferredAspectRatio());

    processor.editorFactory = [] { return new FixedAspectEditor (false); };
    StandaloneContent free (processor, deviceManager, collector, false);

    EXPECT_FALSE (free.getPreferredAspectRatio().has_value());
}

TEST_F (StandaloneContentTests, FillsBoundsMatchingItsAspectRatio)
{
    processor.editorFactory = [] { return new FixedAspectEditor (true); };
    StandaloneContent content (processor, deviceManager, collector, false);

    // Within a unit of the exact ratio, as a window rounded to whole units is
    content.setBounds (0, 0, 801, 457);

    ASSERT_NE (nullptr, content.getEditor());
    EXPECT_EQ (Rectangle<float> (0, 28, 801, 457 - 28), content.getEditor()->getBounds());
}

TEST_F (StandaloneContentTests, LetterboxesBoundsWiderThanItsAspectRatio)
{
    processor.editorFactory = [] { return new FixedAspectEditor (true); };
    StandaloneContent content (processor, deviceManager, collector, false);
    content.setToolbarVisible (false);

    content.setBounds (0, 0, 1000, 200);

    ASSERT_NE (nullptr, content.getEditor());
    EXPECT_EQ (Rectangle<float> (300, 0, 400, 200), content.getEditor()->getBounds());
}

TEST_F (StandaloneContentTests, LetterboxesBoundsTallerThanItsAspectRatio)
{
    processor.editorFactory = [] { return new FixedAspectEditor (true); };
    StandaloneContent content (processor, deviceManager, collector, false);
    content.setToolbarVisible (false);

    content.setBounds (0, 0, 400, 1000);

    ASSERT_NE (nullptr, content.getEditor());
    EXPECT_EQ (Rectangle<float> (0, 400, 400, 200), content.getEditor()->getBounds());
}

TEST_F (StandaloneContentTests, StretchesEditorsThatDoNotKeepTheirAspectRatio)
{
    processor.editorFactory = [] { return new FixedAspectEditor (false); };
    StandaloneContent content (processor, deviceManager, collector, false);
    content.setToolbarVisible (false);

    content.setBounds (0, 0, 1000, 200);

    ASSERT_NE (nullptr, content.getEditor());
    EXPECT_EQ (Rectangle<float> (0, 0, 1000, 200), content.getEditor()->getBounds());
}

TEST_F (StandaloneContentTests, KeyboardIsOnlyOfferedToNotePlayers)
{
    StandaloneContent effect (processor, deviceManager, collector, false);
    effect.setKeyboardVisible (true);
    EXPECT_FALSE (effect.isKeyboardVisible());

    StandaloneContent instrument (processor, deviceManager, collector, true);
    instrument.setKeyboardVisible (true);
    EXPECT_TRUE (instrument.isKeyboardVisible());
    EXPECT_EQ (300 + 28 + 80, instrument.getPreferredSize().getHeight());

    instrument.setToolbarVisible (false);
    EXPECT_TRUE (instrument.isKeyboardVisible());
    EXPECT_EQ (300 + 80, instrument.getPreferredSize().getHeight());
}
