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

namespace juce
{

#if JUCE_LINUX || JUCE_WINDOWS || JUCE_MAC

Watchdog::Watchdog(std::chrono::milliseconds timeout)
    : timeout (timeout)
{
}

Watchdog::~Watchdog()
{
    unwatchAllFolders();
}

void Watchdog::watchFolder (const File& folder)
{
    if (! folder.isDirectory())
        return;

    auto it = watched.find (folder.getFullPathName());
    if (it != watched.end())
        return;

    watched[folder.getFullPathName()] = std::make_shared<Impl> (weak_from_this(), folder);
}

void Watchdog::unwatchFolder (const File& folder)
{
    watched.erase (folder.getFullPathName());
}

void Watchdog::unwatchAllFolders()
{
    watched.clear();
}

std::size_t Watchdog::dispatchEvents (const EventsCallback& callback)
{
    std::vector<Event> currentEvents;
    std::size_t currentEventsCount = 0;

    if (eventsCount == 0)
        return currentEventsCount;

    auto now = std::chrono::steady_clock::now();

    {
        auto lg = std::lock_guard (eventsMutex);

        if (now - lastEventsTime < timeout)
            return currentEventsCount;

        currentEvents = std::exchange (events, {});
        currentEventsCount = eventsCount.exchange (0);

        lastEventsTime = TimePoint();
    }

    if (callback)
        callback (std::move (currentEvents));

    return currentEventsCount;
}

void Watchdog::enqueueEvents (Span<Event> newEvents)
{
    auto lg = std::lock_guard (eventsMutex);

    events.reserve (events.size() + newEvents.size());
    events.insert (events.end(), std::make_move_iterator (newEvents.begin()), std::make_move_iterator (newEvents.end()));

    eventsCount = events.size();
    lastEventsTime = std::chrono::steady_clock::now();
}

std::vector<File> Watchdog::getAllWatchedFolders() const
{
    std::vector<File> result;
    result.reserve (watched.size());

    for (const auto& item : watched)
        result.emplace_back (item.first);

    std::sort (result.begin(), result.end(), std::less<>());

    return result;
}

#endif

} // namespace juce
