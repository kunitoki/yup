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
        Updates the smoothed value of the parameter from its atomic value.

        Call once at the start of each audio block when not using sample-accurate
        automation. For sample-accurate automation use prepareBlock() and
        advanceToSample() instead.

        @returns true if the parameter is currently being smoothed, false otherwise.
    */
    forcedinline bool updateNextAudioBlock() noexcept
    {
        jassert (parameter != nullptr);

        smoothed.setTargetValue (parameter->getValue());

        return smoothed.isSmoothing();
    }

    /**
        Prepares this handle for sample-accurate automation in a processing block.

        Call once at the start of processBlock() in place of updateNextAudioBlock()
        when you intend to use advanceToSample(). Syncs the smoother from the
        parameter's current atomic value and stores a reference to the automation
        buffer so advanceToSample() can apply changes at exact sample positions.

        @param changes   The per-block automation buffer from AudioProcessContext::params.
        @param paramIdx  Index of this parameter — use AudioParameter::getIndexInContainer().
    */
    forcedinline void prepareBlock (const ParameterChangeBuffer& changes, int paramIdx) noexcept
    {
        jassert (parameter != nullptr);

        blockChanges = std::addressof (changes);
        myParamIndex = paramIdx;
        nextChangePtr = changes.begin();

        smoothed.setTargetValue (parameter->getValue());
    }

    /**
        Applies pending automation events up to and including @p samplePosition.

        Call at each sub-block boundary in your event-driven processing loop alongside
        MIDI event iteration. Returns true if at least one automation change was applied
        so the processing loop can react immediately (e.g. re-compute a coefficient).

        The smoother is retargeted to the new parameter value at each change point, so
        getNextValue() continues to produce a smooth ramp even under automation.

        @param samplePosition  Current sample offset within the block.
        @returns true if at least one change was applied.
    */
    forcedinline bool advanceToSample (int samplePosition) noexcept
    {
        if (blockChanges == nullptr || parameter == nullptr)
            return false;

        bool changed = false;

        while (nextChangePtr != blockChanges->end()
               && nextChangePtr->sampleOffset <= samplePosition)
        {
            if (nextChangePtr->parameterIndex == myParamIndex)
            {
                parameter->setNormalizedValue (nextChangePtr->normalizedValue);
                smoothed.setTargetValue (parameter->getValue());
                changed = true;
            }

            ++nextChangePtr;
        }

        return changed;
    }

    /** Returns the next smoothed value of the parameter. */
    forcedinline float getNextValue() noexcept
    {
        return smoothed.getNextValue();
    }

    /** Returns the current smoothed value of the parameter without advancing. */
    forcedinline float getCurrentValue() const noexcept
    {
        return smoothed.getCurrentValue();
    }

    /**
        Skips the next numSamples samples of the parameter.

        Equivalent to calling getNextValue() numSamples times.

        @param numSamples The number of samples to skip.
        @returns The current value after skipping.
    */
    forcedinline float skip (int numSamples) noexcept
    {
        return smoothed.skip (numSamples);
    }

private:
    AudioParameter* parameter = nullptr;
    SmoothedValue<float, ValueSmoothingTypes::Linear> smoothed;

    const ParameterChangeBuffer* blockChanges = nullptr;
    const ParameterChange* nextChangePtr = nullptr;
    int myParamIndex = -1;
};

} // namespace yup
