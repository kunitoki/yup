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

namespace
{

struct MidiWheelComponentIds
{
    static inline const Identifier value { "value" };
};

constexpr int modulationWheelControllerNumber = 1;

} // namespace

//==============================================================================
MidiWheelComponent::MidiWheelComponent (StringRef componentID, Source sourceToFollow, bool shouldResetOnRelease)
    : Component (componentID)
    , source (sourceToFollow)
    , valueRange (sourceToFollow == Source::pitchWheel ? -1.0 : 0.0, 1.0)
    , resetOnRelease (shouldResetOnRelease)
{
    setOpaque (true); // paint() always fully covers its bounds
    setMouseCursor (MouseCursor::Hand);
}

MidiWheelComponent::~MidiWheelComponent()
{
    if (state != nullptr)
        state->removeListener (this);
}

//==============================================================================
void MidiWheelComponent::setState (MidiKeyboardState* newState)
{
    if (state == newState)
        return;

    if (state != nullptr)
        state->removeListener (this);

    // What the previous state reported and isn't shown yet is not read from the new one
    cancelPendingUpdate();
    stateChangedWhileDragging = false;

    state = newState;

    if (state != nullptr)
        state->addListener (this);
}

//==============================================================================
void MidiWheelComponent::setValue (double newValue, NotificationType notification)
{
    newValue = valueRange.clipValue (newValue);

    if (approximatelyEqual (value, newValue))
        return;

    value = newValue;
    repaint();

    const BailOutChecker bailOutChecker (this);

    notifyPropertyChanged (MidiWheelComponentIds::value);
    if (bailOutChecker.shouldBailOut())
        return;

    sendChangeNotification (notification, [this]
    {
        if (onValueChanged)
            onValueChanged (value);
    });
}

void MidiWheelComponent::setDefaultValue (double newDefaultValue)
{
    defaultValue = valueRange.clipValue (newDefaultValue);
}

void MidiWheelComponent::setMidiChannel (int midiChannelNumber)
{
    jassert (midiChannelNumber > 0 && midiChannelNumber <= 16);

    midiChannel.store (jlimit (1, 16, midiChannelNumber));
}

void MidiWheelComponent::setMouseDragSensitivity (double sensitivity)
{
    mouseDragSensitivity = std::max (0.001, sensitivity);
}

//==============================================================================
void MidiWheelComponent::mouseDown (const MouseEvent& event)
{
    if (! isEnabled())
        return;

    isDragging = true;
    lastDragPosition = event.getPosition();
    mouseDragStartY = lastDragPosition.getY();
    valueOnMouseDown = value;

    if (onDragStart)
        onDragStart (event);

    repaint();
}

void MidiWheelComponent::mouseDrag (const MouseEvent& event)
{
    if (! isEnabled() || ! isDragging)
        return;

    lastDragPosition = event.getPosition();

    const auto height = getLocalBounds().getHeight();
    if (height <= 0.0f)
        return;

    const auto normalizedDelta = ((static_cast<double> (mouseDragStartY) - lastDragPosition.getY()) / height) * valueRange.getLength() * mouseDragSensitivity;
    setValue (valueOnMouseDown + normalizedDelta);
}

void MidiWheelComponent::mouseUp (const MouseEvent& event)
{
    endDrag (event);
}

void MidiWheelComponent::mouseDoubleClick (const MouseEvent&)
{
    if (isEnabled())
        setValue (defaultValue);
}

void MidiWheelComponent::mouseEnter (const MouseEvent&)
{
    isMouseOverWheel = true;
    repaint();
}

void MidiWheelComponent::mouseExit (const MouseEvent&)
{
    isMouseOverWheel = false;
    repaint();
}

//==============================================================================
void MidiWheelComponent::enablementChanged()
{
    if (! isEnabled())
        cancelDrag();
}

void MidiWheelComponent::visibilityChanged()
{
    if (! isShowing())
        cancelDrag();
}

//==============================================================================
void MidiWheelComponent::handleNoteOn (MidiKeyboardState*, int, int, float)
{
}

void MidiWheelComponent::handleNoteOff (MidiKeyboardState*, int, int, float)
{
}

void MidiWheelComponent::handlePitchWheelMoved (MidiKeyboardState*, int midiChannelNumber, int)
{
    if (source == Source::pitchWheel && midiChannelNumber == midiChannel.load())
        triggerAsyncUpdate();
}

void MidiWheelComponent::handleControllerMoved (MidiKeyboardState*, int midiChannelNumber, int controllerNumber, int)
{
    if (source == Source::modulationWheel && controllerNumber == modulationWheelControllerNumber && midiChannelNumber == midiChannel.load())
        triggerAsyncUpdate();
}

void MidiWheelComponent::handleAsyncUpdate()
{
    if (state == nullptr)
        return;

    // The wheel is the user's while it is dragged: what the state reports is applied once it is let go
    if (isDragging)
    {
        stateChangedWhileDragging = true;
        return;
    }

    applyStateValue();
}

//==============================================================================
void MidiWheelComponent::endDrag (const MouseEvent& event)
{
    if (! isDragging)
        return;

    isDragging = false;
    repaint();

    const BailOutChecker bailOutChecker (this);

    if (resetOnRelease)
        setValue (defaultValue);

    if (bailOutChecker.shouldBailOut())
        return;

    if (onDragEnd)
        onDragEnd (event);

    if (bailOutChecker.shouldBailOut())
        return;

    if (std::exchange (stateChangedWhileDragging, false) && state != nullptr)
        applyStateValue();
}

void MidiWheelComponent::cancelDrag()
{
    endDrag (MouseEvent (MouseEvent::noButtons, KeyModifiers(), lastDragPosition, this));
}

void MidiWheelComponent::applyStateValue()
{
    const auto channel = midiChannel.load();
    const auto stateValue = source == Source::pitchWheel
                              ? (state->getPitchWheelPosition (channel) - 8192.0) / 8191.0
                              : state->getControllerValue (channel, modulationWheelControllerNumber) / 127.0;

    setValue (stateValue, dontSendNotification);
}

} // namespace yup
