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

struct SysEx
{
    using DataType = std::vector<uint8_t>;

    ManufacturerId manufacturerId { 0 };
    DataType data;

    SysEx() = default;

    explicit SysEx (ManufacturerId manufacturer)
        : manufacturerId (manufacturer)
    {
    }

    SysEx (ManufacturerId manufacturer, size_t capacity)
        : manufacturerId (manufacturer)
    {
        data.reserve (capacity);
    }

    SysEx (ManufacturerId manufacturer, const uint8_t* buffer, size_t bufferSize)
        : manufacturerId (manufacturer)
        , data (buffer, buffer + bufferSize)
    {
    }

    SysEx (ManufacturerId manufacturer, DataType d)
        : manufacturerId (manufacturer)
        , data (std::move (d))
    {
    }

    SysEx (ManufacturerId manufacturer, std::initializer_list<uint8_t> d)
        : manufacturerId (manufacturer)
        , data (d.begin(), d.end())
    {
    }

    size_t totalDataSize() const
    {
        if (manufacturerId == 0)
            return data.size();

        if ((manufacturerId & 0x00ffff) != 0)
            return data.size() + 3u;

        return data.size() + 1u;
    }

    bool isEmpty() const
    {
        return manufacturerId == 0 && data.empty();
    }

    bool is7Bit() const
    {
        for (const auto byte : data)
            if (byte & 0x80)
                return false;

        return true;
    }

    bool is8Bit() const
    {
        for (const auto byte : data)
            if (byte & 0x80)
                return true;

        return false;
    }

    bool operator== (const SysEx& other) const
    {
        return manufacturerId == other.manufacturerId && data == other.data;
    }

    bool operator!= (const SysEx& other) const
    {
        return ! operator== (other);
    }

    void clear()
    {
        manufacturerId = 0;
        data.clear();

        if (data.capacity() > 16384)
            data.shrink_to_fit();
    }
};

struct SysEx7 : SysEx
{
    using SysEx::SysEx;

    enum class Kind : uint8_t
    {
        complete = 0,
        begin = 1,
        continuation = 2,
        end = 3
    };

    struct PacketBytes
    {
        std::array<std::byte, 6> data;
        uint8_t size;
    };

    static constexpr size_t uint7Max = (1u << 7) - 1;
    static constexpr size_t uint14Max = (1u << 14) - 1;
    static constexpr size_t uint28Max = (1u << 28) - 1;

    static uint32_t getNumPacketsRequiredForDataSize (uint32_t size)
    {
        constexpr uint32_t bytesPerPacket = 6;
        return (size / bytesPerPacket) + ((size % bytesPerPacket) != 0);
    }

    static PacketBytes getDataBytes (const PacketX2& packet)
    {
        const auto numBytes = Utils::getChannel (packet[0]);
        constexpr uint8_t maxBytes = 6;
        jassert (numBytes <= maxBytes);

        return {
            { { std::byte { packet.getU8<2>() },
                std::byte { packet.getU8<3>() },
                std::byte { packet.getU8<4>() },
                std::byte { packet.getU8<5>() },
                std::byte { packet.getU8<6>() },
                std::byte { packet.getU8<7>() } } },
            jmin (numBytes, maxBytes)
        };
    }

    bool isValid() const { return ((manufacturerId & 0xff808080u) == 0) && is7Bit(); }

    bool operator== (const SysEx7& other) const { return SysEx::operator== (other); }

    bool operator!= (const SysEx7& other) const { return SysEx::operator!= (other); }

    void addUInt7 (uint7_t value)
    {
        jassert (value <= uint7Max);
        data.push_back (uint8_t (value & 0x7f));
    }

    void addData (const uint7_t* d, size_t dataSize)
    {
        jassert (d != nullptr && dataSize > 0);
        data.insert (data.end(), d, d + dataSize);
    }

    void addUInt14 (uint14_t value)
    {
        jassert (value <= uint14Max);
        const uint7_t d[] { uint7_t (value & 0x7f), uint7_t ((value >> 7) & 0x7f) };
        data.insert (data.end(), std::begin (d), std::end (d));
    }

    uint14_t makeUInt14 (size_t dataPos) const
    {
        jassert (dataPos + 1 < data.size());
        return uint14_t (data[dataPos] | (data[dataPos + 1] << 7));
    }

    void addUInt28 (uint28_t value)
    {
        jassert (value <= uint28Max);
        const uint7_t d[] { uint7_t (value & 0x7f),
                            uint7_t ((value >> 7) & 0x7f),
                            uint7_t ((value >> 14) & 0x7f),
                            uint7_t ((value >> 21) & 0x7f) };
        data.insert (data.end(), std::begin (d), std::end (d));
    }

    uint28_t makeUInt28 (size_t dataPos) const
    {
        jassert (dataPos + 3 < data.size());
        return uint28_t (data[dataPos] | (data[dataPos + 1] << 7)
                         | (data[dataPos + 2] << 14) | (data[dataPos + 3] << 21));
    }

    void addUInt32 (uint32_t value)
    {
        const uint7_t d[] { uint7_t (value & 0x7f),
                            uint7_t ((value >> 7) & 0x7f),
                            uint7_t ((value >> 14) & 0x7f),
                            uint7_t ((value >> 21) & 0x7f),
                            uint7_t ((value >> 28) & 0x0f) };
        data.insert (data.end(), std::begin (d), std::end (d));
    }

