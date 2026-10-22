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

#include <yup_dsp/yup_dsp.h>

#include <gtest/gtest.h>

#include <vector>

using namespace yup;

//==============================================================================
class CurveEnvelopeTests : public ::testing::Test
{
protected:
    using Curve = BreakpointCurve<float>;
    using Envelope = CurveEnvelope<float>;

    static constexpr double sampleRate = 48000.0;

    /** Linear attack, decay, sustain and release, ported from the Prism example's
        SynthEnvelope without its delay and hold stages.
    */
    struct LinearAdsr
    {
        float attackIncrement = 0.0f;
        float decayIncrement = 0.0f;
        float releaseIncrement = 0.0f;
        float sustain = 0.0f;
        int releaseSamples = 1;
        float level = 0.0f;
        int stage = 0;

        LinearAdsr (int attackSamples, int decaySamples, float sustainLevel, int releaseSamplesIn)
            : attackIncrement (1.0f / static_cast<float> (attackSamples))
            , decayIncrement ((1.0f - sustainLevel) / static_cast<float> (decaySamples))
            , sustain (sustainLevel)
            , releaseSamples (releaseSamplesIn)
            , stage (1)
        {
        }

        void noteOff()
        {
            releaseIncrement = level / static_cast<float> (releaseSamples);
            stage = 4;
        }

        float getNextValue()
        {
            switch (stage)
            {
                case 1:
                    level += attackIncrement;
                    if (level >= 1.0f)
                    {
                        level = 1.0f;
                        stage = 2;
                    }
                    break;

                case 2:
                    level -= decayIncrement;
                    if (level <= sustain)
                    {
                        level = sustain;
                        stage = 3;
                    }
                    break;

                case 4:
                    level -= releaseIncrement;
                    if (level <= 0.0f)
                    {
                        level = 0.0f;
                        stage = 0;
                    }
                    break;

                default:
                    break;
            }

            return level;
        }
    };

    static Curve makeAdsr (float attack, float decay, float sustain, float release, float curve = 0.0f)
    {
        Curve result;
        result.addPoint (0.0f, 0.0f);
        result.addPoint (attack, 1.0f, curve);
        result.addPoint (attack + decay, sustain, curve);
        result.addPoint (attack + decay + release, 0.0f, curve);
        result.setSustainIndex (2);
        return result;
    }

    static float seconds (int samples) { return static_cast<float> (samples / sampleRate); }

    static float maxJump (const std::vector<float>& values)
    {
        auto result = 0.0f;

        for (std::size_t i = 1; i < values.size(); ++i)
            result = jmax (result, std::abs (values[i] - values[i - 1]));

        return result;
    }

    Envelope envelope;

    void SetUp() override
    {
        envelope.prepare (sampleRate);
    }
};

//==============================================================================
TEST_F (CurveEnvelopeTests, IdleUntilNoteOn)
{
    envelope.setCurve (makeAdsr (0.01f, 0.01f, 0.5f, 0.01f));

    EXPECT_FALSE (envelope.isActive());
    EXPECT_EQ (0.0f, envelope.getNextValue());
    EXPECT_EQ (0.0f, envelope.getLevel());
}

TEST_F (CurveEnvelopeTests, LinearAdsrMatchesTheSynthEnvelopeReference)
{
    // Power of two lengths and a sustain of 0.5 keep the reference's float
    // increments exact, so both reach each breakpoint on the same sample.
    constexpr int attack = 512;
    constexpr int decay = 1024;
    constexpr int release = 2048;
    constexpr float sustain = 0.5f;

    envelope.setCurve (makeAdsr (seconds (attack), seconds (decay), sustain, seconds (release)));
    envelope.noteOn();

    LinearAdsr reference (attack, decay, sustain, release);

    for (int i = 0; i < attack + decay + 500; ++i)
        ASSERT_NEAR (reference.getNextValue(), envelope.getNextValue(), 1e-4f) << i;

    reference.noteOff();
    envelope.noteOff();

    for (int i = 0; i < release + 10; ++i)
        ASSERT_NEAR (reference.getNextValue(), envelope.getNextValue(), 1e-4f) << i;

    EXPECT_FALSE (envelope.isActive());
    EXPECT_EQ (0.0f, envelope.getLevel());
}

TEST_F (CurveEnvelopeTests, CurvedSegmentFollowsTheShape)
{
    constexpr int attack = 1000;

    envelope.setCurve (makeAdsr (seconds (attack), 0.1f, 0.5f, 0.1f, 0.5f));
    envelope.noteOn();

    for (int i = 1; i < attack; ++i)
    {
        const auto value = envelope.getNextValue();

        if (i % 100 == 0)
            EXPECT_NEAR (Curve::shape (0.5f, static_cast<float> (i) / attack), value, 1e-4f) << i;
    }

    EXPECT_NEAR (0.1824f, Curve::shape (0.5f, 0.5f), 1e-4f);
}

