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

#include "MidiMessageFifo.h"

MidiMessageFifo::MidiMessageFifo (int capacity)
    : slots (static_cast<std::size_t> (capacity))
    , fifo (capacity)
{
}

bool MidiMessageFifo::push (const yup::MidiMessage& message) noexcept
{
    const auto size = message.getRawDataSize();

    if (size <= 0 || size > 3)
        return false;

    const auto scope = fifo.write (1);

    if (scope.blockSize1 + scope.blockSize2 == 0)
        return false;

    auto& slot = slots[static_cast<std::size_t> (scope.blockSize1 > 0 ? scope.startIndex1 : scope.startIndex2)];
    std::copy_n (message.getRawData(), size, slot.bytes.begin());
    slot.size = size;

    return true;
}

void MidiMessageFifo::drainInto (yup::MidiBuffer& buffer, int samplePosition) noexcept
{
    const auto scope = fifo.read (fifo.getNumReady());

    scope.forEach ([&] (int index)
    {
        const auto& slot = slots[static_cast<std::size_t> (index)];
        buffer.addEvent (slot.bytes.data(), slot.size, samplePosition);
    });
}
