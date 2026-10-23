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

#include "SynthThemes.h"

namespace SynthThemes
{
namespace
{
yup::File getSettingsFile()
{
    return yup::File::getSpecialLocation (yup::File::userApplicationDataDirectory)
        .getChildFile ("Prism")
        .getChildFile ("Theme.txt");
}
} // namespace

const std::vector<Theme>& getThemes()
{
    static const std::vector<Theme> themes {
        { "Prism", "https://coolors.co/011627-f71735-41ead4-fdfffc-ff9f1c", yup::ThemePalette::Mode::dark },
        { "Deep Sea", "https://coolors.co/0b132b-1c2541-3a506b-5bc0be-6fffe9", yup::ThemePalette::Mode::dark },
        { "Moss", "https://coolors.co/1f2421-216869-49a078-9cc5a1-dce1de", yup::ThemePalette::Mode::dark },
        { "Ember", "https://coolors.co/1a1423-372549-774c60-b75d69-eacdc2", yup::ThemePalette::Mode::dark },
        { "Slate Mint", "https://coolors.co/2b2d42-8d99ae-edf2f4-06d6a0-ff9f1c", yup::ThemePalette::Mode::dark },
        { "Paper", "https://coolors.co/f4f1de-e07a5f-3d405b-81b29a-f2cc8f", yup::ThemePalette::Mode::light }
    };

    return themes;
}

int loadSelectedTheme()
{
    const auto name = getSettingsFile().loadFileAsString().trim();
    const auto& themes = getThemes();

    for (std::size_t index = 0; index < themes.size(); ++index)
        if (name == themes[index].name)
            return static_cast<int> (index);

    return 0;
}

void saveSelectedTheme (int index)
{
    const auto& themes = getThemes();

    if (! yup::isPositiveAndBelow (index, static_cast<int> (themes.size())))
        return;

    const auto file = getSettingsFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText (themes[static_cast<std::size_t> (index)].name);
}

void applyTheme (int index)
{
    const auto& themes = getThemes();
    const auto& theme = themes[static_cast<std::size_t> (yup::jlimit (0, static_cast<int> (themes.size()) - 1, index))];

    if (auto palette = yup::ThemePalette::fromString (theme.palette, theme.mode); palette.wasOk())
        yup::ApplicationTheme::getGlobalTheme()->setPalette (palette.getReference());
}
} // namespace SynthThemes
