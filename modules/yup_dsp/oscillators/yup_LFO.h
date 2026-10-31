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
    rising and falling sawtooth, square, a sample-and-hold that draws a new value
    every time the phase wraps, a smooth random spline through those values, or one
    cycle of a caller-owned BreakpointCurve. It is not bandlimited, which is what a
    control signal wants, and nothing here allocates.

    An optional one-pole smoother slews the output, so square, sample-and-hold or
    custom shapes can drive pitch or modulation depth without clicks. A retrigger
    mode decides what noteOn() does: nothing, restart at the start phase, or restart
    and stop after one cycle.

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

    @see WavetableOscillator, BreakpointCurve
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
        square,        /**< +1 for the first half period. */
        sampleAndHold, /**< A pseudo-random value held for one period. */
        sawDown,       /**< Falling, +1 at phase 0. */
        smoothRandom,  /**< A Catmull-Rom spline through pseudo-random values, one per period. */
        custom         /**< One cycle of the curve set with setCustomCurve(), levels mapped to [-1, 1]. */
    };

    /** What noteOn() does to the phase. */
    enum class RetriggerMode
    {
        free,      /**< Keeps running; noteOn() does nothing. */
        retrigger, /**< Restarts at the start phase. */
        oneShot    /**< Restarts at the start phase and holds the end of the first cycle. */
    };

    //==============================================================================
    /** Sets the sample rate and restarts from phase zero. */
    void prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;

        updateIncrement();
        updateSmoothing();
        reset();
    }

    /** Restarts at a phase in periods, reseeds the random generator and settles the smoother. */
    void reset (FloatType newPhase = FloatType (0)) noexcept
    {
        phase = wrap (newPhase);
        finished = false;
        reseed();
        smoothed = getRawValue();
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

    /** Seeds the random generator and draws the first values from it. */
    void setSeed (uint32 newSeed) noexcept
    {
        seed = newSeed;
        reseed();
    }

    /** Sets the curve the custom shape plays, or nullptr for silence.

        Times are read as one normalized cycle, 0 to 1, and levels in [0, 1] are mapped
        to [-1, 1]. The curve is not copied: it must stay alive and must not be modified
        while the LFO reads it.
    */
    void setCustomCurve (const BreakpointCurve<FloatType>* newCurve) noexcept { customCurve = newCurve; }

    /** Returns the curve the custom shape plays. */
    const BreakpointCurve<FloatType>* getCustomCurve() const noexcept { return customCurve; }

    //==============================================================================
    /** Sets the time constant of the output smoother in seconds. Zero or less disables it. */
    void setSmoothing (FloatType seconds) noexcept
    {
        const auto wasSmoothing = smoothingCoefficient > FloatType (0);

        smoothingSeconds = jmax (FloatType (0), seconds);
        updateSmoothing();

        if (! wasSmoothing)
            smoothed = getRawValue();
    }

    /** Returns the smoother time constant in seconds. */
    FloatType getSmoothing() const noexcept { return smoothingSeconds; }

    //==============================================================================
    /** Chooses what noteOn() does. */
    void setRetriggerMode (RetriggerMode newMode) noexcept { retriggerMode = newMode; }

    /** Returns the retrigger mode. */
    RetriggerMode getRetriggerMode() const noexcept { return retriggerMode; }

    /** Sets the phase in periods noteOn() restarts from. */
    void setStartPhase (FloatType periods) noexcept { startPhase = wrap (periods); }

    /** Returns the phase noteOn() restarts from. */
    FloatType getStartPhase() const noexcept { return startPhase; }

    /** Restarts the phase as the retrigger mode says. The random sequence and the
        smoother keep going, so a retriggered random LFO differs from note to note and a
        smoothed one glides into the restart.
    */
    void noteOn() noexcept
    {
        if (retriggerMode == RetriggerMode::free)
            return;

        phase = startPhase;
        finished = false;
    }

    //==============================================================================
    /** Returns the value at the current phase without advancing, smoothed when enabled. */
    FloatType getValue() const noexcept
    {
        return smoothingCoefficient > FloatType (0) ? smoothed : getRawValue();
    }

    /** Returns the waveform at the current phase, ignoring the smoother. */
    FloatType getRawValue() const noexcept
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

            case Shape::sawDown:
                return FloatType (1) - FloatType (2) * p;

            case Shape::smoothRandom:
                return catmullRom (p);

            case Shape::custom:
                return customCurve != nullptr ? FloatType (2) * customCurve->evaluate (p) - FloatType (1) : FloatType (0);
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

    /** Advances by a number of samples and returns the value there.

        The smoother treats the waveform as constant over the skipped samples, at its
        value at the end of the skip.
    */
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

    void updateSmoothing() noexcept
    {
        smoothingCoefficient = smoothingSeconds > FloatType (0)
                                 ? static_cast<FloatType> (std::exp (-1.0 / (static_cast<double> (smoothingSeconds) * sampleRate)))
                                 : FloatType (0);
    }

    /** Advances the phase; a wrap during the step draws one new random value. */
    void advance (int numSamples) noexcept
    {
        if (! finished)
        {
            const auto next = phase + increment * static_cast<FloatType> (numSamples);
            const auto wrapped = std::floor (next);

            if (wrapped > FloatType (0) && retriggerMode == RetriggerMode::oneShot)
            {
                phase = std::nextafter (FloatType (1), FloatType (0));
                finished = true;
            }
            else
            {
                phase = next - wrapped;

                if (wrapped > FloatType (0))
                    drawRandom();
            }
        }

        if (smoothingCoefficient > FloatType (0))
        {
            const auto target = getRawValue();
            smoothed = target + (smoothed - target) * std::pow (smoothingCoefficient, static_cast<FloatType> (numSamples));
        }
    }

    /** Sample-and-hold and smooth random draw from separate streams, so adding the
        spline left the sample-and-hold sequence of a seed unchanged.
    */
    void reseed() noexcept
    {
        state = seed;
        held = nextRandom (state);

        smoothState = seed ^ 0x5bd1e995u;

        for (auto& value : history)
            value = nextRandom (smoothState) * smoothRandomScale;
    }

    void drawRandom() noexcept
    {
        held = nextRandom (state);

        std::rotate (history.begin(), history.begin() + 1, history.end());
        history.back() = nextRandom (smoothState) * smoothRandomScale;
    }

    FloatType catmullRom (FloatType t) const noexcept
    {
        const auto p0 = history[0];
        const auto p1 = history[1];
        const auto p2 = history[2];
        const auto p3 = history[3];

        return FloatType (0.5) * (FloatType (2) * p1 + (p2 - p0) * t + (FloatType (2) * p0 - FloatType (5) * p1 + FloatType (4) * p2 - p3) * t * t + (FloatType (3) * (p1 - p2) + p3 - p0) * t * t * t);
    }

    static FloatType nextRandom (uint32& generator) noexcept
    {
        generator = generator * 1664525u + 1013904223u;

        return static_cast<FloatType> (generator >> 8) / static_cast<FloatType> (8388608.0) - FloatType (1);
    }

    //==============================================================================
    /** Catmull-Rom overshoots by up to 1.25 times its control values, so they are
        drawn in [-0.8, 0.8] to keep the spline in [-1, 1].
    */
    static constexpr FloatType smoothRandomScale = FloatType (0.8);

    double sampleRate = 44100.0;
    const BreakpointCurve<FloatType>* customCurve = nullptr;
    FloatType frequency = FloatType (1);
    FloatType increment = FloatType (1) / FloatType (44100);
    FloatType phase = FloatType (0);
    FloatType phaseOffset = FloatType (0);
    FloatType startPhase = FloatType (0);
    FloatType smoothingSeconds = FloatType (0);
    FloatType smoothingCoefficient = FloatType (0);
    FloatType smoothed = FloatType (0);
    Shape shape = Shape::sine;
    RetriggerMode retriggerMode = RetriggerMode::free;
    uint32 seed = 0x9e3779b9u;
    uint32 state = 0x9e3779b9u;
    uint32 smoothState = 0u;
    FloatType held = FloatType (0);
    std::array<FloatType, 4> history {};
    bool finished = false;
};

} // namespace yup
