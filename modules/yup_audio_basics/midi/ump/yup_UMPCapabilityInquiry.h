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

inline bool isCapabilityInquiryMessage (const SysEx7& message)
{
    if (! isUniversalSysExMessage (message))
        return false;

    const auto view = UniversalSysEx::MessageView { message };
    return view.getType() == UniversalSysEx::TypeId::capabilityInquiry;
}

struct CapabilityInquiryView : UniversalSysEx::MessageView
{
    explicit CapabilityInquiryView (const SysEx7& message)
        : UniversalSysEx::MessageView (message)
    {
    }

    explicit CapabilityInquiryView (const UniversalSysEx::MessageView& message)
        : UniversalSysEx::MessageView (message)
    {
    }

    uint7_t getMessageVersion() const { return sysex.data[fieldOffsets.messageVersion]; }

    Muid getSourceMuid() const { return sysex.makeUInt28 (fieldOffsets.sourceMuid); }

    Muid getDestinationMuid() const { return sysex.makeUInt28 (fieldOffsets.destinationMuid); }

    static bool validate (const SysEx7& message)
    {
        return isCapabilityInquiryMessage (message) && message.data.size() >= fieldOffsets.payload;
    }

    struct fieldOffsets
    {
        static constexpr size_t messageVersion = 3u;
        static constexpr size_t sourceMuid = 4u;
        static constexpr size_t destinationMuid = 8u;
        static constexpr size_t payload = 12u;
    };
};

namespace ci
{

template <typename View>
std::optional<View> as (const SysEx7& message)
{
    if (View::validate (message))
        return View { message };

    return std::nullopt;
}

template <typename View>
std::optional<View> as (const CapabilityInquiryView& message)
{
    if (View::validate (message.sysex))
        return View { message.sysex };

    return std::nullopt;
}

constexpr uint7_t messageVersion1 = 0x01;
constexpr uint7_t messageVersion2 = 0x02;
constexpr uint7_t version = messageVersion2;

constexpr Muid broadcastMuid = 0x0fffffff;

namespace Category
{
constexpr uint7_t profileConfiguration = (1 << 2);
constexpr uint7_t propertyExchange = (1 << 3);
constexpr uint7_t processInquiry = (1 << 4);
} // namespace Category

struct Message : UniversalSysEx::Message
{
    static constexpr size_t offsetOfData = 12;

    uint7_t getMessageVersion() const { return data[fieldOffsets.messageVersion]; }

    Muid getSourceMuid() const { return makeUInt28 (fieldOffsets.sourceMuid); }

    Muid getDestinationMuid() const { return makeUInt28 (fieldOffsets.destinationMuid); }

    void setSourceMuid (Muid muid)
    {
        jassert (muid <= uint28Max);
        data[fieldOffsets.sourceMuid] = uint7_t (muid & 0x7f);
        data[fieldOffsets.sourceMuid + 1] = uint7_t ((muid >> 7) & 0x7f);
        data[fieldOffsets.sourceMuid + 2] = uint7_t ((muid >> 14) & 0x7f);
        data[fieldOffsets.sourceMuid + 3] = uint7_t ((muid >> 21) & 0x7f);
    }

    void setDestinationMuid (Muid muid)
    {
        jassert (muid <= uint28Max);
        data[fieldOffsets.destinationMuid] = uint7_t (muid & 0x7f);
        data[fieldOffsets.destinationMuid + 1] = uint7_t ((muid >> 7) & 0x7f);
        data[fieldOffsets.destinationMuid + 2] = uint7_t ((muid >> 14) & 0x7f);
        data[fieldOffsets.destinationMuid + 3] = uint7_t ((muid >> 21) & 0x7f);
    }

    Message (uint7_t subtype, Muid sourceMuid, Muid destinationMuid, uint7_t deviceId = 0x7f)
        : UniversalSysEx::Message (Manufacturer::universalNonRealtime)
    {
        jassert (subtype <= uint7Max);
        jassert (sourceMuid <= uint28Max);
        jassert (destinationMuid <= uint28Max);
        jassert (deviceId <= uint7Max);

        data = { deviceId,
                 0x0d,
                 subtype,
                 version,
                 uint7_t (sourceMuid & 0x7f),
                 uint7_t ((sourceMuid >> 7) & 0x7f),
                 uint7_t ((sourceMuid >> 14) & 0x7f),
                 uint7_t ((sourceMuid >> 21) & 0x7f),
                 uint7_t (destinationMuid & 0x7f),
                 uint7_t ((destinationMuid >> 7) & 0x7f),
                 uint7_t ((destinationMuid >> 14) & 0x7f),
                 uint7_t ((destinationMuid >> 21) & 0x7f) };
    }

