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
    A multistage envelope that plays a BreakpointCurve, with times in seconds.

    noteOn() starts from the first point. While the note is held the envelope stops at
    the curve's sustain point, or, when the curve has a loop, jumps from the loop end
    back into the segment after the loop start, starting it from the current level so
    the jump never clicks. noteOff() leaves for the segment after the sustain point
    (or after the loop end when there is no sustain), again starting from the current
    level, so releasing during the attack ramps down from wherever the attack was. A
    curve with neither marker is a one-shot that ignores noteOff().

    Once the last point is reached the envelope is no longer active and holds the
    last level, which makes it usable as a modulation source that ramps and stays.
    An amplitude envelope simply ends on a zero level.

    Each sample costs a multiply for curved segments and a multiply add for straight
    ones; nothing here allocates.

    @code
    yup::BreakpointCurve<float> curve;
    curve.addPoint (0.0f, 0.0f);
    curve.addPoint (0.01f, 1.0f);         // attack
    curve.addPoint (0.3f, 0.6f, -0.5f);   // decay, fast then slow
    curve.addPoint (0.8f, 0.0f, -0.5f);   // release
    curve.setSustainIndex (2);

    yup::CurveEnvelope<float> envelope;
    envelope.prepare (48000.0);
    envelope.setCurve (curve);
    envelope.noteOn();
    envelope.processBlock (gains, numSamples);
    @endcode

    @tparam FloatType  float or double.

    @see BreakpointCurve
*/
template <typename FloatType = float>
class CurveEnvelope
{
public:
    using Curve = BreakpointCurve<FloatType>;

    /** What noteOn() does while the envelope is still active. */
    enum class RetriggerMode
    {
        restart, /**< Jumps to the first point's level and starts over. */
        legato   /**< Starts over from the current level, without a jump. */
    };

    //==============================================================================
    /** Sets the sample rate and silences the envelope. */
    void prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        reset();
    }

    /** Silences the envelope: inactive at a zero level. */
    void reset() noexcept
    {
        stage = Stage::idle;
        level = FloatType (0);
        segment = 0;
        position = 0.0;
        held = false;
    }

    //==============================================================================
    /** Copies the curve to play.

        Taking effect immediately, a new curve keeps the current segment index and its
        progress when that segment still exists and finishes the envelope otherwise. A
        held sustain moves to the new sustain level, so the level can be modulated.
    */
    void setCurve (const Curve& newCurve) noexcept
    {
        curve = newCurve;

        if (stage == Stage::sustaining && segment == curve.getSustainIndex())
            level = curve.getPoint (segment).level;

        if (stage != Stage::running)
            return;

        if (segment >= curve.getNumPoints())
            finish();
        else
            refreshSegment();
    }

    /** Returns the curve being played. */
    const Curve& getCurve() const noexcept { return curve; }

    /** Multiplies every segment duration, keeping the progress through the current one. */
    void setTimeScale (FloatType newTimeScale) noexcept
    {
        const auto scale = jmax (FloatType (0), newTimeScale);

        if (approximatelyEqual (scale, timeScale))
            return;

        timeScale = scale;

        if (stage == Stage::running)
            refreshSegment();
    }

    /** Returns the duration multiplier. */
    FloatType getTimeScale() const noexcept { return timeScale; }

    /** Chooses what noteOn() does to an active envelope. */
    void setRetriggerMode (RetriggerMode newMode) noexcept { retriggerMode = newMode; }

    /** Returns the retrigger mode. */
    RetriggerMode getRetriggerMode() const noexcept { return retriggerMode; }

    //==============================================================================
    /** Starts from the first point, or from the current level in legato mode. */
    void noteOn() noexcept
    {
        held = true;

        if (curve.getNumPoints() == 0)
        {
            stage = Stage::idle;
            level = FloatType (0);
            return;
        }

        if (retriggerMode == RetriggerMode::restart || stage == Stage::idle)
            level = curve.getPoint (0).level;

        segment = 0;
        position = 0.0;

        if (curve.getSustainIndex() == 0)
        {
            stage = Stage::sustaining;
            level = curve.getPoint (0).level;
            return;
        }

        enterSegment (1);
    }

    /** Leaves for the release segment from the current level. */
    void noteOff() noexcept
    {
        held = false;

        if (stage == Stage::idle || stage == Stage::finished)
            return;

        const auto releasePoint = curve.getSustainIndex() >= 0 ? curve.getSustainIndex() : curve.getLoopEnd();

        if (releasePoint < 0 || (stage == Stage::running && segment > releasePoint))
            return;

        position = 0.0;
        enterSegment (releasePoint + 1);
    }

    /** Returns true until the envelope has played its last point. */
    bool isActive() const noexcept { return stage == Stage::running || stage == Stage::sustaining; }

    /** Returns the level produced last. */
    FloatType getLevel() const noexcept { return level; }

    /** Returns the time along the curve, in seconds of the unscaled curve. */
    FloatType getPosition() const noexcept
    {
        if (stage != Stage::running)
            return segment > 0 && segment < curve.getNumPoints() ? curve.getPoint (segment).time : FloatType (0);

        const auto start = curve.getPoint (segment - 1).time;
        const auto end = curve.getPoint (segment).time;
        const auto t = segmentLength > 0.0 ? position / segmentLength : 1.0;

        return start + static_cast<FloatType> (t) * (end - start);
    }

    /** Returns where the envelope is along the points of its curve: k on point k, and
        k + f a fraction f of the time from point k to point k + 1. A held sustain reads
        exactly its point.

        Unlike getPosition() it does not depend on the points' times, so a display can place
        it on its own copy of a curve that is being edited.
    */
    FloatType getPointPosition() const noexcept
    {
        if (segment <= 0)
            return FloatType (0);

        if (stage != Stage::running)
            return static_cast<FloatType> (jmin (segment, curve.getNumPoints() - 1));

        const auto t = segmentLength > 0.0 ? jlimit (0.0, 1.0, position / segmentLength) : 1.0;
        return static_cast<FloatType> (segment - 1) + static_cast<FloatType> (t);
    }

    //==============================================================================
    /** Advances one sample and returns the new level. */
    FloatType getNextValue() noexcept
    {
        if (stage != Stage::running)
            return level;

        position += 1.0;
        exponential *= exponentialStep;

        if (position >= segmentLength && ! completeSegments())
            return level;

        level = valueAtPosition();
        return level;
    }

    /** Writes consecutive levels. Straight segments are written as a vectorizable ramp. */
    void processBlock (FloatType* output, int numSamples) noexcept
    {
        int index = 0;

        while (index < numSamples)
        {
            if (stage != Stage::running)
            {
                FloatVectorOperations::fill (output + index, level, numSamples - index);
                return;
            }

            const auto inside = static_cast<int> (std::ceil (segmentLength - position)) - 1;
            const auto count = jlimit (0, numSamples - index, inside);

            if (count > 0)
            {
                auto* out = output + index;
                const auto delta = segmentEnd - segmentStart;

                if (isLinear)
                {
                    const auto start = position;
                    const auto inverseLength = 1.0 / segmentLength;

                    for (int i = 0; i < count; ++i)
                        out[i] = segmentStart + delta * static_cast<FloatType> ((start + static_cast<double> (i + 1)) * inverseLength);
                }
                else
                {
                    for (int i = 0; i < count; ++i)
                    {
                        exponential *= exponentialStep;
                        out[i] = segmentStart + delta * static_cast<FloatType> ((exponential - 1.0) * inverseDenominator);
                    }
                }

                position += static_cast<double> (count);
                level = out[count - 1];
                index += count;
            }

            if (index < numSamples)
                output[index++] = getNextValue();
        }
    }

