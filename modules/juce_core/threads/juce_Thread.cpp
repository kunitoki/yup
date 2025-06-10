/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{
//==============================================================================
Thread::Thread (const String& name, size_t stackSize)
    : threadName (name)
    , threadStackSize (stackSize)
{
}

Thread::~Thread()
{
    if (deleteOnThreadEnd)
        return;

    /* If your thread class's destructor has been called without first stopping the thread, that
       means that this partially destructed object is still performing some work - and that's
       probably a Bad Thing!

       To avoid this type of nastiness, always make sure you call stopThread() before or during
       your subclass's destructor.
    */
    jassert (! isThreadRunning());

    stopThread (-1);
}

//==============================================================================
// Use a ref-counted object to hold this shared data, so that it can outlive its static
// shared pointer when threads are still running during static shutdown.
struct CurrentThreadHolder final : public ReferenceCountedObject
{
    CurrentThreadHolder() noexcept {}

    using Ptr = ReferenceCountedObjectPtr<CurrentThreadHolder>;
    ThreadLocalValue<Thread*> value;

    YUP_DECLARE_NON_COPYABLE (CurrentThreadHolder)
};

static char currentThreadHolderLock[sizeof (SpinLock)]; // (statically initialised to zeros).

static SpinLock* castToSpinLockWithoutAliasingWarning (void* s)
{
    return static_cast<SpinLock*> (s);
}

static CurrentThreadHolder::Ptr getCurrentThreadHolder()
{
    static CurrentThreadHolder::Ptr currentThreadHolder;
    SpinLock::ScopedLockType lock (*castToSpinLockWithoutAliasingWarning (currentThreadHolderLock));

    if (currentThreadHolder == nullptr)
        currentThreadHolder = new CurrentThreadHolder();

    return currentThreadHolder;
}

void Thread::threadEntryPoint()
{
    const CurrentThreadHolder::Ptr currentThreadHolder (getCurrentThreadHolder());
    currentThreadHolder->value = this;

    if (threadName.isNotEmpty())
        setCurrentThreadName (threadName);

    // This 'startSuspensionEvent' protects 'threadId' which is initialised after the platform's native 'CreateThread' method.
    // This ensures it has been initialised correctly before it reaches this point.
    if (startSuspensionEvent.wait (10000))
    {
        jassert (getCurrentThreadId() == threadId);

        if (affinityMask != 0)
            setCurrentThreadAffinityMask (affinityMask);

        try
        {
            run();
        }
        catch (...)
        {
            jassertfalse; // Your run() method mustn't throw any exceptions!
        }
    }

    currentThreadHolder->value.releaseCurrentThreadStorage();

    // Once closeThreadHandle is called this class may be deleted by a different
    // thread, so we need to store deleteOnThreadEnd in a local variable.
    auto shouldDeleteThis = deleteOnThreadEnd;
    closeThreadHandle();

    if (shouldDeleteThis)
        delete this;
}

// used to wrap the incoming call from the platform-specific code
void YUP_API juce_threadEntryPoint (void* userData)
{
    static_cast<Thread*> (userData)->threadEntryPoint();
}

//==============================================================================
bool Thread::startThreadInternal (Priority threadPriority)
{
    shouldExit = false;

    // 'priority' is essentially useless on Linux as only realtime
    // has any options but we need to set this here to satisfy
    // later queries, otherwise we get inconsistent results across
    // platforms.
#if YUP_ANDROID || YUP_LINUX || YUP_BSD
    priority = threadPriority;
#endif

    if (createNativeThread (threadPriority))
    {
        startSuspensionEvent.signal();
        return true;
    }

    return false;
}

bool Thread::startThread()
{
    return startThread (Priority::normal);
}

bool Thread::startThread (Priority threadPriority)
{
    const ScopedLock sl (startStopLock);

    if (threadHandle == nullptr)
    {
        realtimeOptions.reset();
        return startThreadInternal (threadPriority);
    }

    return false;
}

bool Thread::startRealtimeThread (const RealtimeOptions& options)
{
    const ScopedLock sl (startStopLock);

    if (threadHandle == nullptr)
    {
        realtimeOptions = std::make_optional (options);

        if (startThreadInternal (Priority::normal))
            return true;

        realtimeOptions.reset();
    }

    return false;
}

bool Thread::isThreadRunning() const
{
    return threadHandle != nullptr;
}

Thread* YUP_CALLTYPE Thread::getCurrentThread()
{
    return getCurrentThreadHolder()->value.get();
}

Thread::ThreadID Thread::getThreadId() const noexcept
{
    return threadId;
}

//==============================================================================
void Thread::signalThreadShouldExit()
{
    shouldExit = true;
    listeners.call ([] (Listener& l)
    {
        l.exitSignalSent();
    });
}

bool Thread::threadShouldExit() const
{
    return shouldExit;
}

bool Thread::currentThreadShouldExit()
{
    if (auto* currentThread = getCurrentThread())
        return currentThread->threadShouldExit();

    return false;
}

bool Thread::waitForThreadToExit (const int timeOutMilliseconds) const
{
    // Doh! So how exactly do you expect this thread to wait for itself to stop??
    jassert (getThreadId() != getCurrentThreadId() || getCurrentThreadId() == ThreadID());

    auto timeoutEnd = Time::getMillisecondCounter() + (uint32) timeOutMilliseconds;

    while (isThreadRunning())
    {
        if (timeOutMilliseconds >= 0 && Time::getMillisecondCounter() > timeoutEnd)
            return false;

        sleep (2);
    }

    return true;
}

bool Thread::stopThread (const int timeOutMilliseconds)
{
    // agh! You can't stop the thread that's calling this method! How on earth
    // would that work??
    jassert (getCurrentThreadId() != getThreadId());

    const ScopedLock sl (startStopLock);

    if (isThreadRunning())
    {
        signalThreadShouldExit();
        notify();

        if (timeOutMilliseconds != 0)
            waitForThreadToExit (timeOutMilliseconds);

        if (isThreadRunning())
        {
            // very bad karma if this point is reached, as there are bound to be
            // locks and events left in silly states when a thread is killed by force..
            jassertfalse;
            Logger::writeToLog ("!! killing thread " + threadName + " by force !!");

            killThread();

            threadHandle = nullptr;
            threadId = {};
            return false;
        }
    }

    return true;
}

void Thread::addListener (Listener* listener)
{
    listeners.add (listener);
}

void Thread::removeListener (Listener* listener)
{
    listeners.remove (listener);
}

bool Thread::isRealtime() const
{
    return realtimeOptions.has_value();
}

void Thread::setAffinityMask (const uint32 newAffinityMask)
{
    affinityMask = newAffinityMask;
}

//==============================================================================
bool Thread::wait (double timeOutMilliseconds) const
{
    return defaultEvent.wait (timeOutMilliseconds);
}

void Thread::notify() const
{
    defaultEvent.signal();
}

//==============================================================================
struct LambdaThread final : public Thread
{
    LambdaThread (std::function<void()>&& f)
        : Thread ("anonymous")
        , fn (std::move (f))
    {
    }

    void run() override
    {
        fn();
        fn = nullptr; // free any objects that the lambda might contain while the thread is still active
    }

    std::function<void()> fn;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LambdaThread)
};

bool Thread::launch (std::function<void()> functionToRun)
{
    return launch (Priority::normal, std::move (functionToRun));
}

bool Thread::launch (Priority priority, std::function<void()> functionToRun)
{
    auto anon = std::make_unique<LambdaThread> (std::move (functionToRun));
    anon->deleteOnThreadEnd = true;

    if (anon->startThread (priority))
    {
        anon.release();
        return true;
    }

    return false;
}

//==============================================================================
void SpinLock::enter() const noexcept
{
    if (! tryEnter())
    {
        for (int i = 20; --i >= 0;)
            if (tryEnter())
                return;

        while (! tryEnter())
            Thread::yield();
    }
}

//==============================================================================
bool YUP_CALLTYPE Process::isRunningUnderDebugger() noexcept
{
    return juce_isRunningUnderDebugger();
}

} // namespace yup
