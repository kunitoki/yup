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

inline bool isUniversalSysExMessage (const SysEx7& message);

namespace UniversalSysEx
{

using Type = uint16_t;
using Subtype = uint7_t;

namespace TypeId
{
constexpr Type sampleDumpHeader = 0x7E01;
constexpr Type sampleDataPacket = 0x7E02;
constexpr Type sampleDumpRequest = 0x7E03;
constexpr Type midiTimeCodeNonRealtime = 0x7E04;
constexpr Type sampleDumpExtensions = 0x7E05;
constexpr Type generalInformation = 0x7E06;
constexpr Type fileDump = 0x7E07;
constexpr Type midiTuningNonRealtime = 0x7E08;
constexpr Type generalMidi = 0x7E09;
constexpr Type downloadableSounds = 0x7E0A;
constexpr Type fileReferenceMessage = 0x7E0B;
constexpr Type midiVisualControl = 0x7E0C;
constexpr Type capabilityInquiry = 0x7E0D;
constexpr Type endOfFile = 0x7E7B;
constexpr Type wait = 0x7E7C;
constexpr Type cancel = 0x7E7D;
constexpr Type nak = 0x7E7E;
constexpr Type ack = 0x7E7F;

constexpr Type midiTimeCodeRealtime = 0x7F01;
constexpr Type midiShowControl = 0x7F02;
constexpr Type notationInformation = 0x7F03;
constexpr Type deviceControl = 0x7F04;
constexpr Type realtimeMtcCueing = 0x7F05;
constexpr Type midiMachineControlCommands = 0x7F06;
constexpr Type midiMachineControlResponses = 0x7F07;
constexpr Type midiTuningRealtime = 0x7F08;
constexpr Type controllerDestinationSetting = 0x7F09;

constexpr Type none = 0x0000;
} // namespace TypeId

namespace SubtypeId
{
constexpr Subtype identityRequest = 0x01;
constexpr Subtype identityReply = 0x02;

constexpr Subtype gm1SystemOn = 0x01;
constexpr Subtype gmSystemOff = 0x02;
constexpr Subtype gm2SystemOn = 0x03;

constexpr Subtype mtcFullMessage = 0x01;
constexpr Subtype mtcUserBits = 0x02;
} // namespace SubtypeId

struct MessageView
{
    explicit MessageView (const SysEx7& message)
        : sysex (message)
    {
        jassert (sysex.manufacturerId == Manufacturer::universalRealtime
                 || sysex.manufacturerId == Manufacturer::universalNonRealtime);
        jassert (sysex.data.size() >= 2);
    }

    uint7_t getDeviceId() const { return sysex.data[0]; }

    Type getType() const { return Type ((sysex.manufacturerId >> 8) | sysex.data[1]); }

    Subtype getSubtype() const { return sysex.data.size() > 2 ? sysex.data[2] : 0; }

    size_t getDataSize() const { return sysex.data.size(); }

    template <typename ViewClass>
    std::optional<ViewClass> as() const
    {
        if (ViewClass::validate (sysex))
            return ViewClass { sysex };

        return std::nullopt;
    }

    template <typename ViewClass>
    static std::optional<ViewClass> makeOptional (const SysEx7& message)
    {
        if (ViewClass::validate (message))
            return ViewClass { message };

        return std::nullopt;
    }

    const SysEx7& sysex;
};

struct Message : SysEx7
{
    using SysEx7::SysEx7;

    uint7_t getDeviceId() const
    {
        if (data.empty())
            return 0;

        return data[0];
    }

    Type getType() const
    {
        if (data.size() < 2)
            return TypeId::none;

        return Type ((manufacturerId >> 8) | data[1]);
    }

    Subtype getSubtype() const
    {
        if (data.size() < 3)
            return 0;

        return data[2];
    }

