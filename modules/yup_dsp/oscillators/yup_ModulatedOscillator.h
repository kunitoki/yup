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
/** Controls for one internal sample interval of a modulated oscillator voice.

    All fields must be finite. Shared by ModulatedOscillator and by any oscillator
    composed over detail::ModulatedOscillatorVoice.

    @see ModulatedOscillator, PrismOscillator
*/
struct ModulatedOscillatorParameters
{
    double frequency = 440.0;        /**< Signed carrier frequency in Hz. */
    double linearFM = 0.0;           /**< Signed frequency deviation in Hz. */
    double exponentialFM = 0.0;      /**< Pitch offset in octaves, clamped to +/-16. */
    double phaseModulation = 0.0;    /**< Read-phase offset in periods; does not reset phase. */
    double morph = 0.0;              /**< Normalized waveform-bank position, clamped to [0, 1]. */
    double phaseDistortion = 0.5;    /**< Phase-map breakpoint, clamped to [0.01, 0.99]. */
    double syncFrequency = 0.0;      /**< Leader frequency in Hz; zero disables hard sync. */
    double bandwidthFrequency = 0.0; /**< Optional lower bound on the frequency used for waveform bandwidth selection, in Hz. */
};

namespace detail
{

//==============================================================================
/** One unoversampled modulated oscillator voice.

    Implements the modulation maths ModulatedOscillator exposes: the signed phase
    accumulator, the two-segment phase map, fractional hard sync and the polynomial
    BLEP/BLAMP residuals that correct value and slope jumps. It runs at the caller's
    internal rate and knows nothing about oversampling or block sizes.

    Nothing here allocates, including prepare(); the owning class is responsible for
    the decimator storage and for the bank's lifetime.

    @tparam SampleType  Output precision.
    @tparam CoeffType   Waveform coefficient precision.

    @see ModulatedOscillator, PrismOscillator
*/
template <typename SampleType, typename CoeffType = double>
class ModulatedOscillatorVoice
{
public:
    using Parameters = ModulatedOscillatorParameters;

    //==============================================================================
    /** Attaches a prepared waveform bank and sets the rate processSample() runs at.

        @param newInternalSampleRate  Rate the voice is clocked at in Hz, positive.
        @param waveforms              Immutable bank; must remain alive throughout playback.
    */
    void prepare (double newInternalSampleRate, const WaveformBank<SampleType, CoeffType>& waveforms) noexcept
    {
        jassert (newInternalSampleRate > 0.0);

        internalSampleRate = jmax (1.0, newInternalSampleRate);
        bank = &waveforms;

        reset();
    }

    /** Resets phase, sync leader and pending residuals.

        @param initialPhase  Initial follower phase in periods, wrapped internally.
    */
    void reset (double initialPhase = 0.0) noexcept
    {
        phase = wrap (initialPhase);
        leaderPhase = 0.0;
        nextCorrection = 0.0;
        previousPhaseModulation = 0.0;
        hasPreviousParameters = false;
    }

    /** Swaps in another prepared bank without allocating or touching phase. */
    void setBank (const WaveformBank<SampleType, CoeffType>& waveforms) noexcept { bank = &waveforms; }

    /** Returns true once a bank has been attached. */
    bool isPrepared() const noexcept { return bank != nullptr; }

    /** Returns the unmodulated follower accumulator in periods. */
    double getPhase() const noexcept { return phase; }

    /** Returns the rate processSample() is expected to be called at. */
    double getInternalSampleRate() const noexcept { return internalSampleRate; }

    //==============================================================================
    /** Advances the voice by one internal sample and returns its corrected value.

        The controls describe the interval that follows; event interpolation assumes
        they stay constant over it. A bank must have been attached first.
    */
    double processSample (Parameters controls) noexcept
    {
        controls.morph = jlimit (0.0, 1.0, controls.morph);
        controls.phaseDistortion = jlimit (0.01, 0.99, controls.phaseDistortion);
        const auto pitchScale = controls.exponentialFM == 0.0 ? 1.0 : std::exp2 (jlimit (-16.0, 16.0, controls.exponentialFM));
        const auto frequency = (controls.frequency + controls.linearFM) * pitchScale;
        const auto increment = jlimit (-0.5, 0.5, frequency / internalSampleRate);
        const auto leaderIncrement = jlimit (0.0, 0.5, controls.syncFrequency / internalSampleRate);
        const auto phaseModulationSpeed = hasPreviousParameters ? (controls.phaseModulation - previousPhaseModulation) * internalSampleRate : 0.0;
        const auto maximumWarpSlope = 0.5 / jmin (controls.phaseDistortion, 1.0 - controls.phaseDistortion);
        const auto carrierSpeed = jmax (std::abs (increment) * internalSampleRate,
                                        jmax (0.0, controls.bandwidthFrequency));
        const auto phaseSpeed = (carrierSpeed + std::abs (phaseModulationSpeed)) * maximumWarpSlope;
        const auto bandwidth = phaseSpeed > 0.0 ? 0.45 * internalSampleRate / phaseSpeed
                                               : 2.0 * bank->getNumHarmonics();
        previousPhaseModulation = controls.phaseModulation;
        hasPreviousParameters = true;

        auto current = value (phase, controls, bandwidth) + nextCorrection;
        nextCorrection = 0.0;

        if (leaderIncrement > 0.0 && leaderPhase + leaderIncrement >= 1.0)
        {
            const auto fraction = (1.0 - leaderPhase) / leaderIncrement;
            const auto beforeReset = phase + increment * fraction;
            correctCorners (phase, increment, fraction, 0.0, controls, bandwidth, current);
            const auto jump = value (0.0, controls, bandwidth) - value (beforeReset, controls, bandwidth);
            const auto slopeJump = (slope (0.0, increment, controls, bandwidth) - slope (beforeReset, increment, controls, bandwidth)) * increment;
            correctEvent (fraction, jump, slopeJump, current);
            correctCorners (0.0, increment, 1.0 - fraction, fraction, controls, bandwidth, current);
            phase = wrap (increment * (1.0 - fraction));
        }
        else
        {
            correctCorners (phase, increment, 1.0, 0.0, controls, bandwidth, current);
            phase = wrap (phase + increment);
        }

        leaderPhase = wrap (leaderPhase + leaderIncrement);
        return current;
    }

private:
    //==============================================================================
    static double wrap (double value) noexcept { return value - std::floor (value); }

