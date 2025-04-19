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

Button::Button (StringRef componentID)
    : Component (componentID)
{
}

//==============================================================================

void Button::paint (Graphics& g)
{
    paintButton (g);
}

//==============================================================================

void Button::mouseEnter (const MouseEvent& event)
{
    isButtonCurrentlyOver = true;

    repaint();
}

void Button::mouseExit (const MouseEvent& event)
{
    isButtonCurrentlyOver = false;

    repaint();
}

void Button::mouseDown (const MouseEvent& event)
{
    isButtonCurrentlyDown = true;

    if (onClick)
        onClick();

    takeKeyboardFocus();

    repaint();
}

void Button::mouseUp (const MouseEvent& event)
{
    isButtonCurrentlyDown = false;

    repaint();
}

} // namespace yup
