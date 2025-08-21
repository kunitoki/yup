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
    A re-entrant spin-lock class that can be used as a simple, low-overhead mutex for uncontended situations.

    Note that unlike a CriticalSection, this type of lock is not re-entrant, and may be less efficient when used in
    a highly contended situation, but it's very small and requires almost no initialisation. It's most appropriate for
    simple situations where you're only going to hold the lock for a very brief time.

    @see CriticalSection, SpinLock

    @tags{Core}
*/
class YUP_API RecursiveSpinLock
{
public:
    inline RecursiveSpinLock() = default;
    inline ~RecursiveSpinLock() = default;

    /** Acquires the lock.

        This will block until the lock has been successfully acquired by this thread.
        Note that a RecursiveSpinLock is re-entrant, and is smart enough to know whether the
        caller thread already has the lock.

        It's strongly recommended that you never call this method directly - instead use the
        ScopedLockType class to manage the locking using an RAII pattern instead.
    */
    void enter() const noexcept;

    /** Attempts to acquire the lock, returning true if this was successful. */
    inline bool tryEnter() const noexcept
    {
        auto current = Thread::getCurrentThreadId();
        if (owner.get() == current)
        {
            ++count;
            return true;
        }

        if (! lock.compareAndSetBool (1, 0))
            return false;

        owner = current;
        count = 1;
        return true;
    }

    /** Releases the lock. */
    inline void exit() const noexcept
    {
        auto current = Thread::getCurrentThreadId();
        jassert (owner.get() == current); // Agh! Releasing a lock that isn't currently held!

        if (--count == 0)
        {
            owner = nullptr;
            lock = 0;
        }
    }

    //==============================================================================
    /** Provides the type of scoped lock to use for locking a RecursiveSpinLock. */
    using ScopedLockType = GenericScopedLock<RecursiveSpinLock>;

    /** Provides the type of scoped unlocker to use with a RecursiveSpinLock. */
    using ScopedUnlockType = GenericScopedUnlock<RecursiveSpinLock>;

    /** Provides the type of scoped try-lock to use for locking a RecursiveSpinLock. */
    using ScopedTryLockType = GenericScopedTryLock<RecursiveSpinLock>;

private:
    //==============================================================================
    mutable Atomic<int> lock = 0;
    mutable Atomic<Thread::ThreadID> owner = nullptr;
    mutable uint32 count = 0;

    YUP_DECLARE_NON_COPYABLE (RecursiveSpinLock)
};

} // namespace yup
