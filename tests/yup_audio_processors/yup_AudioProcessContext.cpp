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
