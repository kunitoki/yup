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
void OnsetPeakPicker::prepare (const Parameters& p, float f)
{
    jassert (f > 0.0f);
    jassert (p.threshold >= 0.0f);
    jassert (p.combineSec >= 0.0f);
    jassert (p.preAvgSec >= 0.0f);
    jassert (p.preMaxSec >= 0.0f);
    jassert (p.postAvgSec >= 0.0f);
    jassert (p.postMaxSec >= 0.0f);

    params = p;
    fps = f;

    preAvgLen = jmax (0, static_cast<int> (std::lround (static_cast<double> (fps) * p.preAvgSec)));
    preMaxLen = jmax (0, static_cast<int> (std::lround (static_cast<double> (fps) * p.preMaxSec)));

    if (p.onlineMode)
    {
        postAvgLen = 0;
        postMaxLen = 0;
    }
    else
    {
        postAvgLen = jmax (0, static_cast<int> (std::lround (static_cast<double> (fps) * p.postAvgSec)));
        postMaxLen = jmax (0, static_cast<int> (std::lround (static_cast<double> (fps) * p.postMaxSec)));
    }

    onsetTimes.clear();
}

//==============================================================================
void OnsetPeakPicker::reset()
{
    onsetTimes.clear();
}

//==============================================================================
void OnsetPeakPicker::detect (const float* activations, int numFrames)
{
    jassert (activations != nullptr);
    jassert (numFrames >= 0);

    onsetTimes.clear();

    if (numFrames == 0)
        return;

    const double combineSeconds = static_cast<double> (params.combineSec);
    const double delaySeconds = static_cast<double> (params.delaySec);

    double lastDetection = -std::numeric_limits<double>::infinity();

    for (int frame = 0; frame < numFrames; ++frame)
    {
        // Skip non-positive activations
        if (activations[frame] <= 0.0f)
            continue;

        // --- Moving maximum ---
        const int maxStart = jmax (0, frame - preMaxLen);
        const int maxStop = jmin (numFrames - 1, frame + postMaxLen);

        float movMax = 0.0f;
        for (int i = maxStart; i <= maxStop; ++i)
        {
            if (activations[i] > movMax)
                movMax = activations[i];
        }

        // Must be the local maximum
        if (activations[frame] != movMax)
            continue;

        // --- Moving average ---
        const int avgStart = jmax (0, frame - preAvgLen);
        const int avgStop = jmin (numFrames - 1, frame + postAvgLen);

        double avg = 0.0;
        for (int i = avgStart; i <= avgStop; ++i)
            avg += activations[i];

        avg /= static_cast<double> (avgStop - avgStart + 1);

        // When the average window is empty, use absolute threshold instead
        if (preAvgLen == 0 && postAvgLen == 0)
        {
            if (static_cast<double> (activations[frame]) < static_cast<double> (params.threshold))
                continue;
        }
        else
        {
            // Must exceed average + threshold
            if (static_cast<double> (activations[frame]) < avg + static_cast<double> (params.threshold))
                continue;
        }

        // --- Convert to time and apply delay ---
        const double time = static_cast<double> (frame) / static_cast<double> (fps) + delaySeconds;

        // --- Combine suppression ---
        if (! onsetTimes.empty() && time - lastDetection <= combineSeconds)
            continue;

        onsetTimes.push_back (time);
        lastDetection = time;
    }
}