    static double warp (double phase, double breakpoint) noexcept
    {
        const auto p = wrap (phase);
        return p < breakpoint ? 0.5 * p / breakpoint
                              : 0.5 + 0.5 * (p - breakpoint) / (1.0 - breakpoint);
    }

    static double warpSlope (double phase, double breakpoint) noexcept
    {
        return wrap (phase) < breakpoint ? 0.5 / breakpoint : 0.5 / (1.0 - breakpoint);
    }

    double value (double p, const Parameters& controls, double bandwidth) const noexcept
    {
        return bank->getValue (warp (p + controls.phaseModulation, controls.phaseDistortion),
                               static_cast<CoeffType> (controls.morph), bandwidth);
    }

    double slope (double p, double increment, const Parameters& controls, double bandwidth) const noexcept
    {
        const auto position = p + controls.phaseModulation;
        auto derivative = warpSlope (position, controls.phaseDistortion);
        if (increment < 0.0)
        {
            if (wrap (position) == 0.0)
                derivative = 0.5 / (1.0 - controls.phaseDistortion);
            else if (wrap (position) == controls.phaseDistortion)
                derivative = 0.5 / controls.phaseDistortion;
        }
        return bank->getSlope (warp (position, controls.phaseDistortion),
                               static_cast<CoeffType> (controls.morph), bandwidth)
             * derivative;
    }

    void correctEvent (double fraction, double jump, double slopeJump, double& current) noexcept
    {
        const auto before = 1.0 - fraction;
        current += 0.5 * jump * before * before + slopeJump * before * before * before / 6.0;
        nextCorrection += -0.5 * jump * fraction * fraction + slopeJump * fraction * fraction * fraction / 6.0;
    }

    void correctCorners (double start, double increment, double duration, double offset,
                         const Parameters& controls, double bandwidth, double& current) noexcept
    {
        if (increment == 0.0 || duration <= 0.0 || controls.phaseDistortion == 0.5)
            return;

        const auto position = start + controls.phaseModulation;
        const auto end = position + increment * duration;
        const auto left = 0.5 / controls.phaseDistortion;
        const auto right = 0.5 / (1.0 - controls.phaseDistortion);

        for (const auto corner : { 0.0, controls.phaseDistortion })
        {
            const auto boundary = increment > 0.0 ? std::floor (position - corner) + 1.0 + corner
                                                  : std::ceil (position - corner) - 1.0 + corner;
            if ((increment > 0.0 && boundary > end) || (increment < 0.0 && boundary < end))
                continue;

            const auto fraction = offset + (boundary - position) / increment;
            const auto mappedPhase = corner == 0.0 ? 0.0 : 0.5;
            const auto derivative = bank->getSlope (mappedPhase, static_cast<CoeffType> (controls.morph), bandwidth);
            const auto change = corner == 0.0 ? left - right : right - left;
            correctEvent (fraction, 0.0, derivative * change * std::abs (increment), current);
        }
    }

    //==============================================================================
    const WaveformBank<SampleType, CoeffType>* bank = nullptr;
    double internalSampleRate = 1.0;
    double phase = 0.0;
    double leaderPhase = 0.0;
    double nextCorrection = 0.0;
    double previousPhaseModulation = 0.0;
    bool hasPreviousParameters = false;
};

} // namespace detail

