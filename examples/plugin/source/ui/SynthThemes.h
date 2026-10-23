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

#include <yup_gui/yup_gui.h>

#include <vector>

//==============================================================================
/** The palettes the instrument can wear, and the one the user picked last.

    Each theme is a coolors palette parsed with yup::ThemePalette::fromString, in dark
    or light mode. The choice belongs to the user rather than to the patch, so it is
    kept in a small settings file in the user's application data folder.

    The palette lives in the global theme, so every Prism editor in a process shows
    the same one.
*/
namespace SynthThemes
{
/** One selectable theme. */
struct Theme
{
    const char* name;
    const char* palette;
    yup::ThemePalette::Mode mode;
};

/** Returns every theme, the default first. */
const std::vector<Theme>& getThemes();

/** Returns the theme the user picked last, or the default. */
int loadSelectedTheme();

/** Remembers a theme for the next session. */
void saveSelectedTheme (int index);

/** Makes a theme's palette the global one. */
void applyTheme (int index);
} // namespace SynthThemes
