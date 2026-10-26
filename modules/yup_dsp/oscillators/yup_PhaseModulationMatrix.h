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
    Phase modulation between wavetable operators through a full modulation matrix.

    Each operator reads a caller-owned WavetableOscillator at its own phase, plus a
    fixed phase offset, plus a phase modulation made of the operators' outputs, its
    own included. Per internal sample:

        pm_i  = sum_j (M_ij * (y_j[n-1] + y_j[n-2]) / 2)
        y_i   = table_i (phase_i + offset_i + pm_i)
        out_i = gain_i * y_i

    Every link is delayed, so the evaluation order never matters and cycles are well
    defined, and averaged over two samples as the DX7 does for its feedback, which
    keeps feedback and cycles from hunting. The average is a half-sample delay and a
    gentle lowpass on the modulators, under 0.5 dB at the top of the audio band at 4x.

    Off-diagonal amounts are in periods of phase per unit of the modulator's output:
    a sine modulator at amount M gives a modulation index of 2 pi M radians, and the
    TX81Z's full scale is 4 periods. Cycles between operators are feedback loops too:
    two sines modulating each other at the same pitch stay clean up to about 0.3
    periods each way, and turn noisy beyond.

    The diagonal is the feedback amount in [0, 1], scaled by the operator's own
    table so that 1 always means a loop gain of maximumFeedbackLoopGain: M_ii is the
    amount times maximumFeedbackLoopGain divided by the table's peak slope. That is
    the largest feedback that stays clear of the chaotic regime, whatever the table
    holds, so a sine gets the full DX7 range and a bright table a subtler one.

    Frequencies, amounts, gains and phase offsets are ramped linearly over each control
    step, so a modulation index never zippers. Once per step the matrix also chooses every
    operator's table bandwidth: the table frequency is set to

        max (factor * |f_i|, |f_i| + sum_j |M_ij| |f_j| peakSlope_j)

    which bounds the fastest the read phase can move, taking the larger end of each
    ramp. The second term is rounded up to quarter octaves, so a moving modulation
    depth only re-renders a table when it crosses a step. The first term caps the
    harmonics at the base rate's Nyquist, so an unmodulated operator renders exactly the
    table it would render without oversampling. In this mode a table's frequency is its
    bandwidth, not its pitch.

    Oversampling, decimation and voice stacking are left to the caller. An operator
    whose output gain and outgoing amounts are all zero is skipped: it only advances
    its phase and its table's crossfade.

    Per control step the caller:
    1. calls setTargets();
    2. renders each active table whose needsRender() is true (render() or renderFrom());
    3. calls process() once or more for the step's samples.

    Peak slopes are read in setTargets(), so a table rendered for the first time only
    counts from the next step. When starting a note, run steps 1 and 2 twice with a
    zero length step. Feedback is scaled at the first process() after setTargets(),
    from the freshly rendered tables.

    Nothing here allocates or locks.

    @tparam SampleType    float or double, the tables' and outputs' sample type.
    @tparam NumOperators  Number of operators.

    @see WavetableOscillator, HalfbandOversampler
*/
template <typename SampleType, int NumOperators = 4>
class PhaseModulationMatrix
{
public:
    static_assert (NumOperators >= 1, "A phase modulation matrix needs at least one operator");

    //==============================================================================
    /** The table type the operators read. */
    using Table = WavetableOscillator<SampleType>;

    /** Number of operators. */
    static constexpr int numOperators = NumOperators;

    /** The loop gain a feedback amount of 1 maps to, just below the chaotic onset of
        two-sample averaged feedback (about 2.15).
    */
    static constexpr double maximumFeedbackLoopGain = 2.0;

    /** Targets for the end of a control step. */
    struct Controls
    {
        /** Operator frequencies in Hz. Negative frequencies run backward. */
        std::array<double, NumOperators> frequencies {};

        /** amounts[i][j] is the modulation of operator i by operator j, in periods per
            unit of output. The diagonal is the feedback amount in [0, 1].
        */
        std::array<std::array<double, NumOperators>, NumOperators> amounts {};

        /** Output gains applied to each operator's output. */
        std::array<double, NumOperators> gains {};

        /** Phase offsets in periods, added to every read of the operator's table. A ramp
            briefly shifts the operator's frequency, which the table bandwidth ignores.
        */
        std::array<double, NumOperators> phaseOffsets {};
    };

    //==============================================================================
    /** Default constructor. Call prepare() and setTable() before processing. */
    PhaseModulationMatrix() = default;

