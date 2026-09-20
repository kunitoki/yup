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
/** Morphs two spectral oscillators without rebuilding spectra for morph changes.

    The endpoints share pitch, phase, sync mode and follower ratio. update() applies
    the synchronization transform independently to each endpoint; because that
    transform is linear, blending their output is equivalent to transforming the
    blended input at a fixed ratio. No transform or FFT runs in processSample().

    Morph is linear amplitude interpolation, clamped to [0, 1]. Moving it creates
    modulation sidebands. Smooth control-rate changes and leave bandwidth headroom;
    use ModulatedOscillator for oversampled audio-rate morphing. Endpoint render
    crossfades remain independent of the morph control.

    @tparam SampleType  Output sample precision.
    @tparam CoeffType   Fourier coefficient precision.
    @see SyncOscillator, ModulatedOscillator
*/
template <typename SampleType, typename CoeffType = double>
class MorphingOscillator
{
public:
    /** Runtime synthesis backend, shared by both endpoints. */
    using Synthesis = typename SyncOscillator<SampleType, CoeffType>::Synthesis;

    /** Allocates both endpoints outside the audio callback. */
    void prepare (double sampleRate, int maxHarmonics = 128, int crossfadeLengthInSamples = 64)
    {
        first.prepare (sampleRate, maxHarmonics, crossfadeLengthInSamples);
        second.prepare (sampleRate, maxHarmonics, crossfadeLengthInSamples);
    }

    /** Copies two endpoint spectra into prepared storage. Call update() afterward. */
    void setSeries (const FourierSeries<CoeffType>& a, const FourierSeries<CoeffType>& b) noexcept
    {
        first.setFollowerSeries (a);
        second.setFollowerSeries (b);
    }

    /** Sets the common leader frequency in Hz. Call update() to refresh bandwidth. */
    void setFrequency (CoeffType frequency) noexcept
    {
        first.setFrequency (frequency);
        second.setFrequency (frequency);
    }

    /** Sets the common synchronization mode, pending update(). */
    void setSyncMode (SyncMode mode) noexcept
    {
        first.setSyncMode (mode);
        second.setSyncMode (mode);
    }

    /** Sets the common follower/leader frequency ratio, pending update(). */
    void setFollowerRatio (CoeffType ratio) noexcept
    {
        first.setFollowerRatio (ratio);
        second.setFollowerRatio (ratio);
    }

    /** Selects both synthesis backends, pending update(). */
    void setSynthesis (Synthesis synthesis) noexcept
    {
        first.setSynthesis (synthesis);
        second.setSynthesis (synthesis);
    }

    /** Sets the common phase in periods. */
    void setPhase (CoeffType phase) noexcept
    {
        first.setPhase (phase);
        second.setPhase (phase);
    }

    /** Returns the common playback phase in periods. */
    CoeffType getPhase() const noexcept { return first.getPhase(); }

    /** Resets both phases, keeping the prepared spectra and tables. */
    void reset() noexcept
    {
        first.reset();
        second.reset();
    }

    /** Selects whether endpoint DC coefficients are synthesized, pending update(). */
    void setIncludeDC (bool include) noexcept
    {
        first.setIncludeDC (include);
        second.setIncludeDC (include);
    }

    /** Returns whether either endpoint needs a control-rate refresh. */
    bool needsUpdate() const noexcept { return first.needsUpdate() || second.needsUpdate(); }

    /** Refreshes endpoint spectra/tables without allocating. Call once per block. */
    void update() noexcept
    {
        first.update();
        second.update();
    }

    /** Produces a sample, with morph 0 selecting the first endpoint and 1 the second. */
    SampleType processSample (CoeffType morph) noexcept
    {
        const auto a = first.processSample();
        const auto b = second.processSample();
        return a + (b - a) * static_cast<SampleType> (jlimit (CoeffType (0), CoeffType (1), morph));
    }

    /** Produces a block with a fixed morph position. */
    void processBlock (SampleType* output, int numSamples, CoeffType morph) noexcept
    {
        if (output == nullptr)
            return;

        for (int i = 0; i < numSamples; ++i)
            output[i] = processSample (morph);
    }

    /** Produces a block with one morph position per output sample. */
    void processBlock (SampleType* output, Span<const CoeffType> morph) noexcept
    {
        if (output == nullptr)
            return;

        for (std::size_t i = 0; i < morph.size(); ++i)
            output[i] = processSample (morph[i]);
    }

private:
    SyncOscillator<SampleType, CoeffType> first;
    SyncOscillator<SampleType, CoeffType> second;
};

} // namespace yup
