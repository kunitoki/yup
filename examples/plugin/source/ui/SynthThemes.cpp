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
    // The other themes follow the same scheme: one frame per theme, and section accents
    // kept within the hues of its character.
    static const std::vector<Theme> themes {
        { "Retrace",
          "https://coolors.co/0c111b-172131-8b97a8-eef2f8-b9c6ff",
          yup::ThemePalette::Mode::dark,
          { "https://coolors.co/0c111b-172131-8b97a8-eef2f8-b9c6ff",
            "https://coolors.co/0c111b-172131-8b97a8-eef2f8-f4878b",
            "https://coolors.co/0c111b-172131-8b97a8-eef2f8-eed27a",
            "https://coolors.co/0c111b-172131-8b97a8-eef2f8-8fd6a4",
            "https://coolors.co/0c111b-172131-8b97a8-eef2f8-7cc7ec",
            "https://coolors.co/0c111b-172131-8b97a8-eef2f8-c79cf0",
            "https://coolors.co/0c111b-172131-8b97a8-eef2f8-9aa6f4",
            "https://coolors.co/0c111b-172131-8b97a8-eef2f8-c79cf0",
            "https://coolors.co/0c111b-172131-8b97a8-eef2f8-f5ad7a" } },
        { "Retrace Classic",
          "https://coolors.co/011627-0b2a45-7d93a8-fdfffc-f71735",
          yup::ThemePalette::Mode::dark,
          { "https://coolors.co/011627-0b2a45-7d93a8-fdfffc-f71735",
            "https://coolors.co/011627-0b2a45-7d93a8-fdfffc-ff9f1c",
            "https://coolors.co/011627-0b2a45-7d93a8-fdfffc-ffd23f",
            "https://coolors.co/011627-0b2a45-7d93a8-fdfffc-41ead4",
            "https://coolors.co/011627-0b2a45-7d93a8-fdfffc-ff5470",
            "https://coolors.co/011627-0b2a45-7d93a8-fdfffc-3ad1f0",
            "https://coolors.co/011627-0b2a45-7d93a8-fdfffc-2ec4b6",
            "https://coolors.co/011627-0b2a45-7d93a8-fdfffc-3ad1f0",
            "https://coolors.co/011627-0b2a45-7d93a8-fdfffc-ffb35c" } },
        { "Obsidian",
          "https://coolors.co/111214-1e2023-7c8189-e4e6e9-f2b84b",
          yup::ThemePalette::Mode::dark,
          { "https://coolors.co/111214-1e2023-7c8189-e4e6e9-f2b84b",
            "https://coolors.co/111214-1e2023-7c8189-e4e6e9-ee8f4f",
            "https://coolors.co/111214-1e2023-7c8189-e4e6e9-e3cf63",
            "https://coolors.co/111214-1e2023-7c8189-e4e6e9-6fcf97",
            "https://coolors.co/111214-1e2023-7c8189-e4e6e9-58b4e6",
            "https://coolors.co/111214-1e2023-7c8189-e4e6e9-b48ae6",
            "https://coolors.co/111214-1e2023-7c8189-e4e6e9-8c9ce8",
            "https://coolors.co/111214-1e2023-7c8189-e4e6e9-b48ae6",
            "https://coolors.co/111214-1e2023-7c8189-e4e6e9-e8738a" } },
        { "Moss",
          "https://coolors.co/1f2421-2c3530-8a9a8f-dce1de-49a078",
          yup::ThemePalette::Mode::dark,
          { "https://coolors.co/1f2421-2c3530-8a9a8f-dce1de-49a078",
            "https://coolors.co/1f2421-2c3530-8a9a8f-dce1de-7dc46a",
            "https://coolors.co/1f2421-2c3530-8a9a8f-dce1de-cfc067",
            "https://coolors.co/1f2421-2c3530-8a9a8f-dce1de-4fb3a9",
            "https://coolors.co/1f2421-2c3530-8a9a8f-dce1de-5ec98d",
            "https://coolors.co/1f2421-2c3530-8a9a8f-dce1de-6fb2cf",
            "https://coolors.co/1f2421-2c3530-8a9a8f-dce1de-b2cf7a",
            "https://coolors.co/1f2421-2c3530-8a9a8f-dce1de-6fb2cf",
            "https://coolors.co/1f2421-2c3530-8a9a8f-dce1de-d99a5b" } },
        { "Ember",
          "https://coolors.co/1a1423-372549-9a8796-eacdc2-b75d69",
          yup::ThemePalette::Mode::dark,
          { "https://coolors.co/1a1423-372549-9a8796-eacdc2-b75d69",
            "https://coolors.co/1a1423-372549-9a8796-eacdc2-e08a5a",
            "https://coolors.co/1a1423-372549-9a8796-eacdc2-e6b46a",
            "https://coolors.co/1a1423-372549-9a8796-eacdc2-d26a8e",
            "https://coolors.co/1a1423-372549-9a8796-eacdc2-c97bb5",
            "https://coolors.co/1a1423-372549-9a8796-eacdc2-9c86d9",
            "https://coolors.co/1a1423-372549-9a8796-eacdc2-f0a58f",
            "https://coolors.co/1a1423-372549-9a8796-eacdc2-9c86d9",
            "https://coolors.co/1a1423-372549-9a8796-eacdc2-e07a8f" } },
        { "Slate Mint",
          "https://coolors.co/2b2d42-3b3f5c-8d99ae-edf2f4-ff9f1c",
          yup::ThemePalette::Mode::dark,
          { "https://coolors.co/2b2d42-3b3f5c-8d99ae-edf2f4-ff9f1c",
            "https://coolors.co/2b2d42-3b3f5c-8d99ae-edf2f4-ff7f51",
            "https://coolors.co/2b2d42-3b3f5c-8d99ae-edf2f4-ffd166",
            "https://coolors.co/2b2d42-3b3f5c-8d99ae-edf2f4-06d6a0",
            "https://coolors.co/2b2d42-3b3f5c-8d99ae-edf2f4-26c6da",
            "https://coolors.co/2b2d42-3b3f5c-8d99ae-edf2f4-8fa8ff",
            "https://coolors.co/2b2d42-3b3f5c-8d99ae-edf2f4-7ee8c4",
            "https://coolors.co/2b2d42-3b3f5c-8d99ae-edf2f4-8fa8ff",
            "https://coolors.co/2b2d42-3b3f5c-8d99ae-edf2f4-ff8fa3" } }
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
