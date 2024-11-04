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

Slider::Slider (StringRef componentID)
    : Component (componentID)
{
    setValue (0.0f);
}

//==============================================================================

void Slider::setValue (float newValue)
{
    value = jlimit (0.0f, 1.0f, newValue);

    sendValueChanged();

    repaint();
}

float Slider::getValue() const
{
    return value;
}

void Slider::valueChanged() {}

//==============================================================================

bool Slider::isMouseOver() const
{
    return isMouseOverSlider;
}

//==============================================================================

void Slider::setStyle (Style::Ptr newStyle)
{
    style = newStyle;
}

Slider::Style::Ptr Slider::getStyle() const
{
    return style;
}

//==============================================================================

void Slider::resized()
{
}

//==============================================================================

void Slider::mouseEnter (const MouseEvent& event)
{
    isMouseOverSlider = true;

    repaint();
}

void Slider::mouseExit (const MouseEvent& event)
{
    isMouseOverSlider = false;

    repaint();
}

void Slider::mouseDown (const MouseEvent& event)
{
    origin = event.getPosition();

    takeFocus();

    repaint();
}

void Slider::mouseUp (const MouseEvent& event)
{
}

void Slider::mouseDrag (const MouseEvent& event)
{
    //auto [x, y] = event.getPosition();

    const float multiplier =
        (event.getModifiers().isShiftDown() || event.isRightButtonDown()) ? 0.0001f : 0.0025f;

    const float distance = -origin.verticalDistanceTo (event.getPosition()) * multiplier;

    origin = event.getPosition();

    setValue (value + distance);
}

void Slider::mouseWheel (const MouseEvent& event, const MouseWheelData& data)
{
    //auto [x, y] = event.getPosition();

    const float multiplier = event.getModifiers().isShiftDown() ? 0.001f : 0.0025f;
    const float distance = (data.getDeltaX() + data.getDeltaY()) * multiplier;

    origin = event.getPosition();

    setValue (value + distance);
}

//==============================================================================

void Slider::paint (Graphics& g)
{
    ApplicationTheme::findComponentStyle (style.get()).onPaint (g, *this);
}

//==============================================================================

void Slider::sendValueChanged()
{
    valueChanged();

    if (onValueChanged)
        onValueChanged (getValue());
}

} // namespace yup
