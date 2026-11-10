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

namespace yup
{

// Platforms without named cross-process shared memory and semaphores
// (Android, WebAssembly): every attempt to create or open fails gracefully.

Result SharedMemory::create (const String&, size_t)
{
    close();
    return Result::fail ("Shared memory is not supported on this platform");
}

Result SharedMemory::open (const String&, size_t)
{
    close();
    return Result::fail ("Shared memory is not supported on this platform");
}

void SharedMemory::close()
{
    data = nullptr;
    size = 0;
    name = {};
    owner = false;
}

//==============================================================================
struct InterProcessSemaphore::Native
{
};

InterProcessSemaphore::InterProcessSemaphore() = default;

InterProcessSemaphore::~InterProcessSemaphore() = default;

Result InterProcessSemaphore::create (const String&)
{
    return Result::fail ("Inter-process semaphores are not supported on this platform");
}

Result InterProcessSemaphore::open (const String&)
{
    return Result::fail ("Inter-process semaphores are not supported on this platform");
}

void InterProcessSemaphore::close()
{
    native.reset();
}

bool InterProcessSemaphore::isOpen() const noexcept
{
    return false;
}

void InterProcessSemaphore::signal() noexcept
{
}

bool InterProcessSemaphore::wait (int) noexcept
{
    return false;
}

} // namespace yup
