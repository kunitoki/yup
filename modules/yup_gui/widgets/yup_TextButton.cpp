/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

namespace yup
{

//==============================================================================

TextButton::TextButton (StringRef componentID)
    : Button (componentID)
{
}

//==============================================================================

void TextButton::paintButton (Graphics& g)
{
    ApplicationTheme::findComponentStyle (*this)->paint (g, *this);

    /*
    auto labelBounds = rectBounds.reduced (10.0f, 10.0f);
    g.setFillColor (isButtonDown ? Color (0xffffffff) : Color (0xff000000));
    g.fillFittedText (styledText, labelBounds);
	*/
}

//==============================================================================

void TextButton::resized()
{
    /*
    auto bounds = getLocalBounds().reduced (proportionOfWidth (0.01f));
    auto rectBounds = bounds.reduced (proportionOfWidth (0.045f));
    auto labelBounds = rectBounds.reduced (10.0f, 10.0f);

    styledText.setMaxSize (labelBounds.getSize());
    styledText.setHorizontalAlign (StyledText::center);
    styledText.setVerticalAlign (StyledText::middle);

    styledText.clear();
    styledText.appendText (getComponentID(), nullptr, font, 32.0f);
    styledText.update();
	*/
}

} // namespace yup