//==============================================================================
/** Oversampled waveform morphing, through-zero FM, PM, phase distortion and hard sync.

    Reads a shared, immutable WaveformBank. The phase accumulator is signed and
    independent of phase modulation. A two-segment phase map places half a waveform
    cycle at phaseDistortion (0.5 is the identity). Positive leader wraps reset the
    follower at their fractional internal-sample position. Two-sample polynomial
    BLEP/BLAMP residuals correct value/slope jumps at resets and phase-map corners.

    The whole modulation path runs at OversampleFactor times the output rate and
    reuses HalfbandOversampler's decimation cascade. This reduces aliasing; finite
    kernels, table interpolation and oversampling do not guarantee alias-free
    arbitrary modulation. Higher waveform derivatives and abrupt parameter changes
    remain approximate. Choose bandwidth, modulation depth and oversampling
    accordingly.

    prepare() allocates; processing and reset() do not. A bank must outlive this
    oscillator and must not be modified during playback. Each voice owns its phase,
    residual and decimator state; voices can share the bank.

    @tparam SampleType       Output precision.
    @tparam OversampleFactor Internal sample-rate multiplier, a power of two (at least 2).
    @tparam CoeffType        Waveform coefficient and decimator precision.
*/
template <typename SampleType, int OversampleFactor = 4, typename CoeffType = double>
class ModulatedOscillator
{
public:
    /** Controls for one internal sample interval. All fields must be finite. */
    using Parameters = ModulatedOscillatorParameters;

    /** Allocates decimator storage and attaches a prepared waveform bank.

        @param sampleRate    Output rate in Hz, positive.
        @param maxBlockSize  Maximum output block size, positive.
        @param waveforms     Immutable bank; must remain alive throughout playback.
        @param decimation    Halfband design of the decimator; the default is the
                             100 dB linear-phase FIR flat to 0.45 of the output rate.
    */
    void prepare (double sampleRate, int maxBlockSize, const WaveformBank<SampleType, CoeffType>& waveforms, const HalfbandOversamplerDesign& decimation = {})
    {
        jassert (sampleRate > 0.0 && maxBlockSize > 0);
        voice.prepare (jmax (1.0, sampleRate) * OversampleFactor, waveforms);
        oversampler.prepare (jmax (1.0, sampleRate), 1, jmax (1, maxBlockSize), decimation);
        reset();
    }

    /** Resets phase, sync leader, residuals and decimator history.

        @param initialPhase  Initial follower phase in periods, wrapped internally.
    */
    void reset (double initialPhase = 0.0) noexcept
    {
        voice.reset (initialPhase);
        oversampler.reset();
    }

    /** Points the oscillator at another prepared bank, keeping phase and residuals.

        Allocation-free, so it is safe between blocks on the audio thread. Use it to
        adopt a bank rebuilt on another thread: publish the replacement first, then
        swap, and keep the outgoing bank alive until every voice has swapped. The
        waveform changes instantly at the next sample and may click, so reserve it
        for edits the listener is already expecting.

        @param waveforms  Prepared bank; must remain alive throughout playback.
    */
    void setBank (const WaveformBank<SampleType, CoeffType>& waveforms) noexcept
    {
        voice.setBank (waveforms);
    }

    /** Returns the unmodulated follower accumulator in periods. */
    double getPhase() const noexcept { return voice.getPhase(); }

    /** Returns the internal rate at which the modulation callback is invoked. */
    double getInternalSampleRate() const noexcept { return voice.getInternalSampleRate(); }

    /** Returns the output latency in samples, including the decimation filter.

        Valid after prepare(); it follows the decimator design (exact for the FIR
        families, the rounded low-frequency group delay for the IIR ones).
    */
    int getLatencyInSamples() const noexcept { return oversampler.getGenerationLatencyInSamples(); }

    /** Produces a block with constant controls. Returns false for invalid block sizes.

        A rejected block leaves output and oscillator state unchanged. Carrier and
        leader frequencies are limited to half the internal rate, bounding the
        number of fractional events per interval. Negative carriers run backward.
    */
    bool processBlock (SampleType* output, int numSamples, const Parameters& parameters) noexcept
    {
        return processModulatedBlock (output, numSamples, [&] (int) { return parameters; });
    }

    /** Produces a block with controls evaluated at the internal sample rate.

        The callback is invoked as Parameters(int internalSampleIndex), in order,
        for numSamples * OversampleFactor samples. Its index restarts at zero for
        each call; keep modulator phase in caller-owned state across blocks. Run
        coupled modulators here or interpolate external control signals to this
        rate. The callback must not allocate, block or throw. Controls describe
        the following internal sample interval; event interpolation assumes they
        remain constant over that interval.

        Returns false without invoking the callback for null output, unprepared
        state, or nonpositive/oversized blocks. Abrupt control changes are not
        automatically smoothed and can create audible transients.
    */
    template <typename Modulation>
    bool processModulatedBlock (SampleType* output, int numSamples, Modulation&& modulation) noexcept
    {
        if (output == nullptr || ! voice.isPrepared() || ! oversampler.beginGeneration (1, numSamples))
            return false;

        auto* internal = oversampler.getOversampledChannelData (0);
        for (int i = 0; i < oversampler.getOversampledNumSamples(); ++i)
            internal[i] = static_cast<SampleType> (voice.processSample (modulation (i)));

        SampleType* channels[] = { output };
        oversampler.downsample (channels, 1, numSamples);
        return true;
    }

private:
    detail::ModulatedOscillatorVoice<SampleType, CoeffType> voice;
    HalfbandOversampler<SampleType, OversampleFactor, CoeffType> oversampler;
};

} // namespace yup
