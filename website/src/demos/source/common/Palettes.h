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

#include <array>

//==============================================================================
/** The widget palettes of the demos; cold is the site's own blue. Ids double as ComboBox item ids. */
enum PaletteId
{
    warmPalette = 1,
    coldPalette,
    brightPalette,
    darkPalette
};

/** Themes every widget of the app with one of the palettes. */
inline void applyPalette (int paletteId)
{
    using Role = yup::ThemePalette::Role;

    // background, surface, surfaceRaised, outline, text, textMuted, accent, onAccent
    using Roles = std::array<yup::uint32, 8>;
    static constexpr Roles warm { 0xff2a1406, 0xff3a1d0a, 0xff4d2810, 0xff8a4a1f, 0xffffe6d0, 0xffc49372, 0xffff7a1a, 0xff1a0d04 };
    static constexpr Roles cold { 0xff06173a, 0xff0b2253, 0xff10306f, 0xff2b4f94, 0xffdbe6ff, 0xff8095c4, 0xff005aff, 0xffffffff };
    static constexpr Roles bright { 0xffe8eefc, 0xfff5f8ff, 0xffffffff, 0xff9db3e0, 0xff0b1d3f, 0xff5a6c90, 0xff005aff, 0xffffffff };
    static constexpr Roles dark { 0xff0c0d10, 0xff15171c, 0xff1d2027, 0xff3a3f4a, 0xffe6e8ec, 0xff8b919c, 0xff9cc2ff, 0xff0c0d10 };

    const auto& roles = paletteId == warmPalette ? warm : paletteId == brightPalette ? bright : paletteId == darkPalette ? dark : cold;
    const auto mode = paletteId == brightPalette ? yup::ThemePalette::Mode::light : yup::ThemePalette::Mode::dark;

    const std::array<yup::Color, 1> sources { yup::Color (roles[6]) };
    yup::ThemePalette palette (sources, mode);

    constexpr std::array<Role, 8> order { Role::background, Role::surface, Role::surfaceRaised, Role::outline, Role::text, Role::textMuted, Role::accent, Role::onAccent };
    for (std::size_t i = 0; i < order.size(); ++i)
        palette.setColor (order[i], yup::Color (roles[i]));

    yup::ApplicationTheme::getGlobalTheme()->setPalette (palette);
}