    /** Sets the rates.

        @param baseSampleRate      The output rate in Hz, after decimation.
        @param oversamplingFactor  The internal rate as a multiple of the base rate. The
                                   tables must be prepared at the internal rate.
    */
    void prepare (double baseSampleRate, int oversamplingFactor) noexcept
    {
        jassert (baseSampleRate > 0.0 && oversamplingFactor >= 1);

        oversampling = jmax (1, oversamplingFactor);
        internalSampleRate = (baseSampleRate > 0.0 ? baseSampleRate : 44100.0) * oversampling;

        reset();
    }

    /** Attaches the table an operator reads, or nullptr to silence it. Not owned. */
    void setTable (int index, Table* table) noexcept
    {
        if (isPositiveAndBelow (index, NumOperators))
            tables[static_cast<std::size_t> (index)] = table;
    }

    /** Restarts the phases, clears the feedback history and makes the next
        setTargets() jump to its targets instead of ramping. Targets already set keep
        playing, without a ramp, until then.

        @param initialPhases  Phases in periods, wrapped.
    */
    void reset (const std::array<double, NumOperators>& initialPhases = {}) noexcept
    {
        for (int i = 0; i < NumOperators; ++i)
        {
            const auto index = static_cast<std::size_t> (i);
            phases[index] = initialPhases[index] - std::floor (initialPhases[index]);
            history1[index] = 0.0;
            history2[index] = 0.0;
        }

        hasState = false;
        rampRemaining = 0;
    }

    //==============================================================================
    /** Sets the values to reach at the end of the next numInternalSamples samples.

        Ramps from the current values, or jumps to the targets after reset() or when
        numInternalSamples is not positive. Also decides which operators are active
        and sets every active table's bandwidth frequency.
    */
    void setTargets (const Controls& targets, int numInternalSamples) noexcept
    {
        for (int i = 0; i < NumOperators; ++i)
        {
            const auto ii = static_cast<std::size_t> (i);

            targetIncrements[ii] = targets.frequencies[ii] / internalSampleRate;
            targetGains[ii] = targets.gains[ii];
            targetOffsets[ii] = targets.phaseOffsets[ii];
            previousFeedback[ii] = hasState ? targetFeedback[ii] : jlimit (0.0, 1.0, targets.amounts[ii][ii]);
            targetFeedback[ii] = jlimit (0.0, 1.0, targets.amounts[ii][ii]);

            for (int j = 0; j < NumOperators; ++j)
                if (i != j)
                    targetAmounts[ii][static_cast<std::size_t> (j)] = targets.amounts[ii][static_cast<std::size_t> (j)];
        }

        if (! hasState || numInternalSamples <= 0 || ! targetsDiffer())
        {
            snapToTargets();
            rampRemaining = 0;
        }
        else
        {
            rampRemaining = numInternalSamples;
            const auto inverse = 1.0 / static_cast<double> (numInternalSamples);

            for (std::size_t i = 0; i < static_cast<std::size_t> (NumOperators); ++i)
            {
                incrementSteps[i] = (targetIncrements[i] - increments[i]) * inverse;
                gainSteps[i] = (targetGains[i] - gains[i]) * inverse;
                offsetSteps[i] = (targetOffsets[i] - offsets[i]) * inverse;

                for (std::size_t j = 0; j < static_cast<std::size_t> (NumOperators); ++j)
                    amountSteps[i][j] = i != j ? (targetAmounts[i][j] - amounts[i][j]) * inverse : 0.0;
            }
        }

        hasState = true;
        feedbackPending = true;

        updateActivity();
        updateBandwidths();
    }

    /** Produces the operator outputs for numInternalSamples samples.

        @param operatorOutputs  One buffer per operator, already scaled by the operator's
                                gain. A null pointer skips writing that operator, which
                                is still computed when it modulates others. Skipped
                                operators write zeros.
    */
    void process (SampleType* const* operatorOutputs, int numInternalSamples) noexcept
    {
        if (operatorOutputs == nullptr || numInternalSamples <= 0)
            return;

        if (feedbackPending)
            updateFeedback();

        int offset = 0;

        while (offset < numInternalSamples)
        {
            if (rampRemaining > 0)
            {
                const auto count = jmin (rampRemaining, numInternalSamples - offset);
                run<true> (operatorOutputs, offset, count);

                rampRemaining -= count;
                offset += count;

                if (rampRemaining == 0)
                    snapToTargets();
            }
            else
            {
                run<false> (operatorOutputs, offset, numInternalSamples - offset);
                offset = numInternalSamples;
            }
        }

        for (int i = 0; i < NumOperators; ++i)
        {
            const auto ii = static_cast<std::size_t> (i);

            if (active[ii])
                continue;

            if (tables[ii] != nullptr)
                tables[ii]->advanceCrossfade (numInternalSamples);

            if (operatorOutputs[i] != nullptr)
                std::fill (operatorOutputs[i], operatorOutputs[i] + numInternalSamples, SampleType (0));
        }
    }

