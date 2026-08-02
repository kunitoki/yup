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

class TestPlayHead final : public AudioPlayHead
{
public:
    std::optional<PositionInfo> getPosition() const override
    {
        PositionInfo position;
        position.setTimeInSamples (128);
        return position;
    }
};

} // namespace

TEST (AudioProcessContextTests, DefaultsToNoPlayHead)
{
    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    AudioProcessContext<float> context { audio, midi, params };

    EXPECT_EQ (nullptr, context.playHead);
}

TEST (AudioProcessContextTests, CarriesPlayHeadPointer)
{
    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    ParameterChangeBuffer params;
    TestPlayHead playHead;

    AudioProcessContext<float> context { audio, midi, params, &playHead };

    EXPECT_EQ (&playHead, context.playHead);
    ASSERT_TRUE (context.playHead->getPosition().has_value());
    EXPECT_EQ (128, *context.playHead->getPosition()->getTimeInSamples());
}

TEST (AudioProcessContextTests, CarriesAudioBufferReference)
{
    AudioBuffer<float> audio (2, 64);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    AudioProcessContext<float> context { audio, midi, params };

    EXPECT_EQ (2, context.audio.getNumChannels());
    EXPECT_EQ (64, context.audio.getNumSamples());
}

TEST (AudioProcessContextTests, CarriesMidiBufferReference)
{
    AudioBuffer<float> audio (1, 8);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    AudioProcessContext<float> context { audio, midi, params };

    EXPECT_TRUE (context.midi.isEmpty());
}

TEST (AudioProcessContextTests, CarriesParameterChangeBufferReference)
{
    AudioBuffer<float> audio (1, 8);
    MidiBuffer midi;
    ParameterChangeBuffer params;
    params.reserve (1);
    EXPECT_TRUE (params.addChange (0, 0.5f, 0));

    AudioProcessContext<float> context { audio, midi, params };

    EXPECT_EQ (1, context.params.getNumChanges());
}

TEST (AudioProcessContextTests, DoubleBufferVersion)
{
    AudioBuffer<double> audio (2, 32);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    AudioProcessContext<double> context { audio, midi, params };

    EXPECT_EQ (2, context.audio.getNumChannels());
    EXPECT_EQ (32, context.audio.getNumSamples());
    EXPECT_EQ (nullptr, context.playHead);
}

//==============================================================================
// Per-bus view tests

TEST (AudioProcessContextTests, PerBusViewsDefaultToEmpty)
{
    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    AudioProcessContext<float> context { audio, midi, params };

    EXPECT_EQ (0u, context.inputs.size());
    EXPECT_EQ (0u, context.outputs.size());
}

TEST (AudioProcessContextTests, GetMainInputReturnsEmptyWhenNoInputs)
{
    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    AudioProcessContext<float> context { audio, midi, params };

    const auto& mainInput = context.getMainInput();
    EXPECT_EQ (0, mainInput.getNumChannels());
}

TEST (AudioProcessContextTests, GetMainOutputReturnsEmptyWhenNoOutputs)
{
    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    AudioProcessContext<float> context { audio, midi, params };

    auto& mainOutput = context.getMainOutput();
    EXPECT_EQ (0, mainOutput.getNumChannels());
}

TEST (AudioProcessContextTests, GetAuxiliaryInputReturnsEmptyWhenNoInputs)
{
    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    AudioProcessContext<float> context { audio, midi, params };

    const auto& auxInput = context.getAuxiliaryInput (0);
    EXPECT_EQ (0, auxInput.getNumChannels());
}

TEST (AudioProcessContextTests, PerBusViewsAreAccessible)
{
    float mainLeft[16], mainRight[16];
    float sideLeft[16], sideRight[16];
    float outLeft[16], outRight[16];

    const float* inputPtrs[] = { mainLeft, mainRight };
    const float* sidePtrs[] = { sideLeft, sideRight };
    float* outputPtrs[] = { outLeft, outRight };

    AudioBusBufferView<const float> inputViewsArr[] = {
        { inputPtrs, 2, AudioBus::Role::Main },
        { sidePtrs, 2, AudioBus::Role::Auxiliary }
    };
    AudioBusBufferView<float> outputViewsArr[] = {
        { outputPtrs, 2, AudioBus::Role::Main }
    };

    AudioBuffer<float> audio (outputPtrs, 2, 0, 16);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    AudioProcessContext<float> context {
        audio, midi, params, nullptr, { inputViewsArr, 2 }, { outputViewsArr, 1 }
    };

    EXPECT_EQ (2u, context.inputs.size());
    EXPECT_EQ (1u, context.outputs.size());
}

