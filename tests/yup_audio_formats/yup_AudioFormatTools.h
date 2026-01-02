/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#include <yup_audio_formats/yup_audio_formats.h>

using namespace yup;

struct AudioValidationResult
{
    bool hasClippedSamples = false;
    bool hasExtremeValues = false;
    float maxAbsValue = 0.0f;
    float minValue = 0.0f;
    float maxValue = 0.0f;
    int clippedSampleCount = 0;
    int extremeValueCount = 0;
};

inline AudioValidationResult validateAudioData (AudioFormatReader& reader)
{
    AudioValidationResult result;

    if (reader.lengthInSamples <= 0)
        return result;

    const int bufferSize = 4096;
    AudioBuffer<float> buffer (static_cast<int> (reader.numChannels), bufferSize);

    int64 samplesRemaining = reader.lengthInSamples;
    int64 currentPos = 0;

    while (samplesRemaining > 0)
    {
        const int samplesToRead = static_cast<int> (std::min ((int64) bufferSize, samplesRemaining));

        if (! reader.read (&buffer, 0, samplesToRead, currentPos, true, true))
            break;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* channelData = buffer.getReadPointer (ch);

            for (int sample = 0; sample < samplesToRead; ++sample)
            {
                const float value = channelData[sample];
                const float absValue = std::abs (value);

                result.minValue = std::min (result.minValue, value);
                result.maxValue = std::max (result.maxValue, value);
                result.maxAbsValue = std::max (result.maxAbsValue, absValue);

                const float clipThreshold = 1.0001f;
                if (absValue > clipThreshold)
                {
                    result.hasClippedSamples = true;
                    result.clippedSampleCount++;
                }

                const float extremeThreshold = 10.0f;
                if (absValue > extremeThreshold)
                {
                    result.hasExtremeValues = true;
                    result.extremeValueCount++;
                }
            }
        }

        currentPos += samplesToRead;
        samplesRemaining -= samplesToRead;
    }

    return result;
}
