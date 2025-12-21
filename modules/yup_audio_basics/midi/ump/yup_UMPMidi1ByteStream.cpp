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

namespace yup::ump
{

void Midi1ByteStreamParser::feed (uint8_t byte)
{
    if (byte >= uint8_t (SystemStatus::clock))
    {
        systemRealtime (byte);
        return;
    }

    if (packet.getType() == PacketType::data)
    {
        if (byte < 0x80)
        {
            if (hasSysExCallback())
                sysExContinueCallback (byte);
            else
                sysExContinuePacket (byte);

            return;
        }

        if (byte != 0xf7)
        {
            packet = UniversalPacket {};
        }
        else
        {
            if (hasSysExCallback())
                sysExEndCallback();
            else
                sysExEndPacket();

            return;
        }
    }

    if (byte == 0xf0)
    {
        sysExStart();
    }
    else if (byte > 0xf0)
    {
        systemCommon (byte);
    }
    else if (byte >= 0x80)
    {
        channelVoice (byte);
    }
    else if (packet.getStatus() && numMissingBytes && (packetByte < 4))
    {
        packet.setByte (packetByte, byte);
        --numMissingBytes;
        if (numMissingBytes == 0)
        {
            if (packet.getStatus() < 0xf0)
            {
                numMissingBytes = packetByte - 1;
                packetByte = 2;
            }

            if (invokeCallbacks)
                packetCallback (packet);

            if (packet.getStatus() >= 0xf0)
                packet = UniversalPacket {};
        }
        else
        {
            ++packetByte;
        }
    }
}

void Midi1ByteStreamParser::feed (const uint8_t* begin, const uint8_t* end)
{
    for (auto it = begin; it < end; ++it)
        feed (*it);
}

void Midi1ByteStreamParser::reset()
{
    packet = UniversalPacket {};
    sysex.clear();

    packetByte = 1;
    numMissingBytes = 0;
}

void Midi1ByteStreamParser::systemRealtime (uint8_t byte)
{
    if ((byte == 0xf9) || (byte == 0xfd))
        return;

    if (invokeCallbacks)
        packetCallback (makeSystemMessage (group, Status (byte)));
}

void Midi1ByteStreamParser::systemCommon (uint8_t byte)
{
    packet = makeSystemMessage (group, Status (byte));
    packetByte = 2;
    numMissingBytes = 0;

    switch (packet.getStatus())
    {
        case Status (SystemStatus::mtc_quarter_frame):
        case Status (SystemStatus::song_select):
            numMissingBytes = 1;
            break;
        case Status (SystemStatus::song_position):
            numMissingBytes = 2;
            break;
        case Status (SystemStatus::tune_request):
            break;
        default:
            packet = UniversalPacket {};
            return;
    }

    if (numMissingBytes == 0)
    {
        if (invokeCallbacks)
            packetCallback (packet);

        packet = UniversalPacket {};
    }
}

void Midi1ByteStreamParser::channelVoice (uint8_t byte)
{
    packet = makeMidi1ChannelVoiceMessage (group, Status (byte));
    packetByte = 2;
    numMissingBytes = 0;

    switch (packet.getStatus() & 0xf0)
    {
        case Status (Midi1ChannelVoiceStatus::note_off):
        case Status (Midi1ChannelVoiceStatus::note_on):
        case Status (Midi1ChannelVoiceStatus::poly_pressure):
        case Status (Midi1ChannelVoiceStatus::control_change):
        case Status (Midi1ChannelVoiceStatus::pitch_bend):
            numMissingBytes = 2;
            break;
        case Status (Midi1ChannelVoiceStatus::program_change):
        case Status (Midi1ChannelVoiceStatus::channel_pressure):
            numMissingBytes = 1;
            break;
        default:
            break;
    }

    if (numMissingBytes == 0 && invokeCallbacks)
        packetCallback (packet);
}

void Midi1ByteStreamParser::sysExStart()
{
    packet = makeSysEx7StartPacket (group);
    packetByte = 2;
    numMissingBytes = 0;

    if (hasSysExCallback())
    {
        sysex.clear();
        if (sysex.data.capacity() < 1024)
            sysex.data.reserve (1024);

        if (! invokeCallbacks)
            packetByte = 0;
    }
}

void Midi1ByteStreamParser::sysExContinueCallback (uint8_t byte)
{
    switch (packetByte)
    {
        case 0:
        case 1:
            return;
        case 2:
            sysex.manufacturerId = (byte << 16);
            ++packetByte;
            return;
        case 3:
        case 4:
            if ((sysex.manufacturerId & 0xff0000) == 0)
            {
                sysex.manufacturerId += (byte << ((4 - packetByte) * 8));
                ++packetByte;
                return;
            }
            break;
        default:
            break;
    }

    if (sysex.data.size() == sysex.data.capacity())
        sysex.data.reserve (sysex.data.capacity() * 2);

    sysex.data.push_back (byte);
}

void Midi1ByteStreamParser::sysExEndCallback()
{
    packet = UniversalPacket {};

    if (((sysex.manufacturerId & 0xff0000) == 0) && (packetByte < 5))
        return;

    if (invokeCallbacks)
        sysexCallback (sysex);

    sysex.clear();
}

void Midi1ByteStreamParser::sysExContinuePacket (uint8_t byte)
{
    packet.setByte (packetByte, byte);
    if (++packetByte == 8)
    {
        if (invokeCallbacks)
        {
            packet.setByte (1, uint8_t ((packet.getStatus() & 0xf0) + 6));
            packetCallback (packet);
        }

        packet = makeSysEx7ContinuePacket (group);
        packetByte = 2;
    }
}

void Midi1ByteStreamParser::sysExEndPacket()
{
    const auto currentSysExStatus = uint8_t (packet.getStatus() & 0xf0);
    const auto currentPacketSize = uint8_t ((packetByte - 2) & 0x0f);

    if (currentSysExStatus == uint8_t (DataStatus::sysex7_start))
    {
        if (packetByte < 3)
        {
            packet = UniversalPacket {};
            return;
        }

        packet.setByte (1, uint8_t (DataStatus::sysex7_complete) + currentPacketSize);
    }
    else
    {
        packet.setByte (1, uint8_t (DataStatus::sysex7_end) + currentPacketSize);
    }

    if (invokeCallbacks)
        packetCallback (packet);

    packet = UniversalPacket {};
    packetByte = 2;
}

size_t toMidi1ByteStream (const UniversalPacket& packet, uint8_t result[8])
{
    const auto payloadBytes = getMidi1ByteStreamSize (packet);
    size_t resultBytes = 0;

    switch (packet.getType())
    {
        case PacketType::system:
        case PacketType::midi1_channel_voice:
            if (payloadBytes > 0)
            {
                for (; resultBytes < payloadBytes; ++resultBytes)
                    result[resultBytes] = packet.getByte (1 + resultBytes);
            }
            break;
        case PacketType::data:
            if (isSysEx7Packet (packet))
            {
                const auto status = packet.getStatus() & 0xf0;

                if ((status == Status (DataStatus::sysex7_complete))
                    || (status == Status (DataStatus::sysex7_start)))
                    result[resultBytes++] = 0xf0;

                for (size_t b = 0; b < payloadBytes; ++b)
                    result[resultBytes++] = packet.getByte (2 + b);

                if ((status == Status (DataStatus::sysex7_complete))
                    || (status == Status (DataStatus::sysex7_end)))
                    result[resultBytes++] = 0xf7;
            }
            break;
        default:
            return 0;
    }

    return resultBytes;
}

} // namespace yup::ump