    void setDeviceId (uint7_t deviceId)
    {
        if (data.empty())
            data.resize (1, 0);

        data[0] = deviceId;
    }
};

struct IdentityRequest : Message
{
    explicit IdentityRequest (uint7_t deviceId = 0x7f)
        : Message (Manufacturer::universalNonRealtime)
    {
        data.push_back (deviceId);
        data.push_back (TypeId::generalInformation & 0x7f);
        data.push_back (SubtypeId::identityRequest);
    }
};

inline Message makeIdentityRequest (uint7_t deviceId = 0x7f)
{
    return IdentityRequest { deviceId };
}

inline bool isIdentityRequest (const SysEx7& message)
{
    if (! isUniversalSysExMessage (message))
        return false;

    return UniversalSysEx::TypeId::generalInformation == UniversalSysEx::MessageView { message }.getType()
        && UniversalSysEx::MessageView { message }.getSubtype() == UniversalSysEx::SubtypeId::identityRequest;
}

struct IdentityReplyView : MessageView
{
    using MessageView::MessageView;

    DeviceIdentity getIdentity() const
    {
        return sysex.makeDeviceIdentity (3);
    }

    static bool validate (const SysEx7& message)
    {
        if (! isUniversalSysExMessage (message))
            return false;

        if (message.data.size() < 13)
            return false;

        const auto view = MessageView { message };
        return view.getType() == TypeId::generalInformation && view.getSubtype() == SubtypeId::identityReply;
    }
};

inline std::optional<IdentityReplyView> asIdentityReplyView (const SysEx7& message)
{
    if (IdentityReplyView::validate (message))
        return IdentityReplyView { message };

    return std::nullopt;
}

struct IdentityReply : Message
{
    IdentityReply (ManufacturerId sysexId,
                   uint14_t family,
                   uint14_t familyMember,
                   uint28_t revision,
                   uint7_t deviceId = 0x7f)
        : Message (Manufacturer::universalNonRealtime)
    {
        data.push_back (deviceId);
        data.push_back (TypeId::generalInformation & 0x7f);
        data.push_back (SubtypeId::identityReply);
        addDeviceIdentity ({ sysexId, family, familyMember, revision });
    }

    explicit IdentityReply (const DeviceIdentity& identity)
        : IdentityReply (identity.manufacturer, identity.family, identity.model, identity.revision)
    {
    }
};

inline Message makeIdentityReply (ManufacturerId sysexId,
                                  uint14_t family,
                                  uint14_t familyMember,
                                  uint28_t revision,
                                  uint7_t deviceId = 0x7f)
{
    return IdentityReply { sysexId, family, familyMember, revision, deviceId };
}

inline Message makeIdentityReply (const DeviceIdentity& identity)
{
    return IdentityReply { identity };
}

inline bool isIdentityReply (const SysEx7& message)
{
    if (! isUniversalSysExMessage (message))
        return false;

    return UniversalSysEx::TypeId::generalInformation == UniversalSysEx::MessageView { message }.getType()
        && UniversalSysEx::MessageView { message }.getSubtype() == UniversalSysEx::SubtypeId::identityReply;
}

} // namespace UniversalSysEx

inline bool isUniversalSysExMessage (const SysEx7& message)
{
    return (message.manufacturerId == Manufacturer::universalRealtime
            || message.manufacturerId == Manufacturer::universalNonRealtime)
        && message.data.size() >= 2;
}

inline UniversalSysEx::Type getUniversalSysExType (const SysEx7& message)
{
    if (! isUniversalSysExMessage (message))
        return UniversalSysEx::TypeId::none;

    return UniversalSysEx::Type ((message.manufacturerId >> 8) | message.data[1]);
}

inline UniversalSysEx::Subtype getUniversalSysExSubtype (const SysEx7& message)
{
    if (! isUniversalSysExMessage (message) || message.data.size() <= 2)
        return 0;

    return message.data[2];
}

inline uint7_t getUniversalSysExDeviceId (const SysEx7& message)
{
    if (! isUniversalSysExMessage (message))
        return 0xff;

    return message.data[0];
}

using UniversalSysExView = UniversalSysEx::MessageView;

inline std::optional<UniversalSysExView> asUniversalSysExView (const SysEx7& message)
{
    if (isUniversalSysExMessage (message))
        return UniversalSysExView { message };

    return std::nullopt;
}

} // namespace yup::ump

#endif