    //==============================================================================
    /** Returns true when the operator is computed this step: it has a table and a
        non-zero gain or outgoing amount at either end of the ramp.
    */
    bool isOperatorActive (int index) const noexcept
    {
        return isPositiveAndBelow (index, NumOperators) && active[static_cast<std::size_t> (index)];
    }

    /** Returns true when the operator is active and its output gain is non-zero at
        either end of the ramp, so its output is worth mixing.
    */
    bool isOperatorAudible (int index) const noexcept
    {
        return isOperatorActive (index) && audible[static_cast<std::size_t> (index)];
    }

    /** Returns an operator's unmodulated phase in periods, without its phase offset. */
    double getPhase (int index) const noexcept
    {
        return isPositiveAndBelow (index, NumOperators) ? phases[static_cast<std::size_t> (index)] : 0.0;
    }

    /** Returns the rate process() runs at, in Hz. */
    double getInternalSampleRate() const noexcept { return internalSampleRate; }

    /** Returns the internal rate as a multiple of the base rate. */
    int getOversamplingFactor() const noexcept { return oversampling; }

private:
    //==============================================================================
    using Row = std::array<double, NumOperators>;
    using Square = std::array<Row, NumOperators>;

    static constexpr auto size = static_cast<std::size_t> (NumOperators);

    template <bool Ramping>
    void run (SampleType* const* outputs, int offset, int numSamples) noexcept
    {
        // The table reads write through pointers, so the state is copied to locals they
        // cannot alias, and nothing has to be reloaded after every read.
        const auto tableList = tables;
        const auto isActive = active;
        const auto links = modulators;
        const auto linkCounts = modulatorCounts;
        auto phase = phases;
        auto increment = increments;
        auto gain = gains;
        auto phaseOffset = offsets;
        auto amount = amounts;
        auto past1 = history1;
        auto past2 = history2;
        std::array<SampleType*, NumOperators> destinations {};
        Row averages {};
        Row values {};

        for (std::size_t i = 0; i < size; ++i)
            destinations[i] = outputs[i] != nullptr ? outputs[i] + offset : nullptr;

        for (int s = 0; s < numSamples; ++s)
        {
            for (std::size_t j = 0; j < size; ++j)
                averages[j] = 0.5 * (past1[j] + past2[j]);

            for (std::size_t i = 0; i < size; ++i)
            {
                if (! isActive[i])
                    continue;

                auto pm = 0.0;

                for (int k = 0; k < linkCounts[i]; ++k)
                {
                    const auto j = links[i][static_cast<std::size_t> (k)];
                    pm += amount[i][j] * averages[j];
                }

                values[i] = static_cast<double> (tableList[i]->processSampleAtPhase (phase[i] + phaseOffset[i] + pm));
            }

            for (std::size_t i = 0; i < size; ++i)
            {
                if (isActive[i])
                {
                    past2[i] = past1[i];
                    past1[i] = values[i];

                    if (auto* output = destinations[i])
                        output[s] = static_cast<SampleType> (gain[i] * values[i]);
                }

                phase[i] += increment[i];
                phase[i] -= std::floor (phase[i]);

                if constexpr (Ramping)
                {
                    increment[i] += incrementSteps[i];
                    gain[i] += gainSteps[i];
                    phaseOffset[i] += offsetSteps[i];

                    for (int k = 0; k < linkCounts[i]; ++k)
                    {
                        const auto j = links[i][static_cast<std::size_t> (k)];
                        amount[i][j] += amountSteps[i][j];
                    }
                }
            }
        }

        phases = phase;
        history1 = past1;
        history2 = past2;

        if constexpr (Ramping)
        {
            increments = increment;
            gains = gain;
            offsets = phaseOffset;
            amounts = amount;
        }
    }

    /** True when the new targets need a ramp. Feedback always ramps, because its scale
        follows the table's peak slope, which a render can move.
    */
    bool targetsDiffer() const noexcept
    {
        for (std::size_t i = 0; i < size; ++i)
        {
            if (targetIncrements[i] != increments[i] || targetGains[i] != gains[i] || targetOffsets[i] != offsets[i]
                || targetFeedback[i] > 0.0 || previousFeedback[i] > 0.0)
                return true;

            for (std::size_t j = 0; j < size; ++j)
                if (i != j && targetAmounts[i][j] != amounts[i][j])
                    return true;
        }

        return false;
    }

