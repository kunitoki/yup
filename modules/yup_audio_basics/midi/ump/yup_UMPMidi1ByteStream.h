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
                case SystemStatus::song_position:
                    return 3u;
                case SystemStatus::mtc_quarter_frame:
                case SystemStatus::song_select:
                    return 2u;
                case SystemStatus::tune_request:
                case SystemStatus::clock:
                case SystemStatus::start:
                case SystemStatus::cont:
                case SystemStatus::stop:
                case SystemStatus::active_sense:
                case SystemStatus::reset:
                    return 1u;
            }
            break;
        case PacketType::midi1_channel_voice:
            switch (packet.getStatus() & 0xf0)
            {
                case Status (Midi1ChannelVoiceStatus::note_off):
                case Status (Midi1ChannelVoiceStatus::note_on):
                case Status (Midi1ChannelVoiceStatus::poly_pressure):
                case Status (Midi1ChannelVoiceStatus::control_change):
                case Status (Midi1ChannelVoiceStatus::pitch_bend):
                    return 3u;
                case Status (Midi1ChannelVoiceStatus::program_change):
                case Status (Midi1ChannelVoiceStatus::channel_pressure):
                    return 2u;
            }
            break;
        case PacketType::data:
            switch (packet.getStatus() & 0xf0)
            {
                case Status (DataStatus::sysex7_complete):
                case Status (DataStatus::sysex7_start):
                case Status (DataStatus::sysex7_end):
                case Status (DataStatus::sysex7_continue):
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
    auto type = PacketType::midi1_channel_voice;

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
