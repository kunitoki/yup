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

namespace yup
{

//==============================================================================
/**
    Describes how a single display point of a logarithmic frequency axis maps onto
    the fractional FFT bin domain.

    A display point never lands exactly on an FFT bin: at low frequencies many
    display points share a single bin, while at high frequencies a single display
    point can span many bins.

    @see SpectrumBinMapping

    @tags{Audio}
*/
struct SpectrumBinRange
{
    float centerFrequency = 0.0f; ///< The frequency in Hz displayed by the point.
    float exactBin = 0.0f;        ///< The fractional FFT bin position of centerFrequency.
    float startBin = 0.0f;        ///< The fractional FFT bin position of the lower band edge.
    float endBin = 0.0f;          ///< The fractional FFT bin position of the upper band edge.
};

//==============================================================================
/**
    Maps a logarithmic frequency axis onto fractional FFT bin positions and evaluates
    the levels of that axis continuously across the bin domain.

    Snapping every display point to its nearest FFT bin makes all points that share a
    bin report an identical level, which renders as a flat staircase. This class avoids
    that: levels are interpolated between neighbouring bins, and band aggregation slides
    continuously with the band edges, so neighbouring display points always produce
    gradually changing levels.

    The level interpolation is performed on the amplitude decibel scale (20 * log10) so
    that the interpolated curve keeps the rounded shape of a parabolic peak refinement,
    evaluated at the fractional bin position rather than snapped to a bin. Bins where the
    interpolation would not be concave fall back to a monotone linear interpolation, so a
    steep bin pair can never produce an undershoot.

    Example usage:

    @code
        SpectrumBinMapping mapping;
        mapping.setFftParameters (fftSize, sampleRate);
        mapping.setFrequencyRange (20.0f, 20000.0f);
        mapping.setNumDisplayPoints (512);

        for (int displayPoint = 0; displayPoint < mapping.getNumDisplayPoints(); ++displayPoint)
            displayLevels[displayPoint] = mapping.getBandLevel (binLevels, displayPoint, SpectrumBinMapping::BandAggregation::peak);
    @endcode

    @see SpectrumAnalyzerComponent, SpectrogramComponent

    @tags{Audio}
*/
class YUP_API SpectrumBinMapping
{
public:
    //==============================================================================
    /** How the levels of the FFT bins covered by a display band are combined. */
    enum class BandAggregation
    {
        peak, ///< The highest interpolated level found across the band.
        sum,  ///< The levels integrated across the band width, expressed in bin units.
        mean  ///< The levels integrated across the band width and divided by it.
    };

    //==============================================================================
    /** Creates an empty mapping, which is configured with the set* methods. */
    SpectrumBinMapping() = default;

    /** Destructor. */
    ~SpectrumBinMapping() = default;

    //==============================================================================
    /** Sets the FFT size and sample rate used to convert frequencies into bin positions.

        @param newFftSize      the FFT size, the mapping covers fftSize / 2 + 1 bins
        @param newSampleRate   the sample rate in Hz
    */
    void setFftParameters (int newFftSize, double newSampleRate);

    /** Sets the displayed frequency range.

        @param newMinFrequency   the lowest displayed frequency in Hz
        @param newMaxFrequency   the highest displayed frequency in Hz
    */
    void setFrequencyRange (float newMinFrequency, float newMaxFrequency);

    /** Sets the number of display points spread logarithmically across the frequency range.

        @param newNumDisplayPoints   the number of display points, at least 2 to span the range
    */
    void setNumDisplayPoints (int newNumDisplayPoints);

    //==============================================================================
    /** Returns the number of display points. */
    int getNumDisplayPoints() const noexcept { return (int) displayRanges.size(); }

    /** Returns the FFT size used by the mapping. */
    int getFftSize() const noexcept { return fftSize; }

    /** Returns the sample rate used by the mapping. */
    double getSampleRate() const noexcept { return sampleRate; }

    /** Returns the lowest displayed frequency in Hz. */
    float getMinFrequency() const noexcept { return minFrequency; }

    /** Returns the highest displayed frequency in Hz. */
    float getMaxFrequency() const noexcept { return maxFrequency; }

    /** Returns true if the mapping spans a valid frequency range. */
    bool isValid() const noexcept;

    //==============================================================================
    /** Returns the bin range covered by a display point.

        @param displayPoint   the display point index, clamped to the valid range
    */
    const SpectrumBinRange& getRange (int displayPoint) const noexcept;

    /** Returns the frequency displayed by a display point.

        @param displayPoint   the display point index, clamped to the valid range
    */
    float getFrequencyForDisplayPoint (int displayPoint) const noexcept;

    //==============================================================================
    /** Returns the linear level of an arbitrary fractional bin position.

        @param binLevels   the non-negative linear level of every FFT bin, from bin 0 to Nyquist
        @param exactBin    the fractional bin position
    */
    static float getInterpolatedLevel (Span<const float> binLevels, float exactBin) noexcept;

    /** Returns the level of the band covered by a display point.

        @param binLevels      the non-negative linear level of every FFT bin, from bin 0 to Nyquist
        @param displayPoint   the display point index
        @param aggregation    how the levels of the bins inside the band are combined
    */
    float getBandLevel (Span<const float> binLevels, int displayPoint, BandAggregation aggregation) const noexcept;

private:
    //==============================================================================
    static float levelToDecibels (float level) noexcept;
    static float decibelsToLevel (float decibels) noexcept;

    void updateRanges();

    //==============================================================================
    static constexpr float minLevelDecibels = -300.0f;

    std::vector<SpectrumBinRange> displayRanges;
    int fftSize = 0;
    double sampleRate = 0.0;
    float minFrequency = 0.0f;
    float maxFrequency = 0.0f;
    float logMinFrequency = 0.0f;
    float logMaxFrequency = 0.0f;
    int numDisplayPoints = 0;
};

} // namespace yup
