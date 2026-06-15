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

    if (onsetTimes.empty())
        return;

    const int refineSamples = jmax (1, static_cast<int> (maxRefineSec * sampleRate));

    for (auto& t : onsetTimes)
    {
        const int onsetSample = static_cast<int> (t * static_cast<double> (sampleRate));
        const int searchStart = jmax (0, onsetSample - refineSamples);
        const int searchEnd = jmin (numSamples - 1, onsetSample + refineSamples);

        if (searchEnd <= searchStart)
            continue;

        // Find peak absolute amplitude in the search window
        float peakAbs = 0.0f;
        for (int i = searchStart; i <= searchEnd; ++i)
            peakAbs = jmax (peakAbs, std::abs (samples[i]));

        if (peakAbs < 1e-10f)
            continue;

        const float absThreshold = threshold * peakAbs;

        // Scan forward from searchStart to find the first sample above threshold
        int refinedSample = onsetSample;
        for (int i = searchStart; i <= onsetSample; ++i)
        {
            if (std::abs (samples[i]) > absThreshold)
            {
                refinedSample = i;
                break;
            }
        }

        t = static_cast<double> (refinedSample) / static_cast<double> (sampleRate);
    }
}

} // namespace yup
