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

struct DataMessage : UniversalPacket
{
    constexpr DataMessage()
        : UniversalPacket (0x30000000u)
    {
    }

    constexpr explicit DataMessage (Status status)
        : UniversalPacket (0x30000000u | (uint32_t (status) << 16u))
    {
    }
};

struct SysEx7Packet : DataMessage
{
    constexpr SysEx7Packet() = default;

    constexpr SysEx7Packet (Status status, Group group)
        : DataMessage (status)
    {
        setGroup (group);
    }

    constexpr PacketFormat getFormat() const { return PacketFormat ((getStatus() >> 4) & 0x3); }

    constexpr uint8_t getPayloadByte (size_t b) const { return getByte (2 + b); }

    constexpr void setPayloadByte (size_t b, uint8_t data) { setByte7Bit (2 + b, data); }

    constexpr size_t getPayloadSize() const { return getStatus() & 0x0f; }

    constexpr void setPayloadSize (size_t size)
    {
        jassert (size <= 6);
        setByte (1, uint8_t ((getStatus() & 0xf0) + (size & 0x0f)));
    }

    constexpr void addPayloadByte (uint8_t byte)
    {
        const auto size = getPayloadSize();
        jassert (size < 6);
        setByte7Bit (2 + size, byte);
        setPayloadSize (size + 1);
    }
};

constexpr bool isDataMessage (const UniversalPacket& p)
{
    return p.getType() == PacketType::data;
}

constexpr bool isSysEx7Packet (const UniversalPacket& p)
{
    return isDataMessage (p)
        && ((p.getStatus() & 0xf0) <= Status (DataStatus::sysex7_end))
        && ((p.getStatus() & 0x0f) <= 6);
}

struct SysEx7PacketView
{
    constexpr explicit SysEx7PacketView (const UniversalPacket& ump)
        : p (ump)
    {
        jassert (isSysEx7Packet (ump));
    }

    constexpr Group getGroup() const { return p.getGroup(); }

    constexpr Status getStatus() const { return Status (p.getStatus() & 0xf0); }

    constexpr PacketFormat getFormat() const { return PacketFormat ((p.getStatus() >> 4) & 0x3); }

    constexpr size_t getPayloadSize() const { return p.getStatus() & 0x0f; }

    constexpr uint8_t getPayloadByte (size_t b) const { return p.getByte (2 + b); }

private:
    const UniversalPacket& p;
};

constexpr std::optional<SysEx7PacketView> asSysEx7PacketView (const UniversalPacket& p)
{
    if (isSysEx7Packet (p))
        return SysEx7PacketView { p };

    return std::nullopt;
}

constexpr SysEx7Packet makeSysEx7CompletePacket (Group group = 0)
{
    return SysEx7Packet { Status (DataStatus::sysex7_complete), group };
}

constexpr SysEx7Packet makeSysEx7StartPacket (Group group = 0)
{
    return SysEx7Packet { Status (DataStatus::sysex7_start), group };
}

constexpr SysEx7Packet makeSysEx7ContinuePacket (Group group = 0)
{
    return SysEx7Packet { Status (DataStatus::sysex7_continue), group };
}

constexpr SysEx7Packet makeSysEx7EndPacket (Group group = 0)
{
    return SysEx7Packet { Status (DataStatus::sysex7_end), group };
}

} // namespace yup::ump

#endif
