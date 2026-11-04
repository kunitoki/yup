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

#include <yup_core/yup_core.h>

#include <memory>

//==============================================================================
/** A read-only bank of presets: a zip archive, usually renamed to a bank extension, holding
    preset files in any folder layout.

    @see PresetLibrary, PresetFormat
*/
class PresetBank
{
public:
    /** Opens a bank file. A file that is missing or not a zip archive opens as an empty bank. */
    explicit PresetBank (const yup::File& file);

    /** Opens a bank held in memory, which must outlive this object. */
    PresetBank (const void* data, std::size_t size);

    ~PresetBank();

    /** Returns the paths, relative to the bank root, of the entries with this extension, sorted. */
    yup::StringArray getEntryPaths (const yup::String& extension) const;

    /** Returns the text of an entry, or an empty string when it is missing, empty or larger
        than maxBytes.
    */
    yup::String readEntry (const yup::String& path, yup::int64 maxBytes) const;

private:
    std::unique_ptr<yup::ZipFile> zip;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBank)
};
