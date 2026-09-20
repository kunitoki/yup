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
    Alias-free synchronizing oscillator, ready to drop into a synthesizer voice.

    Owns a follower FourierSeries, the synced series the spectral resampler
    produces from it, and both synthesis backends, so a voice only has to set a
    frequency, a follower ratio and a sync mode. The output never aliases,
    because the series being synthesized only ever holds harmonics below Nyquist.

    ```
    yup::SyncOscillator<double> osc;
    osc.prepare (44100.0);
    osc.setWaveform (yup::Waveform::sawtooth);
    osc.setSyncMode (yup::SyncMode::hard);
    osc.setFrequency (220.0);
    osc.setFollowerRatio (1.375);
    osc.update();                              // resample + render, once per block
    osc.processBlock (buffer, numSamples);
    ```

    Pitch changes are phase-continuous and need no update(); changing the
    synchronization parameters marks the oscillator dirty and needs one update()
    before the change is heard. update() runs the O (N^2) spectral transform plus,
    for the wavetable backend, one inverse FFT, so it belongs in the block loop,
    not in the sample loop.

    Note that mirrored sync physically has a period of 2 * T_lead, so it sounds an
    octave below the leader, exactly as the reference method describes:
    getOutputFrequency() reports the fundamental that is actually synthesized.

    @tparam SampleType  Type for the synthesized samples (float or double).
    @tparam CoeffType   Type for the coefficients and phase math (default double).

    @see FourierSeries, SyncSpectralResampler, AdditiveOscillator, WavetableOscillator
*/
template <typename SampleType, typename CoeffType = double>
class SyncOscillator
{
public:
    //==============================================================================
    /** Synthesis backend. */
    enum class Synthesis
    {
        wavetable, /**< Inverse FFT into a table, cheap per sample */
        additive   /**< Exact additive synthesis, one multiply accumulate per harmonic and sample */
    };

    //==============================================================================
    /** Default constructor. Call prepare() before processing. */
    SyncOscillator() = default;

    //==============================================================================
    /**
        Allocates every backend and starts from a sine follower.

        @param sampleRate                Sample rate in Hz.
        @param maxHarmonics              Maximum number of follower and output harmonics.
        @param crossfadeLengthInSamples  Length of the wavetable crossfade between renders.
    */
    void prepare (double sampleRate, int maxHarmonics = 128, int crossfadeLengthInSamples = 64)
    {
        jassert (sampleRate > 0.0);
        jassert (maxHarmonics > 0);

        this->sampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        this->maxHarmonics = jmax (1, maxHarmonics);

        follower.resize (this->maxHarmonics);
        follower.setWaveform (Waveform::sine);
        synced.resize (this->maxHarmonics);

        resampler.prepare (this->maxHarmonics);
        additive.prepare (sampleRate, this->maxHarmonics);
        wavetable.prepare (sampleRate, this->maxHarmonics, crossfadeLengthInSamples);

        followerRatio = CoeffType (1);
        dirty = true;

        setFrequency (leaderFrequency);
        setPhase (CoeffType (0));

        additive.setIncludeDC (includeDC);
        wavetable.setIncludeDC (includeDC);

        update();
    }

    /** Resets the playback phase of both backends. */
    void reset() noexcept
    {
        additive.reset();
        wavetable.reset();
    }

    //==============================================================================
    /** Selects the synthesis backend, keeping the phase continuous. */
    void setSynthesis (Synthesis newSynthesis) noexcept
    {
        if (synthesis == newSynthesis)
            return;

        const auto currentPhase = getPhase();

        synthesis = newSynthesis;

        setPhase (currentPhase);
    }

    /** Returns the active synthesis backend. */
    Synthesis getSynthesis() const noexcept { return synthesis; }

    //==============================================================================
    /**
        Sets the leader (note) pitch in Hz.

        The follower's pitch follows immediately as
        frequency * getFundamentalScale (syncMode), which needs no update() because
        both backends are phase-continuous in frequency.
    */
    void setFrequency (CoeffType leaderHz) noexcept
    {
        leaderFrequency = jmax (leaderHz, CoeffType (0));

        const auto outputFrequency = getOutputFrequency();

        additive.setFrequency (outputFrequency);
        wavetable.setFrequency (outputFrequency);
    }

    /** Returns the leader pitch in Hz. */
    CoeffType getFrequency() const noexcept { return leaderFrequency; }

    /** Returns the fundamental that is actually synthesized, in Hz. */
    CoeffType getOutputFrequency() const noexcept
    {
        return leaderFrequency * SyncSpectralResampler<CoeffType>::getFundamentalScale (syncMode);
    }

    //==============================================================================
    /**
        Sets the follower ratio P = T_lead / T_follow = f_follower / f_lead.

        Changing the ratio marks the oscillator dirty; call update() to run the
        spectral transform. Because the ratio is a factor rather than an absolute
        frequency, it stays constant when the pitch changes.
    */
    void setFollowerRatio (CoeffType ratio) noexcept
    {
        const auto sanitized = jmax (ratio, static_cast<CoeffType> (1e-6));

        if (! approximatelyEqual (followerRatio, sanitized))
        {
            followerRatio = sanitized;
            dirty = true;
        }
    }

