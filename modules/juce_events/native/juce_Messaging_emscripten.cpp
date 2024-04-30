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
*/


namespace juce
{

class JUCE_API EmscriptenEventMessage : public Message {};

static void createDirIfNotExists(File::SpecialLocationType type)
{
    File dir = File::getSpecialLocation(type);
    if (! dir.exists()) dir.createDirectory();
}

Thread::ThreadID messageThreadID = nullptr; // JUCE message thread
Thread::ThreadID mainThreadID = nullptr;    // Javascript main thread
std::vector<std::function<void()>> preDispatchLoopFuncs;
std::vector<std::function<void()>> mainThreadLoopFuncs;
double timeDispatchBeginMS = 0.0;

extern bool isMessageThreadProxied();
extern void registerCallbackToMainThread (std::function<void()> f);

int juce_animationFrameCallback (double timestamp);
void juce_dispatchLoop();

class InternalMessageQueue
{
public:
    InternalMessageQueue()
    {
        createDirIfNotExists (File::userHomeDirectory);
        createDirIfNotExists (File::userDocumentsDirectory);
        createDirIfNotExists (File::userMusicDirectory);
        createDirIfNotExists (File::userMoviesDirectory);
        createDirIfNotExists (File::userPicturesDirectory);
        createDirIfNotExists (File::userDesktopDirectory);
        createDirIfNotExists (File::userApplicationDataDirectory);
        createDirIfNotExists (File::commonDocumentsDirectory);
        createDirIfNotExists (File::commonApplicationDataDirectory);
        createDirIfNotExists (File::globalApplicationsDirectory);
        createDirIfNotExists (File::tempDirectory);

        messageThreadID = Thread::getCurrentThreadId();

        MAIN_THREAD_EM_ASM ({
            if (window.juce_animationFrameCallback)
                return;

            window.juce_animationFrameCallback = function (timestamp)
            {
                dynCall("ii", $0, [timestamp]);

                window.requestAnimationFrame (window.juce_animationFrameCallback);
            };

            window.requestAnimationFrame (window.juce_animationFrameCallback);
        }, juce_animationFrameCallback);
    }

    ~InternalMessageQueue()
    {
        clearSingletonInstance();
    }

    //==============================================================================
    void postMessage (MessageManager::MessageBase* const msg) noexcept
    {
        {
            const ScopedLock sl (lock);

            if (dynamic_cast<EmscriptenEventMessage* const> (msg))
                eventQueue.add (msg);
            else
                messageQueue.add (msg);
        }
    }

    //==============================================================================
    void dispatchLoop()
    {
        if (quitReceived.load())
        {
            emscripten_cancel_main_loop();

            //auto* app = JUCEApplicationBase::getInstance();
            //app->shutdownApp();

            return;
        }

        timeDispatchBeginMS = Time::getMillisecondCounterHiRes();

        Timer::callPendingTimersSynchronously();

        dispatchEvents();

        for (auto f : preDispatchLoopFuncs) f();

        ReferenceCountedArray <MessageManager::MessageBase> currentMessages;

        {
            const ScopedLock sl (lock);

            currentMessages = std::move (messageQueue);
            messageQueue.clear();
        }

        while (! currentMessages.isEmpty())
        {
            if (auto message = currentMessages.removeAndReturn (0))
                message->messageCallback();
        }
    }

    void runDispatchLoop()
    {
        constexpr int framesPerSeconds = 0;
        constexpr int simulateInfiniteLoop = 1;
        emscripten_set_main_loop (juce_dispatchLoop, framesPerSeconds, simulateInfiniteLoop);
    }

    void stopDispatchLoop()
    {
        (new QuitCallback(*this))->post();
    }

    //==============================================================================
    JUCE_DECLARE_SINGLETON (InternalMessageQueue, false)

private:
    struct QuitCallback : public CallbackMessage
    {
        InternalMessageQueue& parent;

        QuitCallback(InternalMessageQueue& newParent)
            : parent (newParent)
        {
        }

        void messageCallback() override
        {
            parent.quitReceived = true;
        }
    };

    void dispatchEvents()
    {
        ReferenceCountedArray <MessageManager::MessageBase> currentEvents;

        {
            const ScopedLock sl (lock);

            currentEvents = std::move (eventQueue);
            eventQueue.clear();
        }

        while (! currentEvents.isEmpty())
        {
            if (auto message = currentEvents.removeAndReturn (0))
                message->messageCallback();
        }
    }

    CriticalSection lock;
    ReferenceCountedArray <MessageManager::MessageBase> messageQueue;
    ReferenceCountedArray <MessageManager::MessageBase> eventQueue;
    std::atomic<bool> quitReceived = false;
};

JUCE_IMPLEMENT_SINGLETON (InternalMessageQueue)

bool isMessageThreadProxied()
{
    return messageThreadID != mainThreadID;
}

void registerCallbackToMainThread (std::function<void()> f)
{
    if (mainThreadID == messageThreadID)
        preDispatchLoopFuncs.push_back (std::move (f));
    else
        mainThreadLoopFuncs.push_back (std::move (f));
}

void juce_dispatchLoop()
{
    InternalMessageQueue::getInstance()->dispatchLoop();
}

int juce_animationFrameCallback (double timestamp)
{
    // If timestamp < 0, this callback tests if the calling thread (main thread) is
    //   different from the message thread and return the result.
    // If timestamp >= 0, it always returns 0.
    if (timestamp < 0)
    {
        mainThreadID = Thread::getCurrentThreadId();
        return mainThreadID != messageThreadID;
    }

    //static double prevTimestamp = 0;
    //if (timestamp - prevTimestamp > 20)
    //    Logger::outputDebugString ("juce_animationFrameCallback " + std::to_string (timestamp - prevTimestamp));
    //prevTimestamp = timestamp;

    for (auto f : mainThreadLoopFuncs) f();

    return 0;
}

void MessageManager::doPlatformSpecificInitialisation()
{
    InternalMessageQueue::getInstance();
}

void MessageManager::doPlatformSpecificShutdown()
{
    InternalMessageQueue::deleteInstance();
}

namespace detail
{
bool dispatchNextMessageOnSystemQueue (const bool returnIfNoPendingMessages)
{
    Logger::outputDebugString ("*** Modal loops are not possible in Emscripten!! Exiting...");
    exit (1);

    return true;
}
} // namespace detail

double getTimeSpentInCurrentDispatchCycle()
{
    double currentTimeMS = Time::getMillisecondCounterHiRes();

    // DBG("getTimeSpentInCurrentDispatchCycle: " << currentTimeMS - timeDispatchBeginMS);

    return (currentTimeMS - timeDispatchBeginMS) / 1000.0;
}

bool MessageManager::postMessageToSystemQueue (MessageManager::MessageBase* const message)
{
    InternalMessageQueue::getInstance()->postMessage (message);

    return true;
}

void MessageManager::broadcastMessage (const String&)
{
}

void MessageManager::runDispatchLoop()
{
    InternalMessageQueue::getInstance()->runDispatchLoop();
}

void MessageManager::stopDispatchLoop()
{
    InternalMessageQueue::getInstance()->stopDispatchLoop();

    quitMessagePosted = true;
}

} // namespace juce

