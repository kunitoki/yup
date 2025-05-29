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

#import <Foundation/Foundation.h>

namespace juce
{

static void printEventFlags(FSEventStreamEventFlags flags)
{
    std::vector<String> descriptions;

    // Handle global events
    if (flags & kFSEventStreamEventFlagItemCreated)
        descriptions.emplace_back("Item Created");
    if (flags & kFSEventStreamEventFlagItemRemoved)
        descriptions.emplace_back("Item Removed");
    if (flags & kFSEventStreamEventFlagItemInodeMetaMod)
        descriptions.emplace_back("Inode Metadata Modified");
    if (flags & kFSEventStreamEventFlagItemRenamed)
        descriptions.emplace_back("Item Renamed");
    if (flags & kFSEventStreamEventFlagItemModified)
        descriptions.emplace_back("Item Modified");
    if (flags & kFSEventStreamEventFlagItemFinderInfoMod)
        descriptions.emplace_back("Finder Info Modified");
    if (flags & kFSEventStreamEventFlagItemChangeOwner)
        descriptions.emplace_back("Owner Changed");
    if (flags & kFSEventStreamEventFlagItemXattrMod)
        descriptions.emplace_back("Extended Attributes Modified");
    if (flags & kFSEventStreamEventFlagItemIsFile)
        descriptions.emplace_back("Is File");
    if (flags & kFSEventStreamEventFlagItemIsDir)
        descriptions.emplace_back("Is Directory");
    if (flags & kFSEventStreamEventFlagItemIsSymlink)
        descriptions.emplace_back("Is Symlink");

    // Handle general flags
    if (flags & kFSEventStreamEventFlagMustScanSubDirs)
        descriptions.emplace_back("Must Scan Subdirectories");
    if (flags & kFSEventStreamEventFlagUserDropped)
        descriptions.emplace_back("User Dropped");
    if (flags & kFSEventStreamEventFlagKernelDropped)
        descriptions.emplace_back("Kernel Dropped");
    if (flags & kFSEventStreamEventFlagEventIdsWrapped)
        descriptions.emplace_back("Event IDs Wrapped");
    if (flags & kFSEventStreamEventFlagHistoryDone)
        descriptions.emplace_back("History Done");
    if (flags & kFSEventStreamEventFlagRootChanged)
        descriptions.emplace_back("Root Changed");
    if (flags & kFSEventStreamEventFlagMount)
        descriptions.emplace_back("Mount");
    if (flags & kFSEventStreamEventFlagUnmount)
        descriptions.emplace_back("Unmount");

    // Print all decoded flags
    String output;
    output << "0x" << String::toHexString(flags) << " (";

    if (!descriptions.empty())
    {
        for (std::size_t i = 0; i < descriptions.size(); ++i)
        {
            output << descriptions[i];
            if (i < descriptions.size() - 1)
                output << ", ";
        }
    }
    else
    {
        output << "No flags set";
    }

    output << ")";

    juce::Logger::getCurrentLogger()->writeToLog(output);
}

class Watchdog::Impl final
{
   public:
    Impl(std::weak_ptr<Watchdog> owner, const File& folder)
        : owner(std::move(owner)), folder(folder)
    {
        NSString* newPath = [NSString stringWithUTF8String:folder.getFullPathName().toRawUTF8()];

        paths = [[NSArray arrayWithObject:newPath] retain];

        context.version = 0l;
        context.info = this;
        context.retain = nil;
        context.release = nil;
        context.copyDescription = nil;

        dispatch_queue_t queue = dispatch_queue_create("com.yup.watchdog", DISPATCH_QUEUE_SERIAL);

        stream = FSEventStreamCreate(
            kCFAllocatorDefault, callback, &context, reinterpret_cast<CFArrayRef>(paths),
            kFSEventStreamEventIdSinceNow, 0.1,
            kFSEventStreamCreateFlagNoDefer | kFSEventStreamCreateFlagFileEvents);

        if (stream != nullptr)
        {
            FSEventStreamSetDispatchQueue(stream, queue);
            FSEventStreamStart(stream);
        }
    }

    ~Impl()
    {
        if (stream != nullptr)
        {
            FSEventStreamFlushSync(stream);
            FSEventStreamStop(stream);
            FSEventStreamSetDispatchQueue(stream, nullptr);
            FSEventStreamInvalidate(stream);
            FSEventStreamRelease(stream);
        }
    }

    static void callback(
        ConstFSEventStreamRef streamRef,
        void* clientCallbackInfo,
        std::size_t numEvents,
        void* eventPaths,
        const FSEventStreamEventFlags* eventFlags,
        const FSEventStreamEventId* eventIds)
    {
        ignoreUnused(streamRef, numEvents, eventPaths, eventFlags, eventIds);

        auto* implementation = reinterpret_cast<Impl*>(clientCallbackInfo);
        if (implementation == nullptr)
            return;

        implementation->events.clear();
        implementation->events.reserve(numEvents);

        char** files = reinterpret_cast<char**>(eventPaths);
        auto lastRenamedPath = std::optional<File>{};

        for (int i = 0; i < static_cast<int>(numEvents); ++i)
        {
            auto event = Watchdog::EventType::undefined;
            auto path = File(files[i]);

            if (path.isHidden())
                continue;

            FSEventStreamEventFlags evt = eventFlags[i];

            // DBG (path.getFullPathName());
            // printEventFlags (evt);

            if (evt & kFSEventStreamEventFlagItemModified)
            {
                event = Watchdog::EventType::file_updated;
            }
            else if (evt & kFSEventStreamEventFlagItemRemoved)
            {
                event = Watchdog::EventType::file_deleted;
            }
            else if (evt & kFSEventStreamEventFlagItemCreated)
            {
                event = Watchdog::EventType::file_created;
            }
            else if (evt & kFSEventStreamEventFlagItemRenamed)
            {
                if (lastRenamedPath)
                {
                    event = Watchdog::EventType::file_renamed;

                    if (!path.exists())
                        lastRenamedPath = std::exchange(path, *lastRenamedPath);
                }
                else
                {
                    lastRenamedPath = path;
                }
            }

            if (event != Watchdog::EventType::undefined)
            {
                auto otherPath = std::optional<File>{};

                if (event == Watchdog::EventType::file_renamed)
                    otherPath = std::exchange(lastRenamedPath, std::optional<File>{});

                implementation->events.emplace_back(event, path, otherPath);
            }
        }

        if (lastRenamedPath)
        {
            auto newEventType = Watchdog::EventType::file_created;
            if (lastRenamedPath->exists())
                newEventType = Watchdog::EventType::file_deleted;

            implementation->events.emplace_back(newEventType, *lastRenamedPath);
        }

        if (!implementation->events.empty())
        {
            if (auto lockedOwner = implementation->owner.lock())
            {
                lockedOwner->enqueueEvents (std::move (implementation->events));
                implementation->events.clear();
            }
        }
    }

    std::weak_ptr<Watchdog> owner;
    const File folder;
    std::vector<Watchdog::Event> events;

    NSArray* paths = nil;
    FSEventStreamRef stream = nil;
    struct FSEventStreamContext context;
};

} // namespace juce
