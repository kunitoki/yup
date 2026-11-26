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

Array<File> getSystemFontFolders()
{
    const auto home = File::getSpecialLocation (File::userHomeDirectory);

    return { File ("/usr/share/fonts"),
             File ("/usr/local/share/fonts"),
             home.getChildFile (".fonts"),
             home.getChildFile (".local/share/fonts") };
}

ResultValue<Font> loadPlatformSerifFont()
{
    return Font::loadFontFromFirstAvailableFile ({ "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
                                                   "/usr/share/fonts/noto/NotoSans-Regular.ttf",
                                                   "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                                                   "/usr/share/fonts/dejavu/DejaVuSans.ttf",
                                                   "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
                                                   "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
                                                   "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf" });
}

ResultValue<Font> loadPlatformMonospaceFont()
{
    return Font::loadFontFromFirstAvailableFile ({ "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
                                                   "/usr/share/fonts/dejavu/DejaVuSansMono.ttf",
                                                   "/usr/share/fonts/truetype/liberation2/LiberationMono-Regular.ttf",
                                                   "/usr/share/fonts/liberation/LiberationMono-Regular.ttf",
                                                   "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf" });
}

ResultValue<Font> loadPlatformColorEmojiFont()
{
    return Font::loadFontFromFirstAvailableFile ({ "/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf",
                                                   "/usr/share/fonts/noto/NotoColorEmoji.ttf",
                                                   "/usr/share/fonts/google-noto-emoji/NotoColorEmoji.ttf",
                                                   "/usr/share/fonts/noto-emoji/NotoColorEmoji.ttf",
                                                   "/usr/share/fonts/truetype/noto-color-emoji/NotoColorEmoji.ttf" });
}

} // namespace

} // namespace yup
