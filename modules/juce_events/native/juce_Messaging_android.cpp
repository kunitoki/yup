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

namespace juce
{

class InternalMessageQueue
{
public:
    InternalMessageQueue() = default;

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
    void postMessage (MessageManager::MessageBase* const msg) noexcept
    {
        const ScopedLock sl (lock);

        queue.add (msg);
    }

    //==============================================================================
    bool dispatchNextMessage (bool returnIfNoPendingMessages)
    {
        ReferenceCountedArray<MessageManager::MessageBase> currentEvents;

        for (;;)
        {
            if (loopCallback)
                loopCallback();

            {
                const ScopedLock sl (lock);

                currentEvents = std::move (queue);
                queue.clear();
            }

            if (currentEvents.isEmpty())
            {
                if (returnIfNoPendingMessages)
                    return false;

                Thread::sleep (1); // TODO - Make this better somehow ?
            }
            else
            {
                while (! currentEvents.isEmpty())
                {
                    if (auto message = currentEvents.removeAndReturn (0))
                        message->messageCallback();
                }

                break;
            }
        }

        return true;
    }

    //==============================================================================
    JUCE_DECLARE_SINGLETON (InternalMessageQueue, false)

private:
    CriticalSection lock;
    ReferenceCountedArray<MessageManager::MessageBase> queue;
    std::function<void()> loopCallback;
};

JUCE_IMPLEMENT_SINGLETON (InternalMessageQueue)

//==============================================================================
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
bool dispatchNextMessageOnSystemQueue (bool returnIfNoPendingMessages)
{
    return InternalMessageQueue::getInstance()->dispatchNextMessage (returnIfNoPendingMessages);
}
} // namespace detail

bool MessageManager::postMessageToSystemQueue (MessageManager::MessageBase* const message)
{
    InternalMessageQueue::getInstance()->postMessage (message);

    return true;
}

void MessageManager::broadcastMessage (const String&)
{
}

void MessageManager::registerEventLoopCallback (std::function<void()> loopCallbackToSet)
{
    InternalMessageQueue::getInstance()->registerEventLoopCallback (std::move (loopCallbackToSet));
}

//==============================================================================
/*
extern "C" jint JNIEXPORT JNI_OnLoad2 (JavaVM* vm, void*);
//extern "C" jint JNIEXPORT JNI_OnLoad (JavaVM *vm, void*);

    //JNI_OnLoad (nativeApp->activity->vm, nullptr);
    JNI_OnLoad2 (nativeApp->activity->vm, nullptr);

    // JNIClassBase::initialiseAllClasses (nativeApp->activity->env, nativeApp->activity->clazz);
    Thread::initialiseJUCE (nativeApp->activity->env, nativeApp->activity->clazz);
*/

//==============================================================================
void juce_juceEventsAndroidStartApp()
{
}

} // namespace juce