//==============================================================================
void OnsetPeakPicker::refineOnsetTimes (const float* samples, int numSamples, float sampleRate, double maxRefineSec, float threshold)
{
    jassert (samples != nullptr);
    jassert (numSamples > 0);
    jassert (sampleRate > 0.0f);
    jassert (maxRefineSec >= 0.0);
    jassert (threshold >= 0.0f && threshold <= 1.0f);

    if (samples == nullptr || numSamples <= 1 || sampleRate <= 0.0f || maxRefineSec <= 0.0 || onsetTimes.empty())
        return;

    threshold = jlimit (0.0f, 1.0f, threshold);

    const double sr = static_cast<double> (sampleRate);
    const int refineSamples = static_cast<int> (std::ceil (maxRefineSec * sr));

    if (refineSamples <= 0)
        return;

    const auto secondsToSamples = [sr] (double seconds, int minimum) -> int
    {
        return jmax (minimum, static_cast<int> (std::llround (seconds * sr)));
    };

    const int rmsWindowSamples = secondsToSamples (0.0015, 8);
    const int hopSize = jmin (rmsWindowSamples, secondsToSamples (0.00025, 1));
    const int zeroCrossLookbackSamples = jmax (rmsWindowSamples, secondsToSamples (0.0010, 1));
    constexpr float minUsefulRms = 1.0e-8f;

    std::vector<double> squarePrefix;
    std::vector<float> rmsEnv;
    std::vector<float> rmsScratch;

    const auto findPrecedingZeroLikeSample = [samples] (int searchStart, int candidate) noexcept -> int
    {
        int bestSample = candidate;
        float bestAbs = std::abs (samples[candidate]);

        for (int i = candidate; i >= searchStart; --i)
        {
            const float absValue = std::abs (samples[i]);

            if (absValue < bestAbs)
            {
                bestAbs = absValue;
                bestSample = i;
            }

            if (i <= searchStart)
                continue;

            const float a = samples[i - 1];
            const float b = samples[i];

            const bool crossedZero = (a <= 0.0f && b > 0.0f)
                                  || (a >= 0.0f && b < 0.0f);

            if (crossedZero)
                return std::abs (a) <= std::abs (b) ? i - 1 : i;
        }

        // If no true crossing exists, use the lowest-absolute-amplitude sample.
        return bestSample;
    };

    for (auto& t : onsetTimes)
    {
        if (! std::isfinite (t))
            continue;

        const int onsetSample = jlimit (0,
                                        numSamples - 1,
                                        static_cast<int> (std::llround (t * sr)));

        // Use a half-open range: [searchStart, searchEnd)
        const int searchStart = jmax (0, onsetSample - refineSamples);
        const int searchEnd = jmin (numSamples, onsetSample + refineSamples + 1);
        const int searchLen = searchEnd - searchStart;

        if (searchLen <= 1)
            continue;

        // Prefix sum of squared samples.
        // This makes RMS calculation O(1) per frame and avoids repeated inner loops.
        squarePrefix.resize (static_cast<std::size_t> (searchLen + 1));
        squarePrefix[0] = 0.0;

        for (int i = 0; i < searchLen; ++i)
        {
            const double x = static_cast<double> (samples[searchStart + i]);
            squarePrefix[static_cast<std::size_t> (i + 1)] = squarePrefix[static_cast<std::size_t> (i)] + x * x;
        }

        const int numFrames = 1 + (searchLen - 1) / hopSize;
        rmsEnv.resize (static_cast<std::size_t> (numFrames));

        float peakRms = 0.0f;
        int peakFrame = 0;

        // Causal RMS: the frame at sample N only uses samples <= N.
        // This avoids the early-onset bias caused by centered windows.
        for (int f = 0; f < numFrames; ++f)
        {
            const int frameSample = searchStart + f * hopSize;

            const int winEnd = frameSample + 1; // half-open, includes frameSample
            const int winStart = jmax (searchStart, winEnd - rmsWindowSamples);

            const int localStart = winStart - searchStart;
            const int localEnd = winEnd - searchStart;
            const int windowLen = localEnd - localStart;

            const double sumSq = squarePrefix[static_cast<std::size_t> (localEnd)]
                               - squarePrefix[static_cast<std::size_t> (localStart)];

            const float rms = static_cast<float> (
                std::sqrt (sumSq / static_cast<double> (windowLen)));

            rmsEnv[static_cast<std::size_t> (f)] = rms;

            if (rms > peakRms)
            {
                peakRms = rms;
                peakFrame = f;
            }
        }

        if (peakRms < minUsefulRms)
            continue;

        // Estimate a local floor using a low percentile rather than assuming silence.
        rmsScratch.assign (rmsEnv.begin(), rmsEnv.end());

        const int floorIndex = jlimit (
            0,
            numFrames - 1,
            static_cast<int> (std::floor (0.10 * static_cast<double> (numFrames - 1))));

        std::nth_element (rmsScratch.begin(),
                          rmsScratch.begin() + floorIndex,
                          rmsScratch.end());

        const float noiseRms = rmsScratch[static_cast<std::size_t> (floorIndex)];
        const float dynamicRms = peakRms - noiseRms;

        if (dynamicRms < minUsefulRms)
            continue;

        // Threshold now means: fraction of the local dynamic range above the floor.
        const float triggerRms = noiseRms + threshold * dynamicRms;

        // Find the start of the above-threshold region that leads into the peak.
        // This is more robust than taking the first crossing in the entire search window.
        int riseFrame = peakFrame;

        while (riseFrame > 0
               && rmsEnv[static_cast<std::size_t> (riseFrame - 1)] >= triggerRms)
        {
            --riseFrame;
        }

        double riseSample = static_cast<double> (searchStart + riseFrame * hopSize);

        // Interpolate between envelope frames for sub-hop precision.
        if (riseFrame > 0)
        {
            const float y0 = rmsEnv[static_cast<std::size_t> (riseFrame - 1)];
            const float y1 = rmsEnv[static_cast<std::size_t> (riseFrame)];

            if (y1 > y0)
            {
                const double s0 = static_cast<double> (searchStart + (riseFrame - 1) * hopSize);
                const double s1 = static_cast<double> (searchStart + riseFrame * hopSize);

                const double alpha = jlimit (
                    0.0,
                    1.0,
                    (static_cast<double> (triggerRms) - static_cast<double> (y0))
                        / (static_cast<double> (y1) - static_cast<double> (y0)));

                riseSample = s0 + alpha * (s1 - s0);
            }
        }

        const int candidateSample = jlimit (
            searchStart,
            searchEnd - 1,
            static_cast<int> (std::llround (riseSample)));

        int refinedSample = candidateSample;

        if (candidateSample == 0)
        {
            refinedSample = 0;
        }
        else
        {
            const int zeroSearchStart = jmax (
                searchStart,
                candidateSample - zeroCrossLookbackSamples);

            refinedSample = findPrecedingZeroLikeSample (zeroSearchStart, candidateSample);
        }

        t = static_cast<double> (refinedSample) / sr;
    }
}

} // namespace yup