TEST (AudioProcessContextTests, GetMainInputFindsFirstMainInputBus)
{
    float mainLeft[16], mainRight[16];
    float sideLeft[16];
    float outLeft[16], outRight[16];

    const float* inputPtrs[] = { mainLeft, mainRight };
    const float* sidePtrs[] = { sideLeft };
    float* outputPtrs[] = { outLeft, outRight };

    AudioBusBufferView<const float> inputViewsArr[] = {
        { inputPtrs, 2, AudioBus::Role::Main },
        { sidePtrs, 1, AudioBus::Role::Auxiliary }
    };
    AudioBusBufferView<float> outputViewsArr[] = {
        { outputPtrs, 2, AudioBus::Role::Main }
    };

    AudioBuffer<float> audio (outputPtrs, 2, 0, 16);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    AudioProcessContext<float> context {
        audio, midi, params, nullptr, { inputViewsArr, 2 }, { outputViewsArr, 1 }
    };

    const auto& mainInput = context.getMainInput();
    EXPECT_EQ (2, mainInput.getNumChannels());
    EXPECT_EQ (AudioBus::Role::Main, mainInput.getRole());
}

TEST (AudioProcessContextTests, GetAuxiliaryInputFindsAuxBus)
{
    float mainLeft[16];
    float sideLeft[16], sideRight[16];
    float outLeft[16];

    const float* inputPtrs[] = { mainLeft };
    const float* sidePtrs[] = { sideLeft, sideRight };
    float* outputPtrs[] = { outLeft };

    AudioBusBufferView<const float> inputViewsArr[] = {
        { inputPtrs, 1, AudioBus::Role::Main },
        { sidePtrs, 2, AudioBus::Role::Auxiliary }
    };
    AudioBusBufferView<float> outputViewsArr[] = {
        { outputPtrs, 1, AudioBus::Role::Main }
    };

    AudioBuffer<float> audio (outputPtrs, 1, 0, 16);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    AudioProcessContext<float> context {
        audio, midi, params, nullptr, { inputViewsArr, 2 }, { outputViewsArr, 1 }
    };

    const auto& auxInput = context.getAuxiliaryInput (0);
    EXPECT_EQ (2, auxInput.getNumChannels());
    EXPECT_EQ (AudioBus::Role::Auxiliary, auxInput.getRole());
}

TEST (AudioProcessContextTests, GetAuxiliaryInputOutOfRangeReturnsEmpty)
{
    float mainLeft[16];
    float outLeft[16];

    const float* inputPtrs[] = { mainLeft };
    float* outputPtrs[] = { outLeft };

    AudioBusBufferView<const float> inputViewsArr[] = {
        { inputPtrs, 1, AudioBus::Role::Main }
    };
    AudioBusBufferView<float> outputViewsArr[] = {
        { outputPtrs, 1, AudioBus::Role::Main }
    };

    AudioBuffer<float> audio (outputPtrs, 1, 0, 16);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    AudioProcessContext<float> context {
        audio, midi, params, nullptr, { inputViewsArr, 1 }, { outputViewsArr, 1 }
    };

    const auto& auxInput0 = context.getAuxiliaryInput (0);
    EXPECT_EQ (0, auxInput0.getNumChannels());

    const auto& auxInput1 = context.getAuxiliaryInput (1);
    EXPECT_EQ (0, auxInput1.getNumChannels());
}

TEST (AudioProcessContextTests, GetMainOutputFindsFirstMainOutputBus)
{
    float inLeft[16];
    float outLeft[16], outRight[16];

    const float* inputPtrs[] = { inLeft };
    float* outputPtrs[] = { outLeft, outRight };

    AudioBusBufferView<const float> inputViewsArr[] = {
        { inputPtrs, 1, AudioBus::Role::Main }
    };
    AudioBusBufferView<float> outputViewsArr[] = {
        { outputPtrs, 2, AudioBus::Role::Main }
    };

    AudioBuffer<float> audio (outputPtrs, 2, 0, 16);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    AudioProcessContext<float> context {
        audio, midi, params, nullptr, { inputViewsArr, 1 }, { outputViewsArr, 1 }
    };

    auto& mainOutput = context.getMainOutput();
    EXPECT_EQ (2, mainOutput.getNumChannels());
    EXPECT_EQ (AudioBus::Role::Main, mainOutput.getRole());
}
