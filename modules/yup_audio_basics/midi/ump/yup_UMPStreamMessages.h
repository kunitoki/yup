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

struct StreamMessage : UniversalPacket
{
    constexpr StreamMessage()
        : UniversalPacket (0xF0000000u)
    {
    }

    constexpr explicit StreamMessage (Status status, PacketFormat format = PacketFormat::complete)
        : UniversalPacket (0xF0000000u | (uint32_t (format) << 26u) | (uint32_t (status) << 16u))
    {
    }

    constexpr PacketFormat getFormat() const { return PacketFormat ((data[0] >> 26u) & 0x03u); }

    constexpr void setFormat (PacketFormat format)
    {
        data[0] = (data[0] & 0xF3FFFFFFu) | (uint32_t (format) << 26u);
    }

    static std::string getPayloadAsString (const UniversalPacket& p, uint8_t offset)
    {
        std::string result;
        result.reserve (16 - offset);
        for (uint8_t b = offset; b < 16; ++b)
        {
            if (const auto c = char (p.getByte7Bit (b)))
                result.push_back (c);
            else
                break;
        }
        return result;
    }
};

constexpr bool isStreamMessage (const UniversalPacket& p)
{
    return p.getType() == PacketType::stream;
}

namespace StreamDiscoveryFilter
{
constexpr uint8_t endpointInfo = 0b00001;
constexpr uint8_t deviceIdentity = 0b00010;
constexpr uint8_t endpointName = 0b00100;
constexpr uint8_t productInstanceId = 0b01000;
constexpr uint8_t streamConfiguration = 0b10000;
constexpr uint8_t endpointAll = 0b11111;

constexpr uint8_t functionBlockInfo = 0b01;
constexpr uint8_t functionBlockName = 0b10;
constexpr uint8_t functionBlockAll = 0b11;
} // namespace StreamDiscoveryFilter

struct EndpointDiscoveryView
{
    constexpr explicit EndpointDiscoveryView (const UniversalPacket& ump)
        : p (ump)
    {
        jassert (p.getType() == PacketType::stream);
        jassert (StreamStatus (p.getStatus()) == StreamStatus::endpointDiscovery);
    }

    constexpr uint8_t getUmpVersionMajor() const { return p.getByte3(); }

    constexpr uint8_t getUmpVersionMinor() const { return p.getByte4(); }

    constexpr uint16_t getUmpVersion() const { return uint16_t (p.data[0] & 0xffffu); }

    constexpr uint8_t getFilter() const { return uint8_t (p.data[1] & 0b11111); }

    constexpr bool requestsInfo() const { return (getFilter() & StreamDiscoveryFilter::endpointInfo) != 0; }

    constexpr bool requestsDeviceIdentity() const { return (getFilter() & StreamDiscoveryFilter::deviceIdentity) != 0; }

    constexpr bool requestsName() const { return (getFilter() & StreamDiscoveryFilter::endpointName) != 0; }

    constexpr bool requestsProductInstanceId() const { return (getFilter() & StreamDiscoveryFilter::productInstanceId) != 0; }

    constexpr bool requestsStreamConfiguration() const { return (getFilter() & StreamDiscoveryFilter::streamConfiguration) != 0; }

private:
    const UniversalPacket& p;
};

constexpr std::optional<EndpointDiscoveryView> asEndpointDiscoveryView (const UniversalPacket& p)
{
    if (isStreamMessage (p) && StreamStatus (p.getStatus()) == StreamStatus::endpointDiscovery)
        return EndpointDiscoveryView { p };

    return std::nullopt;
}

struct EndpointInfoView
{
    constexpr explicit EndpointInfoView (const UniversalPacket& ump)
        : p (ump)
    {
        jassert (p.getType() == PacketType::stream);
        jassert (StreamStatus (p.getStatus()) == StreamStatus::endpointInfo);
    }

    constexpr uint8_t getUmpVersionMajor() const { return p.getByte3(); }

    constexpr uint8_t getUmpVersionMinor() const { return p.getByte4(); }

    constexpr uint16_t getUmpVersion() const { return uint16_t (p.data[0] & 0xffffu); }

    constexpr uint8_t getNumFunctionBlocks() const { return p.getByte (4) & 0x7f; }

    constexpr bool hasStaticFunctionBlocks() const { return (p.getByte (4) & 0x80u) != 0; }

