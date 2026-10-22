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
    Wavetable oscillator rendering a FourierSeries with an inverse FFT.

    A FourierSeries is rendered once into a single-cycle table and then played
    back with 4-point Hermite interpolation, which makes the per-sample cost a
    couple of table reads instead of one multiply accumulate per harmonic. The
    table is oversampled 8x with respect to the series bandwidth, which keeps the
    interpolation images below roughly -70 dB.

    Renders are crossfaded: rendering a new table while the previous one is still
    playing ramps between them over a configurable number of samples, so a
    synchronization parameter change does not click. This is the cheap backend of
    the two the oscillators in this folder offer, at the price of a coarser
    control over when the harmonic content is refreshed.

    Rendering costs one inverse FFT (about 10 to 50 us for the default table
    size), so it belongs in an update() called once per block, not in the audio
    callback. Everything else, including processSample(), is allocation-free.

    @tparam SampleType  Type for the synthesized samples (float or double).
    @tparam CoeffType   Type for the coefficients and phase math (default double).

    @see FourierSeries, AdditiveOscillator, SyncOscillator
*/
template <typename SampleType, typename CoeffType = double>
class WavetableOscillator
{
public:
    //==============================================================================
    /** Smallest table the renderer will use. */
    static constexpr int minimumTableSize = 64;

    /** Largest table the renderer will use. */
    static constexpr int maximumTableSize = 32768;

    /** Table oversampling factor relative to the series bandwidth. */
    static constexpr int tableOversampling = 8;

    //==============================================================================
    /** Default constructor. Call prepare() before processing. */
    WavetableOscillator() = default;

    //==============================================================================
    /**
        Allocates the render buffers and the FFT plan.

        @param sampleRate                Sample rate in Hz.
        @param maxHarmonics              Maximum number of harmonics to render.
        @param crossfadeLengthInSamples  Length of the crossfade between two renders.
    */
    void prepare (double sampleRate, int maxHarmonics, int crossfadeLengthInSamples = 64)
    {
        jassert (sampleRate > 0.0);
        jassert (maxHarmonics >= 0);
        jassert (maxHarmonics <= 4096);

        this->sampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        crossfadeLength = jmax (1, crossfadeLengthInSamples);
        crossfadeStep = CoeffType (1) / static_cast<CoeffType> (crossfadeLength);

        series.resize (jmax (0, maxHarmonics));

        tableSize = jlimit (minimumTableSize,
                            maximumTableSize,
                            nextPowerOfTwo (jmax (1, tableOversampling * maxHarmonics)));
        tableMask = tableSize - 1;

        fft = std::make_unique<FFTProcessor<float>> (tableSize);
        fft->setScaling (FFTProcessor<float>::FFTScaling::none);

        spectrum.assign (static_cast<std::size_t> (tableSize) * 2, 0.0f);
        renderBuffer.assign (static_cast<std::size_t> (tableSize), 0.0f);
        current.assign (static_cast<std::size_t> (tableSize), SampleType (0));
        next.assign (static_cast<std::size_t> (tableSize), SampleType (0));

        renderedHarmonics = 0;
        seriesChanged = true;
        crossfadePosition = CoeffType (1);
        currentPeak = {};
        nextPeak = {};

        reset();
    }

    /** Resets the playback phase. The rendered tables are kept. */
    void reset() noexcept
    {
        phase = 0.0;
    }

    //==============================================================================
    /** Sets the playback frequency in Hz. */
    void setFrequency (CoeffType newFrequency) noexcept
    {
        const auto sanitized = jmax (newFrequency, static_cast<CoeffType> (0));

        if (! approximatelyEqual (frequency, sanitized))
            frequency = sanitized;
    }

    /** Returns the playback frequency in Hz. */
    CoeffType getFrequency() const noexcept { return frequency; }

    /** Sets the phase, normalized to one period. */
    void setPhase (CoeffType newPhase) noexcept
    {
        phase = static_cast<double> (newPhase - std::floor (newPhase));
    }

    /** Returns the phase, normalized to one period. */
    CoeffType getPhase() const noexcept { return static_cast<CoeffType> (phase); }

    //==============================================================================
    /** Copies a series into the oscillator. The table is not rendered until render(). */
    void setSeries (const FourierSeries<CoeffType>& newSeries) noexcept
    {
        series.copyFrom (newSeries);
        seriesChanged = true;
    }

    /** Fills the series with one of the convenient presets, pending a render. */
    void setWaveform (Waveform waveform) noexcept
    {
        series.setWaveform (waveform);
        seriesChanged = true;
    }

