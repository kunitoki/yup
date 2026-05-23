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

/**
    A single parameter automation event with a sample-accurate position within a block.

    @see ParameterChangeBuffer
*/
struct ParameterChange
{
    /** Index into AudioProcessor::getParameters(). */
    int parameterIndex = 0;

    /** Normalized value in [0, 1]. */
    float normalizedValue = 0.0f;

    /** Sample position within the current processing block, in [0, blockSize). */
    int sampleOffset = 0;
};

//==============================================================================

/**
    A pre-allocated, sorted buffer of intra-block parameter automation events.

    All memory is reserved at prepare time so that addChange() and clear() never
    allocate on the audio thread. Exceeding the reserved capacity fires an assertion
    in debug builds and silently drops the excess event in release builds, preserving
    realtime safety.

    Typical usage pattern:
    @code
        // At prepare time (not on audio thread):
        paramBuf.reserve (processor.getParameters().size() * 4 + 32);

        // Per block (audio thread):
        paramBuf.clear();
        for (auto& automationPoint : hostAutomationPoints)
            paramBuf.addChange (automationPoint.paramIdx,
                                automationPoint.normalizedValue,
                                automationPoint.sampleOffset);
        paramBuf.sort();

        AudioProcessContext<float> ctx { audioBuffer, midiBuffer, paramBuf, transportPosition };
        processor.processBlock (ctx);
    @endcode

    @see ParameterChange, AudioProcessContext, AudioParameterHandle
*/
class ParameterChangeBuffer
{
public:
    //==============================================================================

    /** Reserves capacity for automation events.

        Call at prepare time (not on the audio thread). A good default is
        @code numParams * 4 + 32 @endcode for manual automation, or
        @code numParams * blockSize @endcode for fully sample-accurate automation.

        @param maxChanges  Maximum number of events per processing block.
    */
    void reserve (int maxChanges)
    {
        changes.reserve (static_cast<size_t> (maxChanges));
    }

    //==============================================================================

    /** Clears all events without releasing memory. Safe to call on the audio thread. */
    void clear() noexcept
    {
        changes.clear();
    }

    /** Returns true when the buffer contains no events. */
    bool isEmpty() const noexcept
    {
        return changes.empty();
    }

    /** Returns the number of events currently held. */
    int getNumChanges() const noexcept
    {
        return static_cast<int> (changes.size());
    }

    //==============================================================================

    /** Adds a parameter automation event.

        Safe on the audio thread when the buffer was reserved with sufficient capacity.
        If the capacity is exceeded the event is dropped and a debug assertion fires.

        @param parameterIndex   Index into AudioProcessor::getParameters().
        @param normalizedValue  Value in [0, 1].
        @param sampleOffset     Sample position within the current block.
        @return true if the event was added, false if it was dropped.
    */
    bool addChange (int parameterIndex, float normalizedValue, int sampleOffset) noexcept
    {
        if (changes.size() >= changes.capacity())
        {
            jassertfalse; // Increase reserved capacity at prepare time
            return false;
        }

        changes.push_back ({ parameterIndex, normalizedValue, sampleOffset });
        return true;
    }

    /** Sorts events by sampleOffset in ascending order.

        Call once after filling the buffer for a block, before passing the buffer to
        processBlock(). Uses std::sort which is in-place and allocation-free.
    */
    void sort() noexcept
    {
        std::sort (changes.begin(), changes.end(), [] (const ParameterChange& a, const ParameterChange& b) noexcept
        {
            return a.sampleOffset < b.sampleOffset;
        });
    }

    //==============================================================================

    /** Returns a pointer to the first event (sorted by sampleOffset). */
    const ParameterChange* begin() const noexcept
    {
        return changes.data();
    }

    /** Returns a pointer one past the last event. */
    const ParameterChange* end() const noexcept
    {
        return changes.data() + changes.size();
    }

    /** Returns a pointer to the first event whose sampleOffset >= samplePosition. */
    const ParameterChange* findNextSamplePosition (int samplePosition) const noexcept
    {
        return std::lower_bound (begin(), end(), samplePosition, [] (const ParameterChange& change, int sample) noexcept
        {
            return change.sampleOffset < sample;
        });
    }

private:
    std::vector<ParameterChange> changes;
};

} // namespace yup
