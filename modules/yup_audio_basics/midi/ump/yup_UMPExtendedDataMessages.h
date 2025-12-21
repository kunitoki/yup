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

struct ExtendedDataMessage : UniversalPacket
{
    constexpr ExtendedDataMessage()
        : UniversalPacket (0x50000000u)
    {
    }

    constexpr explicit ExtendedDataMessage (Status status)
        : UniversalPacket (0x50000000u | (uint32_t (status) << 16u))
    {
    }
};

struct SysEx8Packet : ExtendedDataMessage
{
    constexpr SysEx8Packet()
    {
        data[0] |= 0x00010000u;
    }

    constexpr SysEx8Packet (Status status, uint8_t streamId, Group group)
    {
        data[0] = 0x50010000u | (uint32_t (group) << 24u) | (uint32_t (status) << 16u) | (uint32_t (streamId) << 8u);
    }

    constexpr PacketFormat getFormat() const { return PacketFormat ((getStatus() >> 4) & 0x3); }

    constexpr uint8_t getStreamId() const { return getByte (2); }

    constexpr void setStreamId (uint8_t id) { setByte (2, id); }

    constexpr uint8_t getPayloadByte (size_t b) const { return getByte (3 + b); }

    constexpr void setPayloadByte (size_t b, uint8_t data) { setByte (3 + b, data); }

    constexpr size_t getPayloadSize() const
    {
        const auto s = size_t (getStatus() & 0x0f);
        return (s > 0 ? s - 1 : 0);
    }

    constexpr void setPayloadSize (size_t size)
    {
        jassert (size <= 13);
        setByte (1, uint8_t ((getStatus() & 0xf0) + ((size + 1) & 0x0f)));
    }

    constexpr void addPayloadByte (uint8_t byte)
    {
        const auto size = getPayloadSize();
        jassert (size < 13);
        setByte (3 + size, byte);
        setPayloadSize (size + 1);
    }
};

constexpr bool isExtendedDataMessage (const UniversalPacket& p)
{
    return p.getType() == PacketType::extended_data;
}

constexpr bool isSysEx8Packet (const UniversalPacket& p)
{
    return isExtendedDataMessage (p)
        && ((p.getStatus() & 0xf0) <= Status (ExtendedDataStatus::sysex8_end))
        && ((p.getStatus() & 0x0f) > 0)
        && ((p.getStatus() & 0x0f) <= 14);
}

struct SysEx8PacketView
{
    constexpr explicit SysEx8PacketView (const UniversalPacket& ump)
        : p (ump)
    {
        jassert (isSysEx8Packet (ump));
    }

    constexpr Group getGroup() const { return p.getGroup(); }

    constexpr PacketFormat getFormat() const { return PacketFormat ((p.getStatus() >> 4) & 0x3); }

    constexpr uint8_t getStreamId() const { return p.getByte (2); }

    constexpr size_t getPayloadSize() const
    {
        const auto s = size_t (p.getStatus() & 0x0f);
        return (s > 0 ? s - 1 : 0);
    }

    constexpr uint8_t getPayloadByte (size_t b) const { return p.getByte (3 + b); }

private:
    const UniversalPacket& p;
};

constexpr std::optional<SysEx8PacketView> asSysEx8PacketView (const UniversalPacket& p)
{
    if (isSysEx8Packet (p))
        return SysEx8PacketView { p };

    return std::nullopt;
}

constexpr SysEx8Packet makeSysEx8CompletePacket (uint8_t streamId, Group group = 0)
{
    return SysEx8Packet { Status (ExtendedDataStatus::sysex8_complete), streamId, group };
}

constexpr SysEx8Packet makeSysEx8StartPacket (uint8_t streamId, Group group = 0)
{
    return SysEx8Packet { Status (ExtendedDataStatus::sysex8_start), streamId, group };
}

constexpr SysEx8Packet makeSysEx8ContinuePacket (uint8_t streamId, Group group = 0)
{
    return SysEx8Packet { Status (ExtendedDataStatus::sysex8_continue), streamId, group };
}

constexpr SysEx8Packet makeSysEx8EndPacket (uint8_t streamId, Group group = 0)
{
    return SysEx8Packet { Status (ExtendedDataStatus::sysex8_end), streamId, group };
}

} // namespace yup::ump

#endif
