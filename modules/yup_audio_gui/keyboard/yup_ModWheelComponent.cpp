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

namespace yup
{

//==============================================================================
const Identifier ModWheelComponent::Style::bodyTopColorId { "ModWheel_bodyTopColorId" };
const Identifier ModWheelComponent::Style::bodyBottomColorId { "ModWheel_bodyBottomColorId" };
const Identifier ModWheelComponent::Style::outlineColorId { "ModWheel_outlineColorId" };
const Identifier ModWheelComponent::Style::gripColorId { "ModWheel_gripColorId" };
const Identifier ModWheelComponent::Style::gripOverColorId { "ModWheel_gripOverColorId" };
const Identifier ModWheelComponent::Style::gripDownColorId { "ModWheel_gripDownColorId" };

//==============================================================================
ModWheelComponent::ModWheelComponent (MidiKeyboardState& stateToUse, StringRef componentID)
    : Component (componentID)
    , state (stateToUse)
{
    state.addListener (this);
    setOpaque (true); // paint() always fully covers its bounds
}

ModWheelComponent::~ModWheelComponent()
{
    state.removeListener (this);
}

//==============================================================================
void ModWheelComponent::setValue (double newValue, NotificationType notification)
{
    newValue = constrainValue (newValue);

    if (! approximatelyEqual (value, newValue))
    {
        value = newValue;

        sendChangeNotification (notification, [this]
        {
            if (onValueChanged)
                onValueChanged (value);
        });

        repaint();
    }
}

void ModWheelComponent::setDefaultValue (double newDefaultValue)
{
    defaultValue = constrainValue (newDefaultValue);
}

void ModWheelComponent::setMidiChannel (int midiChannelNumber)
{
    jassert (midiChannelNumber > 0 && midiChannelNumber <= 16);

    midiChannel.store (jlimit (1, 16, midiChannelNumber));
}

void ModWheelComponent::setMouseDragSensitivity (double sensitivity)
{
    mouseDragSensitivity = std::max (0.001, sensitivity);
}

//==============================================================================
void ModWheelComponent::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

void ModWheelComponent::mouseDown (const MouseEvent& event)
{
    if (! isEnabled())
        return;

    isDragging = true;
    mouseDragStartY = event.getPosition().getY();
    valueOnMouseDown = value;

    if (onDragStart)
        onDragStart (event);

    repaint();
}

void ModWheelComponent::mouseDrag (const MouseEvent& event)
{
    if (! isEnabled() || ! isDragging)
        return;

    const auto height = getLocalBounds().getHeight();
    if (height <= 0.0f)
        return;

    const auto normalizedDelta = ((static_cast<double> (mouseDragStartY) - event.getPosition().getY()) / height) * mouseDragSensitivity;
    setValue (valueOnMouseDown + normalizedDelta);
}

void ModWheelComponent::mouseUp (const MouseEvent& event)
{
    if (! isEnabled() || ! isDragging)
        return;

    isDragging = false;

    if (onDragEnd)
        onDragEnd (event);

    repaint();
}

void ModWheelComponent::mouseDoubleClick (const MouseEvent&)
{
    if (isEnabled())
        setValue (defaultValue);
}

void ModWheelComponent::mouseEnter (const MouseEvent&)
{
    isMouseOverWheel = true;
    repaint();
}

void ModWheelComponent::mouseExit (const MouseEvent&)
{
    isMouseOverWheel = false;
    repaint();
}

//==============================================================================
double ModWheelComponent::constrainValue (double newValue) const noexcept
{
    return std::clamp (newValue, 0.0, 1.0);
}

//==============================================================================
void ModWheelComponent::handleNoteOn (MidiKeyboardState*, int, int, float)
{
}

void ModWheelComponent::handleNoteOff (MidiKeyboardState*, int, int, float)
{
}

void ModWheelComponent::handleControllerMoved (MidiKeyboardState*, int midiChannelNumber, int controllerNumber, int)
{
    if (controllerNumber == 1 && midiChannelNumber == midiChannel.load())
        triggerAsyncUpdate();
}

void ModWheelComponent::handleAsyncUpdate()
{
    setValue (state.getControllerValue (midiChannel.load(), 1) / 127.0, dontSendNotification);
}

} // namespace yup
