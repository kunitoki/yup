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
    Collects incoming realtime UMP packets and turns them into blocks suitable for
    processing by a block-based audio callback.

    The class can also be used as a UMPKeyboardState::Listener, and as a UMP
    Receiver so it can easily use a UMP input source or keyboard component.

    @tags{Audio}
*/
class YUP_API UMPPacketCollector
    : public UMPKeyboardState::Listener
    , public ump::Receiver
{
public:
    //==============================================================================
    /** Creates a UMPPacketCollector. */
    explicit UMPPacketCollector (ump::PacketProtocol protocolIn = ump::PacketProtocol::MIDI_2_0,
                                 uint8_t groupIn = 0);

    /** Destructor. */
    ~UMPPacketCollector() override;

    //==============================================================================
    /** Clears any packets from the queue. */
    void reset (double sampleRate);

    /** Takes an incoming packet and adds it to the queue. */
    void addPacketToQueue (const ump::View& packet, double timeStamp);

    /** Removes all the pending packets from the queue as a buffer. */
    void removeNextBlockOfPackets (UMPPacketBuffer& destBuffer, int numSamples);

    /** Preallocates storage for collected packets. */
    void ensureStorageAllocated (size_t bytes);

    /** Sets the UMP protocol used for generated packets. */
    void setProtocol (ump::PacketProtocol protocolIn) noexcept { protocol = protocolIn; }

    /** Returns the UMP protocol used for generated packets. */
    ump::PacketProtocol getProtocol() const noexcept { return protocol; }

    /** Sets the UMP group used for generated packets. */
    void setGroup (uint8_t groupIn) noexcept { group = groupIn; }

    /** Returns the UMP group used for generated packets. */
    uint8_t getGroup() const noexcept { return group; }

    //==============================================================================
    /** @internal */
    void handleNoteOn (UMPKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    /** @internal */
    void handleNoteOff (UMPKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    /** @internal */
    void packetReceived (const ump::View& packet, double time) override;

private:
    //==============================================================================
    double lastCallbackTime = 0;
    CriticalSection midiCallbackLock;
    UMPPacketBuffer incomingPackets;
    double sampleRate = 44100.0;
    ump::PacketProtocol protocol;
    uint8_t group = 0;
#if YUP_DEBUG
    bool hasCalledReset = false;
#endif

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UMPPacketCollector)
};

} // namespace yup
