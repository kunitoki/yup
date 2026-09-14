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
bool SpectrumBinMapping::isValid() const noexcept
{
    return fftSize >= 2
        && sampleRate > 0.0
        && numDisplayPoints >= 2
        && minFrequency > 0.0f
        && maxFrequency > minFrequency;
}

void SpectrumBinMapping::setFftParameters (int newFftSize, double newSampleRate)
{
    if (fftSize == newFftSize && approximatelyEqual (sampleRate, newSampleRate))
        return;

    fftSize = jmax (0, newFftSize);
    sampleRate = jmax (0.0, newSampleRate);

    updateRanges();
}

void SpectrumBinMapping::setFrequencyRange (float newMinFrequency, float newMaxFrequency)
{
    if (approximatelyEqual (minFrequency, newMinFrequency)
        && approximatelyEqual (maxFrequency, newMaxFrequency))
        return;

    minFrequency = newMinFrequency;
    maxFrequency = newMaxFrequency;

    logMinFrequency = minFrequency > 0.0f ? std::log10 (minFrequency) : 0.0f;
    logMaxFrequency = maxFrequency > 0.0f ? std::log10 (maxFrequency) : 0.0f;

    updateRanges();
}

void SpectrumBinMapping::setNumDisplayPoints (int newNumDisplayPoints)
{
    if (numDisplayPoints == newNumDisplayPoints)
        return;

    numDisplayPoints = newNumDisplayPoints;

    updateRanges();
}

//==============================================================================
const SpectrumBinRange& SpectrumBinMapping::getRange (int displayPoint) const noexcept
{
    static const SpectrumBinRange emptyRange;

    if (displayRanges.empty())
        return emptyRange;

    return displayRanges[(size_t) jlimit (0, (int) displayRanges.size() - 1, displayPoint)];
}

float SpectrumBinMapping::getFrequencyForDisplayPoint (int displayPoint) const noexcept
{
    return getRange (displayPoint).centerFrequency;
}

//==============================================================================
float SpectrumBinMapping::levelToDecibels (float level) noexcept
{
    return level > 0.0f ? 20.0f * std::log10 (level) : minLevelDecibels;
}

float SpectrumBinMapping::decibelsToLevel (float decibels) noexcept
{
    return decibels <= minLevelDecibels ? 0.0f : std::pow (10.0f, decibels / 20.0f);
}

//==============================================================================
float SpectrumBinMapping::getInterpolatedLevel (Span<const float> binLevels, float exactBin) noexcept
{
    const int numBins = (int) binLevels.size();

    if (numBins <= 0)
        return 0.0f;

    if (numBins == 1)
        return binLevels[0];

    const int lastBin = numBins - 1;
    const float position = jlimit (0.0f, (float) lastBin, exactBin);
    const int lowerBin = jlimit (0, lastBin - 1, (int) std::floor (position));
    const float fraction = position - (float) lowerBin;

    const float lowerLevel = levelToDecibels (binLevels[(size_t) lowerBin]);
    const float upperLevel = levelToDecibels (binLevels[(size_t) (lowerBin + 1)]);
    const float linearLevel = lowerLevel + fraction * (upperLevel - lowerLevel);

    // There is no bin to the left of the first one, so the edge falls back to linear interpolation.
    if (lowerBin == 0)
        return decibelsToLevel (linearLevel);

    // Parabolic interpolation through the three bins surrounding the position, evaluated at the
    // fractional position instead of at the vertex of the parabola, so the curve stays continuous
    // while still rendering peaks at their refined height.
    const float previousLevel = levelToDecibels (binLevels[(size_t) (lowerBin - 1)]);

    if (previousLevel - 2.0f * lowerLevel + upperLevel >= 0.0f)
        return decibelsToLevel (linearLevel);

    const float interpolated = previousLevel * (0.5f * fraction * (fraction - 1.0f))
                             + lowerLevel * (1.0f - fraction * fraction)
                             + upperLevel * (0.5f * fraction * (fraction + 1.0f));

    return decibelsToLevel (interpolated);
}

//==============================================================================
float SpectrumBinMapping::getBandLevel (Span<const float> binLevels, int displayPoint, BandAggregation aggregation) const noexcept
{
    if (displayRanges.empty() || binLevels.empty())
        return 0.0f;

    const int lastBin = (int) binLevels.size() - 1;
    const auto& range = getRange (displayPoint);
    const float startBin = jlimit (0.0f, (float) lastBin, range.startBin);
    const float endBin = jlimit (startBin, (float) lastBin, range.endBin);
    const int firstBinInside = jmax (0, (int) std::ceil (startBin));
    const int lastBinInside = jmin (lastBin, (int) std::floor (endBin));

    // The band edges are evaluated at their fractional bin positions, so bins entering or leaving
    // the band never make the aggregated level jump.
    const float startLevel = getInterpolatedLevel (binLevels, startBin);

    if (aggregation == BandAggregation::peak)
    {
        float peak = jmax (startLevel, getInterpolatedLevel (binLevels, endBin));

        for (int bin = firstBinInside; bin <= lastBinInside; ++bin)
            peak = jmax (peak, binLevels[(size_t) bin]);

        return peak;
    }

    float integral = 0.0f;
    float previousPosition = startBin;
    float previousLevel = startLevel;

    for (int bin = firstBinInside; bin <= lastBinInside; ++bin)
    {
        const float binLevel = binLevels[(size_t) bin];

        integral += 0.5f * (previousLevel + binLevel) * ((float) bin - previousPosition);
        previousPosition = (float) bin;
        previousLevel = binLevel;
    }

    const float endLevel = getInterpolatedLevel (binLevels, endBin);
    integral += 0.5f * (previousLevel + endLevel) * (endBin - previousPosition);

    if (aggregation == BandAggregation::sum)
        return integral;

    return integral / jmax (1.0e-6f, endBin - startBin);
}

//==============================================================================
void SpectrumBinMapping::updateRanges()
{
    displayRanges.clear();

    if (! isValid())
        return;

    displayRanges.resize ((size_t) numDisplayPoints);

    constexpr float half = 0.5f;
    const float invLastPoint = 1.0f / (float) (numDisplayPoints - 1);
    const float logRange = logMaxFrequency - logMinFrequency;
    const float halvedStep = half * invLastPoint * logRange;
    const float binPerHz = (float) fftSize / (float) sampleRate;

    for (int displayPoint = 0; displayPoint < numDisplayPoints; ++displayPoint)
    {
        const float logFrequency = logMinFrequency + (float) displayPoint * invLastPoint * logRange;

        auto& range = displayRanges[(size_t) displayPoint];
        range.centerFrequency = std::pow (10.0f, logFrequency);
        range.exactBin = range.centerFrequency * binPerHz;

        // The band edges sit halfway (in log-frequency) between neighbouring display points, and
        // the outermost points reach the ends of the displayed range.
        const float startFrequency = displayPoint == 0
                                       ? minFrequency
                                       : std::pow (10.0f, logFrequency - halvedStep);
        const float endFrequency = displayPoint == numDisplayPoints - 1
                                     ? maxFrequency
                                     : std::pow (10.0f, logFrequency + halvedStep);

        range.startBin = startFrequency * binPerHz;
        range.endBin = endFrequency * binPerHz;
    }
}

} // namespace yup
