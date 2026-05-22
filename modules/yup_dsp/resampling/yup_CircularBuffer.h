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
/**
    Fixed-size circular (ring) buffer for efficient sample history access.

    Provides O(1) push and random-read operations. Logically, index 0 refers to
    the oldest element and index BufferSize - 1 to the most recently pushed one.

    Intended as a building block for block-based DSP algorithms that require a
    sliding window of sample history, such as polyphase resampling and FIR
    convolution.

    @tparam SampleType  Element type stored in the buffer.
    @tparam BufferSize  Compile-time capacity (must be > 0).
*/
template <typename SampleType, int BufferSize>
class CircularBuffer
{
public:
    static_assert (BufferSize > 0, "CircularBuffer BufferSize must be greater than zero");

    //==============================================================================
    /** Default constructor. */
    CircularBuffer() noexcept
    {
        buffer.fill (SampleType {});
    }

    /** Constructs a buffer with all entries initialised to initValue. */
    explicit CircularBuffer (SampleType initValue) noexcept
    {
        buffer.fill (initValue);
    }

    //==============================================================================
    /** Inserts value, overwriting the oldest entry and advancing the write pointer. */
    forcedinline void push (SampleType value) noexcept
    {
        buffer[static_cast<std::size_t> (oldestIndex)] = value;

        if (++oldestIndex == BufferSize)
            oldestIndex = 0;
    }

    /** Returns the element at logical index (0 = oldest, BufferSize-1 = newest). */
    forcedinline SampleType& operator[] (int index) noexcept
    {
        return buffer[static_cast<std::size_t> ((oldestIndex + index) % BufferSize)];
    }

    /** Returns the element at logical index (0 = oldest, BufferSize-1 = newest). */
    const forcedinline SampleType& operator[] (int index) const noexcept
    {
        return buffer[static_cast<std::size_t> ((oldestIndex + index) % BufferSize)];
    }

    //==============================================================================
    /** Resets all entries to zero and rewinds the write pointer. */
    void clear() noexcept
    {
        buffer.fill (SampleType {});
        oldestIndex = 0;
    }

private:
    std::array<SampleType, static_cast<std::size_t> (BufferSize)> buffer {};
    int oldestIndex = 0;
};

} // namespace yup