private:
    //==============================================================================
    enum class Stage
    {
        idle,
        running,
        sustaining,
        finished
    };

    /** Starts the segment ending at a point, finishing or holding as the markers say. */
    void enterSegment (int endPoint) noexcept
    {
        if (endPoint >= curve.getNumPoints())
        {
            finish();
            return;
        }

        segment = endPoint;
        stage = Stage::running;
        beginSegment (level, position);
    }

    /** Sets up the current segment from a start level, at a position in samples. */
    void beginSegment (FloatType startLevel, double startPosition) noexcept
    {
        const auto& from = curve.getPoint (segment - 1);
        const auto& to = curve.getPoint (segment);

        segmentStart = startLevel;
        segmentEnd = to.level;
        segmentLength = static_cast<double> (to.time - from.time) * static_cast<double> (timeScale) * sampleRate;
        position = startPosition;

        updateShape();
    }

    /** Re-reads the current segment after the curve or the time scale changed, keeping
        the fraction of it already played.
    */
    void refreshSegment() noexcept
    {
        const auto fraction = segmentLength > 0.0 ? position / segmentLength : 0.0;

        beginSegment (segmentStart, 0.0);
        position = fraction * segmentLength;
        updateShape();
    }

    /** Sets up the exponential recurrence for the current segment and position. */
    void updateShape() noexcept
    {
        const auto kc = static_cast<double> (Curve::curveSteepness * curve.getPoint (segment).curve);
        isLinear = std::abs (kc) < 1e-6 || segmentLength <= 0.0;

        if (isLinear)
        {
            exponential = 1.0;
            exponentialStep = 1.0;
            return;
        }

        exponentialStep = std::exp (kc / segmentLength);
        exponential = std::exp (kc * position / segmentLength);
        inverseDenominator = 1.0 / std::expm1 (kc);
    }

    /** Finishes every segment the position has run past. Returns false when the
        envelope stopped running on the way.
    */
    bool completeSegments() noexcept
    {
        for (int guard = 0; position >= segmentLength; ++guard)
        {
            position -= segmentLength;
            level = segmentEnd;

            // A loop shorter than a sample would spin forever, so it holds instead.
            if (guard > Curve::maxPoints)
            {
                stage = Stage::sustaining;
                return false;
            }

            if (held && curve.hasLoop() && segment == curve.getLoopEnd())
            {
                segment = curve.getLoopStart() + 1;
                beginSegment (level, position);
                continue;
            }

            if (held && segment == curve.getSustainIndex())
            {
                stage = Stage::sustaining;
                return false;
            }

            enterSegment (segment + 1);

            if (stage != Stage::running)
                return false;
        }

        return true;
    }

    FloatType valueAtPosition() const noexcept
    {
        const auto t = isLinear ? position / segmentLength : (exponential - 1.0) * inverseDenominator;
        return segmentStart + (segmentEnd - segmentStart) * static_cast<FloatType> (t);
    }

    void finish() noexcept
    {
        stage = Stage::finished;
        segment = curve.getNumPoints() - 1;

        if (curve.getNumPoints() > 0)
            level = curve.getPoint (segment).level;
    }

    //==============================================================================
    Curve curve;
    double sampleRate = 44100.0;
    double position = 0.0;
    double segmentLength = 0.0;
    double exponential = 1.0;
    double exponentialStep = 1.0;
    double inverseDenominator = 1.0;
    FloatType level = FloatType (0);
    FloatType segmentStart = FloatType (0);
    FloatType segmentEnd = FloatType (0);
    FloatType timeScale = FloatType (1);
    Stage stage = Stage::idle;
    RetriggerMode retriggerMode = RetriggerMode::restart;
    int segment = 0;
    bool held = false;
    bool isLinear = true;
};

} // namespace yup