    /** Returns the follower ratio. */
    CoeffType getFollowerRatio() const noexcept { return followerRatio; }

    /**
        Sets the follower frequency in Hz, converting it to a period ratio.

        The conversion uses the current leader pitch, so call setFrequency() first.
        The ratio then stays constant across later pitch changes, which is what a
        keyboard-tracking sync oscillator wants.
    */
    void setFollowerFrequency (CoeffType followerHz) noexcept
    {
        jassert (leaderFrequency > CoeffType (0));

        if (leaderFrequency <= CoeffType (0))
            return;

        setFollowerRatio (followerHz / leaderFrequency);
    }

    //==============================================================================
    /** Sets the synchronization mode, marking the oscillator dirty. */
    void setSyncMode (SyncMode newSyncMode) noexcept
    {
        if (syncMode == newSyncMode)
            return;

        syncMode = newSyncMode;
        dirty = true;

        setFrequency (leaderFrequency);
    }

    /** Returns the synchronization mode. */
    SyncMode getSyncMode() const noexcept { return syncMode; }

    //==============================================================================
    /** Fills the follower with one of the convenient presets, marking it dirty. */
    void setWaveform (Waveform waveform) noexcept
    {
        follower.setWaveform (waveform);
        dirty = true;
    }

    /** Copies a follower series in, marking the oscillator dirty. */
    void setFollowerSeries (const FourierSeries<CoeffType>& newFollower) noexcept
    {
        follower.copyFrom (newFollower);
        dirty = true;
    }

    /** Returns the follower series. */
    const FourierSeries<CoeffType>& getFollowerSeries() const noexcept { return follower; }

    /** Returns the synchronized series, useful for visualization. */
    const FourierSeries<CoeffType>& getSyncedSeries() const noexcept { return synced; }

    //==============================================================================
    /** Sets the phase of both backends, normalized to one period. */
    void setPhase (CoeffType newPhase) noexcept
    {
        additive.setPhase (newPhase);
        wavetable.setPhase (newPhase);
    }

    /** Returns the phase of the active backend, normalized to one period. */
    CoeffType getPhase() const noexcept
    {
        return synthesis == Synthesis::additive ? additive.getPhase() : wavetable.getPhase();
    }

    /** Selects whether the synthesized series includes its DC coefficient. */
    void setIncludeDC (bool shouldIncludeDC) noexcept
    {
        if (includeDC == shouldIncludeDC)
            return;

        includeDC = shouldIncludeDC;

        additive.setIncludeDC (shouldIncludeDC);
        wavetable.setIncludeDC (shouldIncludeDC);
    }

    /** Returns whether the synthesized series includes its DC coefficient. */
    bool getIncludeDC() const noexcept { return includeDC; }

    //==============================================================================
    /** Returns true when update() would change the output. */
    bool needsUpdate() const noexcept
    {
        return dirty || wavetable.needsRender();
    }

    /**
        Runs the spectral transform and refreshes the wavetable, if needed.

        Costs a worst case O (N^2) transform plus one inverse FFT, and allocates
        nothing, so it can be called from the audio thread once per block. When
        nothing is dirty and the wavetable is up to date it does no work at all,
        which makes it safe to call unconditionally.
    */
    void update() noexcept
    {
        if (dirty)
        {
            const auto numOutputHarmonics = jmin (maxHarmonics,
                                                  SyncSpectralResampler<CoeffType>::getRecommendedOutputHarmonics (follower.getNumHarmonics(),
                                                                                                                   followerRatio,
                                                                                                                   syncMode));

            resampler.transform (follower, followerRatio, syncMode, synced, numOutputHarmonics);

            additive.setSeries (synced);
            wavetable.setSeries (synced);

            dirty = false;
        }

        if (wavetable.needsRender())
            wavetable.render();
    }

    //==============================================================================
    /** Produces one sample with the active backend. */
    SampleType processSample() noexcept
    {
        return synthesis == Synthesis::additive ? additive.processSample()
                                                : wavetable.processSample();
    }

    /** Produces a block of samples with the active backend. */
    void processBlock (SampleType* output, int numSamples) noexcept
    {
        if (output == nullptr)
            return;

        if (synthesis == Synthesis::additive)
            additive.processBlock (output, numSamples);
        else
            wavetable.processBlock (output, numSamples);
    }

    //==============================================================================
private:
    //==============================================================================
    SyncSpectralResampler<CoeffType> resampler;
    AdditiveOscillator<SampleType, CoeffType> additive;
    WavetableOscillator<SampleType, CoeffType> wavetable;
    FourierSeries<CoeffType> follower;
    FourierSeries<CoeffType> synced;
    double sampleRate = 44100.0;
    CoeffType leaderFrequency = static_cast<CoeffType> (440);
    CoeffType followerRatio = CoeffType (1);
    SyncMode syncMode = SyncMode::none;
    Synthesis synthesis = Synthesis::wavetable;
    int maxHarmonics = 128;
    bool includeDC = false;
    bool dirty = true;
};

//==============================================================================
/** @name Convenience type aliases */
using SyncOscillatorFloat = SyncOscillator<float>;   /**< float samples, double coefficients */
using SyncOscillatorDouble = SyncOscillator<double>; /**< double samples and coefficients */

} // namespace yup