    static Message makeWithPayloadSize (size_t payloadSize,
                                        uint7_t subtype,
                                        Muid sourceMuid,
                                        Muid destinationMuid,
                                        uint7_t deviceId = 0x7f)
    {
        Message result (Manufacturer::universalNonRealtime, payloadSize + offsetOfData);
        const uint7_t header[] = { deviceId,
                                   0x0d,
                                   subtype,
                                   version,
                                   uint7_t (sourceMuid & 0x7f),
                                   uint7_t ((sourceMuid >> 7) & 0x7f),
                                   uint7_t ((sourceMuid >> 14) & 0x7f),
                                   uint7_t ((sourceMuid >> 21) & 0x7f),
                                   uint7_t (destinationMuid & 0x7f),
                                   uint7_t ((destinationMuid >> 7) & 0x7f),
                                   uint7_t ((destinationMuid >> 14) & 0x7f),
                                   uint7_t ((destinationMuid >> 21) & 0x7f) };
        result.addData (header, sizeof (header));
        return result;
    }

private:
    explicit Message (ManufacturerId manufacturer, size_t capacity)
        : UniversalSysEx::Message (manufacturer, capacity)
    {
    }

    struct fieldOffsets
    {
        static constexpr size_t messageVersion = 3u;
        static constexpr size_t sourceMuid = 4u;
        static constexpr size_t destinationMuid = 8u;
    };
};

namespace Subtype
{
constexpr uint7_t discoveryInquiry = 0x70;
constexpr uint7_t discoveryReply = 0x71;
constexpr uint7_t endpointInformationInquiry = 0x72;
constexpr uint7_t endpointInformationReply = 0x73;
constexpr uint7_t ack = 0x7d;
constexpr uint7_t invalidateMuid = 0x7e;
constexpr uint7_t nak = 0x7f;
} // namespace Subtype

namespace Discovery
{
struct MessageView : CapabilityInquiryView
{
    using CapabilityInquiryView::CapabilityInquiryView;

    DeviceIdentity getIdentity() const { return sysex.makeDeviceIdentity (fieldOffsets.identity); }

    uint7_t getCategories() const { return sysex.data[fieldOffsets.categories]; }

    uint28_t getMaximumMessageSize() const { return sysex.makeUInt28 (fieldOffsets.maxMessageSize); }

    uint7_t getOutputPathId() const { return sysex.data[fieldOffsets.outputPathId]; }

    struct fieldOffsets
    {
        static constexpr size_t identity = 12u;
        static constexpr size_t categories = 23u;
        static constexpr size_t maxMessageSize = 24u;
        static constexpr size_t outputPathId = 28u;
    };
};
} // namespace Discovery

struct DiscoveryInquiryView : Discovery::MessageView
{
    using Discovery::MessageView::MessageView;

    static bool validate (const SysEx7& message)
    {
        return isCapabilityInquiryMessage (message)
            && message.data.size() >= (Discovery::MessageView::fieldOffsets.outputPathId + 1)
            && message.data[2] == Subtype::discoveryInquiry;
    }
};

inline Message makeDiscoveryInquiry (Muid sourceMuid,
                                     const DeviceIdentity& identity,
                                     uint7_t categories,
                                     uint28_t maxMessageSize,
                                     uint7_t outputPathId = 0)
{
    auto result = Message::makeWithPayloadSize (17, Subtype::discoveryInquiry, sourceMuid, broadcastMuid);
    result.addDeviceIdentity (identity);
    result.addUInt7 (categories);
    result.addUInt28 (maxMessageSize);
    result.addUInt7 (outputPathId);
    return result;
}

inline Message makeDiscoveryInquiryV1 (Muid sourceMuid,
                                       const DeviceIdentity& identity,
                                       uint7_t categories,
                                       uint28_t maxMessageSize)
{
    return makeDiscoveryInquiry (sourceMuid, identity, categories, maxMessageSize);
}

struct DiscoveryReplyView : Discovery::MessageView
{
    using Discovery::MessageView::MessageView;

    uint7_t getFunctionBlock() const { return sysex.data[fieldOffsets.functionBlock]; }

    static bool validate (const SysEx7& message)
    {
        return isCapabilityInquiryMessage (message)
            && message.data.size() >= (fieldOffsets.functionBlock + 1)
            && message.data[2] == Subtype::discoveryReply;
    }

    struct fieldOffsets
    {
        static constexpr size_t functionBlock = 29u;
    };
};

inline Message makeDiscoveryReply (Muid sourceMuid,
                                   Muid destinationMuid,
                                   const DeviceIdentity& identity,
                                   uint7_t categories,
                                   uint28_t maxMessageSize,
                                   uint7_t outputPathId,
                                   uint7_t functionBlock)
{
    auto result = Message::makeWithPayloadSize (18, Subtype::discoveryReply, sourceMuid, destinationMuid);
    result.addDeviceIdentity (identity);
    result.addUInt7 (categories);
    result.addUInt28 (maxMessageSize);
    result.addUInt7 (outputPathId);
    result.addUInt7 (functionBlock);
    return result;
}

struct EndpointInformationInquiryView : CapabilityInquiryView
{
    using CapabilityInquiryView::CapabilityInquiryView;

