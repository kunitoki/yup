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

#include <iomanip>
#include <ios>
#include <istream>
#include <ostream>

namespace yup::ump
{

enum class PacketType : uint4_t
{
    utility = 0x0,
    system = 0x1,
    midi1ChannelVoice = 0x2,
    data = 0x3,
    midi2ChannelVoice = 0x4,
    extendedData = 0x5,
    flexData = 0xD,
    stream = 0xF
};

enum class PacketFormat : uint2_t
{
    complete = 0b00,
    start = 0b01,
    cont = 0b10,
    end = 0b11
};

enum class PacketAddress : uint2_t
{
    channel = 0b00,
    group = 0b01
};

enum class UtilityStatus : Status
{
    noop = 0x00,
    jitterClock = 0x10,
    jitterTimestamp = 0x20
};

enum class SystemStatus : Status
{
    mtcQuarterFrame = 0xF1,
    songPosition = 0xF2,
    songSelect = 0xF3,
    tuneRequest = 0xF6,
    clock = 0xF8,
    start = 0xFA,
    cont = 0xFB,
    stop = 0xFC,
    activeSense = 0xFE,
    reset = 0xFF
};

enum class Midi1ChannelVoiceStatus : Status
{
    noteOff = 0x80,
    noteOn = 0x90,
    polyPressure = 0xA0,
    controlChange = 0xB0,
    programChange = 0xC0,
    channelPressure = 0xD0,
    pitchBend = 0xE0
};

enum class DataStatus : Status
{
    sysex7Complete = (Status (PacketFormat::complete) << 4),
    sysex7Start = (Status (PacketFormat::start) << 4),
    sysex7Continue = (Status (PacketFormat::cont) << 4),
    sysex7End = (Status (PacketFormat::end) << 4)
};

enum class ChannelVoiceStatus : Status
{
    registeredPerNoteController = 0x00,
    assignablePerNoteController = 0x10,
    registeredController = 0x20,
    assignableController = 0x30,
    relativeRegisteredController = 0x40,
    relativeAssignableController = 0x50,
    perNotePitchBend = 0x60,
    noteOff = 0x80,
    noteOn = 0x90,
    polyPressure = 0xA0,
    controlChange = 0xB0,
    programChange = 0xC0,
    channelPressure = 0xD0,
    pitchBend = 0xE0,
    perNoteManagement = 0xF0
};

enum class ExtendedDataStatus : Status
{
    sysex8Complete = (Status (PacketFormat::complete) << 4),
    sysex8Start = (Status (PacketFormat::start) << 4),
    sysex8Continue = (Status (PacketFormat::cont) << 4),
    sysex8End = (Status (PacketFormat::end) << 4),
    mixedDataSetHeader = 0x80,
    mixedDataSetPayload = 0x90
};

enum class StreamProtocol : Protocol
{
    midi1 = 0x1,
    midi2 = 0x2
};

enum class StreamExtensions : Extensions
{
    jitterReductionTransmit = 0x1,
    jitterReductionReceive = 0x2
};

enum class StreamStatus : Status
{
    endpointDiscovery = 0x00,
    endpointInfo = 0x01,
    deviceIdentity = 0x02,
    endpointName = 0x03,
    productInstanceId = 0x04,
    streamConfigurationRequest = 0x05,
    streamConfigurationNotify = 0x06,
    functionBlockDiscovery = 0x10,
    functionBlockInfo = 0x11,
    functionBlockName = 0x12
};

namespace ControlChange
{
constexpr ControllerNumber bankSelectMsb = 0;
constexpr ControllerNumber dataEntryMsb = 6;
constexpr ControllerNumber bankSelectLsb = 32;
constexpr ControllerNumber dataEntryLsb = 38;
constexpr ControllerNumber nrpnLsb = 98;
constexpr ControllerNumber nrpnMsb = 99;
constexpr ControllerNumber rpnLsb = 100;
constexpr ControllerNumber rpnMsb = 101;
constexpr ControllerNumber hiResVelocityPrefix = 88;
} // namespace ControlChange

namespace RegisteredParameterNumber
{
constexpr ControllerNumber pitchBendSensitivity = 0;
constexpr ControllerNumber fineTuning = 1;
constexpr ControllerNumber coarseTuning = 2;
constexpr ControllerNumber tuningProgramSelect = 3;
constexpr ControllerNumber tuningBankSelect = 4;
constexpr ControllerNumber perNotePitchBendSensitivity = 7;
} // namespace RegisteredParameterNumber

namespace RegisteredPerNoteController
{
constexpr ControllerNumber modulation = 1;
constexpr ControllerNumber breath = 2;
constexpr ControllerNumber pitch7_25 = 3;

constexpr ControllerNumber volume = 7;
constexpr ControllerNumber balance = 8;

constexpr ControllerNumber pan = 10;
constexpr ControllerNumber expression = 11;

constexpr ControllerNumber soundController1 = 70;
constexpr ControllerNumber soundVariation = 70;
constexpr ControllerNumber soundController2 = 71;
constexpr ControllerNumber timbre = 71;
constexpr ControllerNumber harmonicIntensity = 71;
constexpr ControllerNumber soundController3 = 72;
constexpr ControllerNumber releaseTime = 72;
constexpr ControllerNumber soundController4 = 73;
constexpr ControllerNumber attackTime = 73;
constexpr ControllerNumber soundController5 = 74;
constexpr ControllerNumber brightness = 74;
constexpr ControllerNumber soundController6 = 75;
constexpr ControllerNumber decayTime = 75;
constexpr ControllerNumber soundController7 = 76;
constexpr ControllerNumber vibratoRate = 76;
constexpr ControllerNumber soundController8 = 77;
constexpr ControllerNumber vibratoDepth = 77;
constexpr ControllerNumber soundController9 = 78;
constexpr ControllerNumber vibratoDelay = 78;
constexpr ControllerNumber soundController10 = 79;

constexpr ControllerNumber effects1Depth = 91;
constexpr ControllerNumber reverbSendLevel = 91;
constexpr ControllerNumber effects2Depth = 92;
constexpr ControllerNumber effects3Depth = 93;
constexpr ControllerNumber chorusSendLevel = 93;
constexpr ControllerNumber effects4Depth = 94;
constexpr ControllerNumber effects5Depth = 95;
} // namespace RegisteredPerNoteController

struct UniversalPacket
{
    uint32_t data[4] { 0u, 0u, 0u, 0u };

