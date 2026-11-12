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

#pragma once

namespace yup
{

//==============================================================================
/** A lock-free set of changed parameter indices.

    Parameters mark themselves here whenever their value changes, from any thread, and an
    editor drains the set on the message thread to refresh only the controls that moved,
    instead of polling every parameter every frame.

    It is a bitset: marking a parameter is a single lock-free atomic OR, safe from
    any number of threads (audio, host and message thread alike) and naturally coalescing. A
    parameter changed many times between two drains is reported once, and the consumer reads
    its latest value. Nothing is ever dropped.

    markChanged() may be called from any thread; readChanges() from one consumer thread only.
    resize() and ensureSize() are neither and must run before any parameter changes concurrently, as
    AudioProcessorBase::addParameter() does while the processor is being built.

    @see AudioProcessorBase, AudioProcessorEditor::startParameterListening, AudioParameterAttachment
*/
class YUP_API ParameterChangeQueue
{
public:
    //==============================================================================
    /** Creates a queue sized for a number of parameters. */
    explicit ParameterChangeQueue (int numParameters = 0);

    //==============================================================================
    /** Resizes the queue to hold a number of parameters and clears it. Not thread-safe. */
    void resize (int numParameters);

    /** Grows the queue without clearing pending changes. Not thread-safe.

        Storage grows geometrically and is reused until another allocation is needed.
        Requests smaller than the current size have no effect.
    */
    void ensureSize (int numParameters);

    /** Returns the number of parameters the queue is sized for. */
    int getNumParameters() const noexcept { return numParameters; }

    //==============================================================================
    /** Marks a parameter as changed. Any thread, lock-free. Out of range indices are ignored. */
    void markChanged (int parameterIndex) noexcept;

    //==============================================================================
    /** Calls fn(int) once for every parameter marked since the last call. Consumer thread only.

        Each word of the set is cleared before its indices are reported, so a change made
        during the drain is reported by this drain or the next one rather than lost.
        The scan costs O(numParameters / 32 + changed parameters). There must be only one
        consumer; when editors are listening, leave draining to their shared dispatcher.
    */
    template <class Fn>
    void readChanges (Fn&& fn)
    {
        const auto numWords = (static_cast<std::size_t> (numParameters) + bitsPerWord - 1) / bitsPerWord;

        for (std::size_t word = 0; word < numWords; ++word)
        {
            if (words[word].load (std::memory_order_relaxed) == 0)
                continue;

            for (auto bits = words[word].exchange (0, std::memory_order_acquire); bits != 0; bits &= bits - 1)
                fn (static_cast<int> (word * bitsPerWord) + std::countr_zero (bits));
        }
    }

private:
    static constexpr int bitsPerWord = 32;
    static_assert (std::atomic<uint32>::is_always_lock_free);

    std::vector<std::atomic<uint32>> words;
    int numParameters = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterChangeQueue)
};

} // namespace yup
