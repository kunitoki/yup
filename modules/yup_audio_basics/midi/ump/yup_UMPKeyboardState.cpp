/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

namespace UMPKeyboardStateHelpers
{
inline float toFloatVelocity (uint8_t velocity) noexcept
{
    return velocity / 127.0f;
}

inline float toFloatVelocity (uint16_t velocity) noexcept
{
    return velocity / 65535.0f;
}

inline uint8_t toVelocity7 (float velocity) noexcept
{
    return static_cast<uint8_t> (jlimit (0, 127, roundToInt (velocity * 127.0f)));
}

inline uint16_t toVelocity16 (float velocity) noexcept
{
    return static_cast<uint16_t> (jlimit (0, 65535, roundToInt (velocity * 65535.0f)));
}

inline bool isAllNotesOffController (uint8_t controller) noexcept
{
    return controller == 123;
}
} // namespace UMPKeyboardStateHelpers

UMPKeyboardState::UMPKeyboardState (ump::PacketProtocol protocolIn)
    : protocol (protocolIn)
{
    zerostruct (noteStates);
}

//==============================================================================
void UMPKeyboardState::reset()
{
    const ScopedLock sl (lock);
    zerostruct (noteStates);
    eventsToAdd.clear();
}

bool UMPKeyboardState::isNoteOn (const int midiChannel, const int n) const noexcept
{
    jassert (midiChannel > 0 && midiChannel <= 16);

    return isPositiveAndBelow (n, 128)
        && (noteStates[n] & (1 << (midiChannel - 1))) != 0;
}

bool UMPKeyboardState::isNoteOnForChannels (const int midiChannelMask, const int n) const noexcept
{
    return isPositiveAndBelow (n, 128)
        && (noteStates[n] & midiChannelMask) != 0;
}

void UMPKeyboardState::noteOn (const int midiChannel, const int midiNoteNumber, const float velocity)
{
    jassert (midiChannel > 0 && midiChannel <= 16);
    jassert (isPositiveAndBelow (midiNoteNumber, 128));

    const ScopedLock sl (lock);

    if (isPositiveAndBelow (midiNoteNumber, 128))
    {
        const int timeNow = (int) Time::getMillisecondCounter();

        if (protocol == ump::PacketProtocol::MIDI_1_0)
        {
            const auto packet = ump::Factory::makeNoteOnV1 (group,
                                                            static_cast<uint8_t> (midiChannel - 1),
                                                            static_cast<uint8_t> (midiNoteNumber),
                                                            UMPKeyboardStateHelpers::toVelocity7 (velocity));
            eventsToAdd.addEvent (ump::View (packet.data()), timeNow);
        }
        else
        {
            const auto packet = ump::Factory::makeNoteOnV2 (group,
                                                            static_cast<uint8_t> (midiChannel - 1),
                                                            static_cast<uint8_t> (midiNoteNumber),
                                                            ump::Factory::NoteAttributeKind::none,
                                                            UMPKeyboardStateHelpers::toVelocity16 (velocity),
                                                            0);
            eventsToAdd.addEvent (ump::View (packet.data()), timeNow);
        }

        eventsToAdd.clear (0, timeNow - 500);

        noteOnInternal (midiChannel, midiNoteNumber, velocity);
    }
}

void UMPKeyboardState::noteOnInternal (const int midiChannel, const int midiNoteNumber, const float velocity)
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

