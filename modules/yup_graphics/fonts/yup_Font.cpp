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

Font::Font (const MemoryBlock& fontBytes, rive::Factory* factory)
{
    loadFromData (fontBytes, factory);
}

Font::Font (const File& fontFile, rive::Factory* factory)
{
    loadFromFile (fontFile, factory);
}

//==============================================================================

Result Font::loadFromData (const MemoryBlock& fontBytes, rive::Factory* factory)
{
    font = factory->decodeFont (rive::Span<const uint8_t> { static_cast<const uint8_t*> (fontBytes.getData()), fontBytes.getSize() });
    return font ? Result::ok() : Result::fail ("Unable to load font");
}

Result Font::loadFromFile (const File& fontFile, rive::Factory* factory)
{
    if (! fontFile.existsAsFile())
        return Result::fail ("Unable to load font from non existing file");

    if (auto is = fontFile.createInputStream(); is != nullptr && is->openedOk())
    {
        yup::MemoryBlock mb;
        is->readIntoMemoryBlock (mb);

        font = factory->decodeFont (rive::Span<const uint8_t> { static_cast<const uint8_t*> (mb.getData()), mb.getSize() });
        if (! font)
            return Result::fail ("Unable to load font");
    }

    return Result::ok();
}

//==============================================================================

rive::rcp<rive::Font> Font::getFont() const
{
    return font;
}

} // namespace yup
