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

//==============================================================================
struct UtilityMessage : UniversalPacket
{
    constexpr UtilityMessage() = default;

    constexpr explicit UtilityMessage (Status status, uint16_t payload = 0u)
        : UniversalPacket (uint32_t (status) << 16u | payload)
    {
    }
};

struct UtilityMessageView
{
    constexpr explicit UtilityMessageView (const UniversalPacket& ump)
        : p (ump)
    {
        jassert (p.getType() == PacketType::utility);
    }

    constexpr Status getStatus() const { return p.getStatus(); }

    constexpr uint16_t getPayload() const { return uint16_t (p.getByte3() << 8u) | p.getByte4(); }

private:
    const UniversalPacket& p;
};

constexpr UtilityMessage makeUtilityMessage (Status status, uint16_t payload)
{
    return UtilityMessage { status, payload };
}

//==============================================================================
struct SystemMessage : UniversalPacket
{
    constexpr SystemMessage()
        : UniversalPacket (0x10000000u)
    {
    }

    constexpr SystemMessage (Group group, Status status, uint7_t data1 = 0, uint7_t data2 = 0)
        : UniversalPacket (0x10000000u | ((group & 0x0f) << 24u) | (uint32_t (status) << 16u)
                           | (uint32_t (data1) << 8u) | data2)
    {
    }
};

constexpr bool isSystemMessage (const UniversalPacket& p)
{
    return p.getType() == PacketType::system;
}

struct SystemMessageView
{
    constexpr explicit SystemMessageView (const UniversalPacket& ump)
        : p (ump)
    {
        jassert (p.getType() == PacketType::system);
    }

    constexpr Group getGroup() const { return p.getGroup(); }

    constexpr Status getStatus() const { return p.getStatus(); }

    constexpr uint7_t getDataByte1() const { return uint7_t (p.getByte3() & 0x7f); }

    constexpr uint7_t getDataByte2() const { return uint7_t (p.getByte4() & 0x7f); }

    constexpr uint14_t getSongPosition() const
    {
        if (SystemStatus (p.getStatus()) == SystemStatus::songPosition)
            return uint14_t (getDataByte1() | (uint14_t (getDataByte2()) << 7u));

        return 0;
    }

private:
    const UniversalPacket& p;
};

constexpr std::optional<SystemMessageView> asSystemMessageView (const UniversalPacket& p)
{
    if (isSystemMessage (p))
        return SystemMessageView { p };

    return std::nullopt;
}

constexpr SystemMessage makeSystemMessage (Group group, Status status, uint7_t data1 = 0, uint7_t data2 = 0)
{
    return SystemMessage { group, status, data1, data2 };
}

constexpr SystemMessage makeSongPositionMessage (Group group, uint14_t position)
{
    return makeSystemMessage (group,
                              Status (SystemStatus::songPosition),
                              static_cast<uint7_t> (position & 0x7f),
                              static_cast<uint7_t> ((position >> 7) & 0x7f));
}

//==============================================================================
struct Midi1ChannelVoiceMessage : UniversalPacket
{
    constexpr Midi1ChannelVoiceMessage() = default;

    constexpr Midi1ChannelVoiceMessage (Group group, Status status, uint7_t data1 = 0, uint7_t data2 = 0)
        : UniversalPacket (0x20000000u | ((group & 0x0f) << 24u) | (uint32_t (status) << 16u)
                           | (uint32_t (data1) << 8u) | data2)
    {
    }

    constexpr explicit Midi1ChannelVoiceMessage (const UniversalPacket& p)
        : UniversalPacket (p)
    {
        jassert (p.getType() == PacketType::midi1ChannelVoice);
    }
};

constexpr bool isMidi1ChannelVoiceMessage (const UniversalPacket& p)
{
    return p.getType() == PacketType::midi1ChannelVoice;
}

