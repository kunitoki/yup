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

class InternalMessageQueue
{
public:
    InternalMessageQueue() = default;

    ~InternalMessageQueue()
    {
        clearSingletonInstance();
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
    YUP_DECLARE_SINGLETON (InternalMessageQueue, false)

private:
    CriticalSection lock;
    ReferenceCountedArray<MessageManager::MessageBase> queue;
};

YUP_IMPLEMENT_SINGLETON (InternalMessageQueue)

//==============================================================================
bool yup_dispatchNextMessageOnSystemQueue (bool returnIfNoPendingMessages)
{
    return InternalMessageQueue::getInstance()->dispatchNextMessage (returnIfNoPendingMessages);
}

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
    InternalMessageQueue::getInstance()->postMessage (message);

    return true;
}

void MessageManager::broadcastMessage (const String&)
{
}

//==============================================================================
void yup_juceEventsAndroidStartApp()
{
}

} // namespace yup
