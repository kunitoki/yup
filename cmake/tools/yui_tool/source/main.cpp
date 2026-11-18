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

#include "yup_YuiCommands.h"

#include <yup_gui/yup_gui.h>

#include <SDL3/SDL_hints.h>

#include <iostream>

int main (int argc, char** argv)
{
    if (const auto exitCode = checkYuiToolCommand (argc, argv))
        return *exitCode;

    // The tool is set up as an application is, default theme included, but opens no window:
    // the dummy driver lets SDL initialise where there is no display, such as a build server
    SDL_SetHint (SDL_HINT_VIDEO_DRIVER, "dummy");

    const yup::ScopedYupInitialiser_GUI gui;
    const yup::ScopedYupInitialiser_Windowing windowing;

    if (yup::ApplicationTheme::getGlobalTheme() == nullptr)
    {
        std::cerr << "yup_yui_tool: can't initialise the windowing system\n";
        return 2;
    }

    return runYuiToolCommand (argc, argv);
}
