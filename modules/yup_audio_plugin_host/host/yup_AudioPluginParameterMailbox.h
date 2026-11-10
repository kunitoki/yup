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
/** Lock-free, coalescing hand-off of parameter values and gestures between threads.

    One slot per parameter: writers only store atomics, so set() and post() are safe on
    the audio thread. flush() reports each pending slot once, with its latest value.
    Resize only while no other thread uses the mailbox.
*/
class AudioPluginParameterMailbox
{
public:
    enum Flags : uint8
    {
        valueChanged = 1 << 0,
        gestureBegan = 1 << 1,
        gestureEnded = 1 << 2
    };

    void resize (int newSize)
    {
        size = jmax (0, newSize);
        slots = std::make_unique<Slot[]> (static_cast<std::size_t> (size));
        anyPending.store (false);
    }

    void set (int index, double value) noexcept
    {
        if (! isPositiveAndBelow (index, size))
            return;

        slots[static_cast<std::size_t> (index)].value.store (value, std::memory_order_relaxed);
        post (index, valueChanged);
    }

    void post (int index, uint8 flags) noexcept
    {
        if (! isPositiveAndBelow (index, size))
            return;

        slots[static_cast<std::size_t> (index)].flags.fetch_or (flags, std::memory_order_acq_rel);
        anyPending.store (true, std::memory_order_release);
    }

    /** Calls callback (int index, double value, uint8 flags) for every pending slot. */
    template <class Callback>
    void flush (Callback&& callback)
    {
        if (! anyPending.exchange (false, std::memory_order_acq_rel))
            return;

        for (int index = 0; index < size; ++index)
        {
            auto& slot = slots[static_cast<std::size_t> (index)];

            if (const auto flags = slot.flags.exchange (0, std::memory_order_acq_rel); flags != 0)
                callback (index, slot.value.load (std::memory_order_relaxed), flags);
        }
    }

private:
    struct Slot
    {
        std::atomic<double> value { 0.0 };
        std::atomic<uint8> flags { 0 };
    };

    std::unique_ptr<Slot[]> slots;
    int size = 0;
    std::atomic<bool> anyPending { false };
};

} // namespace yup
