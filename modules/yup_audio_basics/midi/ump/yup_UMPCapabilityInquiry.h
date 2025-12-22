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

    uint7_t getMessageVersion() const { return sysex.data[fieldOffsets::messageVersion]; }

    Muid getSourceMuid() const { return sysex.makeUInt28 (fieldOffsets::sourceMuid); }

    Muid getDestinationMuid() const { return sysex.makeUInt28 (fieldOffsets::destinationMuid); }

    static bool validate (const SysEx7& message)
    {
        return isCapabilityInquiryMessage (message) && message.data.size() >= fieldOffsets::payload;
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

    uint7_t getMessageVersion() const { return data[fieldOffsets::messageVersion]; }

    Muid getSourceMuid() const { return makeUInt28 (fieldOffsets::sourceMuid); }

    Muid getDestinationMuid() const { return makeUInt28 (fieldOffsets::destinationMuid); }

    void setSourceMuid (Muid muid)
    {
        jassert (muid <= uint28Max);
        data[fieldOffsets::sourceMuid] = uint7_t (muid & 0x7f);
        data[fieldOffsets::sourceMuid + 1] = uint7_t ((muid >> 7) & 0x7f);
        data[fieldOffsets::sourceMuid + 2] = uint7_t ((muid >> 14) & 0x7f);
        data[fieldOffsets::sourceMuid + 3] = uint7_t ((muid >> 21) & 0x7f);
    }

    void setDestinationMuid (Muid muid)
    {
        jassert (muid <= uint28Max);
        data[fieldOffsets::destinationMuid] = uint7_t (muid & 0x7f);
        data[fieldOffsets::destinationMuid + 1] = uint7_t ((muid >> 7) & 0x7f);
        data[fieldOffsets::destinationMuid + 2] = uint7_t ((muid >> 14) & 0x7f);
        data[fieldOffsets::destinationMuid + 3] = uint7_t ((muid >> 21) & 0x7f);
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

    DeviceIdentity getIdentity() const { return sysex.makeDeviceIdentity (fieldOffsets::identity); }

    uint7_t getCategories() const { return sysex.data[fieldOffsets::categories]; }

    uint28_t getMaximumMessageSize() const { return sysex.makeUInt28 (fieldOffsets::maxMessageSize); }

    uint7_t getOutputPathId() const { return sysex.data[fieldOffsets::outputPathId]; }

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
            && message.data.size() >= (Discovery::MessageView::fieldOffsets::outputPathId + 1)
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

    uint7_t getFunctionBlock() const { return sysex.data[fieldOffsets::functionBlock]; }

    static bool validate (const SysEx7& message)
    {
        return isCapabilityInquiryMessage (message)
            && message.data.size() >= (fieldOffsets::functionBlock + 1)
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

    uint7_t getFirstFunctionBlock() const { return sysex.data[fieldOffsets::firstFunctionBlock]; }

    uint7_t getFunctionBlockCount() const { return sysex.data[fieldOffsets::functionBlockCount]; }

    bool hasStaticFunctionBlocks() const { return (sysex.data[fieldOffsets::functionBlockCount] & 0x80u) != 0; }

    uint7_t getProtocol() const { return sysex.data[fieldOffsets::protocol]; }

    uint7_t getExtensions() const { return sysex.data[fieldOffsets::extensions]; }

    static bool validate (const SysEx7& message)
    {
        return CapabilityInquiryView::validate (message)
            && message.data.size() >= (fieldOffsets::extensions + 1)
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

    uint7_t getOriginalSubtype() const { return sysex.data[fieldOffsets::originalSubtype]; }

    uint7_t getStatus() const { return sysex.data[fieldOffsets::status]; }

    static bool validate (const SysEx7& message)
    {
        return CapabilityInquiryView::validate (message)
            && message.data.size() >= (fieldOffsets::status + 1)
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

    uint7_t getOriginalSubtype() const { return sysex.data[fieldOffsets::originalSubtype]; }

    uint7_t getStatus() const { return sysex.data[fieldOffsets::status]; }

    static bool validate (const SysEx7& message)
    {
        return CapabilityInquiryView::validate (message)
            && message.data.size() >= (fieldOffsets::status + 1)
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

//==============================================================================
// Profile Configuration
namespace Subtype
{
constexpr uint7_t profileInquiry = 0x20;
constexpr uint7_t profileInquiryReply = 0x21;
constexpr uint7_t setProfileOn = 0x22;
constexpr uint7_t setProfileOff = 0x23;
constexpr uint7_t profileEnabled = 0x24;
constexpr uint7_t profileDisabled = 0x25;
constexpr uint7_t profileAdded = 0x26;
constexpr uint7_t profileRemoved = 0x27;
constexpr uint7_t profileDetailsInquiry = 0x28;
constexpr uint7_t profileDetailsReply = 0x29;
constexpr uint7_t profileSpecificData = 0x2f;
} // namespace Subtype

struct ProfileId
{
    uint7_t byte1 { 0x7e };
    uint7_t byte2 { 0x00 };
    uint7_t byte3 { 0x00 };
    uint7_t byte4 { 0x00 };
    uint7_t byte5 { 0x00 };

    ProfileId() = default;

    ProfileId (uint7_t b1, uint7_t b2, uint7_t b3, uint7_t b4, uint7_t b5)
        : byte1 (b1)
        , byte2 (b2)
        , byte3 (b3)
        , byte4 (b4)
        , byte5 (b5)
    {
    }
};

inline bool operator== (const ProfileId& a, const ProfileId& b)
{
    return a.byte1 == b.byte1
        && a.byte2 == b.byte2
        && a.byte3 == b.byte3
        && a.byte4 == b.byte4
        && a.byte5 == b.byte5;
}

struct ProfileDestination
{
    uint7_t scope { 0x7f };
    uint14_t numChannels { 0 };

    static constexpr uint7_t group = 0x7e;
    static constexpr uint7_t functionBlock = 0x7f;
};

constexpr ProfileDestination makeChannelProfileDestination (uint7_t firstChannel, uint14_t numChannels = 1)
{
    return ProfileDestination { firstChannel, numChannels };
}

constexpr ProfileDestination groupProfileDestination { ProfileDestination::group, 0 };
constexpr ProfileDestination functionBlockProfileDestination { ProfileDestination::functionBlock, 0 };

struct ProfileInquiryView : CapabilityInquiryView
{
    using CapabilityInquiryView::CapabilityInquiryView;

    static bool validate (const SysEx7& message)
    {
        return isCapabilityInquiryMessage (message)
            && message.data.size() >= CapabilityInquiryView::fieldOffsets::payload
            && message.data[2] == Subtype::profileInquiry;
    }
};

inline Message makeProfileInquiryMessage (Muid sourceMuid, Muid destinationMuid, uint7_t deviceId = 0x7f)
{
    return Message (Subtype::profileInquiry, sourceMuid, destinationMuid, deviceId);
}

struct ProfileInquiryReplyView : CapabilityInquiryView
{
    using CapabilityInquiryView::CapabilityInquiryView;

    bool hasEnabledProfiles() const
    {
        return ((sysex.data[fieldOffsets::numEnabledProfiles]
                 | sysex.data[fieldOffsets::numEnabledProfiles + 1])
                & 0x7f)
            != 0;
    }

    bool hasDisabledProfiles() const
    {
        const auto p = offsetOfDisabledProfiles();
        return ((sysex.data[p] | sysex.data[p + 1]) & 0x7f) != 0;
    }

    uint14_t getNumEnabledProfiles() const { return sysex.makeUInt14 (fieldOffsets::numEnabledProfiles); }

    uint14_t getNumDisabledProfiles() const
    {
        const auto p = offsetOfDisabledProfiles();
        return sysex.makeUInt14 (p);
    }

    std::vector<ProfileId> getEnabledProfiles() const { return makeProfiles (fieldOffsets::numEnabledProfiles); }

    std::vector<ProfileId> getDisabledProfiles() const { return makeProfiles (offsetOfDisabledProfiles()); }

    static bool validate (const SysEx7& message)
    {
        if (! isCapabilityInquiryMessage (message))
            return false;

        size_t expectedSize = fieldOffsets::minimumMessageSize;
        if (message.data.size() < expectedSize)
            return false;

        if (message.data[2] != Subtype::profileInquiryReply)
            return false;

        expectedSize += message.makeUInt14 (fieldOffsets::numEnabledProfiles) * sizeof (ProfileId);
        if (message.data.size() < expectedSize)
            return false;

        expectedSize += message.makeUInt14 (expectedSize - 2) * sizeof (ProfileId);
        return message.data.size() >= expectedSize;
    }

    struct fieldOffsets
    {
        static constexpr size_t numEnabledProfiles = CapabilityInquiryView::fieldOffsets::payload;
        static constexpr size_t numDisabledProfiles = CapabilityInquiryView::fieldOffsets::payload + 2u;
        static constexpr size_t minimumMessageSize = CapabilityInquiryView::fieldOffsets::payload + 4u;
    };

protected:
    size_t offsetOfDisabledProfiles() const
    {
        return fieldOffsets::numDisabledProfiles + getNumEnabledProfiles() * sizeof (ProfileId);
    }

    std::vector<ProfileId> makeProfiles (size_t pos) const
    {
        std::vector<ProfileId> result;

        jassert (sysex.data.size() > pos + 1);
        const auto numProfiles = sysex.makeUInt14 (pos);

        jassert (sysex.data.size() > pos + 1 + numProfiles * sizeof (ProfileId));

        result.reserve (numProfiles);
        const auto* d = sysex.data.data() + pos + 2;
        for (uint14_t i = 0; i < numProfiles; ++i)
        {
            result.push_back ({ d[0], d[1], d[2], d[3], d[4] });
            d += sizeof (ProfileId);
        }

        return result;
    }
};

inline Message makeProfileInquiryReply (Muid sourceMuid,
                                        Muid destinationMuid,
                                        const ProfileId* enabledProfiles,
                                        uint14_t numEnabledProfiles,
                                        const ProfileId* disabledProfiles,
                                        uint14_t numDisabledProfiles,
                                        uint7_t deviceId = 0x7f)
{
    jassert (numEnabledProfiles <= SysEx7::uint14Max);
    jassert (numDisabledProfiles <= SysEx7::uint14Max);

    const size_t payloadSize = ProfileInquiryReplyView::fieldOffsets::minimumMessageSize
                             + (numEnabledProfiles + numDisabledProfiles) * sizeof (ProfileId)
                             - Message::offsetOfData;

    auto result = Message::makeWithPayloadSize (payloadSize,
                                                Subtype::profileInquiryReply,
                                                sourceMuid,
                                                destinationMuid,
                                                deviceId);

    auto addProfile = [&result] (const ProfileId& profile)
    {
        result.addUInt7 (profile.byte1);
        result.addUInt7 (profile.byte2);
        result.addUInt7 (profile.byte3);
        result.addUInt7 (profile.byte4);
        result.addUInt7 (profile.byte5);
    };

    result.addUInt14 (numEnabledProfiles);
    for (uint14_t i = 0; i < numEnabledProfiles; ++i)
        addProfile (enabledProfiles[i]);

    result.addUInt14 (numDisabledProfiles);
    for (uint14_t i = 0; i < numDisabledProfiles; ++i)
        addProfile (disabledProfiles[i]);

    return result;
}

inline Message makeProfileInquiryReply (Muid sourceMuid,
                                        Muid destinationMuid,
                                        const std::vector<ProfileId>& enabledProfiles,
                                        const std::vector<ProfileId>& disabledProfiles,
                                        uint7_t deviceId = 0x7f)
{
    return makeProfileInquiryReply (sourceMuid,
                                    destinationMuid,
                                    enabledProfiles.data(),
                                    uint14_t (enabledProfiles.size()),
                                    disabledProfiles.data(),
                                    uint14_t (disabledProfiles.size()),
                                    deviceId);
}

struct ProfileIdView : CapabilityInquiryView
{
    using CapabilityInquiryView::CapabilityInquiryView;

    ProfileId getProfile() const
    {
        return { sysex.data[fieldOffsets::profileId],
                 sysex.data[fieldOffsets::profileId + 1],
                 sysex.data[fieldOffsets::profileId + 2],
                 sysex.data[fieldOffsets::profileId + 3],
                 sysex.data[fieldOffsets::profileId + 4] };
    }

    static bool validate (const SysEx7& message)
    {
        return (message.manufacturerId == Manufacturer::universalNonRealtime)
            && (message.data.size() >= fieldOffsets::profileId + 5u)
            && (message.data[1] == 0x0d)
            && (message.data[2] >= Subtype::setProfileOn)
            && (message.data[2] <= Subtype::profileRemoved);
    }

    struct fieldOffsets
    {
        static constexpr size_t profileId = CapabilityInquiryView::fieldOffsets::payload;
    };
};

struct ProfileDestinationView : ProfileIdView
{
    using ProfileIdView::ProfileIdView;

    uint14_t getNumChannels() const { return sysex.makeUInt14 (fieldOffsets::numChannels); }

    ProfileDestination getDestination() const { return ProfileDestination { getDeviceId(), getNumChannels() }; }

    static bool validate (const SysEx7& message)
    {
        return (message.manufacturerId == Manufacturer::universalNonRealtime)
            && (message.data.size() >= fieldOffsets::profileId + 7u)
            && (message.data[1] == 0x0d)
            && (message.data[2] >= Subtype::setProfileOn)
            && (message.data[2] <= Subtype::profileDisabled);
    }

    struct fieldOffsets : ProfileIdView::fieldOffsets
    {
        static constexpr size_t numChannels = CapabilityInquiryView::fieldOffsets::payload + 5u;
    };
};

inline Message makeProfileOnRequest (Muid sourceMuid,
                                     Muid destinationMuid,
                                     const ProfileId& profile,
                                     const ProfileDestination& destination)
{
    auto result = Message::makeWithPayloadSize (7,
                                                Subtype::setProfileOn,
                                                sourceMuid,
                                                destinationMuid,
                                                destination.scope);
    result.addUInt7 (profile.byte1);
    result.addUInt7 (profile.byte2);
    result.addUInt7 (profile.byte3);
    result.addUInt7 (profile.byte4);
    result.addUInt7 (profile.byte5);
    result.addUInt14 (destination.numChannels);
    return result;
}

inline Message makeProfileOffRequest (Muid sourceMuid,
                                      Muid destinationMuid,
                                      const ProfileId& profile,
                                      const ProfileDestination& destination)
{
    auto result = Message::makeWithPayloadSize (7,
                                                Subtype::setProfileOff,
                                                sourceMuid,
                                                destinationMuid,
                                                destination.scope);
    result.addUInt7 (profile.byte1);
    result.addUInt7 (profile.byte2);
    result.addUInt7 (profile.byte3);
    result.addUInt7 (profile.byte4);
    result.addUInt7 (profile.byte5);
    result.addUInt14 (0);
    return result;
}

inline Message makeProfileEnabledNotification (Muid sourceMuid,
                                               Muid destinationMuid,
                                               const ProfileId& profile,
                                               const ProfileDestination& destination)
{
    auto result = Message::makeWithPayloadSize (7,
                                                Subtype::profileEnabled,
                                                sourceMuid,
                                                destinationMuid,
                                                destination.scope);
    result.addUInt7 (profile.byte1);
    result.addUInt7 (profile.byte2);
    result.addUInt7 (profile.byte3);
    result.addUInt7 (profile.byte4);
    result.addUInt7 (profile.byte5);
    result.addUInt14 (destination.numChannels);
    return result;
}

inline Message makeProfileDisabledNotification (Muid sourceMuid,
                                                Muid destinationMuid,
                                                const ProfileId& profile,
                                                const ProfileDestination& destination)
{
    auto result = Message::makeWithPayloadSize (7,
                                                Subtype::profileDisabled,
                                                sourceMuid,
                                                destinationMuid,
                                                destination.scope);
    result.addUInt7 (profile.byte1);
    result.addUInt7 (profile.byte2);
    result.addUInt7 (profile.byte3);
    result.addUInt7 (profile.byte4);
    result.addUInt7 (profile.byte5);
    result.addUInt14 (destination.numChannels);
    return result;
}

inline Message makeProfileAddedNotification (Muid sourceMuid,
                                             Muid destinationMuid,
                                             const ProfileId& profile,
                                             uint7_t deviceId = 0x7f)
{
    auto result = Message::makeWithPayloadSize (5,
                                                Subtype::profileAdded,
                                                sourceMuid,
                                                destinationMuid,
                                                deviceId);
    result.addUInt7 (profile.byte1);
    result.addUInt7 (profile.byte2);
    result.addUInt7 (profile.byte3);
    result.addUInt7 (profile.byte4);
    result.addUInt7 (profile.byte5);
    return result;
}

inline Message makeProfileRemovedNotification (Muid sourceMuid,
                                               Muid destinationMuid,
                                               const ProfileId& profile,
                                               uint7_t deviceId = 0x7f)
{
    auto result = Message::makeWithPayloadSize (5,
                                                Subtype::profileRemoved,
                                                sourceMuid,
                                                destinationMuid,
                                                deviceId);
    result.addUInt7 (profile.byte1);
    result.addUInt7 (profile.byte2);
    result.addUInt7 (profile.byte3);
    result.addUInt7 (profile.byte4);
    result.addUInt7 (profile.byte5);
    return result;
}

struct ProfileDetailsInquiryView : CapabilityInquiryView
{
    using CapabilityInquiryView::CapabilityInquiryView;

    ProfileId getProfile() const
    {
        return { sysex.data[fieldOffsets::profileId],
                 sysex.data[fieldOffsets::profileId + 1],
                 sysex.data[fieldOffsets::profileId + 2],
                 sysex.data[fieldOffsets::profileId + 3],
                 sysex.data[fieldOffsets::profileId + 4] };
    }

    uint7_t getTarget() const { return sysex.data[fieldOffsets::target]; }

    static bool validate (const SysEx7& message)
    {
        return (message.manufacturerId == Manufacturer::universalNonRealtime)
            && (message.data.size() >= fieldOffsets::target + 1u)
            && (message.data[1] == 0x0d)
            && (message.data[2] == Subtype::profileDetailsInquiry);
    }

    struct fieldOffsets
    {
        static constexpr size_t profileId = CapabilityInquiryView::fieldOffsets::payload;
        static constexpr size_t target = CapabilityInquiryView::fieldOffsets::payload + 5u;
    };
};

inline Message makeProfileDetailsInquiry (Muid sourceMuid,
                                          Muid destinationMuid,
                                          const ProfileId& profile,
                                          uint7_t target,
                                          uint7_t deviceId = 0x7f)
{
    auto result = Message::makeWithPayloadSize (6,
                                                Subtype::profileDetailsInquiry,
                                                sourceMuid,
                                                destinationMuid,
                                                deviceId);
    result.addUInt7 (profile.byte1);
    result.addUInt7 (profile.byte2);
    result.addUInt7 (profile.byte3);
    result.addUInt7 (profile.byte4);
    result.addUInt7 (profile.byte5);
    result.addUInt7 (target);
    return result;
}

struct ProfileDetailsReplyView : CapabilityInquiryView
{
    using CapabilityInquiryView::CapabilityInquiryView;

    ProfileId getProfile() const
    {
        return { sysex.data[fieldOffsets::profileId],
                 sysex.data[fieldOffsets::profileId + 1],
                 sysex.data[fieldOffsets::profileId + 2],
                 sysex.data[fieldOffsets::profileId + 3],
                 sysex.data[fieldOffsets::profileId + 4] };
    }

    uint7_t getTarget() const { return sysex.data[fieldOffsets::target]; }

    uint14_t getTargetDataLength() const { return sysex.makeUInt14 (fieldOffsets::targetDataLength); }

    const uint7_t* getTargetData() const { return sysex.data.data() + fieldOffsets::targetData; }

    static bool validate (const SysEx7& message)
    {
        return (message.manufacturerId == Manufacturer::universalNonRealtime)
            && (message.data.size() >= fieldOffsets::targetData)
            && (message.data.size() >= fieldOffsets::targetData
                                           + message.makeUInt14 (fieldOffsets::targetDataLength))
            && (message.data[1] == 0x0d)
            && (message.data[2] == Subtype::profileDetailsReply);
    }

    struct fieldOffsets
    {
        static constexpr size_t profileId = CapabilityInquiryView::fieldOffsets::payload;
        static constexpr size_t target = CapabilityInquiryView::fieldOffsets::payload + 5u;
        static constexpr size_t targetDataLength = CapabilityInquiryView::fieldOffsets::payload + 6u;
        static constexpr size_t targetData = CapabilityInquiryView::fieldOffsets::payload + 8u;
    };
};

inline Message makeProfileDetailsReply (Muid sourceMuid,
                                        Muid destinationMuid,
                                        const ProfileId& profile,
                                        uint7_t target,
                                        const uint7_t* data,
                                        size_t dataSize,
                                        uint7_t deviceId = 0x7f)
{
    auto result = Message::makeWithPayloadSize (8 + dataSize,
                                                Subtype::profileDetailsReply,
                                                sourceMuid,
                                                destinationMuid,
                                                deviceId);
    result.addUInt7 (profile.byte1);
    result.addUInt7 (profile.byte2);
    result.addUInt7 (profile.byte3);
    result.addUInt7 (profile.byte4);
    result.addUInt7 (profile.byte5);
    result.addUInt7 (target);
    result.addUInt14 (uint14_t (dataSize));
    if (dataSize != 0 && data != nullptr)
        result.addData (data, dataSize);
    return result;
}

struct ProfileSpecificDataView : CapabilityInquiryView
{
    using CapabilityInquiryView::CapabilityInquiryView;

    size_t getDataSize() const { return sysex.makeUInt28 (fieldOffsets::dataSize); }

    const uint7_t* getDataBegin() const { return sysex.data.data() + fieldOffsets::data; }

    const uint7_t* getDataEnd() const { return getDataBegin() + getDataSize(); }

    static bool validate (const SysEx7& message)
    {
        return (message.manufacturerId == Manufacturer::universalNonRealtime)
            && (message.data.size() >= fieldOffsets::data)
            && (message.data.size() >= fieldOffsets::data + message.makeUInt28 (fieldOffsets::dataSize))
            && (message.data[1] == 0x0d)
            && (message.data[2] == Subtype::profileSpecificData);
    }

    struct fieldOffsets
    {
        static constexpr size_t profileId = CapabilityInquiryView::fieldOffsets::payload;
        static constexpr size_t dataSize = CapabilityInquiryView::fieldOffsets::payload + 5u;
        static constexpr size_t data = CapabilityInquiryView::fieldOffsets::payload + 9u;
    };
};

inline Message makeProfileSpecificDataMessage (Muid sourceMuid,
                                               Muid destinationMuid,
                                               const ProfileId& profile,
                                               const uint7_t* data,
                                               size_t dataSize,
                                               uint7_t deviceId = 0x7f)
{
    auto result = Message::makeWithPayloadSize (9 + dataSize,
                                                Subtype::profileSpecificData,
                                                sourceMuid,
                                                destinationMuid,
                                                deviceId);
    result.addUInt7 (profile.byte1);
    result.addUInt7 (profile.byte2);
    result.addUInt7 (profile.byte3);
    result.addUInt7 (profile.byte4);
    result.addUInt7 (profile.byte5);
    result.addUInt28 (uint28_t (dataSize));
    if (dataSize != 0 && data != nullptr)
        result.addData (data, dataSize);
    return result;
}

inline Message makeProfileSpecificDataMessage (Muid sourceMuid,
                                               Muid destinationMuid,
                                               const ProfileId& profile,
                                               const std::vector<uint7_t>& data,
                                               uint7_t deviceId = 0x7f)
{
    return makeProfileSpecificDataMessage (sourceMuid, destinationMuid, profile, data.data(), data.size(), deviceId);
}

//==============================================================================
// Property Exchange
namespace Subtype
{
constexpr uint7_t propertyExchangeCapabilitiesInquiry = 0x30;
constexpr uint7_t propertyExchangeCapabilitiesReply = 0x31;
constexpr uint7_t getPropertyDataInquiry = 0x34;
constexpr uint7_t getPropertyDataReply = 0x35;
constexpr uint7_t setPropertyDataInquiry = 0x36;
constexpr uint7_t setPropertyDataReply = 0x37;
constexpr uint7_t subscriptionInquiry = 0x38;
constexpr uint7_t subscriptionReply = 0x39;
constexpr uint7_t notify = 0x3f;
} // namespace Subtype

namespace propertyExchange
{
constexpr uint7_t versionMajor = 0x00;
constexpr uint7_t versionMinor = 0x00;

struct HeaderOptions
{
    std::vector<std::pair<std::string_view, std::string_view>> options;
};

struct Tags
{
    static constexpr std::string_view resource { "resource" };
    static constexpr std::string_view command { "command" };
    static constexpr std::string_view status { "status" };
    static constexpr std::string_view id { "id" };
    static constexpr std::string_view offset { "offset" };
    static constexpr std::string_view limit { "limit" };
    static constexpr std::string_view encoding { "encoding" };
    static constexpr std::string_view message { "message" };
    static constexpr std::string_view subscribeId { "subscribeId" };
};

inline std::string makeRjson (std::string_view key, std::string_view value)
{
    return std::string { "{\"" } + std::string { key } + "\":\"" + std::string { value } + "\"}";
}

inline std::string makeRjson (std::string_view key, int value)
{
    return std::string { "{\"" } + std::string { key } + "\":" + std::to_string (value) + "}";
}

namespace detail
{
inline bool isNumber (std::string_view s)
{
    if (s.empty())
        return false;

    if (s[0] == '-')
        s.remove_prefix (1);

    while (! s.empty())
    {
        if ((s[0] < '0') || (s[0] > '9'))
            return false;
        s.remove_prefix (1);
    }

    return true;
}
} // namespace detail

inline std::string makeRjson (std::string_view key, std::string_view value, const HeaderOptions& options)
{
    auto result = std::string { "{\"" } + std::string { key } + "\":\"" + std::string { value } + "\"";
    for (const auto& o : options.options)
    {
        if (detail::isNumber (o.second))
            result += ",\"" + std::string { o.first } + "\":" + std::string { o.second };
        else
            result += ",\"" + std::string { o.first } + "\":\"" + std::string { o.second } + "\"";
    }
    result.push_back ('}');
    return result;
}

inline std::string makeRjson (std::string_view key, int value, const HeaderOptions& options)
{
    auto result = std::string { "{\"" } + std::string { key } + "\":" + std::to_string (value);
    for (const auto& o : options.options)
    {
        if (detail::isNumber (o.second))
            result += ",\"" + std::string { o.first } + "\":" + std::string { o.second };
        else
            result += ",\"" + std::string { o.first } + "\":\"" + std::string { o.second } + "\"";
    }
    result.push_back ('}');
    return result;
}

struct PrivateDataView
{
    const uint7_t* data { nullptr };
    size_t size { 0 };

    PrivateDataView() = default;

    PrivateDataView (const uint7_t* d, size_t s)
        : data (d)
        , size (s)
    {
    }

    explicit PrivateDataView (const std::vector<uint7_t>& d)
        : data (d.data())
        , size (d.size())
    {
    }

    explicit PrivateDataView (std::string_view d)
        : data (reinterpret_cast<const uint7_t*> (d.data()))
        , size (d.length())
    {
    }

    std::string_view asStringView() const
    {
        return std::string_view (reinterpret_cast<const char*> (data), size);
    }
};

struct Header : PrivateDataView
{
    using PrivateDataView::PrivateDataView;
};

struct Chunk : PrivateDataView
{
    using PrivateDataView::PrivateDataView;
};

inline Message makeCapabilitiesMessage (uint7_t subtype,
                                        Muid sourceMuid,
                                        Muid destinationMuid,
                                        uint7_t maxNumRequests,
                                        uint7_t deviceId)
{
    auto result = Message::makeWithPayloadSize (3, subtype, sourceMuid, destinationMuid, deviceId);
    const uint7_t data[] = { maxNumRequests, versionMajor, versionMinor };
    result.addData (data, sizeof (data));
    return result;
}

inline Message makePropertyDataMessage (uint7_t subtype,
                                        Muid sourceMuid,
                                        Muid destinationMuid,
                                        const Header& header,
                                        uint14_t numberOfChunks,
                                        uint14_t numberOfThisChunk,
                                        const Chunk& chunk,
                                        uint7_t requestId = 0,
                                        uint7_t deviceId = 0x7f)
{
    auto result = Message::makeWithPayloadSize (9 + header.size + chunk.size,
                                                subtype,
                                                sourceMuid,
                                                destinationMuid,
                                                deviceId);

    jassert (numberOfThisChunk || ((chunk.data == nullptr) && (chunk.size == 0)));
    jassert ((numberOfChunks == 0) || (numberOfThisChunk <= numberOfChunks));
    jassert (! header.size || header.data);
    jassert (! chunk.size || chunk.data);

    result.addUInt7 (requestId);
    result.addUInt14 (uint14_t (header.size));
    if (header.size)
        result.addData (header.data, header.size);

    result.addUInt14 (numberOfChunks);
    result.addUInt14 (numberOfThisChunk);

    result.addUInt14 (uint14_t (chunk.size));
    if (chunk.size)
        result.addData (chunk.data, chunk.size);

    return result;
}

struct PropertyDataMessageView : CapabilityInquiryView
{
    using CapabilityInquiryView::CapabilityInquiryView;

    uint7_t getRequestId() const { return sysex.data[fieldOffsets::requestId]; }

    const uint7_t* getHeaderBegin() const { return sysex.data.data() + fieldOffsets::headerData; }

    const uint7_t* getHeaderEnd() const { return getHeaderBegin() + getHeaderSize(); }

    size_t getHeaderSize() const { return sysex.makeUInt14 (fieldOffsets::headerSize); }

    uint14_t getNumberOfChunks() const { return sysex.makeUInt14 (fieldOffsets::numChunks + getHeaderSize()); }

    uint14_t getNumberOfThisChunk() const { return sysex.makeUInt14 (fieldOffsets::thisChunk + getHeaderSize()); }

    const uint7_t* getChunkBegin() const { return sysex.data.data() + fieldOffsets::chunkData + getHeaderSize(); }

    const uint7_t* getChunkEnd() const { return getChunkBegin() + getChunkSize(); }

    size_t getChunkSize() const { return sysex.makeUInt14 (fieldOffsets::chunkSize + getHeaderSize()); }

    static bool validate (const SysEx7& message)
    {
        constexpr size_t minMessageSize = fieldOffsets::chunkData;

        if ((message.manufacturerId != Manufacturer::universalNonRealtime)
            || (message.data.size() < minMessageSize)
            || (message.data[1] != 0x0d)
            || (message.data[2] < Subtype::getPropertyDataInquiry))
            return false;

        if ((message.data[2] <= Subtype::subscriptionReply) || (message.data[2] == Subtype::notify))
        {
            const auto headerBytes = message.makeUInt14 (fieldOffsets::headerSize);
            if (message.data.size() < size_t (fieldOffsets::headerData + headerBytes + 6))
                return false;

            const auto numChunks = message.makeUInt14 (fieldOffsets::numChunks + headerBytes);
            const auto curChunk = message.makeUInt14 (fieldOffsets::thisChunk + headerBytes);
            const auto chunkBytes = message.makeUInt14 (fieldOffsets::chunkSize + headerBytes);

            if (numChunks && (curChunk > numChunks))
                return false;

            if (headerBytes && (curChunk > 1))
                return false;
            if (chunkBytes && (curChunk < 1))
                return false;

            return (minMessageSize + headerBytes + chunkBytes <= message.data.size());
        }

        return false;
    }

    struct fieldOffsets
    {
        static constexpr size_t requestId = CapabilityInquiryView::fieldOffsets::payload;
        static constexpr size_t headerSize = CapabilityInquiryView::fieldOffsets::payload + 1u;
        static constexpr size_t headerData = CapabilityInquiryView::fieldOffsets::payload + 3u;
        static constexpr size_t numChunks = CapabilityInquiryView::fieldOffsets::payload + 3u;
        static constexpr size_t thisChunk = CapabilityInquiryView::fieldOffsets::payload + 5u;
        static constexpr size_t chunkSize = CapabilityInquiryView::fieldOffsets::payload + 7u;
        static constexpr size_t chunkData = CapabilityInquiryView::fieldOffsets::payload + 9u;
    };
};

} // namespace propertyExchange

struct PropertyExchangeCapabilitiesView : CapabilityInquiryView
{
    using CapabilityInquiryView::CapabilityInquiryView;

    uint7_t getMaximumNumberOfRequests() const { return sysex.data[fieldOffsets::maxNumRequests]; }

    uint7_t getMajorVersion() const
    {
        return (getMessageVersion() >= messageVersion2) ? sysex.data[fieldOffsets::versionMajor] : 0;
    }

    uint7_t getMinorVersion() const
    {
        return (getMessageVersion() >= messageVersion2) ? sysex.data[fieldOffsets::versionMinor] : 0;
    }

    static bool validate (const SysEx7& message)
    {
        return isCapabilityInquiryMessage (message)
            && message.data.size() >= (fieldOffsets::maxNumRequests + 1u)
            && (message.data[2] >= Subtype::propertyExchangeCapabilitiesInquiry)
            && (message.data[2] <= Subtype::propertyExchangeCapabilitiesReply)
            && ((message.data[3] < messageVersion2) || (message.data.size() >= fieldOffsets::versionMinor + 1u));
    }

    struct fieldOffsets
    {
        static constexpr size_t maxNumRequests = CapabilityInquiryView::fieldOffsets::payload;
        static constexpr size_t versionMajor = CapabilityInquiryView::fieldOffsets::payload + 1u;
        static constexpr size_t versionMinor = CapabilityInquiryView::fieldOffsets::payload + 2u;
    };
};

inline Message makePropertyExchangeCapabilitiesInquiry (Muid sourceMuid,
                                                        Muid destinationMuid,
                                                        uint7_t maxNumRequests = 1,
                                                        uint7_t deviceId = 0x7f)
{
    return propertyExchange::makeCapabilitiesMessage (Subtype::propertyExchangeCapabilitiesInquiry,
                                                      sourceMuid,
                                                      destinationMuid,
                                                      maxNumRequests,
                                                      deviceId);
}

inline Message makePropertyExchangeCapabilitiesReply (Muid sourceMuid,
                                                      Muid destinationMuid,
                                                      uint7_t maxNumRequests = 1,
                                                      uint7_t deviceId = 0x7f)
{
    return propertyExchange::makeCapabilitiesMessage (Subtype::propertyExchangeCapabilitiesReply,
                                                      sourceMuid,
                                                      destinationMuid,
                                                      maxNumRequests,
                                                      deviceId);
}

struct GetPropertyDataView : propertyExchange::PropertyDataMessageView
{
    using propertyExchange::PropertyDataMessageView::PropertyDataMessageView;

    static bool validate (const SysEx7& message)
    {
        return propertyExchange::PropertyDataMessageView::validate (message)
            && (message.data[2] == Subtype::getPropertyDataInquiry);
    }
};

inline Message makeGetPropertyDataInquiry (Muid sourceMuid,
                                           Muid destinationMuid,
                                           std::string_view resource,
                                           uint7_t requestId = 0,
                                           uint7_t deviceId = 0x7f)
{
    propertyExchange::Header header { propertyExchange::makeRjson (propertyExchange::Tags::resource, resource) };
    return propertyExchange::makePropertyDataMessage (Subtype::getPropertyDataInquiry,
                                                      sourceMuid,
                                                      destinationMuid,
                                                      header,
                                                      0,
                                                      0,
                                                      {},
                                                      requestId,
                                                      deviceId);
}

inline Message makeGetPropertyDataInquiry (Muid sourceMuid,
                                           Muid destinationMuid,
                                           std::string_view resource,
                                           const propertyExchange::HeaderOptions& options,
                                           uint7_t requestId = 0,
                                           uint7_t deviceId = 0x7f)
{
    propertyExchange::Header header { propertyExchange::makeRjson (propertyExchange::Tags::resource,
                                                                   resource,
                                                                   options) };
    return propertyExchange::makePropertyDataMessage (Subtype::getPropertyDataInquiry,
                                                      sourceMuid,
                                                      destinationMuid,
                                                      header,
                                                      0,
                                                      0,
                                                      {},
                                                      requestId,
                                                      deviceId);
}

inline Message makeGetPropertyDataReply (Muid sourceMuid,
                                         Muid destinationMuid,
                                         const propertyExchange::Header& header,
                                         uint14_t numberOfChunks,
                                         uint14_t numberOfThisChunk,
                                         const propertyExchange::Chunk& chunk,
                                         uint7_t requestId = 0,
                                         uint7_t deviceId = 0x7f)
{
    return propertyExchange::makePropertyDataMessage (Subtype::getPropertyDataReply,
                                                      sourceMuid,
                                                      destinationMuid,
                                                      header,
                                                      numberOfChunks,
                                                      numberOfThisChunk,
                                                      chunk,
                                                      requestId,
                                                      deviceId);
}

inline Message makeGetPropertyDataReply (Muid sourceMuid,
                                         Muid destinationMuid,
                                         int status,
                                         uint14_t numberOfChunks,
                                         uint14_t numberOfThisChunk,
                                         const propertyExchange::Chunk& chunk,
                                         uint7_t requestId = 0,
                                         uint7_t deviceId = 0x7f)
{
    propertyExchange::Header header { propertyExchange::makeRjson (propertyExchange::Tags::status, status) };
    return makeGetPropertyDataReply (sourceMuid,
                                     destinationMuid,
                                     header,
                                     numberOfChunks,
                                     numberOfThisChunk,
                                     chunk,
                                     requestId,
                                     deviceId);
}

inline Message makeGetPropertyDataReply (Muid sourceMuid,
                                         Muid destinationMuid,
                                         int status,
                                         std::string_view message,
                                         uint14_t numberOfChunks,
                                         uint14_t numberOfThisChunk,
                                         const propertyExchange::Chunk& chunk,
                                         uint7_t requestId = 0,
                                         uint7_t deviceId = 0x7f)
{
    propertyExchange::HeaderOptions options;
    options.options.emplace_back (propertyExchange::Tags::message, message);
    propertyExchange::Header header { propertyExchange::makeRjson (propertyExchange::Tags::status,
                                                                   status,
                                                                   options) };
    return makeGetPropertyDataReply (sourceMuid,
                                     destinationMuid,
                                     header,
                                     numberOfChunks,
                                     numberOfThisChunk,
                                     chunk,
                                     requestId,
                                     deviceId);
}

inline Message makeGetPropertyDataReply (Muid sourceMuid,
                                         Muid destinationMuid,
                                         uint14_t numberOfChunks,
                                         uint14_t numberOfThisChunk,
                                         const propertyExchange::Chunk& chunk,
                                         uint7_t requestId = 0,
                                         uint7_t deviceId = 0x7f)
{
    propertyExchange::Header header;
    return makeGetPropertyDataReply (sourceMuid,
                                     destinationMuid,
                                     header,
                                     numberOfChunks,
                                     numberOfThisChunk,
                                     chunk,
                                     requestId,
                                     deviceId);
}

struct SetPropertyDataView : propertyExchange::PropertyDataMessageView
{
    using propertyExchange::PropertyDataMessageView::PropertyDataMessageView;

    static bool validate (const SysEx7& message)
    {
        return propertyExchange::PropertyDataMessageView::validate (message)
            && (message.data[2] == Subtype::setPropertyDataInquiry);
    }
};

inline Message makeSetPropertyDataInquiry (Muid sourceMuid,
                                           Muid destinationMuid,
                                           std::string_view resource,
                                           uint14_t numberOfChunks,
                                           uint14_t numberOfThisChunk,
                                           const propertyExchange::Chunk& chunk,
                                           uint7_t requestId = 0,
                                           uint7_t deviceId = 0x7f)
{
    propertyExchange::Header header { propertyExchange::makeRjson (propertyExchange::Tags::resource, resource) };
    return propertyExchange::makePropertyDataMessage (Subtype::setPropertyDataInquiry,
                                                      sourceMuid,
                                                      destinationMuid,
                                                      header,
                                                      numberOfChunks,
                                                      numberOfThisChunk,
                                                      chunk,
                                                      requestId,
                                                      deviceId);
}

inline Message makeSetPropertyDataInquiry (Muid sourceMuid,
                                           Muid destinationMuid,
                                           std::string_view resource,
                                           const propertyExchange::HeaderOptions& options,
                                           uint14_t numberOfChunks,
                                           uint14_t numberOfThisChunk,
                                           const propertyExchange::Chunk& chunk,
                                           uint7_t requestId = 0,
                                           uint7_t deviceId = 0x7f)
{
    propertyExchange::Header header { propertyExchange::makeRjson (propertyExchange::Tags::resource,
                                                                   resource,
                                                                   options) };
    return propertyExchange::makePropertyDataMessage (Subtype::setPropertyDataInquiry,
                                                      sourceMuid,
                                                      destinationMuid,
                                                      header,
                                                      numberOfChunks,
                                                      numberOfThisChunk,
                                                      chunk,
                                                      requestId,
                                                      deviceId);
}

inline Message makeSetPropertyDataInquiry (Muid sourceMuid,
                                           Muid destinationMuid,
                                           uint14_t numberOfChunks,
                                           uint14_t numberOfThisChunk,
                                           const propertyExchange::Chunk& chunk,
                                           uint7_t requestId = 0,
                                           uint7_t deviceId = 0x7f)
{
    propertyExchange::Header header;
    return propertyExchange::makePropertyDataMessage (Subtype::setPropertyDataInquiry,
                                                      sourceMuid,
                                                      destinationMuid,
                                                      header,
                                                      numberOfChunks,
                                                      numberOfThisChunk,
                                                      chunk,
                                                      requestId,
                                                      deviceId);
}

inline Message makeSetPropertyDataReply (Muid sourceMuid,
                                         Muid destinationMuid,
                                         int status,
                                         uint7_t requestId = 0,
                                         uint7_t deviceId = 0x7f)
{
    propertyExchange::Header header { propertyExchange::makeRjson (propertyExchange::Tags::status, status) };
    return propertyExchange::makePropertyDataMessage (Subtype::setPropertyDataReply,
                                                      sourceMuid,
                                                      destinationMuid,
                                                      header,
                                                      0,
                                                      0,
                                                      {},
                                                      requestId,
                                                      deviceId);
}

inline Message makeSetPropertyDataReply (Muid sourceMuid,
                                         Muid destinationMuid,
                                         int status,
                                         std::string_view message,
                                         uint7_t requestId = 0,
                                         uint7_t deviceId = 0x7f)
{
    propertyExchange::HeaderOptions options;
    options.options.emplace_back (propertyExchange::Tags::message, message);
    propertyExchange::Header header { propertyExchange::makeRjson (propertyExchange::Tags::status,
                                                                   status,
                                                                   options) };
    return propertyExchange::makePropertyDataMessage (Subtype::setPropertyDataReply,
                                                      sourceMuid,
                                                      destinationMuid,
                                                      header,
                                                      0,
                                                      0,
                                                      {},
                                                      requestId,
                                                      deviceId);
}

struct SubscriptionView : propertyExchange::PropertyDataMessageView
{
    using propertyExchange::PropertyDataMessageView::PropertyDataMessageView;

    static bool validate (const SysEx7& message)
    {
        return propertyExchange::PropertyDataMessageView::validate (message)
            && (message.data[2] == Subtype::subscriptionInquiry);
    }
};

inline Message makeSubscriptionInquiry (Muid sourceMuid,
                                        Muid destinationMuid,
                                        std::string_view resource,
                                        std::string_view command,
                                        std::string_view subscribeId,
                                        uint7_t requestId = 0,
                                        uint7_t deviceId = 0x7f)
{
    propertyExchange::HeaderOptions options;
    options.options.emplace_back (propertyExchange::Tags::command, command);
    options.options.emplace_back (propertyExchange::Tags::subscribeId, subscribeId);
    propertyExchange::Header header { propertyExchange::makeRjson (propertyExchange::Tags::resource,
                                                                   resource,
                                                                   options) };
    return propertyExchange::makePropertyDataMessage (Subtype::subscriptionInquiry,
                                                      sourceMuid,
                                                      destinationMuid,
                                                      header,
                                                      0,
                                                      0,
                                                      {},
                                                      requestId,
                                                      deviceId);
}

inline Message makeSubscriptionInquiry (Muid sourceMuid,
                                        Muid destinationMuid,
                                        std::string_view resource,
                                        std::string_view command,
                                        std::string_view subscribeId,
                                        uint14_t numberOfChunks,
                                        uint14_t numberOfThisChunk,
                                        const propertyExchange::Chunk& chunk,
                                        uint7_t requestId = 0,
                                        uint7_t deviceId = 0x7f)
{
    propertyExchange::HeaderOptions options;
    options.options.emplace_back (propertyExchange::Tags::command, command);
    options.options.emplace_back (propertyExchange::Tags::subscribeId, subscribeId);
    propertyExchange::Header header { propertyExchange::makeRjson (propertyExchange::Tags::resource,
                                                                   resource,
                                                                   options) };
    return propertyExchange::makePropertyDataMessage (Subtype::subscriptionInquiry,
                                                      sourceMuid,
                                                      destinationMuid,
                                                      header,
                                                      numberOfChunks,
                                                      numberOfThisChunk,
                                                      chunk,
                                                      requestId,
                                                      deviceId);
}

inline Message makeSubscriptionInquiry (Muid sourceMuid,
                                        Muid destinationMuid,
                                        uint14_t numberOfChunks,
                                        uint14_t numberOfThisChunk,
                                        const propertyExchange::Chunk& chunk,
                                        uint7_t requestId = 0,
                                        uint7_t deviceId = 0x7f)
{
    propertyExchange::Header header;
    return propertyExchange::makePropertyDataMessage (Subtype::subscriptionInquiry,
                                                      sourceMuid,
                                                      destinationMuid,
                                                      header,
                                                      numberOfChunks,
                                                      numberOfThisChunk,
                                                      chunk,
                                                      requestId,
                                                      deviceId);
}

inline Message makeSubscriptionReply (Muid sourceMuid,
                                      Muid destinationMuid,
                                      int status,
                                      uint7_t requestId = 0,
                                      uint7_t deviceId = 0x7f)
{
    propertyExchange::Header header { propertyExchange::makeRjson (propertyExchange::Tags::status, status) };
    return propertyExchange::makePropertyDataMessage (Subtype::subscriptionReply,
                                                      sourceMuid,
                                                      destinationMuid,
                                                      header,
                                                      0,
                                                      0,
                                                      {},
                                                      requestId,
                                                      deviceId);
}

inline Message makeSubscriptionReply (Muid sourceMuid,
                                      Muid destinationMuid,
                                      int status,
                                      std::string_view message,
                                      uint7_t requestId = 0,
                                      uint7_t deviceId = 0x7f)
{
    propertyExchange::HeaderOptions options;
    options.options.emplace_back (propertyExchange::Tags::message, message);
    propertyExchange::Header header { propertyExchange::makeRjson (propertyExchange::Tags::status,
                                                                   status,
                                                                   options) };
    return propertyExchange::makePropertyDataMessage (Subtype::subscriptionReply,
                                                      sourceMuid,
                                                      destinationMuid,
                                                      header,
                                                      0,
                                                      0,
                                                      {},
                                                      requestId,
                                                      deviceId);
}

struct NotifyView : propertyExchange::PropertyDataMessageView
{
    using propertyExchange::PropertyDataMessageView::PropertyDataMessageView;

    static bool validate (const SysEx7& message)
    {
        return propertyExchange::PropertyDataMessageView::validate (message)
            && (message.data[2] == Subtype::notify);
    }
};

inline Message makeNotifyMessage (Muid sourceMuid,
                                  Muid destinationMuid,
                                  int status,
                                  uint7_t requestId = 0,
                                  uint7_t deviceId = 0x7f)
{
    propertyExchange::Header header { propertyExchange::makeRjson (propertyExchange::Tags::status, status) };
    return propertyExchange::makePropertyDataMessage (Subtype::notify,
                                                      sourceMuid,
                                                      destinationMuid,
                                                      header,
                                                      0,
                                                      0,
                                                      {},
                                                      requestId,
                                                      deviceId);
}

inline Message makeNotifyMessage (Muid sourceMuid,
                                  Muid destinationMuid,
                                  int status,
                                  std::string_view message,
                                  uint7_t requestId = 0,
                                  uint7_t deviceId = 0x7f)
{
    propertyExchange::HeaderOptions options;
    options.options.emplace_back (propertyExchange::Tags::message, message);
    propertyExchange::Header header { propertyExchange::makeRjson (propertyExchange::Tags::status,
                                                                   status,
                                                                   options) };
    return propertyExchange::makePropertyDataMessage (Subtype::notify,
                                                      sourceMuid,
                                                      destinationMuid,
                                                      header,
                                                      0,
                                                      0,
                                                      {},
                                                      requestId,
                                                      deviceId);
}

//==============================================================================
// Process Inquiry
namespace Subtype
{
constexpr uint7_t processInquiryCapabilitiesInquiry = 0x40;
constexpr uint7_t processInquiryCapabilitiesReply = 0x41;
constexpr uint7_t midiMessageReportInquiry = 0x42;
constexpr uint7_t midiMessageReportReply = 0x43;
constexpr uint7_t midiMessageReportEnd = 0x44;
} // namespace Subtype

inline Message makeProcessInquiryCapabilitiesInquiry (Muid sourceMuid,
                                                      Muid destinationMuid,
                                                      uint7_t deviceId = 0x7f)
{
    return Message (Subtype::processInquiryCapabilitiesInquiry, sourceMuid, destinationMuid, deviceId);
}

struct ProcessInquiryCapabilitiesReplyView : CapabilityInquiryView
{
    using CapabilityInquiryView::CapabilityInquiryView;

    uint7_t getSupportedFeatures() const { return sysex.data[fieldOffsets::supportedFeatures]; }

    static bool validate (const SysEx7& message)
    {
        return isCapabilityInquiryMessage (message)
            && message.data.size() >= (fieldOffsets::supportedFeatures + 1)
            && message.data[2] == Subtype::processInquiryCapabilitiesReply;
    }

    struct fieldOffsets
    {
        static constexpr size_t supportedFeatures = CapabilityInquiryView::fieldOffsets::payload;
    };
};

inline Message makeProcessInquiryCapabilitiesReply (Muid sourceMuid,
                                                    Muid destinationMuid,
                                                    uint7_t features,
                                                    uint7_t deviceId = 0x7f)
{
    auto result = Message::makeWithPayloadSize (1,
                                                Subtype::processInquiryCapabilitiesReply,
                                                sourceMuid,
                                                destinationMuid,
                                                deviceId);
    result.addUInt7 (features);
    return result;
}

struct MidiMessageReportInquiryView : CapabilityInquiryView
{
    using CapabilityInquiryView::CapabilityInquiryView;

    uint7_t getMessageDataControl() const { return sysex.data[fieldOffsets::messageDataControl]; }

    uint7_t getSystemMessageTypes() const { return sysex.data[fieldOffsets::systemMessageTypes]; }

    uint7_t getChannelControllerMessageTypes() const { return sysex.data[fieldOffsets::channelControllerMessageTypes]; }

    uint7_t getNoteDataMessageTypes() const { return sysex.data[fieldOffsets::noteDataMessageTypes]; }

    static bool validate (const SysEx7& message)
    {
        return isCapabilityInquiryMessage (message)
            && message.data.size() >= (fieldOffsets::noteDataMessageTypes + 1)
            && message.data[2] == Subtype::midiMessageReportInquiry;
    }

    struct fieldOffsets
    {
        static constexpr size_t messageDataControl = CapabilityInquiryView::fieldOffsets::payload;
        static constexpr size_t systemMessageTypes = CapabilityInquiryView::fieldOffsets::payload + 1u;
        static constexpr size_t channelControllerMessageTypes = CapabilityInquiryView::fieldOffsets::payload + 2u;
        static constexpr size_t noteDataMessageTypes = CapabilityInquiryView::fieldOffsets::payload + 3u;
    };
};

inline Message makeMidiMessageReportInquiry (Muid sourceMuid,
                                             Muid destinationMuid,
                                             uint7_t dataControl,
                                             uint7_t systemMessages,
                                             uint7_t channelControllerMessages,
                                             uint7_t noteDataMessages,
                                             uint7_t deviceId = 0x7f)
{
    auto result = Message::makeWithPayloadSize (4,
                                                Subtype::midiMessageReportInquiry,
                                                sourceMuid,
                                                destinationMuid,
                                                deviceId);
    result.addUInt7 (dataControl);
    result.addUInt7 (systemMessages);
    result.addUInt7 (channelControllerMessages);
    result.addUInt7 (noteDataMessages);
    return result;
}

struct MidiMessageReportReplyView : CapabilityInquiryView
{
    using CapabilityInquiryView::CapabilityInquiryView;

    uint7_t getSystemMessageTypes() const { return sysex.data[fieldOffsets::systemMessageTypes]; }

    uint7_t getChannelControllerMessageTypes() const { return sysex.data[fieldOffsets::channelControllerMessageTypes]; }

    uint7_t getNoteDataMessageTypes() const { return sysex.data[fieldOffsets::noteDataMessageTypes]; }

    static bool validate (const SysEx7& message)
    {
        return isCapabilityInquiryMessage (message)
            && message.data.size() >= (fieldOffsets::noteDataMessageTypes + 1)
            && message.data[2] == Subtype::midiMessageReportReply;
    }

    struct fieldOffsets
    {
        static constexpr size_t systemMessageTypes = CapabilityInquiryView::fieldOffsets::payload;
        static constexpr size_t channelControllerMessageTypes = CapabilityInquiryView::fieldOffsets::payload + 1u;
        static constexpr size_t noteDataMessageTypes = CapabilityInquiryView::fieldOffsets::payload + 2u;
    };
};

inline Message makeMidiMessageReportReply (Muid sourceMuid,
                                           uint7_t systemMessages,
                                           uint7_t channelControllerMessages,
                                           uint7_t noteDataMessages,
                                           uint7_t deviceId = 0x7f)
{
    auto result = Message::makeWithPayloadSize (3,
                                                Subtype::midiMessageReportReply,
                                                sourceMuid,
                                                broadcastMuid,
                                                deviceId);
    result.addUInt7 (systemMessages);
    result.addUInt7 (channelControllerMessages);
    result.addUInt7 (noteDataMessages);
    return result;
}

inline Message makeMidiMessageReportEnd (Muid sourceMuid, uint7_t deviceId = 0x7f)
{
    return Message (Subtype::midiMessageReportEnd, sourceMuid, broadcastMuid, deviceId);
}

} // namespace ci

} // namespace yup::ump

#endif