    constexpr uint8_t getProtocols() const { return p.getByte (6) & 0x3; }

    constexpr uint8_t getExtensions() const { return p.getByte (7) & 0x3; }

private:
    const UniversalPacket& p;
};

constexpr std::optional<EndpointInfoView> asEndpointInfoView (const UniversalPacket& p)
{
    if (isStreamMessage (p) && StreamStatus (p.getStatus()) == StreamStatus::endpointInfo)
        return EndpointInfoView { p };

    return std::nullopt;
}

struct DeviceIdentityView
{
    constexpr explicit DeviceIdentityView (const UniversalPacket& ump)
        : p (ump)
    {
        jassert (p.getType() == PacketType::stream);
        jassert (StreamStatus (p.getStatus()) == StreamStatus::deviceIdentity);
    }

    constexpr DeviceIdentity getIdentity() const
    {
        return {
            p.data[1] & 0x007f7f7fu,
            uint14_t (((p.data[2] >> 24) & 0x7fu) | ((p.data[2] >> 9) & 0x3f80u)),
            uint14_t (((p.data[2] >> 8) & 0x7fu) | ((p.data[2] << 7) & 0x3f80u)),
            uint28_t (((p.data[3] >> 24) & 0x000007fu)
                      | ((p.data[3] >> 9) & 0x0003f80u)
                      | ((p.data[3] << 6) & 0x1fc000u)
                      | ((p.data[3] << 21) & 0xfe00000u))
        };
    }

private:
    const UniversalPacket& p;
};

constexpr std::optional<DeviceIdentityView> asDeviceIdentityView (const UniversalPacket& p)
{
    if (isStreamMessage (p) && StreamStatus (p.getStatus()) == StreamStatus::deviceIdentity)
        return DeviceIdentityView { p };

    return std::nullopt;
}

struct EndpointNameView
{
    constexpr explicit EndpointNameView (const UniversalPacket& ump)
        : p (ump)
    {
        jassert (p.getType() == PacketType::stream);
        jassert (StreamStatus (p.getStatus()) == StreamStatus::endpointName);
    }

    constexpr PacketFormat getFormat() const { return PacketFormat ((p.data[0] >> 26u) & 0x3u); }

    std::string getPayload() const { return StreamMessage::getPayloadAsString (p, 2); }

private:
    const UniversalPacket& p;
};

constexpr std::optional<EndpointNameView> asEndpointNameView (const UniversalPacket& p)
{
    if (isStreamMessage (p) && StreamStatus (p.getStatus()) == StreamStatus::endpointName)
        return EndpointNameView { p };

    return std::nullopt;
}

struct ProductInstanceIdView
{
    constexpr explicit ProductInstanceIdView (const UniversalPacket& ump)
        : p (ump)
    {
        jassert (p.getType() == PacketType::stream);
        jassert (StreamStatus (p.getStatus()) == StreamStatus::productInstanceId);
    }

    constexpr PacketFormat getFormat() const { return PacketFormat ((p.data[0] >> 26u) & 0x3u); }

    std::string getPayload() const { return StreamMessage::getPayloadAsString (p, 2); }

private:
    const UniversalPacket& p;
};

constexpr std::optional<ProductInstanceIdView> asProductInstanceIdView (const UniversalPacket& p)
{
    if (isStreamMessage (p) && StreamStatus (p.getStatus()) == StreamStatus::productInstanceId)
        return ProductInstanceIdView { p };

    return std::nullopt;
}

struct StreamConfigurationView
{
    constexpr explicit StreamConfigurationView (const UniversalPacket& ump)
        : p (ump)
    {
        jassert (p.getType() == PacketType::stream);
        jassert (StreamStatus (p.getStatus()) == StreamStatus::streamConfigurationRequest
                 || StreamStatus (p.getStatus()) == StreamStatus::streamConfigurationNotify);
    }

    constexpr Protocol getProtocol() const { return p.getByte3() & 0x3; }

    constexpr Extensions getExtensions() const { return p.getByte4() & 0x3; }

private:
    const UniversalPacket& p;
};

constexpr std::optional<StreamConfigurationView> asStreamConfigurationView (const UniversalPacket& p)
{
    if (! isStreamMessage (p))
        return std::nullopt;

    const auto status = StreamStatus (p.getStatus());
    if (status == StreamStatus::streamConfigurationRequest || status == StreamStatus::streamConfigurationNotify)
        return StreamConfigurationView { p };

    return std::nullopt;
}

