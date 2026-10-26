/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#include "PresetLibrary.h"

#include <algorithm>
#include <iterator>
#include <unordered_set>

namespace
{
yup::String makePresetLocation (const PresetLibrary::Folder& root, const yup::File& file)
{
    yup::StringArray parts;

    for (auto directory = file.getParentDirectory(); directory.isAChildOf (root.folder); directory = directory.getParentDirectory())
        parts.insert (0, directory.getFileName());

    parts.insert (0, root.displayName);
    return parts.joinIntoString (" / ");
}
} // namespace

PresetLibrary::PresetLibrary (PresetFormat formatToUse)
    : format (std::move (formatToUse))
    , pool (yup::ThreadPoolOptions {}.withThreadName ("Preset scan").withNumberOfThreads (1))
{
}

PresetLibrary::~PresetLibrary()
{
    if (scanCancelled != nullptr)
        scanCancelled->store (true);

    pool.removeAllJobs (true, 2000);
}

const PresetFormat& PresetLibrary::getFormat() const noexcept
{
    return format;
}

void PresetLibrary::setFactoryEntries (std::vector<PresetEntry> newEntries)
{
    entries.erase (entries.begin(), entries.begin() + static_cast<std::ptrdiff_t> (numFactoryEntries));
    numFactoryEntries = newEntries.size();
    entries.insert (entries.begin(), std::make_move_iterator (newEntries.begin()), std::make_move_iterator (newEntries.end()));

    rebuildIndex();
    sendChangeMessage();
}

void PresetLibrary::setFolders (std::vector<Folder> newFolders)
{
    folders.clear();

    for (auto& folder : newFolders)
    {
        if (std::find (folders.begin(), folders.end(), folder) == folders.end())
            folders.push_back (std::move (folder));
    }
}

const std::vector<PresetLibrary::Folder>& PresetLibrary::getFolders() const noexcept
{
    return folders;
}

void PresetLibrary::rescan()
{
    if (scanCancelled != nullptr)
        scanCancelled->store (true);

    auto cancelled = std::make_shared<std::atomic<bool>> (false);
    scanCancelled = cancelled;
    scanning = true;

    pool.addJob ([folders = folders, format = format, cancelled, generation = ++scanGeneration, library = yup::WeakReference<PresetLibrary> (this)]
    {
        auto result = scanFolders (folders, format, *cancelled);

        if (cancelled->load())
            return;

        yup::MessageManager::callAsync ([library, generation, result = std::move (result)]() mutable
        {
            if (auto* owner = library.get(); owner != nullptr && owner->scanGeneration == generation)
                owner->applyScan (std::move (result));
        });
    });
}

bool PresetLibrary::isScanning() const noexcept
{
    return scanning;
}

const std::vector<PresetEntry>& PresetLibrary::getEntries() const noexcept
{
    return entries;
}

const PresetEntry* PresetLibrary::findEntry (const yup::String& key) const
{
    const auto found = indexByKey.find (key);
    return found != indexByKey.end() ? &entries[found->second] : nullptr;
}

yup::Array<yup::File> PresetLibrary::getSubfolders (const yup::File& folder) const
{
    yup::Array<yup::File> result;

    for (const auto& directory : directories)
    {
        if (directory.isAChildOf (folder))
            result.add (directory);
    }

    return result;
}

PresetLibrary::ScanResult PresetLibrary::scanFolders (const std::vector<Folder>& folders, const PresetFormat& format, const std::atomic<bool>& cancelled)
{
    ScanResult result;
    std::unordered_set<yup::String> seenKeys;
    std::unordered_set<yup::String> seenDirectories;

    for (const auto& root : folders)
    {
        if (! root.folder.isDirectory())
            continue;

        for (const auto& child : yup::RangedDirectoryIterator (root.folder, true, "*", yup::File::findFilesAndDirectories, yup::File::FollowSymlinks::noCycles))
        {
            if (cancelled.load())
                return {};

            const auto file = child.getFile();

            if (child.isDirectory())
            {
                if (seenDirectories.insert (file.getFullPathName()).second)
                    result.directories.add (file);

                continue;
            }

            if (! file.hasFileExtension (format.extension) || ! seenKeys.insert (file.getFullPathName()).second)
                continue;

            auto entry = format.readEntry (file);

            if (! entry.has_value())
                continue;

            entry->key = file.getFullPathName();
            entry->file = file;
            entry->factoryIndex = -1;
            entry->location = makePresetLocation (root, file);
            entry->modified = child.getModificationTime();
            result.files.push_back (std::move (*entry));
        }
    }

    result.directories.sort();
    return result;
}

void PresetLibrary::applyScan (ScanResult result)
{
    entries.erase (entries.begin() + static_cast<std::ptrdiff_t> (numFactoryEntries), entries.end());
    entries.insert (entries.end(), std::make_move_iterator (result.files.begin()), std::make_move_iterator (result.files.end()));
    directories = std::move (result.directories);
    scanning = false;

    rebuildIndex();
    sendChangeMessage();
}

void PresetLibrary::rebuildIndex()
{
    indexByKey.clear();

    for (std::size_t index = 0; index < entries.size(); ++index)
        indexByKey.emplace (entries[index].key, index);
}
