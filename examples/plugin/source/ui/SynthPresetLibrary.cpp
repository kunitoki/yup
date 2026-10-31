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

#include "SynthPresetLibrary.h"
#include "SynthPresetFormat.h"
#include "SynthSettings.h"

std::shared_ptr<SynthPresetLibrary> SynthPresetLibrary::acquire()
{
    static std::weak_ptr<SynthPresetLibrary> shared;

    if (auto existing = shared.lock())
        return existing;

    auto created = std::shared_ptr<SynthPresetLibrary> (new SynthPresetLibrary());
    shared = created;
    return created;
}

SynthPresetLibrary::SynthPresetLibrary()
    : library (SynthPresetFormat::create())
{
    std::vector<PresetEntry> factoryEntries;

    for (int index = 0; index < SynthPatch::getNumFactoryPresets(); ++index)
        factoryEntries.push_back (SynthPresetFormat::makeFactoryEntry (index));

    library.setFactoryEntries (std::move (factoryEntries));
    applyFolders();
}

yup::File SynthPresetLibrary::getUserFolder()
{
    auto folder = yup::File::getSpecialLocation (yup::File::userAudioPresetsDirectory)
                      .getChildFile (yup::File::createLegalFileName (YupPlugin_Vendor))
                      .getChildFile (yup::File::createLegalFileName (YupPlugin_Name));
    folder.createDirectory();
    return folder;
}

yup::Array<yup::File> SynthPresetLibrary::getExtraFolders() const
{
    yup::Array<yup::File> folders;

    for (const auto& path : SynthSettings::get().presetFolders)
        folders.add (yup::File (path));

    return folders;
}

void SynthPresetLibrary::addFolder (const yup::File& folder)
{
    if (folder == getUserFolder())
        return;

    const auto added = SynthSettings::updateBrowserData ([&] (SynthSettings::Values& values)
    {
        if (getExtraFolders().contains (folder))
            return false;

        values.presetFolders.add (folder.getFullPathName());
        return true;
    });

    if (added)
        applyFolders();
}

void SynthPresetLibrary::removeFolder (const yup::File& folder)
{
    const auto removed = SynthSettings::updateBrowserData ([&] (SynthSettings::Values& values)
    {
        const auto index = getExtraFolders().indexOf (folder);

        if (index < 0)
            return false;

        values.presetFolders.remove (index);
        return true;
    });

    if (removed)
        applyFolders();
}

void SynthPresetLibrary::addFileListener (FileListener* listener)
{
    fileListeners.add (listener);
}

void SynthPresetLibrary::removeFileListener (FileListener* listener)
{
    fileListeners.remove (listener);
}

void SynthPresetLibrary::presetFileChanged (const yup::File& previousFile, const PresetEntry& entry)
{
    fileListeners.call (&FileListener::presetFileChanged, previousFile, entry);
}

void SynthPresetLibrary::applyFolders()
{
    std::vector<PresetLibrary::Folder> folders { { getUserFolder(), "User" } };

    for (const auto& folder : getExtraFolders())
        folders.push_back ({ folder, folder.getFileName() });

    library.setFolders (std::move (folders));
    library.rescan();
}
