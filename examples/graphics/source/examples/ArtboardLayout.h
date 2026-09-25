/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#include "Artboard.h"

//==============================================================================

class ArtboardLayoutDemo : public ArtboardDemoBase
{
public:
    ArtboardLayoutDemo()
        : ArtboardDemoBase ("data/rive/layout-ui.riv", "keyboard_slot", 8, true)
    {
    }

private:
    std::unique_ptr<yup::Component> createTrackedComponent() override
    {
        auto keyboardArtboard = std::make_unique<yup::Artboard> ("keyboardArtboard");
        keyboardArtboard->setFile (loadedArtboardFile, "Keyboard");
        keyboardArtboard->setFitting (yup::Fitting::fill);
        return keyboardArtboard;
    }
};
