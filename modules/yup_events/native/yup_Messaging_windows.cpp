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

extern HWND yup_messageWindowHandle;

#if YUP_MODULE_AVAILABLE_yup_gui_extra
LRESULT yup_offerEventToActiveXControl (::MSG&);
#endif

using SettingChangeCallbackFunc = void (*) (void);
SettingChangeCallbackFunc settingChangeCallback = nullptr;

//==============================================================================
class InternalMessageQueue
{
public:
    InternalMessageQueue()
    {
        messageWindow = std::make_unique<HiddenMessageWindow> (messageWindowName, (WNDPROC) messageWndProc);
        yup_messageWindowHandle = messageWindow->getHWND();
    }

    ~InternalMessageQueue()
    {
        yup_messageWindowHandle = nullptr;
        clearSingletonInstance();
    }

    YUP_DECLARE_SINGLETON (InternalMessageQueue, false)

    //==============================================================================
    void broadcastMessage (const String& message)
    {
        auto localCopy = message;

        Array<HWND> windows;
        EnumWindows (&broadcastEnumWindowProc, (LPARAM) &windows);

        for (int i = windows.size(); --i >= 0;)
        {
            COPYDATASTRUCT data;
            data.dwData = broadcastMessageMagicNumber;
            data.cbData = (DWORD) (((size_t) localCopy.length() + 1) * sizeof (CharPointer_UTF32::CharType));
            data.lpData = (void*) localCopy.toUTF32().getAddress();

            DWORD_PTR result;
            SendMessageTimeout (windows.getUnchecked (i), WM_COPYDATA, (WPARAM) yup_messageWindowHandle, (LPARAM) &data, SMTO_BLOCK | SMTO_ABORTIFHUNG, 8000, &result);
        }
    }

    void postMessage (MessageManager::MessageBase* message)
    {
        bool shouldTriggerMessageQueueDispatch = false;

        {
            const ScopedLock sl (lock);

            shouldTriggerMessageQueueDispatch = messageQueue.isEmpty();
            messageQueue.add (message);
        }

        if (! shouldTriggerMessageQueueDispatch)
            return;

        if (detail::RunningInUnity::state)
        {
            SendNotifyMessage (yup_messageWindowHandle, customMessageID, 0, 0);
            return;
        }

        PostMessage (yup_messageWindowHandle, customMessageID, 0, 0);
    }

    bool dispatchNextMessage (bool returnIfNoPendingMessages)
    {
        MSG m;

        if (returnIfNoPendingMessages && ! PeekMessage (&m, nullptr, 0, 0, PM_NOREMOVE))
            return false;

        if (GetMessage (&m, nullptr, 0, 0) >= 0)
        {
#if YUP_MODULE_AVAILABLE_yup_gui_extra
            if (yup_offerEventToActiveXControl (m) != S_FALSE)
                return true;
#endif

            if (m.message == customMessageID && m.hwnd == yup_messageWindowHandle)
            {
                dispatchMessages();
            }
            else if (m.message == WM_QUIT)
            {
                if (auto* app = YUPApplicationBase::getInstance())
                    app->systemRequestedQuit();
            }
            else
            {
                if ((m.message == WM_LBUTTONDOWN || m.message == WM_RBUTTONDOWN)
                    && ! YupWindowIdentifier::isYUPWindow (m.hwnd))
                {
                    // if it's someone else's window being clicked on, and the focus is
                    // currently on a yup window, pass the kb focus over..
                    auto currentFocus = GetFocus();

                    if (currentFocus == nullptr || YupWindowIdentifier::isYUPWindow (currentFocus))
                        SetFocus (m.hwnd);
                }

                TranslateMessage (&m);
                DispatchMessage (&m);
            }
        }

        return true;
    }

private:
    //==============================================================================
    static LRESULT CALLBACK messageWndProc (HWND h, UINT message, WPARAM wParam, LPARAM lParam) noexcept
    {
        if (h == yup_messageWindowHandle)
        {
            if (message == customMessageID)
            {
                if (auto* queue = InternalMessageQueue::getInstanceWithoutCreating())
                    queue->dispatchMessages();

                return 0;
            }

            if (message == WM_COPYDATA)
            {
                handleBroadcastMessage (reinterpret_cast<const COPYDATASTRUCT*> (lParam));
                return 0;
            }

            if (message == WM_SETTINGCHANGE)
                NullCheckedInvocation::invoke (settingChangeCallback);
        }

        return DefWindowProc (h, message, wParam, lParam);
    }

