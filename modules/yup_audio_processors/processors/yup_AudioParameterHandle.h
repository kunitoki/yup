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

namespace yup
{

//==============================================================================

/**
    A handle for a parameter of an AudioProcessor.

    This class provides a way to access and modify the value of a parameter of an
    AudioProcessor. It also provides a way to smooth the value of the parameter.

    @see AudioParameter
*/
class AudioParameterHandle
{
public:
    /** Constructs a new AudioParameterHandle. */
    AudioParameterHandle() = default;

    /** Constructs a new AudioParameterHandle. */
    AudioParameterHandle (AudioParameter& parameter, double sampleRate)
        : parameter (std::addressof (parameter))
    {
        if (parameter.isSmoothingEnabled())
        {
            smoothed.reset (sampleRate, parameter.getSmoothingTimeMs() / 1000.0);
        }
        else
        {
            smoothed.reset (sampleRate, 0.0f);
        }

        smoothed.setCurrentAndTargetValue (parameter.getValue());
    }

    /** Constructs a new AudioParameterHandle. */
    AudioParameterHandle (const AudioParameterHandle& other) = default;

    /** Constructs a new AudioParameterHandle. */
    AudioParameterHandle& operator= (const AudioParameterHandle& other) = default;

    /** Destructs the AudioParameterHandle. */
    ~AudioParameterHandle() = default;

    /**
        Updates the smoothed value of the parameter.

        This must be called on the audio thread once per audio block.

        @returns true if the parameter is currently being smoothed, false otherwise.
    */
    forcedinline bool updateNextAudioBlock() noexcept
    {
        jassert (parameter != nullptr);

        smoothed.setTargetValue (parameter->getValue());

        return smoothed.isSmoothing();
    }

    /** Returns the next value of the parameter. */
    forcedinline float getNextValue() noexcept
    {
        return smoothed.getNextValue();
    }

    /** Returns the current value of the parameter. */
    forcedinline float getCurrentValue() const noexcept
    {
        return smoothed.getCurrentValue();
    }

    /**
        Skips the next numSamples samples of the parameter.

        This is identical to calling getNextValue numSamples times.

        @param numSamples The number of samples to skip.

        @returns The current value of the parameter after skipping the samples.
    */
    forcedinline float skip (int numSamples) noexcept
    {
        return smoothed.skip (numSamples);
    }

private:
    AudioParameter* parameter = nullptr;
    SmoothedValue<float, ValueSmoothingTypes::Linear> smoothed;
};

} // namespace yup
