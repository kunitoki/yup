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

/**
    Watches a folder in the file system for changes.

    This is only available on macOS, windows, and Linux.

    The class `watchdog` will also recursively watch all subfolders on macOS, windows, and Linux.

    @tags{Core}
 */
class Watchdog : public std::enable_shared_from_this<Watchdog>
{
    using TimePoint = std::chrono::steady_clock::time_point;

public:
    /**
        This enum represents the different filesystem event types.
     */
    enum class EventType
    {
        undefined,    //< The file change event is unknown.
        file_created, //< The file has been created on disk.
        file_deleted, //< The file has been deleted from disk.
        file_updated, //< The file content has been updated.
        file_renamed  //< The file has been renamed.
    };

    /**
        This struct represents a filesystem event.
     */
    struct Event
    {
        Event (EventType changeEvent, const File& originalFile, std::optional<File> renamedFile = {})
            : changeEvent (changeEvent)
            , originalFile (std::move (originalFile))
            , renamedFile (std::move (renamedFile))
        {
        }

        EventType changeEvent;
        File originalFile;
        std::optional<File> renamedFile;
    };

    /**
        Constructs a new watchdog object.

        @param timeout The timeout for the watchdog.
     */
    Watchdog (std::chrono::milliseconds timeout);

    /**
        Destroys the watchdog object.
     */
    ~Watchdog();

    /**
        Watches a folder for changes.
     */
    void watchFolder (const File& folder);

    /**
        Unwatches a folder for changes.
     */
    void unwatchFolder (const File& folder);

    /**
        Unwatches all folders.
     */
    void unwatchAllFolders();

    /**
        Dispatches the events to the callback.
     */
    using EventsCallback = std::function<void (std::vector<Event>)>;
    std::size_t dispatchEvents (const EventsCallback& callback);

    /**
        Returns all watched folders.
     */
    std::vector<File> getAllWatchedFolders() const;

private:
    class Impl;
    friend class Impl;

    void enqueueEvents (std::vector<Event> newEvents);

    std::unordered_map<String, std::shared_ptr<Impl>> watched;
    std::chrono::milliseconds timeout;

    std::mutex eventsMutex;
    std::vector<Event> events;
    TimePoint lastEventsTime;
    std::atomic_size_t eventsCount = 0;
};

#endif

} // namespace juce