struct Midi1ChannelVoiceMessageView
{
    constexpr explicit Midi1ChannelVoiceMessageView (const UniversalPacket& ump)
        : p (ump)
    {
        jassert (isMidi1ChannelVoiceMessage (p));
    }

    constexpr Group getGroup() const { return p.getGroup(); }

    constexpr Status getStatus() const { return Status (p.getStatus() & 0xf0); }

    constexpr Channel getChannel() const { return Channel (p.getStatus() & 0x0f); }

    constexpr uint7_t getDataByte1() const { return uint7_t (p.getByte3() & 0x7f); }

    constexpr uint7_t getDataByte2() const { return uint7_t (p.getByte4() & 0x7f); }

    constexpr uint14_t get14BitValue() const
    {
        return uint14_t (getDataByte1() | (uint14_t (getDataByte2()) << 7u));
    }

private:
    const UniversalPacket& p;
};

constexpr std::optional<Midi1ChannelVoiceMessageView> asMidi1ChannelVoiceMessageView (const UniversalPacket& p)
{
    if (isMidi1ChannelVoiceMessage (p))
        return Midi1ChannelVoiceMessageView { p };

    return std::nullopt;
}

constexpr Midi1ChannelVoiceMessage makeMidi1ChannelVoiceMessage (Group group,
                                                                 Status status,
                                                                 Channel channel,
                                                                 uint7_t data1,
                                                                 uint7_t data2 = 0)
{
    return Midi1ChannelVoiceMessage { group,
                                      Status (status | (channel & 0x0f)),
                                      uint7_t (data1 & 0x7f),
                                      uint7_t (data2 & 0x7f) };
}

constexpr Midi1ChannelVoiceMessage makeMidi1NoteOffMessage (Group group,
                                                            Channel channel,
                                                            NoteNumber noteNr,
                                                            Velocity vel = {})
{
    return makeMidi1ChannelVoiceMessage (group,
                                         Status (Midi1ChannelVoiceStatus::noteOff),
                                         channel,
                                         noteNr,
                                         vel.asUInt7());
}

constexpr Midi1ChannelVoiceMessage makeMidi1NoteOnMessage (Group group,
                                                           Channel channel,
                                                           NoteNumber noteNr,
                                                           Velocity vel)
{
    return makeMidi1ChannelVoiceMessage (group,
                                         Status (Midi1ChannelVoiceStatus::noteOn),
                                         channel,
                                         noteNr,
                                         vel.asUInt7());
}

constexpr Midi1ChannelVoiceMessage makeMidi1PolyPressureMessage (Group group,
                                                                 Channel channel,
                                                                 NoteNumber noteNr,
                                                                 ControllerValue pressure)
{
    return makeMidi1ChannelVoiceMessage (group,
                                         Status (Midi1ChannelVoiceStatus::polyPressure),
                                         channel,
                                         noteNr,
                                         pressure.asUInt7());
}

constexpr Midi1ChannelVoiceMessage makeMidi1ControlChangeMessage (Group group,
                                                                  Channel channel,
                                                                  ControllerNumber controller,
                                                                  ControllerValue value)
{
    return makeMidi1ChannelVoiceMessage (group,
                                         Status (Midi1ChannelVoiceStatus::controlChange),
                                         channel,
                                         controller,
                                         value.asUInt7());
}

constexpr Midi1ChannelVoiceMessage makeMidi1ProgramChangeMessage (Group group,
                                                                  Channel channel,
                                                                  ProgramNumber program)
{
    return makeMidi1ChannelVoiceMessage (group,
                                         Status (Midi1ChannelVoiceStatus::programChange),
                                         channel,
                                         program);
}

constexpr Midi1ChannelVoiceMessage makeMidi1ChannelPressureMessage (Group group,
                                                                    Channel channel,
                                                                    ControllerValue pressure)
{
    return makeMidi1ChannelVoiceMessage (group,
                                         Status (Midi1ChannelVoiceStatus::channelPressure),
                                         channel,
                                         pressure.asUInt7(),
                                         0);
}

