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

//==============================================================================
/**
    Represents a UMP keyboard, keeping track of which keys are currently pressed.

    This object can parse a stream of UMP channel voice events, using them to
    update its idea of which keys are pressed for each individual channel.
    Incoming packets from other groups are ignored.

    @tags{Audio}
*/
class YUP_API UMPKeyboardState
{
public:
    //==============================================================================
    explicit UMPKeyboardState (ump::PacketProtocol protocolIn = ump::PacketProtocol::MIDI_2_0);

    //==============================================================================
    /** Resets the state of the object. */
    void reset();

    /** Returns true if the given key is currently held down for the given channel. */
    bool isNoteOn (int midiChannel, int midiNoteNumber) const noexcept;

    /** Returns true if the given key is currently held down on any of a set of channels. */
    bool isNoteOnForChannels (int midiChannelMask, int midiNoteNumber) const noexcept;

    /** Turns a specified note on. */
    void noteOn (int midiChannel, int midiNoteNumber, float velocity);

    /** Turns a specified note off. */
    void noteOff (int midiChannel, int midiNoteNumber, float velocity);

    /** Turns off any currently-down notes for the given channel. */
    void allNotesOff (int midiChannel);

    //==============================================================================
    /** Looks at a UMP packet and uses it to update the state of this object. */
    void processNextUMPPacket (const ump::View& packet);

    /** Scans a UMP buffer for up/down events and adds its own events to it. */
    void processNextUMPBuffer (UMPPacketBuffer& buffer,
                               int startSample,
                               int numSamples,
                               bool injectIndirectEvents);

    //==============================================================================
    /** Receives events from a UMPKeyboardState object. */
    class YUP_API Listener
    {
    public:
        virtual ~Listener() = default;

        /** Called when one of the UMPKeyboardState's keys is pressed. */
        virtual void handleNoteOn (UMPKeyboardState* source,
                                   int midiChannel,
                                   int midiNoteNumber,
                                   float velocity) = 0;

        /** Called when one of the UMPKeyboardState's keys is released. */
        virtual void handleNoteOff (UMPKeyboardState* source,
                                    int midiChannel,
                                    int midiNoteNumber,
                                    float velocity) = 0;
    };

    /** Registers a listener for callbacks when keys go up or down. */
    void addListener (Listener* listener);

    /** Deregisters a listener. */
    void removeListener (Listener* listener);

    /** Sets the UMP protocol to use when generating events. */
    void setProtocol (ump::PacketProtocol protocolIn) noexcept { protocol = protocolIn; }

    /** Returns the UMP protocol used when generating events. */
    ump::PacketProtocol getProtocol() const noexcept { return protocol; }

    /** Sets the UMP group to use when generating events. */
    void setGroup (uint8_t groupIn) noexcept { group = groupIn; }

    /** Returns the UMP group used when generating events. */
    uint8_t getGroup() const noexcept { return group; }

private:
    //==============================================================================
    void noteOnInternal (int midiChannel, int midiNoteNumber, float velocity);
    void noteOffInternal (int midiChannel, int midiNoteNumber, float velocity);

    ump::PacketProtocol protocol;
    uint8_t group = 0;

    CriticalSection lock;
    std::atomic<uint16> noteStates[128];
    UMPPacketBuffer eventsToAdd;
    ListenerList<Listener> listeners;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UMPKeyboardState)
};

using UMPKeyboardStateListener = UMPKeyboardState::Listener;

} // namespace yup