    /** Returns the series being rendered. */
    const FourierSeries<CoeffType>& getSeries() const noexcept { return series; }

    /** Selects whether the series' DC coefficient is rendered (off by default). */
    void setIncludeDC (bool shouldIncludeDC) noexcept
    {
        if (includeDC != shouldIncludeDC)
        {
            includeDC = shouldIncludeDC;
            seriesChanged = true;
        }
    }

    /** Returns whether the DC coefficient is rendered. */
    bool getIncludeDC() const noexcept { return includeDC; }

    //==============================================================================
    /** Returns true when the rendered table no longer matches the series and pitch.

        A render is needed when the series changed, when the pitch rose far enough
        that the rendered harmonics would alias, or when the pitch dropped far
        enough that more harmonics than one eighth of the rendered ones became
        available again - the latter keeps a vibrato from triggering an inverse FFT
        on every block.
    */
    bool needsRender() const noexcept
    {
        if (seriesChanged)
            return true;

        const auto limit = jmin (getNyquistHarmonicLimit (frequency, sampleRate, series.getNumHarmonics()),
                                 tableSize / 2 - 1);

        if (limit < renderedHarmonics)
            return true;

        return limit > renderedHarmonics + jmax (1, renderedHarmonics / 8);
    }

    //==============================================================================
    /**
        Renders the series into the next table and starts crossfading towards it.

        An in-progress crossfade is baked into the current table first, so no
        rendition is lost. The inverse FFT is unnormalized, and the bins are filled
        with half the coefficients, which makes the result the series itself: the
        complex bin n of a signal a cos (2 pi n t) + b sin (2 pi n t) is a / 2 for
        the real part and -b / 2 for the imaginary one.

        @param crossfade  When false, installs the table immediately. This is useful
                          for preparing tables before playback. Replacing an audible
                          table immediately may click.
    */
    void render (bool crossfade = true) noexcept
    {
        if (fft == nullptr || tableSize <= 0)
            return;

        bakeCrossfade();

        FloatVectorOperations::clear (spectrum.data(), tableSize * 2);

        const auto limit = jmin (getNyquistHarmonicLimit (frequency, sampleRate, series.getNumHarmonics()),
                                 tableSize / 2 - 1);

        const auto* cosineCoefficients = series.getCosineCoefficients().data();
        const auto* sineCoefficients = series.getSineCoefficients().data();
        const auto half = static_cast<float> (0.5);

        for (int n = 1; n <= limit; ++n)
        {
            const auto index = static_cast<std::size_t> (n - 1);

            spectrum[static_cast<std::size_t> (2 * n)] = static_cast<float> (cosineCoefficients[index]) * half;
            spectrum[static_cast<std::size_t> (2 * n + 1)] = static_cast<float> (-sineCoefficients[index]) * half;
        }

        if (includeDC)
            spectrum[0] = static_cast<float> (series.getDC());

        fft->performRealFFTInverse (spectrum.data(), renderBuffer.data());

        if constexpr (std::is_same_v<SampleType, float>)
            FloatVectorOperations::copy (next.data(), renderBuffer.data(), tableSize);
        else
            FloatVectorOperations::convertFloatToDouble (next.data(), renderBuffer.data(), tableSize);

        nextPeak.dirty = true;
        renderedHarmonics = limit;
        seriesChanged = false;

        startCrossfade (crossfade);
    }

    /**
        Starts crossfading into the table another oscillator rendered last, instead
        of running an inverse FFT.

        This lets oscillators playing the same series at close pitches, like unison
        voices, share one render: render the source, then call renderFrom() on the
        others. The copied table holds the harmonics the source rendered for its own
        frequency, so render the source at the highest pitch any of the sharing
        oscillators plays, or the higher ones will alias. Lower ones simply lack the
        few top harmonics they could have played.

        The source's latest rendition is taken, even while it is still crossfading
        into it. An in-progress crossfade of this oscillator is baked first, as in
        render(). The source's rendered harmonic count is taken over and the series
        is marked as rendered, so needsRender() on this oscillator no longer reflects
        its own series and pitch until the next render().

        @param source     The oscillator to copy from. Must have the same table size.
        @param crossfade  When false, installs the table immediately, as in render().
    */
    void renderFrom (const WavetableOscillator& source, bool crossfade = true) noexcept
    {
        jassert (source.getTableSize() == tableSize);

        if (&source == this || source.getTableSize() != tableSize || tableSize <= 0)
            return;

        bakeCrossfade();

        const auto sourceIsFading = source.crossfadePosition < CoeffType (1);
        const auto& latest = sourceIsFading ? source.next : source.current;
        FloatVectorOperations::copy (next.data(), latest.data(), tableSize);
        nextPeak = sourceIsFading ? source.nextPeak : source.currentPeak;

        renderedHarmonics = source.renderedHarmonics;
        seriesChanged = false;

        startCrossfade (crossfade);
    }

