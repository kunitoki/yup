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

namespace UMPPacketCollectorHelpers
{
inline uint8_t toVelocity7 (float velocity) noexcept
{
    return static_cast<uint8_t> (jlimit (0, 127, roundToInt (velocity * 127.0f)));
}

inline uint16_t toVelocity16 (float velocity) noexcept
{
    return static_cast<uint16_t> (jlimit (0, 65535, roundToInt (velocity * 65535.0f)));
}
} // namespace UMPPacketCollectorHelpers

UMPPacketCollector::UMPPacketCollector (ump::PacketProtocol protocolIn,
                                        uint8_t groupIn)
    : protocol (protocolIn)
    , group (groupIn)
{
}

UMPPacketCollector::~UMPPacketCollector() = default;

//==============================================================================
void UMPPacketCollector::reset (const double newSampleRate)
{
    const ScopedLock sl (midiCallbackLock);

    jassert (newSampleRate > 0);

#if YUP_DEBUG
    hasCalledReset = true;
#endif
    sampleRate = newSampleRate;
    incomingPackets.clear();
    lastCallbackTime = Time::getMillisecondCounterHiRes();
}

void UMPPacketCollector::addPacketToQueue (const ump::View& packet, double timeStamp)
{
    const ScopedLock sl (midiCallbackLock);

#if YUP_DEBUG
    jassert (hasCalledReset);
#endif

    jassert (! approximatelyEqual (timeStamp, 0.0));

    auto sampleNumber = (int) ((timeStamp - 0.001 * lastCallbackTime) * sampleRate);

    incomingPackets.addEvent (packet, sampleNumber);

    if (sampleNumber > sampleRate)
        incomingPackets.clear (0, sampleNumber - (int) sampleRate);
}

void UMPPacketCollector::removeNextBlockOfPackets (UMPPacketBuffer& destBuffer,
                                                   const int numSamples)
{
    const ScopedLock sl (midiCallbackLock);

#if YUP_DEBUG
    jassert (hasCalledReset);
#endif

    jassert (numSamples > 0);

    auto timeNow = Time::getMillisecondCounterHiRes();
    auto msElapsed = timeNow - lastCallbackTime;

    lastCallbackTime = timeNow;

    if (! incomingPackets.isEmpty())
    {
        int numSourceSamples = jmax (1, roundToInt (msElapsed * 0.001 * sampleRate));
        int startSample = 0;
        int scale = 1 << 16;

        if (numSourceSamples > numSamples)
        {
            const int maxBlockLengthToUse = numSamples << 5;

            auto iter = incomingPackets.cbegin();

            if (numSourceSamples > maxBlockLengthToUse)
            {
                startSample = numSourceSamples - maxBlockLengthToUse;
                numSourceSamples = maxBlockLengthToUse;
                iter = incomingPackets.findNextSamplePosition (startSample);
            }

            scale = (numSamples << 10) / numSourceSamples;

            std::for_each (iter, incomingPackets.cend(), [&] (const UMPPacketMetadata& meta)
            {
                const auto pos = ((meta.samplePosition - startSample) * scale) >> 10;
                destBuffer.addEvent (meta.data, meta.numWords, jlimit (0, numSamples - 1, pos));
            });
        }
        else
        {
            startSample = numSamples - numSourceSamples;

            for (const auto metadata : incomingPackets)
                destBuffer.addEvent (metadata.data, metadata.numWords, jlimit (0, numSamples - 1, metadata.samplePosition + startSample));
        }

        incomingPackets.clear();
    }
}

void UMPPacketCollector::ensureStorageAllocated (size_t bytes)
{
    incomingPackets.ensureSize (bytes);
}

//==============================================================================
void UMPPacketCollector::handleNoteOn (UMPKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    const auto timeStamp = Time::getMillisecondCounterHiRes() * 0.001;

    if (protocol == ump::PacketProtocol::MIDI_1_0)
    {
        const auto packet = ump::Factory::makeNoteOnV1 (group,
                                                        static_cast<uint8_t> (midiChannel - 1),
                                                        static_cast<uint8_t> (midiNoteNumber),
                                                        UMPPacketCollectorHelpers::toVelocity7 (velocity));
        addPacketToQueue (ump::View (packet.data()), timeStamp);
    }
    else
    {
        const auto packet = ump::Factory::makeNoteOnV2 (group,
                                                        static_cast<uint8_t> (midiChannel - 1),
                                                        static_cast<uint8_t> (midiNoteNumber),
                                                        ump::Factory::NoteAttributeKind::none,
                                                        UMPPacketCollectorHelpers::toVelocity16 (velocity),
                                                        0);
        addPacketToQueue (ump::View (packet.data()), timeStamp);
    }
}

void UMPPacketCollector::handleNoteOff (UMPKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    const auto timeStamp = Time::getMillisecondCounterHiRes() * 0.001;

    if (protocol == ump::PacketProtocol::MIDI_1_0)
    {
        const auto packet = ump::Factory::makeNoteOffV1 (group,
                                                         static_cast<uint8_t> (midiChannel - 1),
                                                         static_cast<uint8_t> (midiNoteNumber),
                                                         UMPPacketCollectorHelpers::toVelocity7 (velocity));
        addPacketToQueue (ump::View (packet.data()), timeStamp);
    }
    else
    {
        const auto packet = ump::Factory::makeNoteOffV2 (group,
                                                         static_cast<uint8_t> (midiChannel - 1),
                                                         static_cast<uint8_t> (midiNoteNumber),
                                                         ump::Factory::NoteAttributeKind::none,
                                                         UMPPacketCollectorHelpers::toVelocity16 (velocity),
                                                         0);
        addPacketToQueue (ump::View (packet.data()), timeStamp);
    }
}

void UMPPacketCollector::packetReceived (const ump::View& packet, double time)
{
    addPacketToQueue (packet, time);
}

} // namespace yup
