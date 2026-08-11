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
void SuperFluxODF::prepare (const Parameters& p, const float* window, int windowSize, int hopSize)
{
    jassert (window != nullptr);
    jassert (windowSize > 0 && hopSize > 0);
    jassert (p.maxFilterBins > 0);

    maxFilterBins = p.maxFilterBins;
    maxFilterHalf = maxFilterBins / 2;

    if (p.diffFrames > 0)
        diffFrames = p.diffFrames;
    else
        diffFrames = deriveDiffFrames (window, windowSize, p.windowMagRatio, hopSize);

    jassert (diffFrames >= 1);

    activations.clear();
}

//==============================================================================
void SuperFluxODF::reset()
{
    activations.clear();
}

//==============================================================================
void SuperFluxODF::compute (const Spectrogram& spec)
{
    const int numFrames = spec.getNumFrames();
    const int numBins = spec.getNumBins();

    activations.assign (static_cast<std::size_t> (numFrames), 0.0f);

    if (numFrames <= diffFrames || numBins <= 0)
        return;

    const float* magnitude = spec.getMagnitudeData();

    for (int frame = diffFrames; frame < numFrames; ++frame)
    {
        const int prevFrame = frame - diffFrames;
        float sum = 0.0f;

        for (int band = 0; band < numBins; ++band)
        {
            // Maximum filter with shifting window (from reference implementation)
            int first = band - maxFilterHalf;
            int last = first + maxFilterBins - 1;

            if (first < 0)
            {
                last -= first;
                first = 0;
            }

            if (last >= numBins)
            {
                first = jmax (0, first - (last - numBins + 1));
                last = numBins - 1;
            }

            float prevMax = magnitude[static_cast<std::size_t> (prevFrame) * static_cast<std::size_t> (numBins)
                                      + static_cast<std::size_t> (band)];

            for (int k = first; k <= last; ++k)
            {
                const float val = magnitude[static_cast<std::size_t> (prevFrame) * static_cast<std::size_t> (numBins)
                                            + static_cast<std::size_t> (k)];

                if (val > prevMax)
                    prevMax = val;
            }

            const float diff = magnitude[static_cast<std::size_t> (frame) * static_cast<std::size_t> (numBins)
                                         + static_cast<std::size_t> (band)]
                             - prevMax;

            if (diff > 0.0f)
                sum += diff;
        }

        activations[static_cast<std::size_t> (frame)] = sum;
    }
}

//==============================================================================
void SuperFluxODF::prepareStreaming (int numBins)
{
    jassert (numBins > 0);
    jassert (diffFrames >= 1); // prepare() must have run first

    streamNumBins = numBins;
    streamHistory.assign (static_cast<std::size_t> (diffFrames + 1) * static_cast<std::size_t> (numBins), 0.0f);

    resetStreaming();
}

void SuperFluxODF::resetStreaming() noexcept
{
    std::fill (streamHistory.begin(), streamHistory.end(), 0.0f);
    streamFramesSeen = 0;
}

float SuperFluxODF::computeStreamingFrame (const float* frameMagnitudes, int numBins) noexcept
{
    jassert (frameMagnitudes != nullptr);
    jassert (numBins == streamNumBins);
    jassert (! streamHistory.empty()); // prepareStreaming() not called

    const int historyFrames = diffFrames + 1;
    const auto slot = static_cast<std::size_t> (streamFramesSeen % historyFrames);

    std::copy (frameMagnitudes,
               frameMagnitudes + numBins,
               streamHistory.data() + slot * static_cast<std::size_t> (numBins));

    const auto frameIndex = streamFramesSeen;
    ++streamFramesSeen;

    // Matches compute(): the first diffFrames frames have no reference frame
    // and produce zero activation.
    if (frameIndex < diffFrames)
        return 0.0f;

    const auto prevSlot = static_cast<std::size_t> ((frameIndex - diffFrames) % historyFrames);
    const float* previous = streamHistory.data() + prevSlot * static_cast<std::size_t> (numBins);
    const float* current = streamHistory.data() + slot * static_cast<std::size_t> (numBins);

    float sum = 0.0f;

    for (int band = 0; band < numBins; ++band)
    {
        // Maximum filter with shifting window, identical to compute().
        int first = band - maxFilterHalf;
        int last = first + maxFilterBins - 1;

        if (first < 0)
        {
            last -= first;
            first = 0;
        }

        if (last >= numBins)
        {
            first = jmax (0, first - (last - numBins + 1));
            last = numBins - 1;
        }

        float prevMax = previous[band];

        for (int k = first; k <= last; ++k)
        {
            if (previous[k] > prevMax)
                prevMax = previous[k];
        }

        const float diff = current[band] - prevMax;

        if (diff > 0.0f)
            sum += diff;
    }

    return sum;
}

//==============================================================================
int SuperFluxODF::deriveDiffFrames (const float* window, int windowSize, float ratio, int hopSize)
{
    jassert (window != nullptr);
    jassert (ratio >= 0.0f && ratio <= 1.0f);

    int sample = 0;
    while (sample < windowSize / 2 && window[sample] <= ratio)
        ++sample;

    const int diffSamples = windowSize / 2 - sample;
    return jmax (1, static_cast<int> (std::round (static_cast<float> (diffSamples) / static_cast<float> (hopSize))));
}

} // namespace yup