    /** Reads the rendered waveform at a phase in periods without advancing state.

        Includes the current render crossfade. Negative phases wrap periodically.
        For externally driven phases, the caller is responsible for bandwidth;
        phase modulation can create frequencies beyond the rendered harmonics.
    */
    SampleType getValueAtPhase (double normalizedPhase) const noexcept
    {
        return readAtPhase (normalizedPhase, false);
    }

    /** Returns the derivative of the interpolated waveform per phase period.

        Includes the current render crossfade without advancing it. This is the
        analytic derivative of the Hermite interpolant, useful for calculating
        slope jumps at fractional sync events.
    */
    SampleType getSlopeAtPhase (double normalizedPhase) const noexcept
    {
        return readAtPhase (normalizedPhase, true);
    }

    //==============================================================================
    /** Returns the rendered table size in samples. */
    int getTableSize() const noexcept { return tableSize; }

    /** Returns the table being played, one period long.

        While a crossfade is in progress this is the table being faded out, not the
        mix being heard. Empty before prepare().
    */
    Span<const SampleType> getRenderedTable() const noexcept
    {
        return { current.data(), current.size() };
    }

    /** Returns the steepest slope of the audible table, in units per period.

        Measured from the rendered samples on the first call after a render, so
        callers that never ask pay nothing. While a crossfade is in progress it covers
        both tables. A phase modulator's peak slope times its modulation depth and
        frequency bounds the frequency deviation it causes, which is what a phase
        modulation matrix needs to pick a carrier's bandwidth. Zero before the first
        render.
    */
    SampleType getPeakSlope() const noexcept
    {
        const auto currentSlope = peakSlopeOf (current, currentPeak);
        return crossfadePosition < CoeffType (1) ? jmax (currentSlope, peakSlopeOf (next, nextPeak)) : currentSlope;
    }

    /** Returns the number of harmonics present in the rendered table. */
    int getNumRenderedHarmonics() const noexcept { return renderedHarmonics; }

    /** Returns the number of harmonics the oscillator can render. */
    int getNumHarmonics() const noexcept { return series.getNumHarmonics(); }

    //==============================================================================
    /** Produces one sample.

        While a pitch is rising between two update() calls the top rendered harmonic
        can drift slightly past Nyquist until the next render, so callers are
        expected to refresh once per block with update() on the owning facade.
    */
    SampleType processSample() noexcept
    {
        const auto value = tick (phase);

        const auto increment = static_cast<double> (frequency) / sampleRate;

        phase += increment;
        phase -= std::floor (phase);

        return value;
    }

    /** Produces one sample read at an externally driven phase.

        Use this when the caller owns the phase, as a phase modulation operator does.
        Like processSample(), it advances the render crossfade by one sample, so a
        fade lasts the same number of calls whichever method drives the table. The
        oscillator's own phase is left untouched. Negative phases wrap periodically,
        and a non-finite phase reads silence while still advancing the crossfade.

        As with getValueAtPhase(), the caller is responsible for bandwidth.
    */
    SampleType processSampleAtPhase (double normalizedPhase) noexcept
    {
        if (tableSize <= 0 || ! std::isfinite (normalizedPhase))
        {
            advanceCrossfade (1);
            return SampleType (0);
        }

        return tick (normalizedPhase - std::floor (normalizedPhase));
    }

    /** Advances the render crossfade as if numSamples samples had been produced.

        Lets a caller that skips an oscillator for a while keep its fade timing.
        Does nothing when no crossfade is in progress.
    */
    void advanceCrossfade (int numSamples) noexcept
    {
        if (crossfadePosition >= CoeffType (1) || numSamples <= 0)
            return;

        crossfadePosition += crossfadeStep * static_cast<CoeffType> (numSamples);

        if (crossfadePosition >= CoeffType (1))
        {
            crossfadePosition = CoeffType (1);
            std::swap (current, next);
            currentPeak = nextPeak;
        }
    }

    /** Produces a block of samples. */
    void processBlock (SampleType* output, int numSamples) noexcept
    {
        if (output == nullptr)
            return;

        for (int i = 0; i < numSamples; ++i)
            output[i] = processSample();
    }

private:
    //==============================================================================
    SampleType tick (double wrappedPhase) noexcept
    {
        auto value = readTable (current, wrappedPhase, false);

        if (crossfadePosition < CoeffType (1))
        {
            value += (readTable (next, wrappedPhase, false) - value) * static_cast<SampleType> (crossfadePosition);
            advanceCrossfade (1);
        }

        return value;
    }

