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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

MidiKeyboardState::MidiKeyboardState()
{
    zerostruct (noteStates);
    zerostruct (pitchWheelPositions);
}

//==============================================================================
void MidiKeyboardState::reset()
{
    const AudioLockType::ScopedLockType sl (lock);
    zerostruct (noteStates);
    zerostruct (pitchWheelPositions);

    for (auto& controllerList : controllers)
        controllerList.clear();

    eventsToAdd.clear();
}

bool MidiKeyboardState::isNoteOn (const int midiChannel, const int n) const noexcept
{
    jassert (midiChannel > 0 && midiChannel <= 16);

    return isPositiveAndBelow (n, 128)
        && (noteStates[n] & (1 << (midiChannel - 1))) != 0;
}

bool MidiKeyboardState::isNoteOnForChannels (const int midiChannelMask, const int n) const noexcept
{
    return isPositiveAndBelow (n, 128)
        && (noteStates[n] & midiChannelMask) != 0;
}

void MidiKeyboardState::noteOn (const int midiChannel, const int midiNoteNumber, const float velocity)
{
    jassert (midiChannel > 0 && midiChannel <= 16);
    jassert (isPositiveAndBelow (midiNoteNumber, 128));

    const AudioLockType::ScopedLockType sl (lock);

    if (isPositiveAndBelow (midiNoteNumber, 128))
    {
        const int timeNow = (int) Time::getMillisecondCounter();
        eventsToAdd.addEvent (MidiMessage::noteOn (midiChannel, midiNoteNumber, velocity), timeNow);
        eventsToAdd.clear (0, timeNow - 500);

        noteOnInternal (midiChannel, midiNoteNumber, velocity);
    }
}

void MidiKeyboardState::noteOnInternal (const int midiChannel, const int midiNoteNumber, const float velocity)
{
    if (isPositiveAndBelow (midiNoteNumber, 128))
    {
        noteStates[midiNoteNumber] = static_cast<uint16> (noteStates[midiNoteNumber] | (1 << (midiChannel - 1)));
        listeners.call ([&] (Listener& l)
        {
            l.handleNoteOn (this, midiChannel, midiNoteNumber, velocity);
        });
    }
}

void MidiKeyboardState::noteOff (const int midiChannel, const int midiNoteNumber, const float velocity)
{
    const AudioLockType::ScopedLockType sl (lock);

    if (isNoteOn (midiChannel, midiNoteNumber))
    {
        const int timeNow = (int) Time::getMillisecondCounter();
        eventsToAdd.addEvent (MidiMessage::noteOff (midiChannel, midiNoteNumber), timeNow);
        eventsToAdd.clear (0, timeNow - 500);

        noteOffInternal (midiChannel, midiNoteNumber, velocity);
    }
}

void MidiKeyboardState::noteOffInternal (const int midiChannel, const int midiNoteNumber, const float velocity)
{
    if (isNoteOn (midiChannel, midiNoteNumber))
    {
        noteStates[midiNoteNumber] = static_cast<uint16> (noteStates[midiNoteNumber] & ~(1 << (midiChannel - 1)));
        listeners.call ([&] (Listener& l)
        {
            l.handleNoteOff (this, midiChannel, midiNoteNumber, velocity);
        });
    }
}

void MidiKeyboardState::allNotesOff (const int midiChannel)
{
    const AudioLockType::ScopedLockType sl (lock);

    if (midiChannel <= 0)
    {
        for (int i = 1; i <= 16; ++i)
            allNotesOff (i);
    }
    else
    {
        for (int i = 0; i < 128; ++i)
            noteOff (midiChannel, i, 0.0f);
    }
}

void MidiKeyboardState::pitchWheel (const int midiChannel, const int wheelPosition)
{
    jassert (midiChannel >= 1 && midiChannel <= 16);

    if (! isPositiveAndBelow (midiChannel, 17))
        return;

    const AudioLockType::ScopedLockType sl (lock);

    if (pitchWheelPositions[midiChannel - 1] != wheelPosition)
    {
        pitchWheelPositions[midiChannel - 1] = static_cast<uint16> (wheelPosition);

        listeners.call ([&] (Listener& l)
        {
            l.handlePitchWheelMoved (this, midiChannel, wheelPosition);
        });
    }
}

