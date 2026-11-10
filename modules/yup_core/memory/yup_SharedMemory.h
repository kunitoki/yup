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
    A named block of memory shared between processes.

    One process creates the block with create(), others map the same memory with
    open() using the same name and size. The creator removes the name from the
    system when it closes the block; processes that already opened it keep their
    mapping until they close it too.

    The memory is zero-initialised when created. Synchronise access to it, e.g.
    with std::atomic members (lock-free atomics work across processes) or with an
    InterProcessSemaphore.

    On WebAssembly and Android create() and open() always fail.

    @see InterProcessSemaphore

    @tags{Core}
*/
class YUP_API SharedMemory
{
public:
    //==============================================================================
    SharedMemory();
    ~SharedMemory();

    SharedMemory (SharedMemory&& other) noexcept;
    SharedMemory& operator= (SharedMemory&& other) noexcept;

    //==============================================================================
    /**
        Creates a new block of @p numBytes, failing if the name is already in use.

        Names are short alphanumeric strings (at most 24 characters, as some systems
        limit their length), e.g. made with createUniqueName().
    */
    Result create (const String& name, size_t numBytes);

    /** Maps a block of @p numBytes created by another process with create(). */
    Result open (const String& name, size_t numBytes);

    /** Unmaps the block, and removes its name from the system if it was created here. */
    void close();

    //==============================================================================
    /** Returns true when a block is mapped. */
    bool isOpen() const noexcept { return data != nullptr; }

    /** Returns the mapped memory, or nullptr. */
    void* getData() const noexcept { return data; }

    /** Returns the size of the mapped memory in bytes. */
    size_t getSize() const noexcept { return size; }

    /** Returns the name the block was created or opened with. */
    const String& getName() const noexcept { return name; }

    //==============================================================================
    /** Returns a random name suitable for create(), starting with @p prefix (at most 8 characters). */
    static String createUniqueName (const String& prefix = "yup");

private:
    void* data = nullptr;
    size_t size = 0;
    String name;
    bool owner = false;
    intptr_t handle = 0;

    YUP_DECLARE_NON_COPYABLE (SharedMemory)
};

} // namespace yup
