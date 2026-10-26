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

#pragma once

#include "PresetEntry.h"
#include "PresetFormat.h"

#include <atomic>
#include <memory>
#include <unordered_map>
#include <vector>

//==============================================================================
/** Every preset the browser knows: the factory presets plus the preset files found in a
    list of folders.

    The folders are walked and each file's metadata is read on a background thread; the
    message thread only receives the finished list, so a big library never stalls the UI.
    A rescan supersedes a running one. Every change of the list is broadcast.

    All of the methods are for the message thread.

    @see PresetFormat, PresetEntry, PresetBrowseState
*/
class PresetLibrary : public yup::ChangeBroadcaster
{
public:
    /** A folder to scan, with the name its presets show as their location. */
    struct Folder
    {
        yup::File folder;
        yup::String displayName;

        bool operator== (const Folder&) const = default;
    };

    /** Creates an empty library reading files of this format. */
    explicit PresetLibrary (PresetFormat format);

    /** Stops a running scan and waits for it. */
    ~PresetLibrary() override;

    /** Returns the format the files are read and written with. */
    const PresetFormat& getFormat() const noexcept;

    /** Replaces the factory presets, which are listed before the files. */
    void setFactoryEntries (std::vector<PresetEntry> entries);

    /** Replaces the folders to scan, dropping exact duplicates. Call rescan() to list them. */
    void setFolders (std::vector<Folder> newFolders);

    /** Returns the folders to scan. */
    const std::vector<Folder>& getFolders() const noexcept;

    /** Starts a background scan of the folders, superseding a running one. */
    void rescan();

    /** True from rescan() until its result arrives. */
    bool isScanning() const noexcept;

    /** Returns the factory presets, then the files found by the last scan. A file found
        under several folders is listed once, under the first of them.
    */
    const std::vector<PresetEntry>& getEntries() const noexcept;

    /** Returns the entry with this key, or nullptr. */
    const PresetEntry* findEntry (const yup::String& key) const;

    /** Returns every directory under the folder found by the last scan, sorted. */
    yup::Array<yup::File> getSubfolders (const yup::File& folder) const;

private:
    struct ScanResult
    {
        std::vector<PresetEntry> files;
        yup::Array<yup::File> directories;
    };

    static ScanResult scanFolders (const std::vector<Folder>& folders, const PresetFormat& format, const std::atomic<bool>& cancelled);
    void applyScan (ScanResult result);
    void rebuildIndex();

    PresetFormat format;
    std::vector<Folder> folders;
    std::vector<PresetEntry> entries;
    std::size_t numFactoryEntries = 0;
    std::unordered_map<yup::String, std::size_t> indexByKey;
    yup::Array<yup::File> directories;
    std::shared_ptr<std::atomic<bool>> scanCancelled;
    int scanGeneration = 0;
    bool scanning = false;
    yup::ThreadPool pool;

    YUP_DECLARE_WEAK_REFERENCEABLE (PresetLibrary)
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetLibrary)
};
