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

#include <AudioToolbox/AudioToolbox.h>

#include <cstddef>
#include <memory>
#include <new>

//==============================================================================
/** Test helper owning an AudioBufferList with storage for more than one AudioBuffer.

    A plain AudioBufferList only reserves room for its first AudioBuffer, so
    describing two or more buffers requires extra storage. Assigning through
    mBuffers[1] of a stack-allocated instance therefore writes past the object —
    this helper owns a correctly sized allocation and exposes it as an AudioBufferList.
*/
class TestAudioBufferList
{
public:
    /** Creates a list describing the given number of buffers. */
    explicit TestAudioBufferList (UInt32 numBuffers)
        : storage (std::make_unique<std::byte[]> (storageSizeForNumBuffers (numBuffers))),
          list (new (storage.get()) AudioBufferList {})
    {
        list->mNumberBuffers = numBuffers;
    }

    TestAudioBufferList (const TestAudioBufferList&) = delete;
    TestAudioBufferList& operator= (const TestAudioBufferList&) = delete;
    TestAudioBufferList (TestAudioBufferList&&) = delete;
    TestAudioBufferList& operator= (TestAudioBufferList&&) = delete;

    /** Returns the underlying list. */
    AudioBufferList* get() const noexcept { return list; }

    /** Provides direct access to the underlying list. */
    AudioBufferList* operator->() const noexcept { return list; }

private:
    static std::size_t storageSizeForNumBuffers (std::size_t numBuffers) noexcept
    {
        // The allocation returned by operator new [] is suitably aligned for the list.
        static_assert (alignof (AudioBufferList) <= alignof (std::max_align_t));

        return headerSize + (numBuffers * sizeof (AudioBuffer));
    }

    // Everything before the first AudioBuffer, including any padding the struct carries.
    static constexpr std::size_t headerSize = sizeof (AudioBufferList) - sizeof (AudioBuffer);

    std::unique_ptr<std::byte[]> storage;
    AudioBufferList* list = nullptr;
};