constexpr Midi1ChannelVoiceMessage makeMidi1PitchBendMessage (Group group,
                                                              Channel channel,
                                                              PitchBend pb)
{
    const auto pb14 = pb.asUInt14();
    return makeMidi1ChannelVoiceMessage (group,
                                         Status (Midi1ChannelVoiceStatus::pitchBend),
                                         channel,
                                         uint7_t (pb14 & 0x7f),
                                         uint7_t ((pb14 >> 7) & 0x7f));
}

//==============================================================================
struct Midi2ChannelVoiceMessage : UniversalPacket
{
    constexpr Midi2ChannelVoiceMessage() = default;

    constexpr Midi2ChannelVoiceMessage (Group group,
                                        Status status,
                                        Channel channel,
                                        uint8_t byte3,
                                        uint8_t byte4,
                                        uint32_t data)
        : UniversalPacket (0x40000000u | ((group & 0x0f) << 24u)
                               | (uint32_t (Status ((status & 0xf0) | (channel & 0x0f))) << 16u)
                               | (uint32_t (byte3) << 8u) | byte4,
                           data)
    {
    }

    constexpr explicit Midi2ChannelVoiceMessage (const UniversalPacket& p)
        : UniversalPacket (p)
    {
        jassert (p.getType() == PacketType::midi2ChannelVoice);
    }
};

constexpr bool isMidi2ChannelVoiceMessage (const UniversalPacket& p)
{
    return p.getType() == PacketType::midi2ChannelVoice;
}

struct Midi2ChannelVoiceMessageView
{
    constexpr explicit Midi2ChannelVoiceMessageView (const UniversalPacket& ump)
        : p (ump)
    {
        jassert (p.getType() == PacketType::midi2ChannelVoice);
    }

    constexpr Group getGroup() const { return p.getGroup(); }

    constexpr Status getStatus() const { return Status (p.getStatus() & 0xf0); }

    constexpr Channel getChannel() const { return Channel (p.getStatus() & 0x0f); }

    constexpr uint7_t getByte3() const { return uint7_t (p.getByte3() & 0x7f); }

    constexpr uint7_t getByte4() const { return uint7_t (p.getByte4() & 0x7f); }

    constexpr uint32_t getData() const { return p.data[1]; }

private:
    const UniversalPacket& p;
};

constexpr std::optional<Midi2ChannelVoiceMessageView> asMidi2ChannelVoiceMessageView (const UniversalPacket& p)
{
    if (isMidi2ChannelVoiceMessage (p))
        return Midi2ChannelVoiceMessageView { p };

    return std::nullopt;
}

using NoteManagementFlags = uint8_t;

namespace NoteManagement
{
constexpr uint8_t reset = 0x1;
constexpr uint8_t detach = 0x2;
constexpr uint8_t detach_and_reset = 0x3;
} // namespace NoteManagement

namespace NoteAttribute
{
constexpr uint8_t none = 0x0;
constexpr uint8_t manufacturer_specific = 0x1;
constexpr uint8_t profile_specific = 0x2;
constexpr uint8_t pitch_7_9 = 0x3;
} // namespace NoteAttribute

constexpr Midi2ChannelVoiceMessage makeMidi2ChannelVoiceMessage (Group group,
                                                                 Status status,
                                                                 Channel channel,
                                                                 uint7_t index1,
                                                                 uint7_t index2,
                                                                 uint32_t data)
{
    return Midi2ChannelVoiceMessage { group, status, channel, index1, index2, data };
}

constexpr Midi2ChannelVoiceMessage makeMidi2NoteOffMessage (Group group,
                                                            Channel channel,
                                                            NoteNumber noteNr,
                                                            Velocity vel,
                                                            uint8_t attribute = 0,
                                                            uint16_t attributeData = 0)
{
    return Midi2ChannelVoiceMessage { group,
                                      Status (ChannelVoiceStatus::noteOff),
                                      channel,
                                      noteNr,
                                      attribute,
                                      uint32_t ((uint32_t (vel.value) << 16u) | attributeData) };
}