struct FunctionBlockDiscoveryView
{
    constexpr explicit FunctionBlockDiscoveryView (const UniversalPacket& ump)
        : p (ump)
    {
        jassert (p.getType() == PacketType::stream);
        jassert (StreamStatus (p.getStatus()) == StreamStatus::functionBlockDiscovery);
    }

    constexpr uint8_t getFunctionBlock() const { return p.getByte3(); }

    constexpr uint8_t getFilter() const { return p.getByte4() & 0x0f; }

    constexpr bool requestsFunctionBlock (uint8_t block) const
    {
        constexpr uint8_t allBlocks = 0xff;
        return getFunctionBlock() == allBlocks || getFunctionBlock() == block;
    }

    constexpr bool requestsInfo() const { return (getFilter() & StreamDiscoveryFilter::functionBlockInfo) != 0; }

    constexpr bool requestsName() const { return (getFilter() & StreamDiscoveryFilter::functionBlockName) != 0; }

private:
    const UniversalPacket& p;
};

constexpr std::optional<FunctionBlockDiscoveryView> asFunctionBlockDiscoveryView (const UniversalPacket& p)
{
    if (isStreamMessage (p) && StreamStatus (p.getStatus()) == StreamStatus::functionBlockDiscovery)
        return FunctionBlockDiscoveryView { p };

    return std::nullopt;
}

struct FunctionBlockOptions
{
    bool active = true;

    static constexpr uint2_t directionInput = 0b01;
    static constexpr uint2_t directionOutput = 0b10;
    static constexpr uint2_t bidirectional = 0b11;

    uint2_t direction = bidirectional;

    static constexpr uint2_t notMidi1 = 0b00;
    static constexpr uint2_t midi1Unrestricted = 0b01;
    static constexpr uint2_t midi1_31250 = 0b10;

    uint2_t midi1 = notMidi1;

    static constexpr uint2_t uiHintAsDirection = 0b00;
    static constexpr uint2_t uiHintReceiver = 0b01;
    static constexpr uint2_t uiHintSender = 0b10;

    uint2_t uiHint = uiHintAsDirection;

    uint8_t ciMessageVersion = 0x00;
    uint8_t maxNumSysEx8Streams = 0;
};

struct FunctionBlockInfoView
{
    constexpr explicit FunctionBlockInfoView (const UniversalPacket& ump)
        : p (ump)
    {
        jassert (p.getType() == PacketType::stream);
        jassert (StreamStatus (p.getStatus()) == StreamStatus::functionBlockInfo);
    }

    constexpr bool isActive() const { return (p.data[0] & 0x00008000u) != 0; }

    constexpr uint8_t getFunctionBlock() const { return p.getByte7Bit (2); }

    constexpr uint8_t getDirection() const { return uint8_t (p.data[0] & 0x3); }

    constexpr uint8_t getMidi1() const { return uint8_t ((p.data[0] >> 2) & 0x3); }

    constexpr uint8_t getUiHint() const { return uint8_t ((p.data[0] >> 4) & 0x3); }

    constexpr uint8_t getFirstGroup() const { return p.getByte (4); }

    constexpr uint8_t getNumGroupsSpanned() const { return p.getByte (5); }

    constexpr uint7_t getCiMessageVersion() const { return p.getByte (6); }

    constexpr uint8_t getMaxNumSysEx8Streams() const { return p.getByte (7); }

private:
    const UniversalPacket& p;
};

constexpr std::optional<FunctionBlockInfoView> asFunctionBlockInfoView (const UniversalPacket& p)
{
    if (isStreamMessage (p) && StreamStatus (p.getStatus()) == StreamStatus::functionBlockInfo)
        return FunctionBlockInfoView { p };

    return std::nullopt;
}

struct FunctionBlockNameView
{
    constexpr explicit FunctionBlockNameView (const UniversalPacket& ump)
        : p (ump)
    {
        jassert (p.getType() == PacketType::stream);
        jassert (StreamStatus (p.getStatus()) == StreamStatus::functionBlockName);
    }

    constexpr PacketFormat getFormat() const { return PacketFormat ((p.data[0] >> 26u) & 0x3u); }

    constexpr uint8_t getFunctionBlock() const { return p.getByte3() & 0x7f; }

