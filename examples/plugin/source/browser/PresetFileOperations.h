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
#include "PresetUserData.h"

//==============================================================================
/** The browser's operations on preset files.

    Each one either succeeds completely or leaves the files and the user data as they were.
    Factory presets live in code, so every operation refuses them. A file they write gets an
    id when it has none, which becomes its key, and its favorite and rating follow.

    @see PresetLibrary, PresetUserData
*/
namespace PresetFileOperations
{
/** Returns where a preset called name goes in the folder: the legal file name plus the
    format's extension. Invalid for a name with no legal characters.
*/
yup::File getFileFor (const yup::File& folder, const yup::String& name, const PresetFormat& format);

/** Moves the file to the new name in the same folder and writes the new file's name inside
    it, so the two stay in step: characters a file name cannot hold are dropped from both.
    Returns the renamed entry. Fails for factory presets, empty names and names already
    taken.
*/
yup::ResultValue<PresetEntry> rename (const PresetEntry& entry, const yup::String& newName, const PresetFormat& format, PresetUserData& userData);

/** Copies the file to "<name> copy" next to it, numbered when taken, with the name inside
    matching the new file name and an id of its own. The copy gets the original's favorite
    and rating.
*/
yup::ResultValue<yup::File> duplicate (const PresetEntry& entry, const PresetFormat& format, PresetUserData& userData);

/** Deletes the file permanently and forgets its favorite and rating. Fails for factory presets. */
yup::Result remove (const PresetEntry& entry, PresetUserData& userData);

/** Writes the edited metadata into the entry's file and returns the entry as written.
    Fails for factory presets.
*/
yup::ResultValue<PresetEntry> writeMetadata (const PresetEntry& edited, const PresetFormat& format, PresetUserData& userData);
} // namespace PresetFileOperations