constexpr Midi2ChannelVoiceMessage makeMidi2NoteOnMessage (Group group,
                                                           Channel channel,
                                                           NoteNumber noteNr,
                                                           Velocity vel)
{
    return Midi2ChannelVoiceMessage { group,
                                      Status (ChannelVoiceStatus::noteOn),
                                      channel,
                                      noteNr,
                                      0,
                                      uint32_t (vel.value) << 16u };
}

constexpr Midi2ChannelVoiceMessage makeMidi2NoteOnMessage (Group group,
                                                           Channel channel,
                                                           NoteNumber noteNr,
                                                           Velocity vel,
                                                           Pitch7_9 pitch)
{
    return Midi2ChannelVoiceMessage { group,
                                      Status (ChannelVoiceStatus::noteOn),
                                      channel,
                                      noteNr,
                                      NoteAttribute::pitch_7_9,
                                      uint32_t ((uint32_t (vel.value) << 16u) | pitch.value) };
}

constexpr Midi2ChannelVoiceMessage makeMidi2NoteOnMessage (Group group,
                                                           Channel channel,
                                                           NoteNumber noteNr,
                                                           Velocity vel,
                                                           uint8_t attribute,
                                                           uint16_t attributeData)
{
    return Midi2ChannelVoiceMessage { group,
                                      Status (ChannelVoiceStatus::noteOn),
                                      channel,
                                      noteNr,
                                      attribute,
                                      uint32_t ((uint32_t (vel.value) << 16u) | attributeData) };
}

constexpr Midi2ChannelVoiceMessage makeMidi2PolyPressureMessage (Group group,
                                                                 Channel channel,
                                                                 NoteNumber noteNr,
                                                                 ControllerValue pressure)
{
    return Midi2ChannelVoiceMessage { group,
                                      Status (ChannelVoiceStatus::polyPressure),
                                      channel,
                                      noteNr,
                                      0,
                                      pressure.value };
}

constexpr Midi2ChannelVoiceMessage makeRegisteredPerNoteControllerMessage (Group group,
                                                                           Channel channel,
                                                                           NoteNumber noteNr,
                                                                           uint8_t controller,
                                                                           ControllerValue value)
{
    return Midi2ChannelVoiceMessage { group,
                                      Status (ChannelVoiceStatus::registeredPerNoteController),
                                      channel,
                                      noteNr,
                                      controller,
                                      value.value };
}

constexpr Midi2ChannelVoiceMessage makeAssignablePerNoteControllerMessage (Group group,
                                                                           Channel channel,
                                                                           NoteNumber noteNr,
                                                                           uint8_t controller,
                                                                           ControllerValue value)
{
    return Midi2ChannelVoiceMessage { group,
                                      Status (ChannelVoiceStatus::assignablePerNoteController),
                                      channel,
                                      noteNr,
                                      controller,
                                      value.value };
}

constexpr Midi2ChannelVoiceMessage makePerNoteManagementMessage (Group group,
                                                                 Channel channel,
                                                                 NoteNumber noteNr,
                                                                 NoteManagementFlags flags)
{
    return Midi2ChannelVoiceMessage { group,
                                      Status (ChannelVoiceStatus::perNoteManagement),
                                      channel,
                                      noteNr,
                                      flags,
                                      0 };
}

constexpr Midi2ChannelVoiceMessage makeMidi2ControlChangeMessage (Group group,
                                                                  Channel channel,
                                                                  uint7_t controller,
                                                                  ControllerValue value)
{
    return Midi2ChannelVoiceMessage { group,
                                      Status (ChannelVoiceStatus::controlChange),
                                      channel,
                                      controller,
                                      0,
                                      value.value };
}