void UMPKeyboardState::noteOff (const int midiChannel, const int midiNoteNumber, const float velocity)
{
    const ScopedLock sl (lock);

    if (isNoteOn (midiChannel, midiNoteNumber))
    {
        const int timeNow = (int) Time::getMillisecondCounter();

        if (protocol == ump::PacketProtocol::MIDI_1_0)
        {
            const auto packet = ump::Factory::makeNoteOffV1 (group,
                                                             static_cast<uint8_t> (midiChannel - 1),
                                                             static_cast<uint8_t> (midiNoteNumber),
                                                             UMPKeyboardStateHelpers::toVelocity7 (velocity));
            eventsToAdd.addEvent (ump::View (packet.data()), timeNow);
        }
        else
        {
            const auto packet = ump::Factory::makeNoteOffV2 (group,
                                                             static_cast<uint8_t> (midiChannel - 1),
                                                             static_cast<uint8_t> (midiNoteNumber),
                                                             ump::Factory::NoteAttributeKind::none,
                                                             UMPKeyboardStateHelpers::toVelocity16 (velocity),
                                                             0);
            eventsToAdd.addEvent (ump::View (packet.data()), timeNow);
        }

        eventsToAdd.clear (0, timeNow - 500);

        noteOffInternal (midiChannel, midiNoteNumber, velocity);
    }
}

void UMPKeyboardState::noteOffInternal (const int midiChannel, const int midiNoteNumber, const float velocity)
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

void UMPKeyboardState::allNotesOff (const int midiChannel)
{
    const ScopedLock sl (lock);

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

void UMPKeyboardState::processNextUMPPacket (const ump::View& packet)
{
    using Utils = ump::Utils;
    const auto firstWord = packet[0];
    const auto messageType = Utils::getMessageType (firstWord);
    const auto packetGroup = Utils::getGroup (firstWord);

    if (messageType != 0x2 && messageType != 0x4)
        return;

    if (packetGroup != group)
        return;

    const auto status = Utils::getStatus (firstWord);
    const auto channel = Utils::getChannel (firstWord) + 1;

    if (status == 0x8 || status == 0x9)
    {
        const auto note = Utils::U8<2>::get (firstWord);

        if (messageType == 0x2)
        {
            const auto velocity = Utils::U8<3>::get (firstWord);

            if (status == 0x9 && velocity != 0)
                noteOnInternal (channel, note, UMPKeyboardStateHelpers::toFloatVelocity (velocity));
            else
                noteOffInternal (channel, note, UMPKeyboardStateHelpers::toFloatVelocity (velocity));
        }
        else
        {
            const auto velocityWord = static_cast<uint16_t> (packet[1] >> 16);

            if (status == 0x9 && velocityWord != 0)
                noteOnInternal (channel, note, UMPKeyboardStateHelpers::toFloatVelocity (velocityWord));
            else
                noteOffInternal (channel, note, UMPKeyboardStateHelpers::toFloatVelocity (velocityWord));
        }
    }
    else if (status == 0xb)
    {
        const auto controller = Utils::U8<2>::get (firstWord);

        if (UMPKeyboardStateHelpers::isAllNotesOffController (controller))
            for (int i = 0; i < 128; ++i)
                noteOffInternal (channel, i, 0.0f);
    }
}

void UMPKeyboardState::processNextUMPBuffer (UMPPacketBuffer& buffer,
                                             const int startSample,
                                             const int numSamples,
                                             const bool injectIndirectEvents)
{
    const ScopedLock sl (lock);

    for (const auto metadata : buffer)
        processNextUMPPacket (metadata.getView());

    if (injectIndirectEvents)
    {
        const int firstEventToAdd = eventsToAdd.getFirstEventTime();
        const double scaleFactor = numSamples / (double) (eventsToAdd.getLastEventTime() + 1 - firstEventToAdd);

        for (const auto metadata : eventsToAdd)
        {
            const auto pos = jlimit (0, numSamples - 1, roundToInt ((metadata.samplePosition - firstEventToAdd) * scaleFactor));
            buffer.addEvent (metadata.data, metadata.numWords, startSample + pos);
        }
    }

    eventsToAdd.clear();
}

//==============================================================================
void UMPKeyboardState::addListener (Listener* listener)
{
    const ScopedLock sl (lock);
    listeners.add (listener);
}

void UMPKeyboardState::removeListener (Listener* listener)
{
    const ScopedLock sl (lock);
    listeners.remove (listener);
}

} // namespace yup
