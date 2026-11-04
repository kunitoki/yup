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

#include <yup_events/yup_events.h>

//==============================================================================
/** One preset the browser lists: a factory preset shipped with the plugin, a preset file,
    or a preset inside a bank file.

    @see PresetLibrary, PresetQuery, PresetBank
*/
struct PresetEntry
{
    yup::String key; /**< The id, or the full path of the file (and of the entry in a bank) for a preset without one. */
    yup::String id; /**< The UUID stored with the preset, empty for a file saved without one. */
    yup::File file; /**< The preset file or the bank holding the preset, invalid for a factory preset. */
    yup::String bankPath; /**< The preset's path inside the bank file, empty for a preset that is not in one. */
    int factoryIndex = -1; /**< The factory preset's index, or -1 for a file. */
    yup::String name, category, author, bank, description, notes;
    yup::StringArray tags;
    yup::String location; /**< "Factory", or the root folder's display name and subfolders: "User / Pads". */
    yup::Time modified; /**< Time() for factory presets. */

    /** True for a preset shipped with the plugin rather than found in a folder. */
    bool isFactory() const noexcept { return factoryIndex >= 0; }

    /** True for a preset read from a bank file. */
    bool isInBank() const noexcept { return bankPath.isNotEmpty(); }

    /** True for a preset that can't be renamed, duplicated, deleted or edited. */
    bool isReadOnly() const noexcept { return isFactory() || isInBank(); }

    /** Splits comma separated tags, trimmed, without empty ones, and keeps the first spelling
        of tags that differ only in case.
    */
    static yup::StringArray parseTags (const yup::String& commaSeparated);

    /** Joins tags with ", ", the inverse of parseTags(). */
    static yup::String joinTags (const yup::StringArray& tags);
};
