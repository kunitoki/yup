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

namespace yup
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
        emscripten_set_main_loop ([] {}, 0, 0);

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

        eventQueue.ensureStorageAllocated (queueCapacity);
        deliveryQueue.ensureStorageAllocated (queueCapacity);
    }

    ~InternalMessageQueue()
    {
        emscripten_cancel_main_loop();

        clearSingletonInstance();
    }

    //==============================================================================
    bool postMessage (MessageManager::MessageBase* const msg)
    {
        {
            const SpinLock::ScopedLockType sl (lock);

            eventQueue.add (msg);
        }

        return true;
    }

    //==============================================================================
    void deliverNextMessages()
    {
        {
            const SpinLock::ScopedLockType sl (lock);

            deliveryQueue.swapWith (eventQueue);
        }

        for (auto* message : deliveryQueue)
        {
            if (message != nullptr)
                message->messageCallback();
        }

        deliveryQueue.clearQuick();
    }

    //==============================================================================
    YUP_DECLARE_SINGLETON (InternalMessageQueue, false)

private:
    static constexpr int queueCapacity = 256;

    SpinLock lock;
    ReferenceCountedArray<MessageManager::MessageBase> eventQueue;
    ReferenceCountedArray<MessageManager::MessageBase> deliveryQueue;
};

YUP_IMPLEMENT_SINGLETON (InternalMessageQueue)

//==============================================================================
void MessageManager::doPlatformSpecificInitialisation()
{
    InternalMessageQueue::getInstance();
}

void MessageManager::doPlatformSpecificShutdown()
{
    InternalMessageQueue::deleteInstance();
}

bool MessageManager::postMessageToSystemQueue (MessageManager::MessageBase* const message)
{
    return InternalMessageQueue::getInstance()->postMessage (message);
}

void MessageManager::broadcastMessage (const String&)
{
}

//==============================================================================
void MessageManager::runDispatchLoop()
{
    emscripten_cancel_main_loop();

    auto mainLoop = [] (void* arg)
    {
        Timer::callPendingTimersSynchronously();

        auto* messageManager = static_cast<MessageManager*> (arg);
        jassert (messageManager != nullptr);
        if (messageManager != nullptr && messageManager->loopCallback != nullptr)
            messageManager->loopCallback();

        InternalMessageQueue::getInstance()->deliverNextMessages();
    };

    constexpr int framesPerSeconds = 0;
    constexpr int simulateInfiniteLoop = 1;
    emscripten_set_main_loop_arg (mainLoop, this, framesPerSeconds, simulateInfiniteLoop);
}

void MessageManager::stopDispatchLoop()
{
    quitMessagePosted = true;

    emscripten_cancel_main_loop();

    for (const auto& func : shutdownCallbacks)
        func();
}

#if YUP_MODAL_LOOPS_PERMITTED
bool MessageManager::runDispatchLoopUntil (int millisecondsToRunFor)
{
    Logger::outputDebugString ("*** Modal loops are not possible in Emscripten!! Exiting...");
    exit (1);
}
#endif

} // namespace yup
