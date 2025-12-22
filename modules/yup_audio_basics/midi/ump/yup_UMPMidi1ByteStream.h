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

#ifndef DOXYGEN

namespace yup::ump
{

class Midi1ByteStreamParser
{
public:
    using PacketCallback = std::function<void (UniversalPacket)>;
    using SysExCallback = std::function<void (const SysEx7&)>;

    explicit Midi1ByteStreamParser (PacketCallback packetCallback,
                                    SysExCallback sysexCallback = {},
                                    bool enableCallbacks = true);

    Midi1ByteStreamParser (Group group,
                           PacketCallback packetCallback,
                           SysExCallback sysexCallback = {},
                           bool enableCallbacks = true);

    bool callbacksEnabled() const { return invokeCallbacks; }

    void enableCallbacks (bool enable) { invokeCallbacks = enable; }

    Group getGroup() const;
    void setGroup (Group group);

    void feed (uint8_t byte);
    void feed (const uint8_t* data, size_t numBytes);
    void feed (const uint8_t* begin, const uint8_t* end);

    void reset();

protected:
    void systemRealtime (uint8_t byte);
    void systemCommon (uint8_t byte);
    void channelVoice (uint8_t byte);

    bool hasSysExCallback() const { return static_cast<bool> (sysexCallback); }

    void sysExStart();
    void sysExContinueCallback (uint8_t byte);
    void sysExEndCallback();
    void sysExContinuePacket (uint8_t byte);
    void sysExEndPacket();

private:
    Group group { 0 };
    PacketCallback packetCallback;
    SysExCallback sysexCallback;
    bool invokeCallbacks { true };

    UniversalPacket packet;
    SysEx7 sysex;

    uint8_t packetByte { 0 };
    uint8_t numMissingBytes { 0 };
};

constexpr UniversalPacket fromMidi1ByteStream (uint8_t status, uint7_t data1, uint7_t data2);

constexpr size_t getMidi1ByteStreamSize (const UniversalPacket& packet);

size_t toMidi1ByteStream (const UniversalPacket& packet, uint8_t bytes[8]);

inline Midi1ByteStreamParser::Midi1ByteStreamParser (PacketCallback packetCallbackIn,
                                                     SysExCallback sysexCallbackIn,
                                                     bool enableCallbacks)
    : packetCallback (std::move (packetCallbackIn))
    , sysexCallback (std::move (sysexCallbackIn))
    , invokeCallbacks (enableCallbacks)
{
}

inline Midi1ByteStreamParser::Midi1ByteStreamParser (Group groupIn,
                                                     PacketCallback packetCallbackIn,
                                                     SysExCallback sysexCallbackIn,
                                                     bool enableCallbacks)
    : group (groupIn)
    , packetCallback (std::move (packetCallbackIn))
    , sysexCallback (std::move (sysexCallbackIn))
    , invokeCallbacks (enableCallbacks)
{
}

inline Group Midi1ByteStreamParser::getGroup() const
{
    return group;
}

inline void Midi1ByteStreamParser::setGroup (Group groupIn)
{
    group = groupIn;
}

inline void Midi1ByteStreamParser::feed (const uint8_t* data, size_t numBytes)
{
    feed (data, data + numBytes);
}

constexpr size_t getMidi1ByteStreamSize (const UniversalPacket& packet)
{
    switch (packet.getType())
    {
        case PacketType::system:
            switch (SystemStatus (packet.getStatus()))
            {
                case SystemStatus::songPosition:
                    return 3u;
                case SystemStatus::mtcQuarterFrame:
                case SystemStatus::songSelect:
                    return 2u;
                case SystemStatus::tuneRequest:
                case SystemStatus::clock:
                case SystemStatus::start:
                case SystemStatus::cont:
                case SystemStatus::stop:
                case SystemStatus::activeSense:
                case SystemStatus::reset:
                    return 1u;
            }
            break;
        case PacketType::midi1ChannelVoice:
            switch (packet.getStatus() & 0xf0)
            {
                case Status (Midi1ChannelVoiceStatus::noteOff):
                case Status (Midi1ChannelVoiceStatus::noteOn):
                case Status (Midi1ChannelVoiceStatus::polyPressure):
                case Status (Midi1ChannelVoiceStatus::controlChange):
                case Status (Midi1ChannelVoiceStatus::pitchBend):
                    return 3u;
                case Status (Midi1ChannelVoiceStatus::programChange):
                case Status (Midi1ChannelVoiceStatus::channelPressure):
                    return 2u;
            }
            break;
        case PacketType::data:
            switch (packet.getStatus() & 0xf0)
            {
                case Status (DataStatus::sysex7Complete):
                case Status (DataStatus::sysex7Start):
                case Status (DataStatus::sysex7End):
                case Status (DataStatus::sysex7Continue):
                    if ((packet.getStatus() & 0x0f) <= 6)
                        return packet.getStatus() & 0x0f;
                    break;
            }
            break;
        default:
            break;
    }

    return 0u;
}

constexpr UniversalPacket fromMidi1ByteStream (uint8_t status, uint7_t data1, uint7_t data2)
{
    auto type = PacketType::midi1ChannelVoice;

    if ((status & 0xf0) == 0xf0)
    {
        type = PacketType::system;

        switch (status)
        {
            case 0xf0:
            case 0xf7:
            case 0xf4:
            case 0xf5:
            case 0xf9:
            case 0xfd:
                return {};
            default:
                break;
        }
    }
    else if (status < 0x80)
    {
        return {};
    }

    return UniversalPacket { uint32_t ((uint32_t (type) << 28)
                                       | (uint32_t (status) << 16u)
                                       | (uint32_t (data1) << 8u)
                                       | uint32_t (data2)) };
}

} // namespace yup::ump

#endif