TEST_F (CurveEnvelopeTests, HoldsAtSustainUntilNoteOff)
{
    envelope.setCurve (makeAdsr (0.001f, 0.001f, 0.3f, 0.001f));
    envelope.noteOn();

    for (int i = 0; i < 48000; ++i)
        envelope.getNextValue();

    EXPECT_TRUE (envelope.isActive());
    EXPECT_NEAR (0.3f, envelope.getLevel(), 1e-6f);
}

TEST_F (CurveEnvelopeTests, ReleaseFromMidAttackStartsAtTheCurrentLevel)
{
    constexpr int attack = 1000;
    constexpr int release = 500;

    envelope.setCurve (makeAdsr (seconds (attack), 0.1f, 0.5f, seconds (release)));
    envelope.noteOn();

    for (int i = 0; i < attack / 4; ++i)
        envelope.getNextValue();

    const auto levelAtNoteOff = envelope.getLevel();
    EXPECT_NEAR (0.25f, levelAtNoteOff, 1e-3f);

    envelope.noteOff();

    std::vector<float> values { levelAtNoteOff };

    while (envelope.isActive())
        values.push_back (envelope.getNextValue());

    EXPECT_LT (maxJump (values), 1.5f * levelAtNoteOff / release);
    EXPECT_NEAR (static_cast<float> (release + 1), static_cast<float> (values.size()), 2.0f);
    EXPECT_EQ (0.0f, values.back());
}

TEST_F (CurveEnvelopeTests, LoopRepeatsWhileHeldWithoutClicks)
{
    Curve curve;
    curve.addPoint (0.0f, 0.0f);
    curve.addPoint (seconds (100), 1.0f);
    curve.addPoint (seconds (300), 0.2f);
    curve.addPoint (seconds (400), 0.8f);
    curve.addPoint (seconds (500), 0.0f);
    curve.setLoop (1, 3);

    envelope.setCurve (curve);
    envelope.noteOn();

    std::vector<float> values;

    for (int i = 0; i < 5000; ++i)
        values.push_back (envelope.getNextValue());

    EXPECT_TRUE (envelope.isActive());

    // The loop jumps from 0.8 back into the 1 -> 0.2 segment from the current level
    // instead of from 1, so the steepest step stays the 1 / 100 of the first segment.
    EXPECT_LT (maxJump (values), 0.011f);

    // A loop spans 300 samples, so the waveform repeats.
    EXPECT_NEAR (values[1000], values[1300], 1e-3f);
    EXPECT_NEAR (values[2000], values[2900], 1e-3f);

    envelope.noteOff();

    int remaining = 0;

    while (envelope.isActive() && remaining < 1000)
    {
        values.push_back (envelope.getNextValue());
        ++remaining;
    }

    EXPECT_FALSE (envelope.isActive());
    EXPECT_NEAR (100.0f, static_cast<float> (remaining), 2.0f);
    EXPECT_LT (maxJump (values), 0.011f);
}

TEST_F (CurveEnvelopeTests, ZeroLengthLoopHoldsInsteadOfSpinning)
{
    Curve curve;
    curve.addPoint (0.0f, 0.0f);
    curve.addPoint (seconds (10), 1.0f);
    curve.addPoint (seconds (10), 0.5f);
    curve.addPoint (seconds (20), 0.0f);
    curve.setLoop (1, 2);

    envelope.setCurve (curve);
    envelope.noteOn();

    for (int i = 0; i < 100; ++i)
        envelope.getNextValue();

    EXPECT_TRUE (envelope.isActive());
    EXPECT_NEAR (0.5f, envelope.getLevel(), 1e-6f);
}

TEST_F (CurveEnvelopeTests, ZeroLengthSegmentsAreCrossedInstantly)
{
    Curve curve;
    curve.addPoint (0.0f, 0.0f);
    curve.addPoint (0.0f, 1.0f);
    curve.addPoint (seconds (100), 0.0f);

    envelope.setCurve (curve);
    envelope.noteOn();

    EXPECT_NEAR (0.99f, envelope.getNextValue(), 1e-4f);

    for (int i = 0; i < 100; ++i)
        envelope.getNextValue();

    EXPECT_FALSE (envelope.isActive());
}

