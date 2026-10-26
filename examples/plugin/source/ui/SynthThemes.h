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

#include "SynthTheme.h"

#include <array>
#include <vector>

//==============================================================================
/** The palettes the instrument can wear, and the one the user picked last.

    Each theme is a set of coolors palettes parsed with yup::ThemePalette::fromString, in
    dark or light mode: a base, and one for every section that has its own. Each plugin
    instance keeps its own theme with its session (see RetraceProcessor::getThemeName()),
    and the palettes are set on its interface, so editors in one process can wear
    different themes. The theme picked last is kept in a small settings file in the
    user's application data folder, as the one new instances start with.
*/
namespace SynthThemes
{
/** One selectable theme: a base palette, and a palette for each section that has one of
    its own, in the order of SynthTheme::Section. A section without one wears the base. */
struct Theme
{
    const char* name;
    const char* palette;
    yup::ThemePalette::Mode mode;
    std::array<const char*, SynthTheme::sectionCount> sections {};
};

/** Returns every theme, the default first. */
const std::vector<Theme>& getThemes();

/** Returns the theme the user picked last, or the default. */
int loadSelectedTheme();

/** Remembers a theme as the one new instances start with. */
void saveSelectedTheme (int index);

/** Returns the index of the theme with a name, or -1. */
int findTheme (const yup::String& name);

/** Themes an interface, see SynthTheme::applyTheme(). */
void applyTheme (yup::Component& root, int index);
} // namespace SynthThemes
