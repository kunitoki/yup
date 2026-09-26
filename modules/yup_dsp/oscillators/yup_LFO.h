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

namespace yup
{

//==============================================================================
/**
    A low frequency oscillator for control signals.

    Produces a bipolar waveform in [-1, 1] from a phase accumulator: sine, triangle,
    rising sawtooth, square, or a sample-and-hold that draws a new value every time
    the phase wraps. It is not bandlimited, which is what a control signal wants,
    and nothing here allocates.

    A control-rate consumer calls getValue() for the value at the top of a block and
    skip() to advance by the block length. An audio-rate consumer uses processSample()
    or processBlock().

    @code
    yup::LFO<float> lfo;
    lfo.prepare (48000.0);
    lfo.setShape (yup::LFO<float>::Shape::triangle);
    lfo.setFrequency (2.0f);

    const auto depth = lfo.getValue(); // value at the start of this block
    lfo.skip (numSamples);             // advance to the next block
    @endcode

    @see WavetableOscillator
*/
template <typename FloatType = float>
class LFO
{
public:
    //==============================================================================
    /** The waveform read from the phase. */
    enum class Shape
    {
        sine,
        triangle,     /**< -1 at phase 0, +1 at phase 0.5. */
        sawtooth,     /**< Rising, -1 at phase 0. */
        square,       /**< +1 for the first half period. */
        sampleAndHold /**< A pseudo-random value held for one period. */
    };

    //==============================================================================
    /** Sets the sample rate and restarts from phase zero. */
    void prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;

        updateIncrement();
        reset();
    }

    /** Restarts at a phase in periods and reseeds the sample-and-hold generator. */
    void reset (FloatType newPhase = FloatType (0)) noexcept
    {
        phase = wrap (newPhase);
        state = seed;
        held = nextRandom();
    }

    //==============================================================================
    /** Sets the rate in Hz. Negative rates are clamped to zero, which freezes the phase. */
    void setFrequency (FloatType hz) noexcept
    {
        frequency = jmax (FloatType (0), hz);
        updateIncrement();
    }

    /** Returns the rate in Hz. */
    FloatType getFrequency() const noexcept { return frequency; }

    /** Selects the waveform. */
    void setShape (Shape newShape) noexcept { shape = newShape; }

    /** Returns the waveform. */
    Shape getShape() const noexcept { return shape; }

    /** Shifts where the waveform is read, in periods. Does not move the phase itself. */
    void setPhaseOffset (FloatType periods) noexcept { phaseOffset = wrap (periods); }

    /** Returns the read offset in periods. */
    FloatType getPhaseOffset() const noexcept { return phaseOffset; }

    /** Seeds the sample-and-hold generator and draws the first value from it. */
    void setSeed (uint32 newSeed) noexcept
    {
        seed = newSeed;
        state = seed;
        held = nextRandom();
    }

    //==============================================================================
    /** Returns the value at the current phase without advancing. */
    FloatType getValue() const noexcept
    {
        const auto p = wrap (phase + phaseOffset);

        switch (shape)
        {
            case Shape::sine:
                return std::sin (MathConstants<FloatType>::twoPi * p);

            case Shape::triangle:
                return FloatType (1) - FloatType (4) * std::abs (p - FloatType (0.5));

            case Shape::sawtooth:
                return FloatType (2) * p - FloatType (1);

            case Shape::square:
                return p < FloatType (0.5) ? FloatType (1) : FloatType (-1);

            case Shape::sampleAndHold:
                return held;
        }

        return FloatType (0);
    }

    /** Returns the current value, then advances one sample. */
    FloatType processSample() noexcept
    {
        const auto value = getValue();

        advance (1);

        return value;
    }

    /** Writes consecutive samples. */
    void processBlock (FloatType* output, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
            output[i] = processSample();
    }

    /** Advances by a number of samples and returns the value there. */
    FloatType skip (int numSamples) noexcept
    {
        advance (numSamples);

        return getValue();
    }

    /** Returns the phase in periods, without the offset. */
    FloatType getPhase() const noexcept { return phase; }

private:
    //==============================================================================
    static FloatType wrap (FloatType value) noexcept
    {
        return value - std::floor (value);
    }

    void updateIncrement() noexcept
    {
        increment = static_cast<FloatType> (frequency / sampleRate);
    }

    /** Advances the phase; a wrap during the step draws one new sample-and-hold value. */
    void advance (int numSamples) noexcept
    {
        const auto next = phase + increment * static_cast<FloatType> (numSamples);
        const auto wrapped = std::floor (next);

        phase = next - wrapped;

        if (wrapped > FloatType (0))
            held = nextRandom();
    }

    FloatType nextRandom() noexcept
    {
        state = state * 1664525u + 1013904223u;

        return static_cast<FloatType> (state >> 8) / static_cast<FloatType> (8388608.0) - FloatType (1);
    }

    //==============================================================================
    double sampleRate = 44100.0;
    FloatType frequency = FloatType (1);
    FloatType increment = FloatType (1) / FloatType (44100);
    FloatType phase = FloatType (0);
    FloatType phaseOffset = FloatType (0);
    Shape shape = Shape::sine;
    uint32 seed = 0x9e3779b9u;
    uint32 state = 0x9e3779b9u;
    FloatType held = FloatType (0);
};

} // namespace yup