constexpr Midi2ChannelVoiceMessage makeRegisteredControllerMessage (Group group,
                                                                    Channel channel,
                                                                    uint7_t bank,
                                                                    uint7_t index,
                                                                    ControllerValue value)
{
    return Midi2ChannelVoiceMessage { group,
                                      Status (ChannelVoiceStatus::registeredController),
                                      channel,
                                      bank,
                                      index,
                                      value.value };
}

constexpr Midi2ChannelVoiceMessage makeAssignableControllerMessage (Group group,
                                                                    Channel channel,
                                                                    uint7_t bank,
                                                                    uint7_t index,
                                                                    ControllerValue value)
{
    return Midi2ChannelVoiceMessage { group,
                                      Status (ChannelVoiceStatus::assignableController),
                                      channel,
                                      bank,
                                      index,
                                      value.value };
}

constexpr Midi2ChannelVoiceMessage makeRelativeRegisteredControllerMessage (Group group,
                                                                            Channel channel,
                                                                            uint7_t bank,
                                                                            uint7_t index,
                                                                            ControllerIncrement inc)
{
    return Midi2ChannelVoiceMessage { group,
                                      Status (ChannelVoiceStatus::relativeRegisteredController),
                                      channel,
                                      bank,
                                      index,
                                      uint32_t (inc.value) };
}

constexpr Midi2ChannelVoiceMessage makeRelativeAssignableControllerMessage (Group group,
                                                                            Channel channel,
                                                                            uint7_t bank,
                                                                            uint7_t index,
                                                                            ControllerIncrement inc)
{
    return Midi2ChannelVoiceMessage { group,
                                      Status (ChannelVoiceStatus::relativeAssignableController),
                                      channel,
                                      bank,
                                      index,
                                      uint32_t (inc.value) };
}

constexpr Midi2ChannelVoiceMessage makeMidi2ProgramChangeMessage (Group group,
                                                                  Channel channel,
                                                                  ProgramNumber program)
{
    return Midi2ChannelVoiceMessage { group,
                                      Status (ChannelVoiceStatus::programChange),
                                      channel,
                                      0,
                                      0,
                                      uint32_t ((program & 0x7f) << 24u) };
}

constexpr Midi2ChannelVoiceMessage makeMidi2ProgramChangeMessage (Group group,
                                                                  Channel channel,
                                                                  ProgramNumber program,
                                                                  uint14_t bank)
{
    return Midi2ChannelVoiceMessage { group,
                                      Status (ChannelVoiceStatus::programChange),
                                      channel,
                                      0,
                                      1,
                                      uint32_t (((program & 0x7f) << 24u) | ((bank & 0x3f80u) << 1u) | (bank & 0x7fu)) };
}

constexpr Midi2ChannelVoiceMessage makeMidi2ChannelPressureMessage (Group group,
                                                                    Channel channel,
                                                                    ControllerValue pressure)
{
    return Midi2ChannelVoiceMessage { group,
                                      Status (ChannelVoiceStatus::channelPressure),
                                      channel,
                                      0,
                                      0,
                                      pressure.value };
}

constexpr Midi2ChannelVoiceMessage makeMidi2PitchBendMessage (Group group,
                                                              Channel channel,
                                                              PitchBend bend)
{
    return Midi2ChannelVoiceMessage { group,
                                      Status (ChannelVoiceStatus::pitchBend),
                                      channel,
                                      0,
                                      0,
                                      bend.value };
}

constexpr Midi2ChannelVoiceMessage makePerNotePitchBendMessage (Group group,
                                                                Channel channel,
                                                                NoteNumber noteNr,
                                                                PitchBend bend)
{
    return Midi2ChannelVoiceMessage { group,
                                      Status (ChannelVoiceStatus::perNotePitchBend),
                                      channel,
                                      noteNr,
                                      0,
                                      bend.value };
}

} // namespace yup::ump

#endif
