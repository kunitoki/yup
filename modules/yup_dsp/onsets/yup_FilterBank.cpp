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

void FilterBank::build (int bandsPerOctave, float fMin, float fMax, int numFFTBins_, float sampleRate, bool equalizeArea)
{
    jassert (bandsPerOctave > 0);
    jassert (numFFTBins_ > 0);
    jassert (sampleRate > 0.0f);

    numFFTBins = numFFTBins_;

    const auto nyquist = sampleRate * 0.5f;
    if (fMax > nyquist)
        fMax = nyquist;

    auto frequencies = generateFrequencies (bandsPerOctave, fMin, fMax);

    const float factor = nyquist / static_cast<float> (numFFTBins);

    for (auto& f : frequencies)
        f = std::round (f / factor);

    std::vector<float> uniqueFrequencies;
    {
        float prev = -1.0f;
        for (auto f : frequencies)
        {
            if (f != prev)
                uniqueFrequencies.push_back (f);
            prev = f;
        }
    }

    frequencies = std::move (uniqueFrequencies);
    frequencies.erase (
        std::remove_if (frequencies.begin(), frequencies.end(), [&] (float f)
    {
        return f >= static_cast<float> (numFFTBins);
    }),
        frequencies.end());

    const int bands = static_cast<int> (frequencies.size()) - 2;
    jassert (bands >= 3);

    matrix.assign (static_cast<std::size_t> (numFFTBins) * static_cast<std::size_t> (bands), 0.0f);
    numBands = bands;

    for (int band = 0; band < bands; ++band)
    {
        const int start = static_cast<int> (frequencies[static_cast<std::size_t> (band)]);
        const int mid = static_cast<int> (frequencies[static_cast<std::size_t> (band) + 1]);
        const int stop = static_cast<int> (frequencies[static_cast<std::size_t> (band) + 2]);

        if (mid <= start || stop <= mid)
            continue;

        const float height = equalizeArea ? 2.0f / static_cast<float> (stop - start) : 1.0f;

        for (int bin = start; bin < mid; ++bin)
        {
            const float t = static_cast<float> (bin - start) / static_cast<float> (mid - start);
            matrix[static_cast<std::size_t> (bin) * static_cast<std::size_t> (numBands)
                   + static_cast<std::size_t> (band)] = t * height;
        }

        for (int bin = mid; bin < stop; ++bin)
        {
            const float t = static_cast<float> (bin - mid) / static_cast<float> (stop - mid);
            matrix[static_cast<std::size_t> (bin) * static_cast<std::size_t> (numBands)
                   + static_cast<std::size_t> (band)] = (1.0f - t) * height;
        }
    }
}

void FilterBank::applySingleFrame (const float* magnitudeIn, float* magnitudeOut) const noexcept
{
    jassert (magnitudeIn != nullptr && magnitudeOut != nullptr);

    for (int band = 0; band < numBands; ++band)
    {
        float sum = 0.0f;

        for (int bin = 0; bin < numFFTBins; ++bin)
            sum += magnitudeIn[bin] * matrix[static_cast<std::size_t> (bin) * static_cast<std::size_t> (numBands) + static_cast<std::size_t> (band)];

        magnitudeOut[band] = sum;
    }
}

void FilterBank::applyMultipleFrames (const float* spectrogram, float* filtered, int numFrames) const noexcept
{
    jassert (spectrogram != nullptr && filtered != nullptr);
    jassert (numFrames > 0);

    for (int frame = 0; frame < numFrames; ++frame)
    {
        applySingleFrame (spectrogram + static_cast<std::size_t> (frame) * static_cast<std::size_t> (numFFTBins),
                          filtered + static_cast<std::size_t> (frame) * static_cast<std::size_t> (numBands));
    }
}

std::vector<float> FilterBank::generateFrequencies (int bandsPerOctave, float fMin, float fMax)
{
    constexpr float a = 440.0f;
    const float factor = std::pow (2.0f, 1.0f / static_cast<float> (bandsPerOctave));

    std::vector<float> frequencies;
    frequencies.push_back (a);

    float freq = a;
    while (freq <= fMax)
    {
        freq *= factor;
        frequencies.push_back (freq);
    }

    freq = a;
    while (freq >= fMin)
    {
        freq /= factor;
        frequencies.push_back (freq);
    }

    std::sort (frequencies.begin(), frequencies.end());
    return frequencies;
}

} // namespace yup
