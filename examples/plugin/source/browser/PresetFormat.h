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

#include <functional>
#include <optional>

//==============================================================================
/** How one kind of preset file is recognized, read and written.

    @see PresetLibrary, PresetFileOperations, PresetBank
*/
struct PresetFormat
{
    /** The largest preset read, a guard against loading anything else by mistake. */
    static constexpr yup::int64 maxPresetBytes = 4 * 1024 * 1024;

    yup::String extension; /**< With the dot: ".retrace". */
    yup::String bankExtension; /**< With the dot: ".rebank", the read-only zip archives of presets; empty for no banks. */

    /** Reads the id and metadata (name, category, author, bank, description, notes and tags)
        of one preset's content, leaving the key to the library and an empty name to the
        name of its file.

        Runs on the scan thread, so it must not touch shared state, and it never writes:
        files without an id get one only when they are next written. Returning nullopt
        skips the preset.
    */
    std::function<std::optional<PresetEntry> (const yup::String&)> readEntry;

    /** Writes the entry's id, name, category, author, bank, description, notes and tags into
        the file, keeping the rest of it. Called on the message thread.
    */
    std::function<yup::Result (const yup::File&, const PresetEntry&)> writeMetadata;
};
