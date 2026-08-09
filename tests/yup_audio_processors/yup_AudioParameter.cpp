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

using namespace yup;

namespace
{

class TestAudioProcessor final : public AudioProcessor
{
public:
    TestAudioProcessor (AudioBusLayout layout = AudioBusLayout ({}, {}))
        : AudioProcessor ("Test", std::move (layout))
    {
    }

    void prepareToPlay (const AudioSpec& spec) override
    {
        ++prepareCallCount;
        preparedSampleRate = spec.sampleRate;
        preparedSamplesPerBlock = spec.maxBlockSize;
    }

    void releaseResources() override
    {
        ++releaseCallCount;
    }

    void processBlock (AudioProcessContext<float>& context) override
    {
        ignoreUnused (context);
    }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    Result loadStateFromMemory (const MemoryBlock&) override { return Result::ok(); }

    Result saveStateIntoMemory (MemoryBlock&) override { return Result::ok(); }

    bool hasEditor() const override { return false; }

    int prepareCallCount = 0;
    int releaseCallCount = 0;
    float preparedSampleRate = 0.0f;
    int preparedSamplesPerBlock = 0;
};

class ParameterListener final : public AudioParameter::Listener
{
public:
    void parameterValueChanged (const AudioParameter::Ptr& parameter, int indexInContainer) override
    {
        ++valueChangedCount;
        lastParameter = parameter.get();
        lastIndex = indexInContainer;
    }

    void parameterGestureBegin (const AudioParameter::Ptr& parameter, int indexInContainer) override
    {
        ++gestureBeginCount;
        lastParameter = parameter.get();
        lastIndex = indexInContainer;
    }

    void parameterGestureEnd (const AudioParameter::Ptr& parameter, int indexInContainer) override
    {
        ++gestureEndCount;
        lastParameter = parameter.get();
        lastIndex = indexInContainer;
    }

    int valueChangedCount = 0;
    int gestureBeginCount = 0;
    int gestureEndCount = 0;
    AudioParameter* lastParameter = nullptr;
    int lastIndex = -1;
};

class ProcessorListener final : public AudioProcessor::Listener
{
public:
    void audioProcessorChanged (AudioProcessorBase* processor, const AudioProcessor::ChangeDetails& details) override
    {
        ++changedCount;
        lastProcessor = dynamic_cast<AudioProcessor*> (processor);
        lastDetails = details;
    }

    int changedCount = 0;
    AudioProcessor* lastProcessor = nullptr;
    AudioProcessor::ChangeDetails lastDetails;
};

class DefaultStateAudioProcessor : public AudioProcessor
{
public:
    DefaultStateAudioProcessor()
        : AudioProcessor ("Default State", AudioBusLayout ({}, {}))
    {
    }

    void prepareToPlay (const AudioSpec&) override {}

    void releaseResources() override {}

    void processBlock (AudioProcessContext<float>& context) override
    {
        ignoreUnused (context);
    }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    bool hasEditor() const override { return false; }
};

class DataTreeStateAudioProcessor final : public DefaultStateAudioProcessor
{
public:
    bool supportsDataTreeState() const noexcept override { return true; }

    Result loadStateFromDataTree (const DataTree& state) override
    {
        if (! state.isValid() || state.getType() != stateType)
            return Result::fail ("Invalid state");

        value = static_cast<float> (static_cast<double> (state.getProperty ("value", 0.0)));
        return Result::ok();
    }

    Result saveStateIntoDataTree (DataTree& state) override
    {
        state = DataTree (stateType);
        auto transaction = state.beginTransaction();
        transaction.setProperty ("value", value);
        return Result::ok();
    }

    const yup::Identifier stateType = "DataTreeStateAudioProcessorState";
    float value = 0.0f;
};

AudioParameter::Ptr makeParameter (StringRef id, StringRef name)
{
    return AudioParameterBuilder()
        .withID (id)
        .withName (name)
        .withRange (0.0f, 1.0f)
        .withDefault (0.5f)
        .build();
}

MemoryBlock memoryBlockFromString (const String& text)
{
    MemoryBlock result;
    MemoryOutputStream stream (result, false);
    stream << text;
    stream.flush();
    return result;
}

} // namespace

