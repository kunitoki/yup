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
#include "SynthIconButton.h"
#include "SynthTheme.h"

SynthIconButton::SynthIconButton (yup::StringRef componentID, const char* iconGlyph, float iconHeight, bool drawsTile)
    : yup::Button (componentID)
    , glyph (yup::String::fromUTF8 (iconGlyph))
    , height (iconHeight)
    , tile (drawsTile)
{
    setOpaque (false);
}

void SynthIconButton::paintButton (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();

    if (tile)
        SynthTheme::paintTile (g, *this, bounds);

    if (tile && isButtonDown())
    {
        g.setFillColor (yup::Colors::black.withAlpha (0.25f));
        g.fillRoundedRect (bounds, SynthTheme::panelCorner);
    }

    const auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultIconFont().withHeight (height);
    const auto alpha = isEnabled() ? 0.85f : 0.3f;
    g.setFillColor (isButtonOver() || highlighted ? SynthTheme::accent (*this) : SynthTheme::textPrimary (*this).withAlpha (alpha));
    g.fillFittedText (glyph, font, bounds, yup::Justification::center);
}

void SynthIconButton::setHighlighted (bool shouldBeHighlighted)
{
    if (highlighted == shouldBeHighlighted)
        return;

    highlighted = shouldBeHighlighted;
    repaint();
}

void SynthIconButton::mouseDown (const yup::MouseEvent& event)
{
    // Releasing any button clicks, so the press remembers which one it was.
    rightClicked = event.isRightButtonDown();
    yup::Button::mouseDown (event);
}

void SynthIconButton::enablementChanged()
{
    repaint();
}