    void bakeCrossfade() noexcept
    {
        if (crossfadePosition >= CoeffType (1))
            return;

        FloatVectorOperations::subtract (next.data(), current.data(), tableSize);
        FloatVectorOperations::addWithMultiply (current.data(), next.data(), static_cast<SampleType> (crossfadePosition), tableSize);

        currentPeak.dirty = true;
    }

    void startCrossfade (bool crossfade) noexcept
    {
        crossfadePosition = CoeffType (0);

        if (crossfade)
            return;

        std::swap (current, next);
        currentPeak = nextPeak;
        crossfadePosition = CoeffType (1);
    }

    struct PeakSlope
    {
        SampleType value = SampleType (0);
        bool dirty = false;
    };

    SampleType peakSlopeOf (const std::vector<SampleType>& table, PeakSlope& peak) const noexcept
    {
        if (! peak.dirty)
            return peak.value;

        auto largest = SampleType (0);

        for (int i = 0; i < tableSize; ++i)
            largest = jmax (largest, std::abs (table[static_cast<std::size_t> ((i + 1) & tableMask)] - table[static_cast<std::size_t> (i)]));

        peak.value = largest * static_cast<SampleType> (tableSize);
        peak.dirty = false;
        return peak.value;
    }

    SampleType readAtPhase (double normalizedPhase, bool derivative) const noexcept
    {
        if (tableSize <= 0 || ! std::isfinite (normalizedPhase))
            return SampleType (0);

        normalizedPhase -= std::floor (normalizedPhase);
        auto value = readTable (current, normalizedPhase, derivative);

        if (crossfadePosition < CoeffType (1))
            value += (readTable (next, normalizedPhase, derivative) - value) * static_cast<SampleType> (crossfadePosition);

        return value;
    }

    SampleType readTable (const std::vector<SampleType>& table, double normalizedPhase, bool derivative) const noexcept
    {
        const auto position = normalizedPhase * static_cast<double> (tableSize);
        const auto index = static_cast<int> (position) & tableMask;
        const auto fraction = static_cast<CoeffType> (position - std::floor (position));

        const auto y0 = static_cast<CoeffType> (table[static_cast<std::size_t> ((index - 1) & tableMask)]);
        const auto y1 = static_cast<CoeffType> (table[static_cast<std::size_t> (index)]);
        const auto y2 = static_cast<CoeffType> (table[static_cast<std::size_t> ((index + 1) & tableMask)]);
        const auto y3 = static_cast<CoeffType> (table[static_cast<std::size_t> ((index + 2) & tableMask)]);

        const auto c0 = y1;
        const auto c1 = static_cast<CoeffType> (0.5) * (y2 - y0);
        const auto c2 = y0 - static_cast<CoeffType> (2.5) * y1 + static_cast<CoeffType> (2) * y2 - static_cast<CoeffType> (0.5) * y3;
        const auto c3 = static_cast<CoeffType> (0.5) * (y3 - y0) + static_cast<CoeffType> (1.5) * (y1 - y2);

        if (derivative)
            return static_cast<SampleType> ((CoeffType (3) * c3 * fraction * fraction + CoeffType (2) * c2 * fraction + c1) * static_cast<CoeffType> (tableSize));

        return static_cast<SampleType> (((c3 * fraction + c2) * fraction + c1) * fraction + c0);
    }

    //==============================================================================
    std::unique_ptr<FFTProcessor<float>> fft;
    FourierSeries<CoeffType> series;
    std::vector<float> spectrum;
    std::vector<float> renderBuffer;
    std::vector<SampleType> current;
    std::vector<SampleType> next;
    double sampleRate = 44100.0;
    double phase = 0.0;
    CoeffType frequency = static_cast<CoeffType> (440);
    CoeffType crossfadeStep = CoeffType (0);
    CoeffType crossfadePosition = CoeffType (1);
    mutable PeakSlope currentPeak;
    mutable PeakSlope nextPeak;
    int tableSize = 0;
    int tableMask = 0;
    int crossfadeLength = 64;
    int renderedHarmonics = 0;
    bool seriesChanged = true;
    bool includeDC = false;
};

//==============================================================================
/** @name Convenience type aliases */
using WavetableOscillatorFloat = WavetableOscillator<float>;   /**< float samples, double coefficients */
using WavetableOscillatorDouble = WavetableOscillator<double>; /**< double samples and coefficients */

} // namespace yup