    void snapToTargets() noexcept
    {
        for (std::size_t i = 0; i < size; ++i)
        {
            increments[i] = targetIncrements[i];
            gains[i] = targetGains[i];
            offsets[i] = targetOffsets[i];

            for (std::size_t j = 0; j < size; ++j)
                if (i != j)
                    amounts[i][j] = targetAmounts[i][j];

            if (! feedbackPending)
                amounts[i][i] = targetAmounts[i][i];
        }
    }

    /** Scales the feedback targets by the tables' current peak slopes. */
    void updateFeedback() noexcept
    {
        feedbackPending = false;

        for (std::size_t i = 0; i < size; ++i)
        {
            // The peak slope is only measured for operators that feed back.
            const auto slope = tables[i] != nullptr && targetFeedback[i] > 0.0 ? static_cast<double> (tables[i]->getPeakSlope()) : 0.0;
            targetAmounts[i][i] = slope > 0.0 ? targetFeedback[i] * maximumFeedbackLoopGain / slope : 0.0;

            if (rampRemaining > 0)
                amountSteps[i][i] = (targetAmounts[i][i] - amounts[i][i]) / static_cast<double> (rampRemaining);
            else
                amounts[i][i] = targetAmounts[i][i];
        }

        updateModulators();
    }

    /** Lists each operator's links that are non-zero at either end of the ramp. The
        others stay exactly zero all step, so they are neither summed nor ramped.
    */
    void updateModulators() noexcept
    {
        for (std::size_t i = 0; i < size; ++i)
        {
            auto count = 0;

            for (std::size_t j = 0; j < size; ++j)
                if (amounts[i][j] != 0.0 || targetAmounts[i][j] != 0.0)
                    modulators[i][static_cast<std::size_t> (count++)] = j;

            modulatorCounts[i] = count;
        }
    }

    void updateActivity() noexcept
    {
        for (std::size_t j = 0; j < size; ++j)
        {
            bool modulatesOthers = false;

            for (std::size_t i = 0; i < size; ++i)
                if (i != j && (amounts[i][j] != 0.0 || targetAmounts[i][j] != 0.0))
                    modulatesOthers = true;

            audible[j] = gains[j] != 0.0 || targetGains[j] != 0.0;
            active[j] = tables[j] != nullptr && (audible[j] || modulatesOthers);

            if (! active[j])
            {
                history1[j] = 0.0;
                history2[j] = 0.0;
            }
        }
    }

    void updateBandwidths() noexcept
    {
        Row speeds {};

        for (std::size_t i = 0; i < size; ++i)
            speeds[i] = jmax (std::abs (increments[i]), std::abs (targetIncrements[i])) * internalSampleRate;

        for (std::size_t i = 0; i < size; ++i)
        {
            if (! active[i])
                continue;

            auto deviation = jmax (previousFeedback[i], targetFeedback[i]) * maximumFeedbackLoopGain * speeds[i];

            for (std::size_t j = 0; j < size; ++j)
            {
                if (j == i || ! active[j])
                    continue;

                const auto depth = jmax (std::abs (amounts[i][j]), std::abs (targetAmounts[i][j]));
                deviation += depth * speeds[j] * static_cast<double> (tables[j]->getPeakSlope());
            }

            // A moving modulation depth would change the harmonic limit, and so cost a
            // render, on every step: the modulated bandwidth is rounded up to quarter
            // octaves, which only ever removes harmonics. The unmodulated term is exact.
            auto modulated = speeds[i] + deviation;

            if (deviation > 0.0 && modulated > 0.0)
                modulated = std::exp2 (std::ceil (std::log2 (modulated) * 4.0) * 0.25);

            const auto bandwidth = jmax (oversampling * speeds[i], modulated);
            tables[i]->setFrequency (jmin (bandwidth, 0.49 * internalSampleRate));
        }
    }

    //==============================================================================
    std::array<Table*, NumOperators> tables {};
    Row phases {};
    Row increments {};
    Row incrementSteps {};
    Row targetIncrements {};
    Row gains {};
    Row gainSteps {};
    Row targetGains {};
    Row offsets {};
    Row offsetSteps {};
    Row targetOffsets {};
    Row targetFeedback {};
    Row previousFeedback {};
    Row history1 {};
    Row history2 {};
    Square amounts {};
    Square amountSteps {};
    Square targetAmounts {};
    std::array<std::array<std::size_t, NumOperators>, NumOperators> modulators {};
    std::array<int, NumOperators> modulatorCounts {};
    std::array<bool, NumOperators> active {};
    std::array<bool, NumOperators> audible {};
    double internalSampleRate = 44100.0;
    int oversampling = 1;
    int rampRemaining = 0;
    bool hasState = false;
    bool feedbackPending = false;
};

} // namespace yup
