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

struct FlexDataMessage : UniversalPacket
{
    constexpr explicit FlexDataMessage (Group group = 0)
        : UniversalPacket (0xD0000000u | (uint32_t (group & 0x0f) << 24u))
    {
    }

    constexpr FlexDataMessage (Group group,
                               PacketFormat format,
                               PacketAddress address,
                               uint4_t channel,
                               Status statusBank,
                               Status status,
                               uint32_t data1 = 0,
                               uint32_t data2 = 0,
                               uint32_t data3 = 0)
        : UniversalPacket (0xD0000000u | (uint32_t (group & 0x0f) << 24u)
                               | ((uint32_t ((uint32_t (format) << 6u)
                                             | (uint32_t (address) << 4u)
                                             | (channel & 0x0f)))
                                  << 16u)
                               | (uint32_t (statusBank) << 8u)
                               | uint32_t (status),
                           data1,
                           data2,
                           data3)
    {
        jassert (channel <= 15u);
        jassert (address == PacketAddress::channel || channel == 0);
    }

    constexpr PacketFormat getFormat() const { return PacketFormat ((getByte2() & 0xc0u) >> 6u); }

    constexpr PacketAddress getAddress() const { return PacketAddress ((getByte2() & 0x30u) >> 4u); }

    constexpr Status getStatusBank() const { return getByte3(); }

    constexpr Status getStatus() const { return getByte4(); }

    std::string getPayloadAsString() const
    {
        return getPayloadAsString (*this);
    }

    static std::string getPayloadAsString (const UniversalPacket& packet)
    {
        std::string result;
        result.reserve (12);
        for (uint8_t b = 4; b < 16; ++b)
        {
            if (const auto c = char (packet.getByte (b)))
                result.push_back (c);
            else
                break;
        }
        return result;
    }
};

constexpr bool isFlexDataMessage (const UniversalPacket& p)
{
    return p.getType() == PacketType::flex_data;
}

struct FlexDataMessageView
{
    constexpr explicit FlexDataMessageView (const UniversalPacket& ump)
        : p (ump)
    {
        jassert (p.getType() == PacketType::flex_data);
    }

    constexpr Group getGroup() const { return p.getGroup(); }

    constexpr PacketFormat getFormat() const { return PacketFormat ((p.getByte2() & 0xc0u) >> 6u); }

    constexpr PacketAddress getAddress() const { return PacketAddress ((p.getByte2() & 0x30u) >> 4u); }

    constexpr Channel getChannel() const { return Channel (p.getByte2() & 0x0f); }

    constexpr Status getStatusBank() const { return p.getByte3(); }

    constexpr Status getStatus() const { return p.getByte4(); }

    constexpr uint32_t getData1() const { return p.data[1]; }

    constexpr uint32_t getData2() const { return p.data[2]; }

    constexpr uint32_t getData3() const { return p.data[3]; }

    std::string getPayloadAsString() const { return FlexDataMessage::getPayloadAsString (p); }

private:
    const UniversalPacket& p;
};

constexpr std::optional<FlexDataMessageView> asFlexDataMessageView (const UniversalPacket& p)
{
    if (isFlexDataMessage (p))
        return FlexDataMessageView { p };

    return std::nullopt;
}

constexpr FlexDataMessage makeFlexDataMessage (Group group,
                                               PacketFormat format,
                                               PacketAddress address,
                                               Channel channel,
                                               Status statusBank,
                                               Status status,
                                               uint32_t data1 = 0,
                                               uint32_t data2 = 0,
                                               uint32_t data3 = 0)
{
    return FlexDataMessage (group, format, address, channel, statusBank, status, data1, data2, data3);
}

constexpr FlexDataMessage makeFlexDataTextMessage (Group group,
                                                   PacketFormat format,
                                                   PacketAddress address,
                                                   Channel channel,
                                                   Status statusBank,
                                                   Status status,
                                                   const std::string_view& text)
{
    jassert (text.length() <= 12);
    jassert (format == PacketFormat::complete || format == PacketFormat::end || text.length() == 12);

    auto result = FlexDataMessage (group, format, address, channel, statusBank, status);
    size_t byte = 4;
    for (const auto c : text)
    {
        result.setByte (byte, uint8_t (c));
        if (++byte >= 16)
            break;
    }
    return result;
}

constexpr FlexDataMessage makeSetTempoMessage (Group group, uint32_t tenNsPerQuarterNote)
{
    return makeFlexDataMessage (group,
                                PacketFormat::complete,
                                PacketAddress::group,
                                0,
                                0x00,
                                0x00,
                                tenNsPerQuarterNote);
}

constexpr FlexDataMessage makeSetTimeSignatureMessage (Group group,
                                                       uint8_t numerator,
                                                       uint8_t denominator,
                                                       uint8_t num32ndNotes)
{
    auto result = makeFlexDataMessage (group, PacketFormat::complete, PacketAddress::group, 0, 0x00, 0x01);
    result.setByte (4, numerator);
    result.setByte (5, denominator);
    result.setByte (6, num32ndNotes);
    return result;
}

constexpr FlexDataMessage makeSetMetronomeMessage (Group group,
                                                   uint8_t numClocksPerPrimaryClick,
                                                   uint8_t barAccentPart1,
                                                   uint8_t barAccentPart2,
                                                   uint8_t barAccentPart3,
                                                   uint8_t numSubdivisionClicks1,
                                                   uint8_t numSubdivisionClicks2)
{
    auto result = makeFlexDataMessage (group, PacketFormat::complete, PacketAddress::group, 0, 0x00, 0x02);
    result.setByte (4, numClocksPerPrimaryClick);
    result.setByte (5, barAccentPart1);
    result.setByte (6, barAccentPart2);
    result.setByte (7, barAccentPart3);
    result.setByte (8, numSubdivisionClicks1);
    result.setByte (9, numSubdivisionClicks2);
    return result;
}

constexpr FlexDataMessage makeSetKeySignatureMessage (Group group,
                                                      PacketAddress address,
                                                      Channel channel,
                                                      uint4_t sharpsOrFlats,
                                                      uint4_t tonicNote)
{
    auto result = makeFlexDataMessage (group, PacketFormat::complete, address, channel, 0x00, 0x05);
    result.setByte (4, uint8_t ((sharpsOrFlats << 4) | (tonicNote & 0x0f)));
    return result;
}

constexpr FlexDataMessage makeSetChordMessage (Group group,
                                               PacketAddress address,
                                               Channel channel,
                                               uint32_t data1,
                                               uint32_t data2,
                                               uint32_t data3)
{
    return makeFlexDataMessage (group, PacketFormat::complete, address, channel, 0x00, 0x06, data1, data2, data3);
}

} // namespace yup::ump

#endif