TEST (AudioParameterTests, UsesIndexAsHostIDByDefault)
{
    TestAudioProcessor processor;
    auto first = makeParameter ("first", "First");
    auto second = makeParameter ("second", "Second");

    processor.addParameter (first);
    processor.addParameter (second);

    EXPECT_FALSE (first->hasExplicitHostParameterID());
    EXPECT_FALSE (second->hasExplicitHostParameterID());
    EXPECT_EQ (0u, first->getHostParameterID());
    EXPECT_EQ (1u, second->getHostParameterID());
    EXPECT_EQ (first.get(), processor.getParameterByHostID (0u).get());
    EXPECT_EQ (second.get(), processor.getParameterByHostID (1u).get());
}

TEST (AudioProcessorStateTests, DefaultBinaryStateFailsWithoutDataTreeSupport)
{
    DefaultStateAudioProcessor processor;
    MemoryBlock state;

    EXPECT_FALSE (processor.supportsDataTreeState());
    EXPECT_TRUE (processor.saveStateIntoMemory (state).failed());
    EXPECT_TRUE (processor.loadStateFromMemory (state).failed());
}

TEST (AudioProcessorStateTests, DataTreeStateRoundTripsThroughBinaryXml)
{
    DataTreeStateAudioProcessor source;
    source.value = 12.5f;

    MemoryBlock state;
    ASSERT_TRUE (source.saveStateIntoMemory (state).wasOk());
    EXPECT_FALSE (state.isEmpty());

    MemoryInputStream stream (state, false);
    auto xml = parseXML (stream.readEntireStreamAsString());
    ASSERT_NE (nullptr, xml.get());
    EXPECT_TRUE (xml->hasTagName (source.stateType));
    EXPECT_DOUBLE_EQ (12.5, xml->getDoubleAttribute ("value"));

    DataTreeStateAudioProcessor destination;
    ASSERT_TRUE (destination.loadStateFromMemory (state).wasOk());
    EXPECT_FLOAT_EQ (12.5f, destination.value);
}

TEST (AudioProcessorStateTests, DataTreeBinaryStateRejectsInvalidXml)
{
    DataTreeStateAudioProcessor processor;

    EXPECT_TRUE (processor.loadStateFromMemory (memoryBlockFromString ("not xml")).failed());
    EXPECT_TRUE (processor.loadStateFromMemory (memoryBlockFromString ("<WrongState />")).failed());
}

TEST (AudioParameterTests, UsesExplicitStableHostIDWhenProvided)
{
    TestAudioProcessor processor;
    auto parameter = AudioParameterBuilder()
                         .withID ("gain")
                         .withName ("Gain")
                         .withHostID (1001u)
                         .withRange (0.0f, 1.0f)
                         .withDefault (0.5f)
                         .build();

    processor.addParameter (parameter);

    EXPECT_TRUE (parameter->hasExplicitHostParameterID());
    EXPECT_EQ (1001u, parameter->getHostParameterID());
    EXPECT_EQ (0, processor.getParameterIndexByHostID (1001u));
    EXPECT_EQ (parameter.get(), processor.getParameterByHostID (1001u).get());
    EXPECT_EQ (nullptr, processor.getParameterByHostID (0u).get());
}

TEST (AudioParameterTests, IsNotModulatableByDefault)
{
    auto parameter = makeParameter ("gain", "Gain");

    EXPECT_FALSE (parameter->isModulatable());
    EXPECT_FALSE (parameter->isPerNoteModulatable());
}

TEST (AudioParameterTests, WithModulatableSetsFlag)
{
    auto parameter = AudioParameterBuilder()
                         .withID ("gain")
                         .withName ("Gain")
                         .withRange (0.0f, 1.0f)
                         .withDefault (0.5f)
                         .withModulatable (true)
                         .build();

    EXPECT_TRUE (parameter->isModulatable());
    EXPECT_FALSE (parameter->isPerNoteModulatable());
}

