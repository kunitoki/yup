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

#include "PresetBank.h"

PresetBank::PresetBank (const yup::File& file)
    : zip (std::make_unique<yup::ZipFile> (file))
{
}

PresetBank::PresetBank (const void* data, std::size_t size)
    : zip (std::make_unique<yup::ZipFile> (new yup::MemoryInputStream (data, size, false), true))
{
}

PresetBank::~PresetBank() = default;

yup::StringArray PresetBank::getEntryPaths (const yup::String& extension) const
{
    yup::StringArray paths;

    for (int index = 0; index < zip->getNumEntries(); ++index)
    {
        const auto* entry = zip->getEntry (index);

        if (! entry->isSymbolicLink && entry->filename.endsWithIgnoreCase (extension))
            paths.add (entry->filename);
    }

    paths.sortNatural();
    return paths;
}

yup::String PresetBank::readEntry (const yup::String& path, yup::int64 maxBytes) const
{
    const auto* entry = zip->getEntry (path);

    if (entry == nullptr || entry->isSymbolicLink || entry->uncompressedSize <= 0 || entry->uncompressedSize > maxBytes)
        return {};

    const std::unique_ptr<yup::InputStream> stream (zip->createStreamForEntry (*entry));
    return stream != nullptr ? stream->readEntireStreamAsString() : yup::String();
}
