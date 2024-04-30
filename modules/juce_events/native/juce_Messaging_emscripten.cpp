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

//static bool appIsInsideEmrun{ false };

static std::deque<MessageManager::MessageBase*> messageQueue;
static std::deque<MessageManager::MessageBase*> eventQueue;
static std::mutex queueMtx;
static std::atomic<bool> quitReceived{ false };
static double timeDispatchBeginMS{ 0 };

static Thread::ThreadID messageThreadID{ nullptr }; // JUCE message thread
static Thread::ThreadID mainThreadID{ nullptr };    // Javascript main thread

std::unique_ptr<juce::ScopedJuceInitialiser_GUI> libraryInitialiser;

std::vector<std::function<void()>> preDispatchLoopFuncs;
// These callbacks are only executed if main thread isn't message thread.
std::vector<std::function<void()>> mainThreadLoopFuncs;

extern bool isMessageThreadProxied()
{
    return messageThreadID != mainThreadID;
}

extern void registerCallbackToMainThread (std::function<void()> f)
{
    if (mainThreadID == messageThreadID)
        preDispatchLoopFuncs.push_back (std::move (f));
    else
        mainThreadLoopFuncs.push_back (std::move (f));
}

extern std::deque<std::string> debugPrintQueue;
extern std::mutex debugPrintQueueMtx;

// If timestamp < 0, this callback tests if the calling thread (main thread) is
//   different from the message thread and return the result.
// If timestamp >= 0, it always returns 0.
extern "C" int juce_animationFrameCallback (double timestamp)
{
    if (timestamp < 0)
    {
        mainThreadID = Thread::getCurrentThreadId();
        return mainThreadID != messageThreadID;
    }

    static double prevTimestamp = 0;
    //if (timestamp - prevTimestamp > 20)
    //    Logger::outputDebugString ("juce_animationFrameCallback " + std::to_string (timestamp - prevTimestamp));

    prevTimestamp = timestamp;

    for (auto f : mainThreadLoopFuncs) f();

    return 0;
}

void MessageManager::doPlatformSpecificInitialisation()
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

    /*
    appIsInsideEmrun = MAIN_THREAD_EM_ASM_INT ({
        return document.title == "Emscripten-Generated Code";
    });
    */

    MAIN_THREAD_EM_ASM ({
        if (window.juce_animationFrameCallback)
            return;

        window.juce_animationFrameCallback = function (time) // Module.cwrap ("juce_animationFrameCallback", "int", ["number"]);
        {
            return dynCall("ii", $0, [time]);
        };

        //if (window.juce_animationFrameCallback (-1.0) == 1)
        {
            window.juce_animationFrameWrapper = function (timestamp)
            {
                window.juce_animationFrameCallback (timestamp);
                window.requestAnimationFrame (window.juce_animationFrameWrapper);
            };

            window.requestAnimationFrame (window.juce_animationFrameWrapper);
        }
    }, juce_animationFrameCallback);
}

void MessageManager::doPlatformSpecificShutdown() {}

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

static void dispatchEvents()
{
    std::deque<MessageManager::MessageBase*> currentEvents;

    {
        const std::lock_guard lg (queueMtx);

        currentEvents = std::move (eventQueue);
        eventQueue.clear();
    }

    // TODO
    if (currentEvents.size() > 0)
        printf("currentEvents=%d\n", (int)currentEvents.size());

    while (! currentEvents.empty())
    {
        auto* message = currentEvents.front();
        currentEvents.pop_front();

        message->messageCallback();
        message->decReferenceCount();
    }
}

static void dispatchLoop()
{
    if (quitReceived.load())
    {
        emscripten_cancel_main_loop();

        auto* app = JUCEApplicationBase::getInstance();
        app->shutdownApp();

        libraryInitialiser.reset (nullptr);
        return;
    }

    timeDispatchBeginMS = Time::getMillisecondCounterHiRes();

    Timer::callPendingTimersSynchronously();

    dispatchEvents();

    for (auto f : preDispatchLoopFuncs) f();

   #if JUCE_DEBUG
    {
        const std::lock_guard lg (debugPrintQueueMtx);

        while (! debugPrintQueue.empty())
        {
            std::cout << debugPrintQueue.front() << std::endl;
            debugPrintQueue.pop_front();
        }
    }
   #endif

    std::deque<MessageManager::MessageBase*> currentMessages;

    {
        const std::lock_guard lg (queueMtx);

        currentMessages = std::move (messageQueue);
        messageQueue.clear();
    }

    while (! currentMessages.empty())
    {
        auto* message = currentMessages.front();
        currentMessages.pop_front();

        message->messageCallback();
        message->decReferenceCount();
    }

    /*
    if (appIsInsideEmrun)
    {
        MAIN_THREAD_EM_ASM ({
            var logArea = document.querySelector("#output");
            var n = logArea.value.length;
            if (n > 1000)
                logArea.value = logArea.value.substring(n - 1000, n);
        });
    }
    */
}

bool MessageManager::postMessageToSystemQueue (MessageManager::MessageBase* const message)
{
    const std::lock_guard lg (queueMtx);

    if (dynamic_cast<EmscriptenEventMessage* const> (message))
        eventQueue.push_back (message);
    else
        messageQueue.push_back (message);

    message->incReferenceCount();

    return true;
}

void MessageManager::broadcastMessage (const String&)
{
}

void MessageManager::runDispatchLoop()
{
    constexpr int framesPerSeconds = 0;
    constexpr int simulateInfiniteLoop = 1;
    emscripten_set_main_loop (dispatchLoop, framesPerSeconds, simulateInfiniteLoop);
}

struct QuitCallback : public CallbackMessage
{
    QuitCallback() {}
    void messageCallback() override
    {
        quitReceived = true;
    }
};

void MessageManager::stopDispatchLoop()
{
    (new QuitCallback())->post();
    quitMessagePosted = true;
}

} // namespace juce

