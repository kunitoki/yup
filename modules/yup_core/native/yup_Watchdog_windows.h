/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

class Watchdog::Impl final
{
public:
    Impl (std::weak_ptr<Watchdog> owner, const File& folder)
        : owner (std::move (owner))
        , folder (folder)
    {
        WCHAR path[_MAX_PATH] = { 0 };
        wcsncpy_s (path, folder.getFullPathName().toWideCharPointer(), _MAX_PATH - 1);

        folderHandle = CreateFileW (path,
                                    FILE_LIST_DIRECTORY,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                    nullptr,
                                    OPEN_EXISTING,
                                    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                                    nullptr);

        if (folderHandle != INVALID_HANDLE_VALUE)
        {
            overlapped.hEvent = CreateEvent (nullptr, TRUE, FALSE, nullptr);

            thread = std::thread ([this]
            {
                threadCallback();
            });
        }
    }

    ~Impl()
    {
        if (thread.joinable())
        {
            threadShouldExit = true;

            if (folderHandle != INVALID_HANDLE_VALUE)
            {
                CancelIoEx (folderHandle, &overlapped);

                if (overlapped.hEvent)
                    SetEvent (overlapped.hEvent);
            }

            thread.join();
        }

        if (folderHandle != INVALID_HANDLE_VALUE)
            CloseHandle (folderHandle);

        if (overlapped.hEvent)
            CloseHandle (overlapped.hEvent);
    }

private:
    void threadCallback()
    {
        constexpr DWORD bufferSize = 16 * 1024;
        std::vector<uint8_t> buffer (bufferSize);

        DWORD bytesOut = 0;
        auto lastRenamedPath = std::optional<File> {};

        while (! threadShouldExit)
        {
            ResetEvent (overlapped.hEvent);

            const BOOL success = ReadDirectoryChangesW (folderHandle,
                                                        buffer.data(),
                                                        bufferSize,
                                                        TRUE,
                                                        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
                                                        nullptr,
                                                        &overlapped,
                                                        nullptr);

            if (! success)
            {
                DWORD err = GetLastError();
                if (err == ERROR_OPERATION_ABORTED || threadShouldExit)
                    break;

                continue;
            }

            DWORD waitResult = WaitForSingleObjectEx (overlapped.hEvent, INFINITE, TRUE);

            if (threadShouldExit)
                break;

            if (waitResult != WAIT_OBJECT_0)
                continue;

            if (! GetOverlappedResult (folderHandle, &overlapped, &bytesOut, FALSE))
            {
                DWORD err = GetLastError();
                if (err == ERROR_OPERATION_ABORTED || threadShouldExit)
                    break;

                continue;
            }

            if (bytesOut == 0)
                continue;

            if (! parseNotifications (buffer.data(), bytesOut, lastRenamedPath))
                break;
        }
    }

    bool parseNotifications (uint8_t* data, DWORD bytesOut, std::optional<File>& lastRenamedPath)
    {
        std::vector<Watchdog::Event> localEvents;

        uint8_t* raw = data;

        while (true)
        {
            const FILE_NOTIFY_INFORMATION* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*> (rawData);

            auto path = folder.getChildFile (String (fni->FileName, fni->FileNameLength / sizeof (wchar_t)));
            if (path.isHidden())
                return true;

            auto event = Watchdog::EventType::undefined;
            switch (fni->Action)
            {
                case FILE_ACTION_ADDED:
                    event = Watchdog::EventType::file_created;
                    break;

                case FILE_ACTION_MODIFIED:
                    event = Watchdog::EventType::file_updated;
                    break;

                case FILE_ACTION_REMOVED:
                    event = Watchdog::EventType::file_deleted;
                    break;

                case FILE_ACTION_RENAMED_NEW_NAME:
                case FILE_ACTION_RENAMED_OLD_NAME:
                {
                    if (lastRenamedPath)
                    {
                        event = Watchdog::EventType::file_renamed;

                        if (fni->Action == FILE_ACTION_RENAMED_OLD_NAME && ! path.exists())
                            lastRenamedPath = std::exchange (path, *lastRenamedPath);
                    }
                    else
                    {
                        lastRenamedPath = path;
                    }

                    break;
                }

                default:
                    break;
            }

            if (event != Watchdog::EventType::undefined)
            {
                auto otherPath = std::optional<File> {};

                if (event == Watchdog::EventType::file_renamed)
                    otherPath = std::exchange (lastRenamedPath, std::optional<File> {});

                events.emplace_back (event, path, otherPath);
            }

            if (fni->NextEntryOffset > 0)
                rawData += fni->NextEntryOffset;
            else
                return false;
        }

        if (lastRenamedPath)
        {
            auto newEventType = Watchdog::EventType::file_created;
            if (! lastRenamedPath->exists())
                newEventType = Watchdog::EventType::file_deleted;

            events.emplace_back (newEventType, *lastRenamedPath);
        }

        if (! events.empty())
        {
            if (auto lockedOwner = owner.lock())
            {
                lockedOwner->enqueueEvents (std::move (events));
                events.clear();
            }
            else
            {
                return false;
            }
        }

        return true;
    }

    std::weak_ptr<Watchdog> owner;
    const File folder;
    std::vector<Watchdog::Event> events;
    std::thread thread;
    std::atomic_bool threadShouldExit = false;

    HANDLE folderHandle = INVALID_HANDLE_VALUE;
    OVERLAPPED overlapped = {};
};

} // namespace yup
