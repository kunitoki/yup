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

//==============================================================================
/** The user's display settings: the theme, the frame rate and the post effect switches.

    They belong to the user rather than to a patch, so they are kept in a small file in
    the user's application data folder, read once per process and written on every
    change.
*/
namespace SynthSettings
{
/** Every display setting. */
struct Values
{
    yup::String theme;
    int frameRate = 60; /**< Frames per second, or 0 to follow the display refresh. */
    bool postEffect = true;
    SynthNeonEffect::Options neon;
};

/** Returns the settings, read from the file the first time. */
Values& get();

/** Writes the settings to the file. */
void save();
} // namespace SynthSettings
