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

#pragma once

#include "SynthEnvelopeParameters.h"

//==============================================================================
/** A delay, attack, hold, decay, sustain and release envelope with linear segments.

    yup_dsp has no envelope generator, so the example carries its own.

    The stage is advanced one sample at a time and the note ends when the envelope
    falls idle, so a voice stays allocated for exactly as long as it is audible.

    @see SynthVoice
*/
class SynthEnvelope
{
public:
    /** Prepares the envelope and leaves it idle. */
    void prepare (double newSampleRate) noexcept;

    /** Silences the envelope and returns it to the idle stage. */
    void reset() noexcept;

    /** Converts the control values into per sample increments. Safe to call every block. */
    void setParameters (const SynthEnvelopeValues& values) noexcept;

    /** Starts a new note from the delay stage. */
    void noteOn() noexcept;

    /** Begins the release from whatever level the envelope currently sits at. */
    void noteOff() noexcept;

    /** Silences the envelope immediately, ending the note without a tail. */
    void noteOffImmediate() noexcept { reset(); }

    /** Returns true while the envelope still contributes to the output. */
    bool isActive() const noexcept { return stage != Stage::idle; }

    /** Returns the gain the envelope last produced, for use as a modulation source. */
    float getLevel() const noexcept { return level; }

    /** Advances one sample and returns the new gain. */
    float getNextValue() noexcept
    {
        switch (stage)
        {
            case Stage::idle:
                return 0.0f;

            case Stage::delay:
                if (++stageSample >= delaySamples)
                    advanceTo (Stage::attack);

                return 0.0f;

            case Stage::attack:
                level += attackIncrement;

                if (level >= 1.0f)
                {
                    level = 1.0f;
                    advanceTo (Stage::hold);
                }

                return level;

            case Stage::hold:
                if (++stageSample >= holdSamples)
                    advanceTo (Stage::decay);

                return level;

            case Stage::decay:
                level -= decayIncrement;

                if (level <= sustainLevel || decayIncrement <= 0.0f)
                {
                    level = sustainLevel;
                    advanceTo (Stage::sustain);
                }

                return level;

            case Stage::sustain:
                level = sustainLevel;

                return level;

            case Stage::release:
                level -= releaseIncrement;

                if (level <= 0.0f || releaseIncrement <= 0.0f)
                {
                    level = 0.0f;
                    advanceTo (Stage::idle);
                }

                return level;
        }

        return level;
    }

private:
    //==============================================================================
    enum class Stage
    {
        idle,
        delay,
        attack,
        hold,
        decay,
        sustain,
        release
    };

    void advanceTo (Stage newStage) noexcept
    {
        stage = newStage;
        stageSample = 0;
    }

    int toSamples (float seconds) const noexcept;

    //==============================================================================
    Stage stage = Stage::idle;

    double sampleRate = 44100.0;
    float level = 0.0f;
    float sustainLevel = 0.7f;
    float attackIncrement = 1.0f;
    float decayIncrement = 1.0f;
    float releaseIncrement = 1.0f;

    int delaySamples = 1;
    int holdSamples = 1;
    int releaseSamples = 1;
    int stageSample = 0;
};