TEST_F (CurveEnvelopeTests, OneShotIgnoresNoteOffAndEnds)
{
    Curve curve;
    curve.addPoint (0.0f, 0.0f);
    curve.addPoint (seconds (10), 1.0f);
    curve.addPoint (seconds (20), 0.0f);

    envelope.setCurve (curve);
    envelope.noteOn();
    envelope.getNextValue();
    envelope.noteOff();

    for (int i = 0; i < 8; ++i)
        envelope.getNextValue();

    EXPECT_NEAR (0.9f, envelope.getLevel(), 1e-4f);

    for (int i = 0; i < 20; ++i)
        envelope.getNextValue();

    EXPECT_FALSE (envelope.isActive());
}

TEST_F (CurveEnvelopeTests, FinishedEnvelopeHoldsItsLastLevel)
{
    Curve curve;
    curve.addPoint (0.0f, 0.0f);
    curve.addPoint (seconds (10), 0.75f);

    envelope.setCurve (curve);
    envelope.noteOn();

    for (int i = 0; i < 20; ++i)
        envelope.getNextValue();

    EXPECT_FALSE (envelope.isActive());
    EXPECT_EQ (0.75f, envelope.getLevel());
    EXPECT_EQ (0.75f, envelope.getNextValue());

    envelope.reset();
    EXPECT_EQ (0.0f, envelope.getLevel());
}

TEST_F (CurveEnvelopeTests, TimeScaleStretchesSegments)
{
    envelope.setCurve (makeAdsr (seconds (100), seconds (100), 0.5f, seconds (100)));
    envelope.setTimeScale (2.0f);
    envelope.noteOn();

    for (int i = 0; i < 100; ++i)
        envelope.getNextValue();

    EXPECT_NEAR (0.5f, envelope.getLevel(), 1e-3f);

    // Changing the scale mid-segment keeps the progress through it.
    envelope.setTimeScale (1.0f);

    for (int i = 0; i < 50; ++i)
        envelope.getNextValue();

    EXPECT_NEAR (1.0f, envelope.getLevel(), 1e-3f);
}

TEST_F (CurveEnvelopeTests, RestartJumpsToTheFirstPointAndLegatoContinues)
{
    envelope.setCurve (makeAdsr (seconds (100), seconds (100), 0.5f, seconds (100)));
    envelope.noteOn();

    for (int i = 0; i < 1000; ++i)
        envelope.getNextValue();

    envelope.setRetriggerMode (Envelope::RetriggerMode::legato);
    envelope.noteOn();
    EXPECT_NEAR (0.5f + 0.5f / 100.0f, envelope.getNextValue(), 1e-4f);

    for (int i = 0; i < 1000; ++i)
        envelope.getNextValue();

    envelope.setRetriggerMode (Envelope::RetriggerMode::restart);
    envelope.noteOn();
    EXPECT_NEAR (0.01f, envelope.getNextValue(), 1e-4f);
}

TEST_F (CurveEnvelopeTests, ProcessBlockMatchesGetNextValue)
{
    Envelope reference;
    reference.prepare (sampleRate);

    Curve curve = makeAdsr (seconds (77), seconds (130), 0.4f, seconds (91), -0.7f);
    curve.setPoint (1, { seconds (77), 1.0f, 0.0f });

    for (auto* e : { &envelope, &reference })
    {
        e->setCurve (curve);
        e->setTimeScale (1.3f);
        e->noteOn();
    }

    std::vector<float> block (64);

    for (int b = 0; b < 12; ++b)
    {
        if (b == 8)
        {
            envelope.noteOff();
            reference.noteOff();
        }

        envelope.processBlock (block.data(), static_cast<int> (block.size()));

        for (std::size_t i = 0; i < block.size(); ++i)
            ASSERT_NEAR (reference.getNextValue(), block[i], 1e-5f) << b << " " << i;
    }
}

TEST_F (CurveEnvelopeTests, SustainOnFirstPointHoldsImmediately)
{
    Curve curve;
    curve.addPoint (0.0f, 0.4f);
    curve.addPoint (seconds (10), 0.0f);
    curve.setSustainIndex (0);

    envelope.setCurve (curve);
    envelope.noteOn();

    EXPECT_EQ (0.4f, envelope.getNextValue());
    EXPECT_TRUE (envelope.isActive());

    envelope.noteOff();

    for (int i = 0; i < 11; ++i)
        envelope.getNextValue();

    EXPECT_FALSE (envelope.isActive());
}

TEST_F (CurveEnvelopeTests, GetPositionTracksCurveTime)
{
    envelope.setCurve (makeAdsr (seconds (100), seconds (100), 0.5f, seconds (100)));
    envelope.noteOn();

    for (int i = 0; i < 150; ++i)
        envelope.getNextValue();

    EXPECT_NEAR (seconds (150), envelope.getPosition(), 1e-6f);
}