TEST (AudioParameterTests, WithPerNoteModulatableImpliesModulatable)
{
    auto parameter = AudioParameterBuilder()
                         .withID ("gain")
                         .withName ("Gain")
                         .withRange (0.0f, 1.0f)
                         .withDefault (0.5f)
                         .withPerNoteModulatable (true)
                         .build();

    EXPECT_TRUE (parameter->isModulatable());
    EXPECT_TRUE (parameter->isPerNoteModulatable());
}

TEST (AudioParameterTests, ClearingPerNoteModulatablePreservesModulatable)
{
    auto parameter = AudioParameterBuilder()
                         .withID ("gain")
                         .withName ("Gain")
                         .withRange (0.0f, 1.0f)
                         .withDefault (0.5f)
                         .withModulatable (true)
                         .withPerNoteModulatable (true)
                         .withPerNoteModulatable (false)
                         .build();

    EXPECT_TRUE (parameter->isModulatable());
    EXPECT_FALSE (parameter->isPerNoteModulatable());
}

TEST (AudioParameterTests, ReadOnlyParameterIsNotAutomatable)
{
    auto parameter = AudioParameterBuilder()
                         .withID ("gain")
                         .withName ("Gain")
                         .withRange (0.0f, 1.0f)
                         .withDefault (0.5f)
                         .withReadOnly (true)
                         .build();

    EXPECT_TRUE (parameter->isReadOnly());
    EXPECT_FALSE (parameter->isAutomatable());
}

TEST (AudioParameterTests, AutomatableParameterIsNotReadOnly)
{
    auto parameter = AudioParameterBuilder()
                         .withID ("gain")
                         .withName ("Gain")
                         .withRange (0.0f, 1.0f)
                         .withDefault (0.5f)
                         .withReadOnly (true)
                         .withAutomatable (true)
                         .build();

    EXPECT_FALSE (parameter->isReadOnly());
    EXPECT_TRUE (parameter->isAutomatable());
}

TEST (AudioParameterTests, SmoothingCanBeEnabledAndDisabled)
{
    auto smoothed = AudioParameterBuilder()
                        .withID ("gain")
                        .withName ("Gain")
                        .withRange (0.0f, 1.0f)
                        .withDefault (0.5f)
                        .withSmoothing (25.0f)
                        .build();

    EXPECT_TRUE (smoothed->isSmoothingEnabled());
    EXPECT_FLOAT_EQ (25.0f, smoothed->getSmoothingTimeMs());

    auto unsmoothed = AudioParameterBuilder()
                          .withID ("mix")
                          .withName ("Mix")
                          .withRange (0.0f, 1.0f)
                          .withDefault (0.5f)
                          .withSmoothing (25.0f)
                          .withSmoothing (0.0f)
                          .build();

    EXPECT_FALSE (unsmoothed->isSmoothingEnabled());
    EXPECT_FLOAT_EQ (0.0f, unsmoothed->getSmoothingTimeMs());
}

TEST (AudioParameterTests, EnumImpliesSteppedAndClearingSteppedClearsEnum)
{
    auto enumParameter = AudioParameterBuilder()
                             .withID ("shape")
                             .withName ("Shape")
                             .withRange (0.0f, 3.0f)
                             .withDefault (1.0f)
                             .withEnum (true)
                             .build();

    EXPECT_TRUE (enumParameter->isEnum());
    EXPECT_TRUE (enumParameter->isStepped());

    auto continuousParameter = AudioParameterBuilder()
                                   .withID ("shape")
                                   .withName ("Shape")
                                   .withRange (0.0f, 3.0f)
                                   .withDefault (1.0f)
                                   .withEnum (true)
                                   .withStepped (false)
                                   .build();

    EXPECT_FALSE (continuousParameter->isEnum());
    EXPECT_FALSE (continuousParameter->isStepped());
}

