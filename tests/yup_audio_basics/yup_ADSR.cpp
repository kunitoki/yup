/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <yup_audio_basics/yup_audio_basics.h>
#include <gtest/gtest.h>

using namespace yup;

namespace
{
void advanceADSR (ADSR& adsr, int numSamplesToAdvance)
{
    while (--numSamplesToAdvance >= 0)
        adsr.getNextSample();
}

AudioBuffer<float> getTestBuffer (double sampleRate, float lengthInSeconds)
{
    AudioBuffer<float> buffer { 2, roundToInt (lengthInSeconds * sampleRate) };

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            buffer.setSample (channel, sample, 1.0f);

    return buffer;
}

bool isIncreasing (const AudioBuffer<float>& b)
{
    jassert (b.getNumChannels() > 0 && b.getNumSamples() > 0);

    for (int channel = 0; channel < b.getNumChannels(); ++channel)
    {
        float previousSample = -1.0f;

        for (int sample = 0; sample < b.getNumSamples(); ++sample)
        {
            const auto currentSample = b.getSample (channel, sample);

            if (currentSample <= previousSample)
                return false;

            previousSample = currentSample;
        }
    }

    return true;
}

bool isDecreasing (const AudioBuffer<float>& b)
{
    jassert (b.getNumChannels() > 0 && b.getNumSamples() > 0);

    for (int channel = 0; channel < b.getNumChannels(); ++channel)
    {
        float previousSample = std::numeric_limits<float>::max();

        for (int sample = 0; sample < b.getNumSamples(); ++sample)
        {
            const auto currentSample = b.getSample (channel, sample);

            if (currentSample >= previousSample)
                return false;

            previousSample = currentSample;
        }
    }

    return true;
}

bool isSustained (const AudioBuffer<float>& b, float sustainLevel)
{
    jassert (b.getNumChannels() > 0 && b.getNumSamples() > 0);

    for (int channel = 0; channel < b.getNumChannels(); ++channel)
        if (b.findMinMax (channel, 0, b.getNumSamples()) != Range<float> { sustainLevel, sustainLevel })
            return false;

    return true;
}
} // namespace

class ADSRTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        adsr.setSampleRate (sampleRate);
        adsr.setParameters (parameters);
    }

    constexpr static double sampleRate = 44100.0;
    const ADSR::Parameters parameters { 0.1f, 0.1f, 0.5f, 0.1f };
    ADSR adsr;
};

TEST_F (ADSRTest, Idle)
{
    adsr.reset();

    EXPECT_FALSE (adsr.isActive());
    EXPECT_EQ (adsr.getNextSample(), 0.0f);
}

TEST_F (ADSRTest, Attack)
{
    adsr.reset();

    adsr.noteOn();
    EXPECT_TRUE (adsr.isActive());

    auto buffer = getTestBuffer (sampleRate, parameters.attack);
    adsr.applyEnvelopeToBuffer (buffer, 0, buffer.getNumSamples());

    EXPECT_TRUE (isIncreasing (buffer));
}

TEST_F (ADSRTest, Decay)
{
    adsr.reset();

    adsr.noteOn();
    advanceADSR (adsr, roundToInt (parameters.attack * sampleRate));

    auto buffer = getTestBuffer (sampleRate, parameters.decay);
    adsr.applyEnvelopeToBuffer (buffer, 0, buffer.getNumSamples());

    EXPECT_TRUE (isDecreasing (buffer));
}

TEST_F (ADSRTest, Sustain)
{
    adsr.reset();

    adsr.noteOn();
    advanceADSR (adsr, roundToInt ((parameters.attack + parameters.decay + 0.01) * sampleRate));

    Random random (12345);

    for (int numTests = 0; numTests < 100; ++numTests)
    {
        const auto sustainLevel = random.nextFloat();
        const auto sustainLength = jmax (0.1f, random.nextFloat());

        adsr.setParameters ({ parameters.attack, parameters.decay, sustainLevel, parameters.release });

        auto buffer = getTestBuffer (sampleRate, sustainLength);
        adsr.applyEnvelopeToBuffer (buffer, 0, buffer.getNumSamples());

        EXPECT_TRUE (isSustained (buffer, sustainLevel));
    }
}

TEST_F (ADSRTest, Release)
{
    adsr.reset();

    adsr.noteOn();
    advanceADSR (adsr, roundToInt ((parameters.attack + parameters.decay) * sampleRate));
    adsr.noteOff();

    auto buffer = getTestBuffer (sampleRate, parameters.release);
    adsr.applyEnvelopeToBuffer (buffer, 0, buffer.getNumSamples());

    EXPECT_TRUE (isDecreasing (buffer));
}

TEST_F (ADSRTest, ZeroLengthAttackJumpsToDecay)
{
    adsr.reset();
    adsr.setParameters ({ 0.0f, parameters.decay, parameters.sustain, parameters.release });

    adsr.noteOn();

    auto buffer = getTestBuffer (sampleRate, parameters.decay);
    adsr.applyEnvelopeToBuffer (buffer, 0, buffer.getNumSamples());

    EXPECT_TRUE (isDecreasing (buffer));
}

TEST_F (ADSRTest, ZeroLengthDecayJumpsToSustain)
{
    adsr.reset();
    adsr.setParameters ({ parameters.attack, 0.0f, parameters.sustain, parameters.release });

    adsr.noteOn();
    advanceADSR (adsr, roundToInt (parameters.attack * sampleRate));
    adsr.getNextSample();

    EXPECT_EQ (adsr.getNextSample(), parameters.sustain);

    auto buffer = getTestBuffer (sampleRate, 1);
    adsr.applyEnvelopeToBuffer (buffer, 0, buffer.getNumSamples());

    EXPECT_TRUE (isSustained (buffer, parameters.sustain));
}

TEST_F (ADSRTest, ZeroLengthAttackAndDecayJumpsToSustain)
{
    adsr.reset();
    adsr.setParameters ({ 0.0f, 0.0f, parameters.sustain, parameters.release });

    adsr.noteOn();

    EXPECT_EQ (adsr.getNextSample(), parameters.sustain);

    auto buffer = getTestBuffer (sampleRate, 1);
    adsr.applyEnvelopeToBuffer (buffer, 0, buffer.getNumSamples());

    EXPECT_TRUE (isSustained (buffer, parameters.sustain));
}

TEST_F (ADSRTest, ZeroLengthAttackAndDecayReleasesCorrectly)
{
    adsr.reset();
    adsr.setParameters ({ 0.0f, 0.0f, parameters.sustain, parameters.release });

    adsr.noteOn();
    adsr.noteOff();

    auto buffer = getTestBuffer (sampleRate, parameters.release);
    adsr.applyEnvelopeToBuffer (buffer, 0, buffer.getNumSamples());

    EXPECT_TRUE (isDecreasing (buffer));
}

TEST_F (ADSRTest, ZeroLengthReleaseResetsToIdle)
{
    adsr.reset();
    adsr.setParameters ({ parameters.attack, parameters.decay, parameters.sustain, 0.0f });

    adsr.noteOn();
    advanceADSR (adsr, roundToInt ((parameters.attack + parameters.decay) * sampleRate));
    adsr.noteOff();

    EXPECT_FALSE (adsr.isActive());
}
