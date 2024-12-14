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

namespace
{
void createDirIfNotExists (File::SpecialLocationType type)
{
    File dir = File::getSpecialLocation (type);
    if (! dir.exists())
        dir.createDirectory();
}
} // namespace

class InternalMessageQueue
{
public:
    InternalMessageQueue()
    {
        emscripten_set_main_loop (dispatchLoopInternal, 0, 0);

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
    }

    ~InternalMessageQueue()
    {
        clearSingletonInstance();
    }

    //==============================================================================
    void registerEventLoopCallback (std::function<void()> loopCallbackToSet)
    {
        loopCallback = std::move (loopCallbackToSet);
    }

    //==============================================================================
    bool postMessage (MessageManager::MessageBase* const msg)
    {
        {
            const ScopedLock sl (lock);

            eventQueue.add (msg);
        }

        return true;
    }

    //==============================================================================
    void runDispatchLoop()
    {
        emscripten_cancel_main_loop();

        constexpr int framesPerSeconds = 0;
        constexpr int simulateInfiniteLoop = 1;
        emscripten_set_main_loop (dispatchLoopInternal, framesPerSeconds, simulateInfiniteLoop);
    }

    void stopDispatchLoop()
    {
        emscripten_cancel_main_loop();
    }

    //==============================================================================
    JUCE_DECLARE_SINGLETON (InternalMessageQueue, false)

private:
    static void dispatchLoopInternal()
    {
        InternalMessageQueue::getInstance()->dispatchLoop();
    }

    void dispatchLoop()
    {
        Timer::callPendingTimersSynchronously();

        if (loopCallback)
            loopCallback();

        ReferenceCountedArray<MessageManager::MessageBase> currentEvents;

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
    ReferenceCountedArray<MessageManager::MessageBase> eventQueue;
    std::function<void()> loopCallback;
};

JUCE_IMPLEMENT_SINGLETON (InternalMessageQueue)

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

bool MessageManager::postMessageToSystemQueue (MessageManager::MessageBase* const message)
{
    return InternalMessageQueue::getInstance()->postMessage (message);
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

void MessageManager::registerEventLoopCallback (std::function<void()> loopCallbackToSet)
{
    InternalMessageQueue::getInstance()->registerEventLoopCallback (std::move (loopCallbackToSet));
}
} // namespace juce
