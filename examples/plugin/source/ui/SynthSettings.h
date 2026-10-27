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

#include "SynthNeonEffect.h"

#include <functional>
#include <map>
#include <set>

//==============================================================================
/** The user's settings: the theme, the frame rate, the post effect switches, undo, and
    the preset browser's favorites, ratings and folders.

    They belong to the user rather than to a patch, so they are kept in a small XML file in
    the user's preferences folder, next to the standalone's state, read once per process
    and written on every change. The preset browser's data is read again before each of
    its changes, so instances in other processes (a standalone and a host) keep each
    other's favorites, ratings and folders.
*/
namespace SynthSettings
{
/** Every setting. */
struct Values
{
    yup::String theme; /**< The theme picked last, which new instances start with. */
    int frameRate = 60; /**< Frames per second, or 0 to follow the display refresh. */
    bool postEffect = true;
    bool undo = true; /**< Keeps an undo history of edits; off forgets it. */
    SynthNeonEffect::Options neon;
    std::set<yup::String> favorites; /**< The browser keys of the favorite presets. */
    std::map<yup::String, int> ratings; /**< 1 to 5 stars by browser key; unrated presets are absent. */
    yup::StringArray presetFolders; /**< The full paths of the preset folders added to the browser. */
};

/** Returns the settings, read from the file the first time. */
Values& get();

/** Writes the settings to the file. */
void save();

/** Reads the favorites, ratings and preset folders from the file again, applies a change
    to them and writes the file when the change reports that it changed something.

    @param change   Edits the values and returns true when it changed them.
    @returns what the change returned.
*/
bool updateBrowserData (const std::function<bool (Values&)>& change);
} // namespace SynthSettings
