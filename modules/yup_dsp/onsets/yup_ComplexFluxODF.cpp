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
void ComplexFluxODF::prepare (const Parameters& p, const float* window, int windowSize, int hopSize)
{
    jassert (window != nullptr);
    jassert (windowSize > 0 && hopSize > 0);
    jassert (p.maxFilterBins > 0);
    jassert (p.temporalFilter >= 0);

    maxFilterBins = p.maxFilterBins;
    maxFilterHalf = maxFilterBins / 2;
    temporalFilter = p.temporalFilter;
    temporalOrigin = p.temporalOrigin;

    if (p.diffFrames > 0)
        diffFrames = p.diffFrames;
    else
        diffFrames = SuperFluxODF::deriveDiffFrames (window, windowSize, p.windowMagRatio, hopSize);

    jassert (diffFrames >= 1);
    activations.clear();
}

//==============================================================================
void ComplexFluxODF::reset()
{
    activations.clear();
}

//==============================================================================
void ComplexFluxODF::compute (const Spectrogram& spec)
{
    const int numFrames = spec.getNumFrames();
    const int numBins = spec.getNumBins();
    const int numRawBins = spec.getNumRawBins();

    activations.assign (static_cast<std::size_t> (numFrames), 0.0f);

    if (numFrames <= diffFrames || numBins <= 0)
        return;

    const float* magnitude = spec.getMagnitudeData();
    const float* lgd = spec.getLGDData();

    if (lgd == nullptr)
        return;

    // --- Step 1: Compute absolute LGD and apply temporal max filter ---
    std::vector<float> absLgd (static_cast<std::size_t> (numFrames) * static_cast<std::size_t> (numRawBins));

    for (int f = 0; f < numFrames; ++f)
    {
        for (int bin = 0; bin < numRawBins; ++bin)
            absLgd[static_cast<std::size_t> (f) * static_cast<std::size_t> (numRawBins)
                   + static_cast<std::size_t> (bin)] = std::abs (lgd[static_cast<std::size_t> (f) * static_cast<std::size_t> (numRawBins)
                                                                     + static_cast<std::size_t> (bin)]);
    }

    if (temporalFilter > 0)
    {
        const int half = temporalFilter / 2;
        std::vector<float> filteredLgd (absLgd.size());

        for (int f = 0; f < numFrames; ++f)
        {
            for (int bin = 0; bin < numRawBins; ++bin)
            {
                float maxVal = 0.0f;
                const int fStart = jmax (0, f + temporalOrigin - half);
                const int fEnd = jmin (numFrames - 1, f + temporalOrigin + half);

                for (int tf = fStart; tf <= fEnd; ++tf)
                {
                    const float v = absLgd[static_cast<std::size_t> (tf) * static_cast<std::size_t> (numRawBins)
                                           + static_cast<std::size_t> (bin)];
                    if (v > maxVal)
                        maxVal = v;
                }

                filteredLgd[static_cast<std::size_t> (f) * static_cast<std::size_t> (numRawBins)
                            + static_cast<std::size_t> (bin)] = maxVal;
            }
        }

        absLgd.swap (filteredLgd);
    }

    // --- Step 2: Create LGD weighting mask at numBins resolution ---
    std::vector<float> mask (static_cast<std::size_t> (numFrames) * static_cast<std::size_t> (numBins));
    const FilterBank* filterBank = spec.getFilterBank();

    if (filterBank != nullptr)
    {
        const float* filterMatrix = filterBank->getMatrixData();
        const int fbBands = filterBank->getNumBands();
        const int fbBins = filterBank->getNumFFTBins();

        for (int b = 0; b < numBins; ++b)
        {
            // Find the corner bins for this band
            int startBin = numRawBins;
            int stopBin = 0;

            for (int bin = 0; bin < fbBins; ++bin)
            {
                if (filterMatrix[static_cast<std::size_t> (bin) * static_cast<std::size_t> (fbBands)
                                 + static_cast<std::size_t> (b)]
                    > 0.0f)
                {
                    if (bin < startBin)
                        startBin = bin;
                    if (bin > stopBin)
                        stopBin = bin;
                }
            }

            startBin = jmax (0, startBin - 1);
            stopBin = jmin (numRawBins - 1, stopBin + 1);

            for (int f = 0; f < numFrames; ++f)
            {
                float minVal = std::numeric_limits<float>::max();

                for (int bin = startBin; bin <= stopBin; ++bin)
                {
                    const float v = absLgd[static_cast<std::size_t> (f) * static_cast<std::size_t> (numRawBins)
                                           + static_cast<std::size_t> (bin)];
                    if (v < minVal)
                        minVal = v;
                }

                mask[static_cast<std::size_t> (f) * static_cast<std::size_t> (numBins)
                     + static_cast<std::size_t> (b)] = minVal / MathConstants<float>::pi;
            }
        }
    }
    else
    {
        // No filter bank: min over current bin +- 1
        for (int f = 0; f < numFrames; ++f)
        {
            for (int bin = 0; bin < numRawBins; ++bin)
            {
                const int s = jmax (0, bin - 1);
                const int e = jmin (numRawBins - 1, bin + 1);

                float minVal = std::numeric_limits<float>::max();
                for (int k = s; k <= e; ++k)
                {
                    const float v = absLgd[static_cast<std::size_t> (f) * static_cast<std::size_t> (numRawBins)
                                           + static_cast<std::size_t> (k)];
                    if (v < minVal)
                        minVal = v;
                }

                mask[static_cast<std::size_t> (f) * static_cast<std::size_t> (numRawBins)
                     + static_cast<std::size_t> (bin)] = minVal / MathConstants<float>::pi;
            }
        }
    }

    // --- Step 3: Compute diff spectrogram (SuperFlux-style) ---
    std::vector<float> diffSpec (static_cast<std::size_t> (numFrames) * static_cast<std::size_t> (numBins), 0.0f);

    for (int frame = diffFrames; frame < numFrames; ++frame)
    {
        const int prevFrame = frame - diffFrames;

        for (int band = 0; band < numBins; ++band)
        {
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

            float diff = magnitude[static_cast<std::size_t> (frame) * static_cast<std::size_t> (numBins)
                                   + static_cast<std::size_t> (band)]
                       - prevMax;

            diffSpec[static_cast<std::size_t> (frame) * static_cast<std::size_t> (numBins)
                     + static_cast<std::size_t> (band)] = (diff > 0.0f) ? diff : 0.0f;
        }
    }

    // --- Step 4: Weight diff spec by LGD mask and sum ---
    for (int frame = diffFrames; frame < numFrames; ++frame)
    {
        float sum = 0.0f;

        for (int band = 0; band < numBins; ++band)
        {
            const auto idx = static_cast<std::size_t> (frame) * static_cast<std::size_t> (numBins)
                           + static_cast<std::size_t> (band);

            sum += diffSpec[idx] * mask[idx];
        }

        activations[static_cast<std::size_t> (frame)] = sum;
    }
}

} // namespace yup
