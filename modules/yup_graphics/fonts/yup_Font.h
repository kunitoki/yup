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

class JUCE_API Font
{
public:
    //==============================================================================
    Font() = default;

    //==============================================================================
    Font (const MemoryBlock& fontBytes, rive::Factory* factory);
    Font (const File& fontFile, rive::Factory* factory);

    //==============================================================================
    /** Copy and move constructors and assignment operators. */
    Font (const Font& other) noexcept = default;
    Font (Font&& other) noexcept = default;
    Font& operator= (const Font& other) noexcept = default;
    Font& operator= (Font&& other) noexcept = default;

    //==============================================================================
    Result loadFromData (const MemoryBlock& fontBytes, rive::Factory* factory);

    //==============================================================================
    Result loadFromFile (const File& fontFile, rive::Factory* factory);

    //==============================================================================
    rive::rcp<rive::Font> getFont() const;

private:
    rive::rcp<rive::Font> font;
};

} // namespace yup
