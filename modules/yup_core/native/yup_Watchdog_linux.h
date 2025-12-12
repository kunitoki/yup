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
        fd = inotify_init();
        if (fd < 0)
            return;

        int flags = fcntl (fd, F_GETFL, 0);
        fcntl (fd, F_SETFL, flags | O_NONBLOCK);

        addPaths (folder);

        thread = std::thread ([this]
        {
            threadCallback();
        });
    }

    ~Impl()
    {
        if (thread.joinable())
        {
            threadShouldExit = true;
            thread.join();

            removeAllPaths();

            if (fd >= 0)
                close (fd);
        }
    }

private:
    void addPath (const File& path)
    {
        if (fd < 0 || ! path.isDirectory())
            return;

        auto pathString = path.getFullPathName();

        auto it = watchDescriptors.find (pathString);
        if (it != watchDescriptors.end())
            return;

        const auto wd = inotify_add_watch (fd,
                                           pathString.toRawUTF8(),
                                           IN_ATTRIB | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF | IN_MOVED_TO | IN_MOVED_FROM);
        if (wd < 0)
            return;

        watchDescriptors[pathString] = wd;
    }

    void removePath (const File& path)
    {
        if (fd < 0)
            return;

        auto removedPath = path.getFullPathName();

        {
            auto it = watchDescriptors.find (removedPath);
            if (it != watchDescriptors.end())
            {
                inotify_rm_watch (fd, it->second);
                watchDescriptors.erase (it);
            }
        }

        if (! removedPath.endsWith ("/"))
            removedPath << "/";

        for (auto it = watchDescriptors.begin(); it != watchDescriptors.end();)
        {
            if (it->first.startsWith (removedPath))
            {
                inotify_rm_watch (fd, it->second);
                it = watchDescriptors.erase (it);
            }
            else
            {
                ++it;
            }
        }
    }

    void addPaths (const File& path)
    {
        if (path.isDirectory())
        {
            addPath (path);

            for (const auto& entry : path.findChildFiles (File::findDirectories, false))
                addPaths (entry);
        }
    }

    void removeAllPaths()
    {
        if (fd < 0)
            return;

        for (const auto& [path, wd] : watchDescriptors)
            inotify_rm_watch (fd, wd);

        watchDescriptors.clear();
    }

    void updatePathFromEvent (const Watchdog::Event& event)
    {
        if (event.changeEvent == Watchdog::EventType::file_renamed)
        {
            if (event.renamedFile)
                removePath (*event.renamedFile);

            addPaths (event.originalFile);
        }
        else if (event.changeEvent == Watchdog::EventType::file_deleted)
        {
            removePath (event.originalFile);
        }
        else if (event.changeEvent == Watchdog::EventType::file_created)
        {
            addPaths (event.originalFile);
        }
    }

    void threadCallback()
    {
        constexpr std::size_t bufferSize = (32 * (sizeof (struct inotify_event) + NAME_MAX + 1));
        std::vector<char> buffer (bufferSize, 0);

        auto lastRenamedPath = std::optional<File> {};

        while (! threadShouldExit)
        {
            const ssize_t numRead = read (fd, buffer.data(), bufferSize);
            if (numRead < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                std::this_thread::sleep_for (std::chrono::milliseconds (50));
                continue;
            }

            if (threadShouldExit)
                break;

            if (numRead <= 0)
                continue;

            const inotify_event* notifyEvent = nullptr;
            for (const char* ptr = buffer.data(); ptr < buffer.data() + numRead; ptr += offsetof (struct inotify_event, name) + notifyEvent ? notifyEvent->len : 0)
            {
                notifyEvent = reinterpret_cast<const inotify_event*> (ptr);

                auto path = folder.getChildFile (String::fromUTF8 (notifyEvent->name));
                if (path.isHidden())
                    continue;

                auto event = Watchdog::EventType::undefined;
                if (notifyEvent->mask & IN_CREATE)
                    event = Watchdog::EventType::file_created;
                else if (notifyEvent->mask & IN_CLOSE_WRITE)
                    event = Watchdog::EventType::file_updated;
                else if (notifyEvent->mask & IN_MODIFY)
                    event = Watchdog::EventType::file_updated;
                else if (notifyEvent->mask & IN_DELETE)
                    event = Watchdog::EventType::file_deleted;
                else if ((notifyEvent->mask & IN_MOVED_FROM) || (notifyEvent->mask & IN_MOVED_TO))
                {
                    if (lastRenamedPath)
                    {
                        event = Watchdog::EventType::file_renamed;

                        if ((notifyEvent->mask & IN_MOVED_FROM) && ! path.exists())
                            lastRenamedPath = std::exchange (path, *lastRenamedPath);
                    }
                    else
                    {
                        lastRenamedPath = path;
                    }
                }

                if (event != Watchdog::EventType::undefined)
                {
                    auto otherPath = std::optional<File> {};

                    if (event == Watchdog::EventType::file_renamed)
                        otherPath = std::exchange (lastRenamedPath, std::optional<File> {});

                    events.emplace_back (event, path, otherPath);
                }
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
                for (const auto& event : events)
                    updatePathFromEvent (event);

                if (auto lockedOwner = owner.lock())
                    lockedOwner->enqueueEvents (std::move (events));

                events.clear();
            }
        }
    }

    std::weak_ptr<Watchdog> owner;
    const File folder;
    std::vector<Watchdog::Event> events;
    std::thread thread;
    std::atomic_bool threadShouldExit = false;

    std::unordered_map<String, int> watchDescriptors;
    int fd = -1;
};

} // namespace yup
