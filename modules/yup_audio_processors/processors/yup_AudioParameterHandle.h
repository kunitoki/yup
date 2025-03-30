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

    /** Updates the smoothed value of the parameter. */
    void update() noexcept
    {
        if (parameter == nullptr) return;

        smoothed.setTargetValue (parameter->getValue());
    }

    /** Returns the next value of the parameter. */
    float getNextValue() noexcept
    {
        return smoothed.getNextValue();
    }

    /** Returns the current value of the parameter. */
    float getCurrentValue() const noexcept
    {
        return smoothed.getCurrentValue();
    }

private:
    AudioParameter* parameter = nullptr;
    SmoothedValue<float, ValueSmoothingTypes::Linear> smoothed;
};

} // namespace yup