    std::string getPayload() const { return StreamMessage::getPayloadAsString (p, 3); }

private:
    const UniversalPacket& p;
};

constexpr std::optional<FunctionBlockNameView> asFunctionBlockNameView (const UniversalPacket& p)
{
    if (isStreamMessage (p) && StreamStatus (p.getStatus()) == StreamStatus::functionBlockName)
        return FunctionBlockNameView { p };

    return std::nullopt;
}

//==============================================================================
constexpr StreamMessage makeEndpointDiscoveryMessage (uint8_t filter,
                                                      uint8_t umpVersionMajor = 1,
                                                      uint8_t umpVersionMinor = 1)
{
    StreamMessage message { Status (StreamStatus::endpointDiscovery), PacketFormat::complete };
    message.setByte (2, umpVersionMajor);
    message.setByte (3, umpVersionMinor);
    message.data[1] = filter;
    return message;
}

constexpr StreamMessage makeEndpointInfoMessage (uint8_t numFunctionBlocks,
                                                 bool staticFunctionBlocks,
                                                 uint8_t protocols,
                                                 uint8_t extensions,
                                                 uint8_t umpVersionMajor = 1,
                                                 uint8_t umpVersionMinor = 1)
{
    StreamMessage message { Status (StreamStatus::endpointInfo), PacketFormat::complete };
    message.setByte (2, umpVersionMajor);
    message.setByte (3, umpVersionMinor);
    message.setByte (4, uint8_t ((staticFunctionBlocks ? 0x80u : 0x00u) | numFunctionBlocks));
    message.setByte (6, protocols);
    message.setByte (7, extensions);
    return message;
}

constexpr StreamMessage makeDeviceIdentityMessage (const DeviceIdentity& identity)
{
    StreamMessage message { Status (StreamStatus::deviceIdentity), PacketFormat::complete };
    message.data[1] = identity.manufacturer;
    message.data[2] = ((uint32_t (identity.family) << 24) & 0x7F000000u)
                    | ((uint32_t (identity.family) << 9) & 0x007F0000u)
                    | ((uint32_t (identity.model) << 8) & 0x00007F00u)
                    | ((identity.model >> 7) & 0x0000007Fu);
    message.data[3] = ((identity.revision << 24) & 0x7F000000u)
                    | ((identity.revision << 9) & 0x007F0000u)
                    | ((identity.revision >> 6) & 0x00007F00u)
                    | ((identity.revision >> 21) & 0x0000007Fu);
    return message;
}

constexpr StreamMessage makeEndpointNameMessage (PacketFormat format, const std::string_view& name)
{
    jassert (name.length() <= 14);
    StreamMessage message { Status (StreamStatus::endpointName), format };
    uint8_t byte = 2;
    for (const auto c : name)
    {
        message.setByte (byte, uint8_t (c));
        if (++byte >= 16)
            break;
    }
    return message;
}

constexpr StreamMessage makeProductInstanceIdMessage (PacketFormat format, const std::string_view& name)
{
    jassert (name.length() <= 14);
    jassert (format != PacketFormat::cont);
    StreamMessage message { Status (StreamStatus::productInstanceId), format };
    uint8_t byte = 2;
    for (const auto c : name)
    {
        message.setByte7Bit (byte, uint8_t (c));
        if (++byte >= 16)
            break;
    }
    return message;
}

constexpr StreamMessage makeStreamConfigurationRequest (Protocol protocol, Extensions extensions = 0)
{
    jassert (protocol && protocol < 0x3);
    StreamMessage message { Status (StreamStatus::streamConfigurationRequest), PacketFormat::complete };
    message.setByte (2, protocol);
    message.setByte (3, extensions);
    return message;
}

constexpr StreamMessage makeStreamConfigurationNotification (Protocol protocol, Extensions extensions = 0)
{
    jassert (protocol && protocol < 0x3);
    StreamMessage message { Status (StreamStatus::streamConfigurationNotify), PacketFormat::complete };
    message.setByte (2, protocol);
    message.setByte (3, extensions);
    return message;
}

constexpr StreamMessage makeFunctionBlockDiscoveryMessage (uint8_t functionBlock, uint8_t filter)
{
    jassert (functionBlock == 0xff || functionBlock < 32);
    StreamMessage message { Status (StreamStatus::functionBlockDiscovery), PacketFormat::complete };
    message.setByte (2, functionBlock);
    message.setByte (3, filter);
    return message;
}

