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
    using Counter = std::atomic<uint32>;
    static_assert (sizeof (Counter) == sizeof (uint32) && Counter::is_always_lock_free);

    static String getMemoryName (const String& name) { return name + "-sem"; }

    Counter& getCounter() const noexcept { return *static_cast<Counter*> (memory.getData()); }

    void waitWhileZero (int timeoutMs) const noexcept
    {
        auto* address = static_cast<void*> (memory.getData());

#if YUP_LINUX
        timespec timeout { timeoutMs / 1000, (timeoutMs % 1000) * 1000000L };
        syscall (SYS_futex, address, FUTEX_WAIT, 0, timeoutMs < 0 ? nullptr : &timeout, nullptr, 0);
#else
#if __has_include(<os/os_sync_wait_on_address.h>)
        if (__builtin_available (macOS 14.4, iOS 17.4, *))
        {
            if (timeoutMs < 0)
                os_sync_wait_on_address (address, 0, sizeof (uint32), OS_SYNC_WAIT_ON_ADDRESS_SHARED);
            else
                os_sync_wait_on_address_with_timeout (address, 0, sizeof (uint32), OS_SYNC_WAIT_ON_ADDRESS_SHARED, OS_CLOCK_MACH_ABSOLUTE_TIME, static_cast<uint64> (timeoutMs) * 1000000);

            return;
        }
#endif
        ignoreUnused (address, timeoutMs);
        std::this_thread::sleep_for (std::chrono::microseconds (50));
#endif
    }

    void wakeOne() const noexcept
    {
        auto* address = static_cast<void*> (memory.getData());

#if YUP_LINUX
        syscall (SYS_futex, address, FUTEX_WAKE, 1, nullptr, nullptr, 0);
#elif __has_include(<os/os_sync_wait_on_address.h>)
        if (__builtin_available (macOS 14.4, iOS 17.4, *))
            os_sync_wake_by_address_any (address, sizeof (uint32), OS_SYNC_WAKE_BY_ADDRESS_SHARED);
#else
        ignoreUnused (address);
#endif
    }

    SharedMemory memory;
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
    if (auto result = newNative->memory.create (Native::getMemoryName (name), sizeof (Native::Counter)); result.failed())
        return result;

    new (newNative->memory.getData()) Native::Counter (0);
    native = std::move (newNative);
    return Result::ok();
}

Result InterProcessSemaphore::open (const String& name)
{
    close();

    auto newNative = std::make_unique<Native>();
    if (auto result = newNative->memory.open (Native::getMemoryName (name), sizeof (Native::Counter)); result.failed())
        return result;

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
    if (native == nullptr)
        return;

    native->getCounter().fetch_add (1, std::memory_order_release);
    native->wakeOne();
}

bool InterProcessSemaphore::wait (int timeoutMs) noexcept
{
    if (native == nullptr)
        return false;

    auto& counter = native->getCounter();
    const auto startTime = Time::getMillisecondCounterHiRes();

    for (;;)
    {
        auto value = counter.load (std::memory_order_acquire);

        while (value > 0)
        {
            if (counter.compare_exchange_weak (value, value - 1, std::memory_order_acq_rel))
                return true;
        }

        int remainingMs = -1;

        if (timeoutMs >= 0)
        {
            remainingMs = timeoutMs - static_cast<int> (Time::getMillisecondCounterHiRes() - startTime);
            if (remainingMs <= 0)
                return false;
        }

        native->waitWhileZero (remainingMs);
    }
}

} // namespace yup
