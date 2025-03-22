/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

namespace yup
{

//==============================================================================

ArtboardFile::ArtboardFile (std::unique_ptr<rive::File> rivFile)
    : rivFile (std::move (rivFile))
{
}

//==============================================================================

const rive::File* ArtboardFile::getRiveFile() const
{
    return rivFile.get();
}

rive::File* ArtboardFile::getRiveFile()
{
    return rivFile.get();
}

//==============================================================================

ArtboardFile::LoadResult ArtboardFile::load(const File& file, rive::Factory& factory)
{
    if (! file.existsAsFile())
        return LoadResult::fail ("Failed to find artboard file to load");

    auto is = file.createInputStream();
    if (is == nullptr || ! is->openedOk())
        return LoadResult::fail ("Failed to open artboard file for reading");

    return load (*is, factory);
}

ArtboardFile::LoadResult ArtboardFile::load (InputStream& is, rive::Factory& factory)
{
    yup::MemoryBlock mb;
    is.readIntoMemoryBlock (mb);

    rive::ImportResult result;
    auto rivFile = rive::File::import (
        { static_cast<const uint8_t*> (mb.getData()), mb.getSize() },
        std::addressof(factory),
        std::addressof(result));

    if (result == rive::ImportResult::malformed)
        return LoadResult::fail ("Malformed artboard file");

    if (result == rive::ImportResult::unsupportedVersion)
        return LoadResult::fail ("Unsupported artboard file for current runtime");

    if (rivFile == nullptr)
        return LoadResult::fail ("Failed to import artboard file");

    return LoadResult::ok (std::shared_ptr<ArtboardFile> (new ArtboardFile{ std::move (rivFile) }));
}

} // namespace yup