TEST (AudioParameterTests, DefaultAndValueAreSnappedToLegalRange)
{
    auto parameter = AudioParameterBuilder()
                         .withID ("steps")
                         .withName ("Steps")
                         .withRange (NormalisableRange<float> (0.0f, 10.0f, 2.0f))
                         .withDefault (5.1f)
                         .build();

    EXPECT_FLOAT_EQ (6.0f, parameter->getDefaultValue());
    EXPECT_FLOAT_EQ (6.0f, parameter->getValue());
    EXPECT_TRUE (parameter->isStepped());
    EXPECT_EQ (5, parameter->getNumSteps());

    parameter->setValue (11.0f);
    EXPECT_FLOAT_EQ (10.0f, parameter->getValue());
}

TEST (AudioParameterTests, NumStepsReportsContinuousAndFlagOnlySteppedParameters)
{
    auto continuous = makeParameter ("gain", "Gain");
    EXPECT_FALSE (continuous->isStepped());
    EXPECT_EQ (0, continuous->getNumSteps());

    auto stepped = AudioParameterBuilder()
                       .withID ("mode")
                       .withName ("Mode")
                       .withRange (0.0f, 3.0f)
                       .withDefault (0.0f)
                       .withStepped (true)
                       .build();

    EXPECT_TRUE (stepped->isStepped());
    EXPECT_EQ (1, stepped->getNumSteps());
}

TEST (AudioParameterTests, CustomStringConvertersAreUsed)
{
    auto parameter = AudioParameterBuilder()
                         .withID ("mode")
                         .withName ("Mode")
                         .withRange (0.0f, 1.0f)
                         .withDefault (0.25f)
                         .withValueToString ([] (float value)
    {
        return value >= 0.5f ? String ("high") : String ("low");
    }).withStringToValue ([] (const String& text)
    {
        return text == "high" ? 0.75f : 0.25f;
    }).build();

    EXPECT_EQ ("low", parameter->toString());
    EXPECT_EQ ("high", parameter->convertToString (0.75f));
    EXPECT_FLOAT_EQ (0.75f, parameter->convertFromString ("high"));

    parameter->fromString ("high");
    EXPECT_FLOAT_EQ (0.75f, parameter->getValue());
    EXPECT_EQ ("high", parameter->toString());
}

TEST (AudioParameterTests, ListenerReceivesValueAndGestureChanges)
{
    auto parameter = makeParameter ("gain", "Gain");
    parameter->setIndexInContainer (7);

    ParameterListener listener;
    parameter->addListener (&listener);

    parameter->setValueNotifyingHost (0.75f);
    EXPECT_EQ (1, listener.valueChangedCount);
    EXPECT_EQ (parameter.get(), listener.lastParameter);
    EXPECT_EQ (7, listener.lastIndex);

    parameter->beginChangeGesture();
    parameter->beginChangeGesture();
    EXPECT_TRUE (parameter->isPerformingChangeGesture());
    EXPECT_EQ (1, listener.gestureBeginCount);

    parameter->endChangeGesture();
    EXPECT_TRUE (parameter->isPerformingChangeGesture());
    EXPECT_EQ (0, listener.gestureEndCount);

    parameter->endChangeGesture();
    EXPECT_FALSE (parameter->isPerformingChangeGesture());
    EXPECT_EQ (1, listener.gestureEndCount);
}

TEST (AudioParameterTests, RemovedListenerDoesNotReceiveFurtherChanges)
{
    auto parameter = makeParameter ("gain", "Gain");

    ParameterListener listener;
    parameter->addListener (&listener);
    parameter->removeListener (&listener);

    parameter->setValueNotifyingHost (0.75f);
    EXPECT_EQ (0, listener.valueChangedCount);
}

//==============================================================================
// ParameterUnit tests
//==============================================================================

TEST (AudioParameterTests, GenericUnitIsDefault)
{
    auto parameter = makeParameter ("gain", "Gain");
    EXPECT_EQ (AudioParameter::ParameterUnit::Generic, parameter->getUnit());
    EXPECT_TRUE (parameter->getUnitName().isEmpty());
}

