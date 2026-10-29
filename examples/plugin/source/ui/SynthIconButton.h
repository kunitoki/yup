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

//==============================================================================
/** A button drawn as a header tile with a glyph of the theme's icon font.

    @see SynthHeader
*/
class SynthIconButton : public yup::Button
{
public:
    /** Creates the button; a button without a tile draws only its glyph, for use inside a tile. */
    SynthIconButton (yup::StringRef componentID, const char* iconGlyph, float iconHeight = 15.0f, bool drawsTile = true);

    /** True when the last press was made with the right button, for buttons with a second action. */
    bool wasRightClicked() const noexcept { return rightClicked; }

    /** Draws the glyph in the accent color, as when hovered, to show that what it opens is open. */
    void setHighlighted (bool shouldBeHighlighted);

    void paintButton (yup::Graphics& g) override;
    void mouseDown (const yup::MouseEvent& event) override;
    void enablementChanged() override;

private:
    yup::String glyph;
    float height;
    bool tile;
    bool rightClicked = false;
    bool highlighted = false;
};