constexpr StreamMessage makeFunctionBlockInfoMessage (uint7_t functionBlock,
                                                      uint4_t direction,
                                                      Group firstGroup,
                                                      uint4_t numGroupsSpanned = 1)
{
    jassert (functionBlock < 32);
    jassert (direction > 0 && direction < 4);
    StreamMessage message { Status (StreamStatus::functionBlockInfo), PacketFormat::complete };
    message.setByte (2, uint8_t (0x80u | (functionBlock & 0x1f)));
    message.setByte (3, uint8_t (((direction & 0x03u) << 4) | (direction & 0x03u)));
    message.setByte (4, firstGroup & 0x0f);
    message.setByte (5, numGroupsSpanned & 0x0f);
    return message;
}

constexpr StreamMessage makeFunctionBlockInfoMessage (uint7_t functionBlock,
                                                      const FunctionBlockOptions& options,
                                                      Group firstGroup,
                                                      uint4_t numGroupsSpanned = 1)
{
    jassert (functionBlock < 32);
    jassert (options.direction > 0 && options.direction < 4);
    jassert (options.midi1 < 3);
    jassert (options.uiHint < 4);
    jassert (options.uiHint == 0 || (options.direction & options.uiHint));

    StreamMessage message { Status (StreamStatus::functionBlockInfo), PacketFormat::complete };
    message.setByte (2, uint8_t ((options.active ? 0x80u : 0x00u) | (functionBlock & 0x1f)));
    message.setByte (3, uint8_t ((((options.uiHint ? options.uiHint : options.direction) & 0x03u) << 4) | ((options.midi1 & 0x03u) << 2) | (options.direction & 0x03u)));
    message.setByte (4, firstGroup & 0x0f);
    message.setByte (5, numGroupsSpanned & 0x0f);
    message.setByte (6, options.ciMessageVersion);
    message.setByte (7, options.maxNumSysEx8Streams);
    return message;
}

constexpr StreamMessage makeFunctionBlockNameMessage (PacketFormat format,
                                                      uint7_t functionBlock,
                                                      const std::string_view& name)
{
    jassert (name.length() <= 13);
    StreamMessage message { Status (StreamStatus::functionBlockName), format };
    message.setByte (2, functionBlock);

    uint8_t byte = 3;
    for (const auto c : name)
    {
        message.setByte (byte, uint8_t (c));
        if (++byte >= 16)
            break;
    }
    return message;
}

template <typename Sender>
void sendEndpointName (std::string_view name, Sender&& sender)
{
    if (name.length() <= 14)
    {
        sender (makeEndpointNameMessage (PacketFormat::complete, name));
        return;
    }

    sender (makeEndpointNameMessage (PacketFormat::start, name.substr (0, 14)));
    name.remove_prefix (14);

    while (name.size() > 14)
    {
        sender (makeEndpointNameMessage (PacketFormat::cont, name.substr (0, 14)));
        name.remove_prefix (14);
    }

    sender (makeEndpointNameMessage (PacketFormat::end, name));
}

template <typename Sender>
void sendProductInstanceId (std::string_view productInstanceId, Sender&& sender)
{
    jassert (productInstanceId.length() <= 16);

    if (productInstanceId.length() <= 14)
    {
        sender (makeProductInstanceIdMessage (PacketFormat::complete, productInstanceId));
        return;
    }

    sender (makeProductInstanceIdMessage (PacketFormat::start, productInstanceId.substr (0, 14)));
    productInstanceId.remove_prefix (14);
    sender (makeProductInstanceIdMessage (PacketFormat::end, productInstanceId.substr (0, 2)));
}

template <typename Sender>
void sendFunctionBlockName (uint7_t functionBlock, std::string_view name, Sender&& sender)
{
    if (name.length() <= 13)
    {
        sender (makeFunctionBlockNameMessage (PacketFormat::complete, functionBlock, name));
        return;
    }

    sender (makeFunctionBlockNameMessage (PacketFormat::start, functionBlock, name.substr (0, 13)));
    name.remove_prefix (13);

    while (name.size() > 13)
    {
        sender (makeFunctionBlockNameMessage (PacketFormat::cont, functionBlock, name.substr (0, 13)));
        name.remove_prefix (13);
    }

    sender (makeFunctionBlockNameMessage (PacketFormat::end, functionBlock, name));
}

} // namespace yup::ump

#endif
