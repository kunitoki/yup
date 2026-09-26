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
/** Prepared, shareable waveform frames with progressively reduced bandwidth.

    prepare() renders each Fourier series at harmonic limits N, N/2, ... 1 and
    zero (DC only). All frames use the same table size and phase convention.
    Playback interpolates adjacent frames linearly using Hermite table reads.
    Bandwidth transitions blend two safe levels. Align the phases of input frames
    when cancellation during morphing is undesirable.

    Preparation allocates FFT plans and tables and must happen off the audio thread.
    After preparation, const reads are allocation-free and may be shared by voices
    and threads. Do not prepare or destroy a bank while a voice is reading it.
    Morphing and phase modulation introduce sidebands; table bandlimiting alone
    does not make arbitrary modulation alias-free.

    @tparam SampleType  Table sample precision.
    @tparam CoeffType   Fourier coefficient precision.
    @see ModulatedOscillator
*/
template <typename SampleType, typename CoeffType = double>
class WaveformBank
{
public:
    /** Renders the frames and their bandwidth levels, including each frame's DC.

        @param frames  Series collection with at most 4096 harmonics per
                       frame. Empty collections produce a silent bank.
    */
    void prepare (Span<const FourierSeries<CoeffType>> frames)
    {
        tables.clear();
        harmonicLimits.clear();
        frameCount = static_cast<int> (frames.size());
        int maxHarmonics = 0;

        for (const auto& frame : frames)
            maxHarmonics = jmax (maxHarmonics, frame.getNumHarmonics());

        jassert (maxHarmonics <= 4096);
        maxHarmonics = jlimit (0, 4096, maxHarmonics);

        for (int limit = maxHarmonics; limit > 0; limit /= 2)
            harmonicLimits.push_back (limit);
        harmonicLimits.push_back (0);

        tables.reserve (frames.size() * harmonicLimits.size());

        for (const auto& frame : frames)
        {
            for (const auto limit : harmonicLimits)
            {
                auto& table = tables.emplace_back();
                table.prepare (1.0, maxHarmonics);
                table.setSeries (frame);
                table.setIncludeDC (true);
                table.setFrequency (static_cast<CoeffType> (0.5 / (limit + 0.5)));
                table.render (false);
            }
        }
    }

    /** Replaces the contents of every prepared frame without allocating.

        The counterpart of WavetableOscillator's setSeries/render split: the tables,
        their sizes and the per-level harmonic limits chosen by prepare() are kept,
        and only the coefficients are re-rendered. Frames carrying fewer harmonics
        than the bank was prepared for are zero-extended.

        Rendering is still one inverse FFT per frame and level, so this belongs off
        the audio thread, and the bank must not be read while it runs.

        @param frames  Replacement series, one per prepared frame, each with at most
                       getNumHarmonics() harmonics.

        @returns false, leaving the bank untouched and readable, when the frame count
                 differs from prepare() or a frame carries too many harmonics.
    */
    bool refreshFrames (Span<const FourierSeries<CoeffType>> frames) noexcept
    {
        if (static_cast<int> (frames.size()) != frameCount)
            return false;

        for (const auto& frame : frames)
            if (frame.getNumHarmonics() > getNumHarmonics())
                return false;

        for (std::size_t index = 0; index < tables.size(); ++index)
        {
            auto& table = tables[index];
            table.setSeries (frames[index / harmonicLimits.size()]);
            table.render (false);
        }

        return true;
    }

    /** Returns the number of prepared frames. */
    int getNumFrames() const noexcept { return frameCount; }

    /** Returns the largest prepared harmonic count. */
    int getNumHarmonics() const noexcept
    {
        return harmonicLimits.empty() ? 0 : harmonicLimits.front();
    }

    /** Reads a waveform value without modifying the bank.

        @param phase         Phase in periods, wrapped internally.
        @param position      Frame position in [0, 1], clamped at the endpoints.
        @param maxHarmonics  Exclusive harmonic bandwidth in harmonics. Two levels
                             below this bound are blended continuously. Use twice
                             getNumHarmonics() for the full spectrum; zero reads DC.
    */
    SampleType getValue (double phase, CoeffType position, double maxHarmonics) const noexcept
    {
        return read (phase, position, maxHarmonics, false);
    }

    /** Reads the derivative per phase period with the same frame/bandwidth selection.

        This differentiates the Hermite interpolant analytically, not the phase
        trajectory or the morph signal.
    */
    SampleType getSlope (double phase, CoeffType position, double maxHarmonics) const noexcept
    {
        return read (phase, position, maxHarmonics, true);
    }

private:
    SampleType read (double phase, CoeffType position, double maxHarmonics, bool derivative) const noexcept
    {
        if (frameCount == 0 || ! std::isfinite (position))
            return SampleType (0);

        const auto framePosition = jlimit (CoeffType (0), CoeffType (1), position) * static_cast<CoeffType> (frameCount - 1);
        const auto firstFrame = static_cast<int> (framePosition);
        const auto secondFrame = jmin (firstFrame + 1, frameCount - 1);
        const auto fraction = static_cast<SampleType> (framePosition - static_cast<CoeffType> (firstFrame));
        std::size_t level = 0;

        while (level + 1 < harmonicLimits.size() && harmonicLimits[level] >= maxHarmonics)
            ++level;

        const auto readFrame = [&] (int frame)
        {
            const auto base = static_cast<std::size_t> (frame) * harmonicLimits.size();
            const auto readLevel = [&] (std::size_t index)
            {
                const auto& table = tables[base + index];
                return derivative ? table.getSlopeAtPhase (phase) : table.getValueAtPhase (phase);
            };

            const auto value = readLevel (level);
            if (level + 1 == harmonicLimits.size())
                return value;

            const auto upper = level == 0 ? 2.0 * harmonicLimits[0] : static_cast<double> (harmonicLimits[level - 1]);
            const auto blend = static_cast<SampleType> (jlimit (0.0, 1.0, (maxHarmonics - harmonicLimits[level]) / (upper - harmonicLimits[level])));
            if (blend == SampleType (1))
                return value;

            const auto darker = readLevel (level + 1);
            return darker + (value - darker) * blend;
        };

        const auto first = readFrame (firstFrame);
        if (fraction == SampleType (0))
            return first;

        return first + (readFrame (secondFrame) - first) * fraction;
    }

    std::vector<WavetableOscillator<SampleType, CoeffType>> tables;
    std::vector<int> harmonicLimits;
    int frameCount = 0;
};

} // namespace yup
