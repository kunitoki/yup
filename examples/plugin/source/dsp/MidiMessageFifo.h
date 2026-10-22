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

#include <yup_audio_basics/yup_audio_basics.h>

#include <array>
#include <vector>

//==============================================================================
/** A lock-free single producer, single consumer queue of short MIDI messages.

    Carries the pitch bends and controllers the editor's wheels send to the audio thread.
    Messages longer than three bytes, such as SysEx, are refused, and so is anything pushed
    while the queue is full: neither side ever waits.
*/
class MidiMessageFifo
{
public:
    /** Allocates room for a number of messages. */
    explicit MidiMessageFifo (int capacity);

    /** Queues a message. Returns false when it was dropped. Producer thread only. */
    bool push (const yup::MidiMessage& message) noexcept;

    /** Moves every queued message into a buffer at one sample position. Consumer thread only. */
    void drainInto (yup::MidiBuffer& buffer, int samplePosition) noexcept;

private:
    struct Slot
    {
        std::array<yup::uint8, 3> bytes {};
        int size = 0;
    };

    std::vector<Slot> slots;
    yup::AbstractFifo fifo;

    YUP_DECLARE_NON_COPYABLE (MidiMessageFifo)
};
