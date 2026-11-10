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

struct InterProcessSemaphore::Native
{
    ~Native()
    {
        if (handle != nullptr)
            CloseHandle (handle);
    }

    static String getSemaphoreName (const String& name) { return "Local\\yup_sem_" + name; }

    HANDLE handle = nullptr;
};

InterProcessSemaphore::InterProcessSemaphore() = default;

InterProcessSemaphore::~InterProcessSemaphore()
{
    close();
}

Result InterProcessSemaphore::create (const String& name)
{
    close();

    auto newNative = std::make_unique<Native>();
    newNative->handle = CreateSemaphoreW (nullptr, 0, std::numeric_limits<LONG>::max(), Native::getSemaphoreName (name).toWideCharPointer());

    if (newNative->handle == nullptr)
        return Result::fail ("Cannot create semaphore " + name);

    if (GetLastError() == ERROR_ALREADY_EXISTS)
        return Result::fail ("Semaphore " + name + " already exists");

    native = std::move (newNative);
    return Result::ok();
}

Result InterProcessSemaphore::open (const String& name)
{
    close();

    auto newNative = std::make_unique<Native>();
    newNative->handle = OpenSemaphoreW (SEMAPHORE_ALL_ACCESS, FALSE, Native::getSemaphoreName (name).toWideCharPointer());

    if (newNative->handle == nullptr)
        return Result::fail ("Cannot open semaphore " + name);

    native = std::move (newNative);
    return Result::ok();
}

void InterProcessSemaphore::close()
{
    native.reset();
}

bool InterProcessSemaphore::isOpen() const noexcept
{
    return native != nullptr;
}

void InterProcessSemaphore::signal() noexcept
{
    if (native != nullptr)
        ReleaseSemaphore (native->handle, 1, nullptr);
}

bool InterProcessSemaphore::wait (int timeoutMs) noexcept
{
    if (native == nullptr)
        return false;

    return WaitForSingleObject (native->handle, timeoutMs < 0 ? INFINITE : static_cast<DWORD> (timeoutMs)) == WAIT_OBJECT_0;
}

} // namespace yup
