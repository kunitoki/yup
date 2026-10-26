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
#include "SynthSettings.h"

namespace SynthThemes
{
const std::vector<Theme>& getThemes()
{
    // Retrace scans its signal in colored lines: each section takes one of their colors,
    // softened, over the same deep blue grey, and the header keeps the white of the beam.
    static const std::vector<Theme> themes {
        { "Retrace",
          "https://coolors.co/0c111b-172131-8b97a8-eef2f8-b9c6ff",
          yup::ThemePalette::Mode::dark,
          { "https://coolors.co/0c111b-172131-8b97a8-eef2f8-b9c6ff",
            "https://coolors.co/0c111b-172131-8b97a8-eef2f8-f4878b",
            "https://coolors.co/0c111b-172131-8b97a8-eef2f8-f5ad7a",
            "https://coolors.co/0c111b-172131-8b97a8-eef2f8-eed27a",
            "https://coolors.co/0c111b-172131-8b97a8-eef2f8-8fd6a4",
            "https://coolors.co/0c111b-172131-8b97a8-eef2f8-7cc7ec",
            "https://coolors.co/0c111b-172131-8b97a8-eef2f8-9aa6f4",
            "https://coolors.co/0c111b-172131-8b97a8-eef2f8-c79cf0" } },
        { "Retrace Classic", "https://coolors.co/011627-f71735-41ead4-fdfffc-ff9f1c", yup::ThemePalette::Mode::dark },
        { "Deep Sea", "https://coolors.co/0b132b-1c2541-3a506b-5bc0be-6fffe9", yup::ThemePalette::Mode::dark },
        { "Moss", "https://coolors.co/1f2421-216869-49a078-9cc5a1-dce1de", yup::ThemePalette::Mode::dark },
        { "Ember", "https://coolors.co/1a1423-372549-774c60-b75d69-eacdc2", yup::ThemePalette::Mode::dark },
        { "Slate Mint", "https://coolors.co/2b2d42-8d99ae-edf2f4-06d6a0-ff9f1c", yup::ThemePalette::Mode::dark }
    };

    return themes;
}

int loadSelectedTheme()
{
    return yup::jmax (0, findTheme (SynthSettings::get().theme));
}

void saveSelectedTheme (int index)
{
    const auto& themes = getThemes();

    if (! yup::isPositiveAndBelow (index, static_cast<int> (themes.size())))
        return;

    SynthSettings::get().theme = themes[static_cast<std::size_t> (index)].name;
    SynthSettings::save();
}

int findTheme (const yup::String& name)
{
    const auto& themes = getThemes();

    for (std::size_t index = 0; index < themes.size(); ++index)
        if (name == themes[index].name)
            return static_cast<int> (index);

    return -1;
}

void applyTheme (yup::Component& root, int index)
{
    const auto& themes = getThemes();
    const auto& theme = themes[static_cast<std::size_t> (yup::jlimit (0, static_cast<int> (themes.size()) - 1, index))];

    auto base = yup::ThemePalette::fromString (theme.palette, theme.mode);
    SynthTheme::SectionPalettes sections;

    for (std::size_t index = 0; index < sections.size(); ++index)
    {
        if (theme.sections[index] == nullptr)
            continue;

        if (auto palette = yup::ThemePalette::fromString (theme.sections[index], theme.mode); palette.wasOk())
            sections[index] = palette.getReference();
    }

    SynthTheme::applyTheme (root, base.wasOk() ? base.getReference() : yup::ThemePalette(), sections);
}
} // namespace SynthThemes
