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


namespace yup
{

namespace
{

//==============================================================================

// WebAssembly has no system fonts: apps ship their own fonts and load them from data or files

ResultValue<Font> loadPlatformSerifFont()
{
    return yup::makeResultValueFail ("No system serif font available on this platform");
}

ResultValue<Font> loadPlatformMonospaceFont()
{
    return yup::makeResultValueFail ("No system monospace font available on this platform");
}

ResultValue<Font> loadPlatformColorEmojiFont()
{
    return yup::makeResultValueFail ("No system color emoji font available on this platform");
}

StringArray getSystemFontFamilyNames()
{
    return {};
}

std::vector<SystemFontFace> findSystemFontFaces (const String&)
{
    return {};
}

rive::rcp<rive::Font> loadSystemFontFace (const SystemFontFace&)
{
    return nullptr;
}

} // namespace

} // namespace yup