    constexpr PacketType getType() const { return static_cast<PacketType> ((data[0] >> 28u) & 0x0f); }

    constexpr size_t getSize() const
    {
        constexpr size_t sizeLookup[16] { 1, 1, 1, 2, 2, 4, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4 };
        return sizeLookup[static_cast<unsigned> (getType())];
    }

    constexpr Group getGroup() const { return Group ((data[0] >> 24u) & 0x0f); }

    constexpr Status getStatus() const { return getByte2(); }

    constexpr uint8_t getByte2() const { return uint8_t ((data[0] >> 16u) & 0xff); }

    constexpr uint8_t getByte3() const { return uint8_t ((data[0] >> 8u) & 0xff); }

    constexpr uint8_t getByte4() const { return uint8_t (data[0] & 0xff); }

    constexpr uint8_t getByte (size_t b) const
    {
        jassert (b < 16);
        const auto word = b / 4;
        const auto byte = b % 4;
        const auto shift = (3 - byte) * 8;
        return uint8_t ((data[word] >> shift) & 0xff);
    }

    constexpr uint7_t getByte7Bit (size_t b) const { return uint7_t (getByte (b) & 0x7f); }

    constexpr bool hasChannel() const
    {
        return getType() == PacketType::midi1ChannelVoice
            || getType() == PacketType::midi2ChannelVoice
            || getType() == PacketType::flexData;
    }

    constexpr Channel getChannel() const
    {
        jassert (hasChannel());
        return Channel (getByte2() & 0x0f);
    }

    constexpr void setType (PacketType t)
    {
        data[0] = (data[0] & 0x0fffffff) | (uint32_t (t) << 28u);
    }

    constexpr void setGroup (Group c)
    {
        data[0] = (data[0] & 0xf0ffffff) | (uint32_t (c) << 24u);
    }

