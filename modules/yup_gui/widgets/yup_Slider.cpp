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
    setMouseCursor (MouseCursor::Hand);

    setValue (defaultValue, dontSendNotification);
}

//==============================================================================

void Slider::setValue (float newValue, NotificationType notification)
{
    value = range.getRange().clipValue (newValue);

    sendValueChanged (notification);

    repaint();
}

float Slider::getValue() const
{
    return value;
}

void Slider::setValueNormalised (float newValue, NotificationType notification)
{
    setValue (range.convertFrom0to1 (jlimit (0.0f, 1.0f, newValue)), notification);
}

float Slider::getValueNormalised() const
{
    return range.convertTo0to1 (value);
}

void Slider::valueChanged() {}

//==============================================================================

void Slider::setDefaultValue (float newDefaultValue)
{
    defaultValue = newDefaultValue;
}

float Slider::getDefaultValue() const
{
    return defaultValue;
}

//==============================================================================

void Slider::setRange (const Range<float>& newRange)
{
    range = newRange;

    setDefaultValue (range.getRange().clipValue (defaultValue));

    setValue (range.getRange().clipValue (value), dontSendNotification);
}

Range<float> Slider::getRange() const
{
    return range.getRange();
}

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
    if (! isDragging)
    {
        isDragging = true;

        if (onDragStart)
            onDragStart (event);
    }

    origin = event.getPosition();

    takeKeyboardFocus();

    repaint();
}

void Slider::mouseUp (const MouseEvent& event)
{
    if (isDragging && ! event.isAnyButtonDown())
    {
        if (onDragEnd)
            onDragEnd (event);

        isDragging = false;
    }
}

void Slider::mouseDrag (const MouseEvent& event)
{
    const float multiplier =
        (event.getModifiers().isShiftDown() || event.isRightButtonDown()) ? 0.0001f : 0.0025f;

    const float distance = -origin.verticalDistanceTo (event.getPosition()) * multiplier;

    origin = event.getPosition();

    setValueNormalised (getValueNormalised() + distance);
}

void Slider::mouseWheel (const MouseEvent& event, const MouseWheelData& data)
{
    const float multiplier = event.getModifiers().isShiftDown() ? 0.001f : 0.0025f;
    const float distance = (data.getDeltaX() + data.getDeltaY()) * multiplier;

    origin = event.getPosition();

    setValueNormalised (getValueNormalised() + distance);
}

//==============================================================================

void Slider::paint (Graphics& g)
{
    ApplicationTheme::findComponentStyle (style.get()).onPaint (g, *this);
}

//==============================================================================

void Slider::sendValueChanged (NotificationType notification)
{
    if (notification == dontSendNotification)
        return;

    auto notificationSender = [this, bailOutChecker = BailOutChecker (this)]
    {
        if (bailOutChecker.shouldBailOut())
            return;

        valueChanged();

        if (onValueChanged)
            onValueChanged (getValue());
    };

    if (notification == sendNotificationAsync || ! MessageManager::getInstance()->isThisTheMessageThread())
        MessageManager::callAsync (std::move (notificationSender));
    else
        notificationSender();
}

} // namespace yup
