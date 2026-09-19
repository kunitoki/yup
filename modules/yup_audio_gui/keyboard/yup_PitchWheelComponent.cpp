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
const Identifier PitchWheelComponent::Style::bodyTopColorId { "PitchWheel_bodyTopColorId" };
const Identifier PitchWheelComponent::Style::bodyBottomColorId { "PitchWheel_bodyBottomColorId" };
const Identifier PitchWheelComponent::Style::outlineColorId { "PitchWheel_outlineColorId" };
const Identifier PitchWheelComponent::Style::gripColorId { "PitchWheel_gripColorId" };
const Identifier PitchWheelComponent::Style::gripOverColorId { "PitchWheel_gripOverColorId" };
const Identifier PitchWheelComponent::Style::gripDownColorId { "PitchWheel_gripDownColorId" };

//==============================================================================
PitchWheelComponent::PitchWheelComponent (MidiKeyboardState& stateToUse, StringRef componentID)
    : Component (componentID)
    , state (stateToUse)
{
    state.addListener (this);
    setOpaque (true); // paint() always fully covers its bounds
}

PitchWheelComponent::~PitchWheelComponent()
{
    state.removeListener (this);
}

//==============================================================================
void PitchWheelComponent::setValue (double newValue, NotificationType notification)
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

void PitchWheelComponent::setDefaultValue (double newDefaultValue)
{
    defaultValue = constrainValue (newDefaultValue);
}

void PitchWheelComponent::setMidiChannel (int midiChannelNumber)
{
    jassert (midiChannelNumber > 0 && midiChannelNumber <= 16);

    midiChannel.store (jlimit (1, 16, midiChannelNumber));
}

void PitchWheelComponent::setMouseDragSensitivity (double sensitivity)
{
    mouseDragSensitivity = std::max (0.001, sensitivity);
}

//==============================================================================
void PitchWheelComponent::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

void PitchWheelComponent::mouseDown (const MouseEvent& event)
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

void PitchWheelComponent::mouseDrag (const MouseEvent& event)
{
    if (! isEnabled() || ! isDragging)
        return;

    const auto height = getLocalBounds().getHeight();
    if (height <= 0.0f)
        return;

    const auto normalizedDelta = ((static_cast<double> (mouseDragStartY) - event.getPosition().getY()) / height) * 2.0 * mouseDragSensitivity;
    setValue (valueOnMouseDown + normalizedDelta);
}

void PitchWheelComponent::mouseUp (const MouseEvent& event)
{
    if (! isEnabled() || ! isDragging)
        return;

    isDragging = false;

    if (resetOnRelease)
        setValue (defaultValue);

    if (onDragEnd)
        onDragEnd (event);

    repaint();
}

void PitchWheelComponent::mouseDoubleClick (const MouseEvent&)
{
    if (isEnabled())
        setValue (defaultValue);
}

void PitchWheelComponent::mouseEnter (const MouseEvent&)
{
    isMouseOverWheel = true;
    repaint();
}

void PitchWheelComponent::mouseExit (const MouseEvent&)
{
    isMouseOverWheel = false;
    repaint();
}

//==============================================================================
double PitchWheelComponent::constrainValue (double newValue) const noexcept
{
    return std::clamp (newValue, -1.0, 1.0);
}

//==============================================================================
void PitchWheelComponent::handleNoteOn (MidiKeyboardState*, int, int, float)
{
}

void PitchWheelComponent::handleNoteOff (MidiKeyboardState*, int, int, float)
{
}

void PitchWheelComponent::handlePitchWheelMoved (MidiKeyboardState*, int midiChannelNumber, int)
{
    if (midiChannelNumber == midiChannel.load())
        triggerAsyncUpdate();
}

void PitchWheelComponent::handleAsyncUpdate()
{
    setValue ((state.getPitchWheelPosition (midiChannel.load()) - 8192.0) / 8191.0, dontSendNotification);
}

} // namespace yup