    static bool validate (const SysEx7& message)
    {
        return CapabilityInquiryView::validate (message)
            && message.data[2] == Subtype::endpointInformationInquiry;
    }
};

inline Message makeEndpointInformationInquiry (Muid sourceMuid, Muid destinationMuid)
{
    return Message (Subtype::endpointInformationInquiry, sourceMuid, destinationMuid);
}

struct EndpointInformationReplyView : CapabilityInquiryView
{
    using CapabilityInquiryView::CapabilityInquiryView;

    uint7_t getFirstFunctionBlock() const { return sysex.data[fieldOffsets.firstFunctionBlock]; }

    uint7_t getFunctionBlockCount() const { return sysex.data[fieldOffsets.functionBlockCount]; }

    bool hasStaticFunctionBlocks() const { return (sysex.data[fieldOffsets.functionBlockCount] & 0x80u) != 0; }

    uint7_t getProtocol() const { return sysex.data[fieldOffsets.protocol]; }

    uint7_t getExtensions() const { return sysex.data[fieldOffsets.extensions]; }

    static bool validate (const SysEx7& message)
    {
        return CapabilityInquiryView::validate (message)
            && message.data.size() >= (fieldOffsets.extensions + 1)
            && message.data[2] == Subtype::endpointInformationReply;
    }

    struct fieldOffsets
    {
        static constexpr size_t firstFunctionBlock = 12u;
        static constexpr size_t functionBlockCount = 13u;
        static constexpr size_t protocol = 14u;
        static constexpr size_t extensions = 15u;
    };
};

inline Message makeEndpointInformationReply (Muid sourceMuid,
                                             Muid destinationMuid,
                                             uint7_t firstFunctionBlock,
                                             uint7_t functionBlockCount,
                                             bool staticFunctionBlocks,
                                             uint7_t protocol,
                                             uint7_t extensions)
{
    auto result = Message::makeWithPayloadSize (4, Subtype::endpointInformationReply, sourceMuid, destinationMuid);
    result.addUInt7 (firstFunctionBlock);
    result.addUInt7 (uint7_t ((staticFunctionBlocks ? 0x80u : 0x00u) | (functionBlockCount & 0x7f)));
    result.addUInt7 (protocol);
    result.addUInt7 (extensions);
    return result;
}

struct AckView : CapabilityInquiryView
{
    using CapabilityInquiryView::CapabilityInquiryView;

    uint7_t getOriginalSubtype() const { return sysex.data[fieldOffsets.originalSubtype]; }

    uint7_t getStatus() const { return sysex.data[fieldOffsets.status]; }

    static bool validate (const SysEx7& message)
    {
        return CapabilityInquiryView::validate (message)
            && message.data.size() >= (fieldOffsets.status + 1)
            && message.data[2] == Subtype::ack;
    }

    struct fieldOffsets
    {
        static constexpr size_t originalSubtype = 12u;
        static constexpr size_t status = 13u;
    };
};

inline Message makeAck (Muid sourceMuid, Muid destinationMuid, uint7_t originalSubtype, uint7_t status)
{
    auto result = Message::makeWithPayloadSize (2, Subtype::ack, sourceMuid, destinationMuid);
    result.addUInt7 (originalSubtype);
    result.addUInt7 (status);
    return result;
}

struct NakView : CapabilityInquiryView
{
    using CapabilityInquiryView::CapabilityInquiryView;

    uint7_t getOriginalSubtype() const { return sysex.data[fieldOffsets.originalSubtype]; }

    uint7_t getStatus() const { return sysex.data[fieldOffsets.status]; }

    static bool validate (const SysEx7& message)
    {
        return CapabilityInquiryView::validate (message)
            && message.data.size() >= (fieldOffsets.status + 1)
            && message.data[2] == Subtype::nak;
    }

    struct fieldOffsets
    {
        static constexpr size_t originalSubtype = 12u;
        static constexpr size_t status = 13u;
    };
};

inline Message makeNak (Muid sourceMuid, Muid destinationMuid, uint7_t originalSubtype, uint7_t status)
{
    auto result = Message::makeWithPayloadSize (2, Subtype::nak, sourceMuid, destinationMuid);
    result.addUInt7 (originalSubtype);
    result.addUInt7 (status);
    return result;
}

struct InvalidateMuidView : CapabilityInquiryView
{
    using CapabilityInquiryView::CapabilityInquiryView;

    static bool validate (const SysEx7& message)
    {
        return CapabilityInquiryView::validate (message)
            && message.data[2] == Subtype::invalidateMuid;
    }
};

inline Message makeInvalidateMuid (Muid sourceMuid, Muid destinationMuid)
{
    return Message (Subtype::invalidateMuid, sourceMuid, destinationMuid);
}

} // namespace ci

} // namespace yup::ump

#endif