TEST (AudioParameterTests, WithUnitSetsCorrectUnit)
{
    auto parameter = AudioParameterBuilder()
                         .withID ("freq")
                         .withName ("Frequency")
                         .withRange (20.0f, 20000.0f)
                         .withDefault (1000.0f)
                         .withUnit (AudioParameter::ParameterUnit::Hertz)
                         .build();

    EXPECT_EQ (AudioParameter::ParameterUnit::Hertz, parameter->getUnit());
    EXPECT_TRUE (parameter->getUnitName().isEmpty());
}

TEST (AudioParameterTests, WithUnitCustomSetsUnitName)
{
    auto parameter = AudioParameterBuilder()
                         .withID ("level")
                         .withName ("Level")
                         .withRange (-60.0f, 0.0f)
                         .withDefault (-18.0f)
                         .withUnit (AudioParameter::ParameterUnit::Custom, "dBFS")
                         .build();

    EXPECT_EQ (AudioParameter::ParameterUnit::Custom, parameter->getUnit());
    EXPECT_EQ (String ("dBFS"), parameter->getUnitName());
}

TEST (AudioParameterTests, AllUnitValuesCanBeSet)
{
    // Verify every ParameterUnit value compiles and stores correctly
    struct TestCase
    {
        AudioParameter::ParameterUnit unit;
        String expectedLabel;
    };

    const TestCase cases[] = {
        { AudioParameter::ParameterUnit::Generic, "" },
        { AudioParameter::ParameterUnit::Percent, "%" },
        { AudioParameter::ParameterUnit::Decibels, "dB" },
        { AudioParameter::ParameterUnit::Hertz, "Hz" },
        { AudioParameter::ParameterUnit::Milliseconds, "ms" },
        { AudioParameter::ParameterUnit::Seconds, "s" },
        { AudioParameter::ParameterUnit::Degrees, "deg" },
        { AudioParameter::ParameterUnit::Cents, "ct" },
        { AudioParameter::ParameterUnit::Semitones, "st" },
        { AudioParameter::ParameterUnit::Octaves, "oct" },
        { AudioParameter::ParameterUnit::BPM, "bpm" },
        { AudioParameter::ParameterUnit::Beats, "beats" },
        { AudioParameter::ParameterUnit::Ratio, "" },
        { AudioParameter::ParameterUnit::LinearGain, "" },
        { AudioParameter::ParameterUnit::Pan, "" },
        { AudioParameter::ParameterUnit::MIDINoteNumber, "" },
    };

    for (const auto& tc : cases)
    {
        auto parameter = AudioParameterBuilder()
                             .withID ("p")
                             .withName ("Param")
                             .withRange (0.0f, 1.0f)
                             .withDefault (0.5f)
                             .withUnit (tc.unit)
                             .build();

        EXPECT_EQ (tc.unit, parameter->getUnit());
        EXPECT_STREQ (tc.expectedLabel.toRawUTF8(),
                      AudioParameter::getParameterUnitShortName (parameter->getUnit(), parameter->getUnitName()));
    }
}

TEST (AudioParameterTests, UnitIsRoundTrippedThroughBuilder)
{
    auto parameter = AudioParameterBuilder()
                         .withID ("pan")
                         .withName ("Pan")
                         .withRange (-1.0f, 1.0f)
                         .withDefault (0.0f)
                         .withUnit (AudioParameter::ParameterUnit::Pan)
                         .build();

    EXPECT_EQ (AudioParameter::ParameterUnit::Pan, parameter->getUnit());
    EXPECT_EQ (-1.0f, parameter->getMinimumValue());
    EXPECT_EQ (1.0f, parameter->getMaximumValue());
    EXPECT_EQ (0.0f, parameter->getDefaultValue());
}

