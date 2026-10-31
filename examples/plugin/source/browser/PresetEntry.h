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
/** One preset the browser lists: a factory preset defined in code, or a preset file.

    @see PresetLibrary, PresetQuery
*/
struct PresetEntry
{
    yup::String key; /**< The id, or the file's full path for a file without one. */
    yup::String id; /**< The UUID stored with the preset, empty for a file saved without one. */
    yup::File file; /**< Invalid for a factory preset. */
    int factoryIndex = -1; /**< The factory preset's index, or -1 for a file. */
    yup::String name, category, author, bank, description, notes;
    yup::StringArray tags;
    yup::String location; /**< "Factory", or the root folder's display name and subfolders: "User / Pads". */
    yup::Time modified; /**< Time() for factory presets. */

    /** True for a preset defined in code rather than in a file. */
    bool isFactory() const noexcept { return factoryIndex >= 0; }

    /** Splits comma separated tags, trimmed, without empty ones, and keeps the first spelling
        of tags that differ only in case.
    */
    static yup::StringArray parseTags (const yup::String& commaSeparated);

    /** Joins tags with ", ", the inverse of parseTags(). */
    static yup::String joinTags (const yup::StringArray& tags);
};