void MidiKeyboardState::controlChange (const int midiChannel, const int controllerNumber, const int controllerValue)
{
    jassert (midiChannel >= 1 && midiChannel <= 16);

    if (! isPositiveAndBelow (midiChannel, 17))
        return;

    const AudioLockType::ScopedLockType sl (lock);

    auto& controllersForChannel = controllers[midiChannel - 1];

    for (int i = controllersForChannel.size(); --i >= 0;)
    {
        auto& controller = controllersForChannel.getReference (i);

        if (controller.number == controllerNumber)
        {
            if (controller.value == controllerValue)
                return;

            controller.value = controllerValue;

            listeners.call ([&] (Listener& l)
            {
                l.handleControllerMoved (this, midiChannel, controllerNumber, controllerValue);
            });

            return;
        }
    }

    controllersForChannel.add ({ controllerNumber, controllerValue });

    listeners.call ([&] (Listener& l)
    {
        l.handleControllerMoved (this, midiChannel, controllerNumber, controllerValue);
    });
}

int MidiKeyboardState::getPitchWheelPosition (const int midiChannel) const noexcept
{
    jassert (midiChannel >= 1 && midiChannel <= 16);

    if (! isPositiveAndBelow (midiChannel, 17))
        return 0;

    const AudioLockType::ScopedLockType sl (lock);

    return pitchWheelPositions[midiChannel - 1];
}

int MidiKeyboardState::getControllerValue (const int midiChannel, const int controllerNumber) const noexcept
{
    jassert (midiChannel >= 1 && midiChannel <= 16);

    if (! isPositiveAndBelow (midiChannel, 17))
        return -1;

    const AudioLockType::ScopedLockType sl (lock);

    for (const auto& controller : controllers[midiChannel - 1])
    {
        if (controller.number == controllerNumber)
            return controller.value;
    }

    return -1;
}

void MidiKeyboardState::processNextMidiEvent (const MidiMessage& message)
{
    if (message.isNoteOn())
    {
        noteOnInternal (message.getChannel(), message.getNoteNumber(), message.getFloatVelocity());
    }
    else if (message.isNoteOff())
    {
        noteOffInternal (message.getChannel(), message.getNoteNumber(), message.getFloatVelocity());
    }
    else if (message.isAllNotesOff())
    {
        for (int i = 0; i < 128; ++i)
            noteOffInternal (message.getChannel(), i, 0.0f);
    }
    else if (message.isPitchWheel())
    {
        pitchWheel (message.getChannel(), message.getPitchWheelValue());
    }
    else if (message.isController())
    {
        controlChange (message.getChannel(), message.getControllerNumber(), message.getControllerValue());
    }
}

void MidiKeyboardState::processNextMidiBuffer (MidiBuffer& buffer,
                                               const int startSample,
                                               const int numSamples,
                                               const bool injectIndirectEvents)
{
    const AudioLockType::ScopedLockType sl (lock);

    for (const auto metadata : buffer)
        processNextMidiEvent (metadata.getMessage());

    if (injectIndirectEvents)
    {
        const int firstEventToAdd = eventsToAdd.getFirstEventTime();
        const double scaleFactor = numSamples / (double) (eventsToAdd.getLastEventTime() + 1 - firstEventToAdd);

        for (const auto metadata : eventsToAdd)
        {
            const auto pos = jlimit (0, numSamples - 1, roundToInt ((metadata.samplePosition - firstEventToAdd) * scaleFactor));
            buffer.addEvent (metadata.getMessage(), startSample + pos);
        }
    }

    eventsToAdd.clear();
}

//==============================================================================
void MidiKeyboardState::addListener (Listener* listener)
{
    const AudioLockType::ScopedLockType sl (lock);
    listeners.add (listener);
}

void MidiKeyboardState::removeListener (Listener* listener)
{
    const AudioLockType::ScopedLockType sl (lock);
    listeners.remove (listener);
}

} // namespace yup
