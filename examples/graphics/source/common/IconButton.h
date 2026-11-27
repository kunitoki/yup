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

//==============================================================================
/** A themed button showing a Font Awesome glyph instead of text.

    The button can also show an "on" state, drawing its glyph in the accent color, for
    toolbar toggles such as loop or labels.
*/
class IconButton final : public yup::TextButton
{
public:
    IconButton (yup::StringRef componentID, const char* utf8Glyph)
        : TextButton (componentID)
        , glyph (yup::CharPointer_UTF8 (utf8Glyph))
    {
        setButtonText ({});
    }

    void setGlyph (const char* utf8Glyph)
    {
        glyph = yup::CharPointer_UTF8 (utf8Glyph);
        repaint();
    }

    void setOn (bool shouldBeOn)
    {
        if (on == shouldBeOn)
            return;

        on = shouldBeOn;
        repaint();
    }

    bool isOn() const noexcept { return on; }

    void paintButton (yup::Graphics& g) override
    {
        TextButton::paintButton (g);

        const auto theme = yup::ApplicationTheme::getGlobalTheme();
        const auto& palette = theme->getPalette (*this);

        auto color = isButtonDown()
                       ? theme->findColor (*this, Style::textPressedColorId).value_or (palette.getColor (yup::ThemePalette::Role::onAccent))
                       : theme->findColor (*this, Style::textColorId).value_or (palette.getColor (yup::ThemePalette::Role::text));

        if (on && ! isButtonDown())
            color = palette.getColor (yup::ThemePalette::Role::accent);

        if (! isEnabled())
            color = palette.getColor (yup::ThemePalette::Role::textMuted);

        g.setFillColor (color);
        g.fillFittedText (glyph, theme->getDefaultIconFont().withHeight (getHeight() * 0.45f), getLocalBounds(), yup::Justification::center);
    }

private:
    yup::String glyph;
    bool on = false;
};