    static BOOL CALLBACK broadcastEnumWindowProc (HWND hwnd, LPARAM lParam)
    {
        if (hwnd != yup_messageWindowHandle)
        {
            TCHAR windowName[64] = { 0 }; // no need to read longer strings than this
            GetWindowText (hwnd, windowName, 63);

            if (String (windowName) == messageWindowName)
                reinterpret_cast<Array<HWND>*> (lParam)->add (hwnd);
        }

        return TRUE;
    }

    static void dispatchMessage (MessageManager::MessageBase* message)
    {
        YUP_TRY
        {
            message->messageCallback();
        }
        YUP_CATCH_EXCEPTION

        message->decReferenceCount();
    }

    static void handleBroadcastMessage (const COPYDATASTRUCT* data)
    {
        if (data != nullptr && data->dwData == broadcastMessageMagicNumber)
        {
            struct BroadcastMessage final : public CallbackMessage
            {
                BroadcastMessage (CharPointer_UTF32 text, size_t length)
                    : message (text, length)
                {
                }

                void messageCallback() override { MessageManager::getInstance()->deliverBroadcastMessage (message); }

                String message;
            };

            (new BroadcastMessage (CharPointer_UTF32 ((const CharPointer_UTF32::CharType*) data->lpData),
                                   data->cbData / sizeof (CharPointer_UTF32::CharType)))
                ->post();
        }
    }

    void dispatchMessages()
    {
        ReferenceCountedArray<MessageManager::MessageBase> messagesToDispatch;

        {
            const ScopedLock sl (lock);

            if (messageQueue.isEmpty())
                return;

            messagesToDispatch.swapWith (messageQueue);
        }

        for (int i = 0; i < messagesToDispatch.size(); ++i)
        {
            auto message = messagesToDispatch.getUnchecked (i);
            message->incReferenceCount();
            dispatchMessage (message.get());
        }
    }

    //==============================================================================
    static constexpr unsigned int customMessageID = WM_USER + 123;
    static constexpr unsigned int broadcastMessageMagicNumber = 0xc403;
    static const TCHAR messageWindowName[];

    std::unique_ptr<HiddenMessageWindow> messageWindow;

    CriticalSection lock;
    ReferenceCountedArray<MessageManager::MessageBase> messageQueue;
    std::function<void()> loopCallback;
    std::atomic_bool loopCallbackRecursiveCheck = false;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InternalMessageQueue)
};

YUP_IMPLEMENT_SINGLETON (InternalMessageQueue)

const TCHAR InternalMessageQueue::messageWindowName[] = _T("YUPWindow");

//==============================================================================
bool yup_dispatchNextMessageOnSystemQueue (bool returnIfNoPendingMessages)
{
    if (auto* queue = InternalMessageQueue::getInstanceWithoutCreating())
        return queue->dispatchNextMessage (returnIfNoPendingMessages);

    return false;
}

//==============================================================================
bool MessageManager::postMessageToSystemQueue (MessageManager::MessageBase* const message)
{
    if (auto* queue = InternalMessageQueue::getInstanceWithoutCreating())
    {
        queue->postMessage (message);
        return true;
    }

    return false;
}

void MessageManager::broadcastMessage (const String& value)
{
    if (auto* queue = InternalMessageQueue::getInstanceWithoutCreating())
        queue->broadcastMessage (value);
}

//==============================================================================
void MessageManager::doPlatformSpecificInitialisation()
{
    [[maybe_unused]] const auto result = OleInitialize (nullptr);

    InternalMessageQueue::getInstance();
}

void MessageManager::doPlatformSpecificShutdown()
{
    InternalMessageQueue::deleteInstance();
    OleUninitialize();
}

//==============================================================================
struct MountedVolumeListChangeDetector::Pimpl
{
    explicit Pimpl (MountedVolumeListChangeDetector& d)
        : owner (d)
    {
        File::findFileSystemRoots (lastVolumeList);
    }

    void systemDeviceChanged()
    {
        Array<File> newList;
        File::findFileSystemRoots (newList);

        if (std::exchange (lastVolumeList, newList) != newList)
            owner.mountedVolumeListChanged();
    }

    DeviceChangeDetector detector { L"MountedVolumeList", [this]
    {
        systemDeviceChanged();
    } };
    MountedVolumeListChangeDetector& owner;
    Array<File> lastVolumeList;
};

MountedVolumeListChangeDetector::MountedVolumeListChangeDetector() { pimpl.reset (new Pimpl (*this)); }

MountedVolumeListChangeDetector::~MountedVolumeListChangeDetector() {}

} // namespace yup