    constexpr void setByte (size_t b, uint8_t v)
    {
        jassert (b < 16);
        if (b >= 16)
            return;

        uint32_t& word = data[b / 4];
        const auto byteIndex = b % 4;
        const auto shift = (3 - byteIndex) * 8;
        const auto mask = uint32_t (0xffu << shift);
        word = (word & ~mask) | (uint32_t (v) << shift);
    }

    constexpr void setByte7Bit (size_t b, uint8_t v) { setByte (b, uint8_t (v & 0x7f)); }

    constexpr UniversalPacket() = default;

    constexpr explicit UniversalPacket (uint32_t w) { data[0] = w; }

    constexpr UniversalPacket (uint32_t w1, uint32_t w2)
    {
        data[0] = w1;
        data[1] = w2;
    }

    constexpr UniversalPacket (uint32_t w1, uint32_t w2, uint32_t w3)
    {
        data[0] = w1;
        data[1] = w2;
        data[2] = w3;
    }

    constexpr UniversalPacket (uint32_t w1, uint32_t w2, uint32_t w3, uint32_t w4)
    {
        data[0] = w1;
        data[1] = w2;
        data[2] = w3;
        data[3] = w4;
    }

    constexpr bool operator== (const UniversalPacket& other) const
    {
        if (this == &other)
            return true;

        const auto len = getSize();
        for (size_t i = 0; i < len; ++i)
            if (data[i] != other.data[i])
                return false;

        return true;
    }

    constexpr bool operator!= (const UniversalPacket& other) const { return ! operator== (other); }

    constexpr void reset()
    {
        for (auto& d : data)
            d = 0;
    }

    constexpr bool isUtilityMessage() const { return getType() == PacketType::utility; }

    constexpr bool isSystemMessage() const { return getType() == PacketType::system; }

    constexpr bool isChannelVoiceMessage() const
    {
        return getType() == PacketType::midi1ChannelVoice || getType() == PacketType::midi2ChannelVoice;
    }

    constexpr bool isDataMessage() const
    {
        return getType() == PacketType::data || getType() == PacketType::extendedData;
    }

    constexpr bool isMidi1ProtocolMessage() const
    {
        if (getType() == PacketType::system)
        {
            switch (SystemStatus (getStatus()))
            {
                case SystemStatus::mtcQuarterFrame:
                case SystemStatus::songPosition:
                case SystemStatus::songSelect:
                case SystemStatus::tuneRequest:
                case SystemStatus::clock:
                case SystemStatus::start:
                case SystemStatus::cont:
                case SystemStatus::stop:
                case SystemStatus::activeSense:
                case SystemStatus::reset:
                    return true;
                default:
                    return false;
            }
        }

        if (getType() == PacketType::midi1ChannelVoice)
            return getStatus() >= 0x80 && getStatus() < 0xf0;

        return false;
    }
};

namespace detail
{
struct IosBaseFlagsRestorer
{
    explicit IosBaseFlagsRestorer (std::ios_base& stream)
        : strm (stream)
        , flags (stream.flags())
    {
    }

    ~IosBaseFlagsRestorer() { strm.flags (flags); }

    std::ios_base& strm;
    std::ios_base::fmtflags flags;
};
} // namespace detail

inline std::ostream& operator<< (std::ostream& out, const UniversalPacket& p)
{
    detail::IosBaseFlagsRestorer flagRestorer (out);

    for (size_t word = 0; word < p.getSize(); ++word)
    {
        if (word != 0)
            out << ' ';
        out << std::hex << std::setfill ('0') << std::setw (8) << p.data[word];
    }

    return out;
}

inline std::istream& operator>> (std::istream& in, UniversalPacket& p)
{
    detail::IosBaseFlagsRestorer flagRestorer (in);
    in >> std::hex >> p.data[0];

    if (in.good())
    {
        const auto words = p.getSize();
        for (size_t word = 1; word < words; ++word)
        {
            in >> std::hex >> p.data[word];
            if (! in.good())
                break;
        }
    }

    return in;
}

} // namespace yup::ump

#endif
