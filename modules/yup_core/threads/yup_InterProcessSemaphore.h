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

/**
    A counting semaphore shared between processes by name.

    One process creates the semaphore with create(), others open it with open()
    using the same name. signal() increments the count and wakes one waiter, wait()
    blocks until the count is positive, then decrements it.

    signal() never blocks or allocates, so it can be used from realtime threads.

    Not available on WebAssembly and Android.

    @see SharedMemory

    @tags{Core}
*/
class YUP_API InterProcessSemaphore
{
public:
    //==============================================================================
    InterProcessSemaphore();
    ~InterProcessSemaphore();

    //==============================================================================
    /** Creates a new semaphore with a count of zero, see SharedMemory::create() for valid names. */
    Result create (const String& name);

    /** Opens a semaphore created by another process with create(). */
    Result open (const String& name);

    /** Closes the semaphore, and removes its name from the system if it was created here. */
    void close();

    /** Returns true when the semaphore was created or opened. */
    bool isOpen() const noexcept;

    //==============================================================================
    /** Increments the count, waking one waiting thread. */
    void signal() noexcept;

    /**
        Waits until the count is positive, then decrements it.

        @param timeoutMs  Maximum time to wait in milliseconds, or a negative value to wait forever.
        @returns          True if the count was decremented, false on timeout.
    */
    bool wait (int timeoutMs = -1) noexcept;

private:
    struct Native;
    std::unique_ptr<Native> native;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InterProcessSemaphore)
};

} // namespace yup