    uint32_t makeUInt32 (size_t dataPos) const
    {
        jassert (dataPos + 4 < data.size());
        return uint32_t (data[dataPos] | (data[dataPos + 1] << 7)
                         | (data[dataPos + 2] << 14) | (data[dataPos + 3] << 21)
                         | ((data[dataPos + 4] & 0x0f) << 28));
    }

    void addDeviceIdentity (const DeviceIdentity& identity)
    {
        jassert ((identity.manufacturer & 0xff808080u) == 0);
        jassert ((identity.family & 0xc000u) == 0);
        jassert ((identity.model & 0xc000u) == 0);
        jassert ((identity.revision & 0xf0000000u) == 0);

        data.push_back (uint8_t ((identity.manufacturer >> 16) & 0x7f));
        data.push_back (uint8_t ((identity.manufacturer >> 8) & 0x7f));
        data.push_back (uint8_t (identity.manufacturer & 0x7f));

        addUInt14 (identity.family);
        addUInt14 (identity.model);
        addUInt28 (identity.revision);
    }

    DeviceIdentity makeDeviceIdentity (size_t dataPos) const
    {
        jassert (dataPos + 10 < data.size());

        const auto* d = data.data() + dataPos;
        DeviceIdentity result;
        result.manufacturer = (uint14_t (d[0]) << 16) | (uint14_t (d[1]) << 8) | d[2];
        result.family = makeUInt14 (dataPos + 3);
        result.model = makeUInt14 (dataPos + 5);
        result.revision = makeUInt28 (dataPos + 7);
        return result;
    }
};

struct SysEx8 : SysEx
{
    using SysEx::SysEx;

    bool operator== (const SysEx8& other) const { return SysEx::operator== (other); }

    bool operator!= (const SysEx8& other) const { return SysEx::operator!= (other); }
};

template <typename Sender>
void sendSysEx7 (const SysEx7& sysex, Group group, Sender&& sender)
{
    constexpr size_t maxPayloadSize = 6;

    auto packet = (sysex.totalDataSize() <= maxPayloadSize) ? makeSysEx7CompletePacket (group)
                                                            : makeSysEx7StartPacket (group);

    packet.addPayloadByte ((sysex.manufacturerId >> 16) & 0x7f);
    if (sysex.manufacturerId & 0x00ffff)
    {
        packet.addPayloadByte ((sysex.manufacturerId >> 8) & 0x7f);
        packet.addPayloadByte (sysex.manufacturerId & 0x7f);
    }

    size_t bytesLeft = sysex.data.size();
    for (const auto b : sysex.data)
    {
        packet.addPayloadByte (b);
        --bytesLeft;

        if (packet.getPayloadSize() == maxPayloadSize)
        {
            sender (packet);
            packet = (bytesLeft <= maxPayloadSize) ? makeSysEx7EndPacket (group)
                                                   : makeSysEx7ContinuePacket (group);
        }
    }

    if (packet.getPayloadSize() > 0)
        sender (packet);
}

inline std::vector<DataMessage> asSysEx7Packets (const SysEx7& sysex, Group group = 0)
{
    std::vector<DataMessage> result;
    result.reserve (sysex.totalDataSize() / 6 + 1);

    sendSysEx7 (sysex, group, [&] (const DataMessage& packet)
    {
        result.push_back (packet);
    });

    return result;
}

template <typename Sender>
void sendSysEx8 (const SysEx8& sysex, uint8_t streamId, Group group, Sender&& sender)
{
    constexpr size_t maxPayloadSize = 13;

    auto packet = (sysex.totalDataSize() <= maxPayloadSize) ? makeSysEx8CompletePacket (streamId, group)
                                                            : makeSysEx8StartPacket (streamId, group);

    if (sysex.manufacturerId & 0x00ffff)
    {
        packet.addPayloadByte (uint8_t (0x80 + ((sysex.manufacturerId >> 8) & 0x7f)));
        packet.addPayloadByte (uint8_t (sysex.manufacturerId & 0x7f));
    }
    else
    {
        packet.addPayloadByte (0);
        packet.addPayloadByte (uint8_t ((sysex.manufacturerId >> 16) & 0x7f));
    }

    size_t bytesLeft = sysex.data.size();
    for (const auto b : sysex.data)
    {
        packet.addPayloadByte (b);
        --bytesLeft;

        if (packet.getPayloadSize() == maxPayloadSize)
        {
            sender (packet);
            packet = (bytesLeft <= maxPayloadSize) ? makeSysEx8EndPacket (streamId, group)
                                                   : makeSysEx8ContinuePacket (streamId, group);
        }
    }

    if (packet.getPayloadSize() > 0)
        sender (packet);
}

inline std::vector<ExtendedDataMessage> asSysEx8Packets (const SysEx8& sysex, uint8_t streamId, Group group = 0)
{
    std::vector<ExtendedDataMessage> result;
    result.reserve ((sysex.totalDataSize() + 1) / 14 + 1);

    sendSysEx8 (sysex, streamId, group, [&] (const ExtendedDataMessage& packet)
    {
        result.push_back (packet);
    });

    return result;
}

} // namespace yup::ump

#endif