TEST (AudioProcessorTests, DuplicateParameterIDsAreIgnored)
{
    TestAudioProcessor processor;
    auto first = makeParameter ("gain", "Gain");
    auto duplicateID = makeParameter ("gain", "Duplicate Gain");
    auto hostID = AudioParameterBuilder()
                      .withID ("mix")
                      .withName ("Mix")
                      .withHostID (10u)
                      .withRange (0.0f, 1.0f)
                      .withDefault (0.5f)
                      .build();

    processor.addParameter (first);
    processor.addParameter (duplicateID);
    processor.addParameter (hostID);

    EXPECT_EQ (2, static_cast<int> (processor.getParameters().size()));
    EXPECT_EQ (first.get(), processor.getParameterByID ("gain").get());
    EXPECT_EQ (hostID.get(), processor.getParameterByHostID (10u).get());
    EXPECT_EQ (-1, processor.getParameterIndexByHostID (11u));
}

TEST (AudioProcessorTests, CountsOnlyAudioBuses)
{
    TestAudioProcessor processor (AudioBusLayout (
        { AudioBus ("Audio In", AudioBus::Type::Audio, AudioBus::Direction::Input, 2),
          AudioBus ("MIDI In", AudioBus::Type::Midi, AudioBus::Direction::Input, 0) },
        { AudioBus ("Audio Out", AudioBus::Type::Audio, AudioBus::Direction::Output, 2),
          AudioBus ("MIDI Out", AudioBus::Type::Midi, AudioBus::Direction::Output, 0) }));

    EXPECT_EQ (1, processor.getNumAudioInputs());
    EXPECT_EQ (1, processor.getNumAudioOutputs());
}

TEST (AudioProcessorTests, LatencyChangeNotifiesOnlyWhenValueChanges)
{
    TestAudioProcessor processor;
    ProcessorListener listener;
    processor.addListener (&listener);

    processor.setLatencySamples (-12);
    EXPECT_EQ (0, processor.getLatencySamples());
    EXPECT_EQ (0, listener.changedCount);

    processor.setLatencySamples (64);
    EXPECT_EQ (64, processor.getLatencySamples());
    EXPECT_EQ (1, listener.changedCount);
    EXPECT_EQ (&processor, listener.lastProcessor);
    EXPECT_TRUE (listener.lastDetails.latencyChanged);

    processor.setLatencySamples (64);
    EXPECT_EQ (1, listener.changedCount);

    processor.removeListener (&listener);
    processor.setLatencySamples (128);
    EXPECT_EQ (1, listener.changedCount);
}

TEST (AudioProcessorTests, ChangeDetailsSetRequestedFlags)
{
    const auto details = AudioProcessor::ChangeDetails()
                             .withLatencyChanged (true)
                             .withTailChanged (true)
                             .withParameterValuesChanged (true)
                             .withParameterInfoChanged (true)
                             .withNonParameterStateChanged (true);

    EXPECT_TRUE (details.latencyChanged);
    EXPECT_TRUE (details.tailChanged);
    EXPECT_TRUE (details.parameterValuesChanged);
    EXPECT_TRUE (details.parameterInfoChanged);
    EXPECT_TRUE (details.nonParameterStateChanged);

    const auto cleared = details.withLatencyChanged (false)
                             .withTailChanged (false)
                             .withParameterValuesChanged (false)
                             .withParameterInfoChanged (false)
                             .withNonParameterStateChanged (false);

    EXPECT_FALSE (cleared.latencyChanged);
    EXPECT_FALSE (cleared.tailChanged);
    EXPECT_FALSE (cleared.parameterValuesChanged);
    EXPECT_FALSE (cleared.parameterInfoChanged);
    EXPECT_FALSE (cleared.nonParameterStateChanged);
}

TEST (AudioProcessorTests, PlaybackConfigurationReleasesBeforePreparing)
{
    TestAudioProcessor processor;

    processor.setPlaybackConfiguration (48000.0f, 256);

    EXPECT_EQ (1, processor.releaseCallCount);
    EXPECT_EQ (1, processor.prepareCallCount);
    EXPECT_FLOAT_EQ (48000.0f, processor.getSampleRate());
    EXPECT_EQ (256, processor.getSamplesPerBlock());
    EXPECT_FLOAT_EQ (48000.0f, processor.preparedSampleRate);
    EXPECT_EQ (256, processor.preparedSamplesPerBlock);
}
