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

#include <gtest/gtest.h>

#include <yup_audio_basics/yup_audio_basics.h>

using namespace yup;

//==============================================================================
namespace
{
#if YUP_WASM
using ExpectedAudioLockType = RecursiveSpinLock;
#else
using ExpectedAudioLockType = CriticalSection;
#endif
} // namespace

//==============================================================================
TEST (AudioLockType, ResolvesToThePlatformAppropriateLock)
{
    static_assert (std::is_same_v<AudioLockType, ExpectedAudioLockType>);
    EXPECT_TRUE ((std::is_same_v<AudioLockType, ExpectedAudioLockType>) );
}

TEST (AudioLockType, ScopedLockAcquiresAndReleases)
{
    AudioLockType lock;

    {
        const AudioLockType::ScopedLockType sl (lock);
        EXPECT_TRUE (lock.tryEnter()); // re-entrant on the same thread, so it succeeds
        lock.exit();
    }

    EXPECT_TRUE (lock.tryEnter()); // released again after the scoped lock went out of scope
    lock.exit();
}

TEST (AudioLockType, ReentrantAcquisitionOnTheSameThread)
{
    AudioLockType lock;

    lock.enter();
    lock.enter(); // must succeed without deadlocking on both CriticalSection and RecursiveSpinLock
    lock.exit();
    lock.exit();

    EXPECT_TRUE (lock.tryEnter());
    lock.exit();
}
