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

constexpr ControllerValue getControllerValue (const UniversalPacket& p);
constexpr uint8_t getPerNoteControllerIndex (const UniversalPacket& p);

constexpr bool isChannelVoiceMessageWithStatus (const UniversalPacket& p, Status status)
{
    return ((p.getStatus() & 0xf0) == status) && p.isChannelVoiceMessage();
}

constexpr bool isNoteOnMessage (const UniversalPacket& p)
{
    if (isChannelVoiceMessageWithStatus (p, Status (ChannelVoiceStatus::noteOn)))
    {
        return (p.getType() == PacketType::midi2ChannelVoice) || (p.getByte4() != 0);
    }

    return false;
}

constexpr bool isNoteOffMessage (const UniversalPacket& p)
{
    if (isChannelVoiceMessageWithStatus (p, Status (ChannelVoiceStatus::noteOff)))
        return true;

    return (p.getType() == PacketType::midi1ChannelVoice)
        && ((p.getStatus() & 0xf0) == uint8_t (ChannelVoiceStatus::noteOn))
        && (p.getByte4() == 0);
}

constexpr NoteNumber getNoteNumber (const UniversalPacket& p)
{
    return NoteNumber (p.getByte3() & 0x7f);
}

constexpr Pitch7_9 getNotePitch (const UniversalPacket& p)
{
    if ((p.getType() == PacketType::midi2ChannelVoice)
        && ((p.getStatus() & 0xf0) == uint8_t (ChannelVoiceStatus::noteOn))
        && (p.getByte4() == NoteAttribute::pitch_7_9))
    {
        return Pitch7_9 { uint16_t (p.data[1] & 0xffff) };
    }

    return Pitch7_9 { getNoteNumber (p) };
}

constexpr Velocity getNoteVelocity (const UniversalPacket& p)
{
    if (p.getType() == PacketType::midi2ChannelVoice)
        return Velocity { uint16_t (p.data[1] >> 16) };

    if ((p.getType() == PacketType::midi1ChannelVoice)
        && ((p.getStatus() & 0xf0) == uint8_t (ChannelVoiceStatus::noteOn))
        && (p.getByte4() == 0))
    {
        return Velocity { uint7_t { 64 } };
    }

    return Velocity { uint7_t (p.getByte4() & 0x7f) };
}

constexpr bool isPolyPressureMessage (const UniversalPacket& p)
{
    return isChannelVoiceMessageWithStatus (p, Status (ChannelVoiceStatus::polyPressure));
}

constexpr ControllerValue getPolyPressureValue (const UniversalPacket& p)
{
    return getControllerValue (p);
}

constexpr bool isControlChangeMessage (const UniversalPacket& p)
{
    return isChannelVoiceMessageWithStatus (p, Status (ChannelVoiceStatus::controlChange));
}

constexpr ControllerNumber getControllerNumber (const UniversalPacket& p)
{
    return ControllerNumber (p.getByte3() & 0x7f);
}

constexpr ControllerValue getControllerValue (const UniversalPacket& p)
{
    if (p.getType() == PacketType::midi1ChannelVoice)
        return ControllerValue { uint7_t (p.getByte4() & 0x7f) };

    return ControllerValue { p.data[1] };
}

constexpr bool isProgramChangeMessage (const UniversalPacket& p)
{
    return isChannelVoiceMessageWithStatus (p, Status (ChannelVoiceStatus::programChange));
}

constexpr uint7_t getProgramValue (const UniversalPacket& p)
{
    switch (p.getType())
    {
        case PacketType::midi1ChannelVoice:
            return uint7_t (p.getByte3() & 0x7f);
        case PacketType::midi2ChannelVoice:
            return uint7_t ((p.data[1] >> 24u) & 0x7f);
        default:
            break;
    }

    return 0xff;
}

constexpr bool isChannelPressureMessage (const UniversalPacket& p)
{
    return isChannelVoiceMessageWithStatus (p, Status (ChannelVoiceStatus::channelPressure));
}

constexpr ControllerValue getChannelPressureValue (const UniversalPacket& p)
{
    if (p.getType() == PacketType::midi1ChannelVoice)
        return ControllerValue { uint7_t (p.getByte3() & 0x7f) };

    return ControllerValue { p.data[1] };
}

constexpr bool isChannelPitchBendMessage (const UniversalPacket& p)
{
    return isChannelVoiceMessageWithStatus (p, Status (ChannelVoiceStatus::pitchBend));
}

constexpr PitchBend getChannelPitchBendValue (const UniversalPacket& p)
{
    if (p.getType() == PacketType::midi1ChannelVoice)
        return PitchBend { uint14_t (p.getByte3() | (p.getByte4() << 7)) };

    return PitchBend { p.data[1] };
}

//==============================================================================
constexpr std::optional<Midi1ChannelVoiceMessage> asMidi1ChannelVoiceMessage (const Midi2ChannelVoiceMessageView& m)
{
    switch (m.getStatus())
    {
        case Status (ChannelVoiceStatus::noteOff):
            if (m.getByte4() == 0)
                return makeMidi1NoteOffMessage (m.getGroup(), m.getChannel(), m.getByte3(), Velocity { uint16_t (m.getData() >> 16) });
            break;
        case Status (ChannelVoiceStatus::noteOn):
            if (m.getByte4() == 0)
            {
                auto vel = Velocity { uint16_t (m.getData() >> 16) };
                if (vel.asUInt7() == 0)
                    vel = Velocity { uint7_t { 1 } };
                return makeMidi1NoteOnMessage (m.getGroup(), m.getChannel(), m.getByte3(), vel);
            }
            break;
        case Status (ChannelVoiceStatus::polyPressure):
            return makeMidi1PolyPressureMessage (m.getGroup(), m.getChannel(), m.getByte3(), ControllerValue { m.getData() });
        case Status (ChannelVoiceStatus::controlChange):
            switch (m.getByte3())
            {
                case ControlChange::bankSelectMsb:
                case ControlChange::dataEntryMsb:
                case ControlChange::bankSelectLsb:
                case ControlChange::dataEntryLsb:
                case ControlChange::hiResVelocityPrefix:
                case ControlChange::nrpnLsb:
                case ControlChange::nrpnMsb:
                case ControlChange::rpnLsb:
                case ControlChange::rpnMsb:
                    break;
                default:
                    return makeMidi1ControlChangeMessage (m.getGroup(), m.getChannel(), m.getByte3(), ControllerValue { m.getData() });
            }
            break;
        case Status (ChannelVoiceStatus::programChange):
            if ((m.getByte4() & 0x1) == 0)
                return makeMidi1ProgramChangeMessage (m.getGroup(), m.getChannel(), uint7_t (m.getData() >> 24));
            break;
        case Status (ChannelVoiceStatus::channelPressure):
            return makeMidi1ChannelPressureMessage (m.getGroup(), m.getChannel(), ControllerValue { m.getData() });
        case Status (ChannelVoiceStatus::pitchBend):
            return makeMidi1PitchBendMessage (m.getGroup(), m.getChannel(), PitchBend { m.getData() });
        default:
            break;
    }

    return std::nullopt;
}

constexpr std::optional<Midi2ChannelVoiceMessage> asMidi2ChannelVoiceMessage (const Midi1ChannelVoiceMessageView& m)
{
    switch (m.getStatus())
    {
        case Status (ChannelVoiceStatus::noteOff):
            return makeMidi2NoteOffMessage (m.getGroup(), m.getChannel(), m.getDataByte1(), Velocity { m.getDataByte2() });
        case Status (ChannelVoiceStatus::noteOn):
            if (m.getDataByte2() == 0)
                return makeMidi2NoteOffMessage (m.getGroup(), m.getChannel(), m.getDataByte1(), Velocity { uint7_t { 64 } });
            return makeMidi2NoteOnMessage (m.getGroup(), m.getChannel(), m.getDataByte1(), Velocity { m.getDataByte2() });
        case Status (ChannelVoiceStatus::polyPressure):
            return makeMidi2PolyPressureMessage (m.getGroup(), m.getChannel(), m.getDataByte1(), ControllerValue { m.getDataByte2() });
        case Status (ChannelVoiceStatus::controlChange):
            switch (m.getDataByte1())
            {
                case ControlChange::bankSelectMsb:
                case ControlChange::dataEntryMsb:
                case ControlChange::bankSelectLsb:
                case ControlChange::dataEntryLsb:
                case ControlChange::hiResVelocityPrefix:
                case ControlChange::nrpnLsb:
                case ControlChange::nrpnMsb:
                case ControlChange::rpnLsb:
                case ControlChange::rpnMsb:
                    break;
                default:
                    return makeMidi2ControlChangeMessage (m.getGroup(), m.getChannel(), m.getDataByte1(), ControllerValue { m.getDataByte2() });
            }
            break;
        case Status (ChannelVoiceStatus::programChange):
            return makeMidi2ProgramChangeMessage (m.getGroup(), m.getChannel(), m.getDataByte1());
        case Status (ChannelVoiceStatus::channelPressure):
            return makeMidi2ChannelPressureMessage (m.getGroup(), m.getChannel(), ControllerValue { m.getDataByte1() });
        case Status (ChannelVoiceStatus::pitchBend):
            return makeMidi2PitchBendMessage (m.getGroup(), m.getChannel(), PitchBend { m.get14BitValue() });
        default:
            break;
    }

    return std::nullopt;
}

//==============================================================================
constexpr bool isRegisteredControllerMessage (const UniversalPacket& p)
{
    return isMidi2ChannelVoiceMessage (p)
        && (p.getStatus() & 0xf0) == uint8_t (ChannelVoiceStatus::registeredController);
}

constexpr bool isAssignableControllerMessage (const UniversalPacket& p)
{
    return isMidi2ChannelVoiceMessage (p)
        && (p.getStatus() & 0xf0) == uint8_t (ChannelVoiceStatus::assignableController);
}

constexpr bool isRegisteredPerNoteControllerMessage (const UniversalPacket& p)
{
    return isMidi2ChannelVoiceMessage (p)
        && (p.getStatus() & 0xf0) == uint8_t (ChannelVoiceStatus::registeredPerNoteController);
}

constexpr bool isRegisteredPerNoteControllerPitchMessage (const UniversalPacket& p)
{
    return isRegisteredPerNoteControllerMessage (p)
        && getPerNoteControllerIndex (p) == RegisteredPerNoteController::pitch7_25;
}

constexpr bool isAssignablePerNoteControllerMessage (const UniversalPacket& p)
{
    return isMidi2ChannelVoiceMessage (p)
        && (p.getStatus() & 0xf0) == uint8_t (ChannelVoiceStatus::assignablePerNoteController);
}

constexpr bool isPerNotePitchBendMessage (const UniversalPacket& p)
{
    return isMidi2ChannelVoiceMessage (p)
        && (p.getStatus() & 0xf0) == uint8_t (ChannelVoiceStatus::perNotePitchBend);
}

constexpr bool isNoteOnWithAttribute (const UniversalPacket& p, uint8_t attribute)
{
    return isMidi2ChannelVoiceMessage (p)
        && ((p.getStatus() & 0xf0) == uint8_t (ChannelVoiceStatus::noteOn))
        && (p.getByte4() == attribute);
}

constexpr bool isNoteOffWithAttribute (const UniversalPacket& p, uint8_t attribute)
{
    return isMidi2ChannelVoiceMessage (p)
        && ((p.getStatus() & 0xf0) == uint8_t (ChannelVoiceStatus::noteOff))
        && (p.getByte4() == attribute);
}

constexpr bool isNoteOnWithPitch7_9 (const UniversalPacket& p)
{
    return isNoteOnWithAttribute (p, NoteAttribute::pitch_7_9);
}

constexpr uint8_t getMidi2NoteAttribute (const UniversalPacket& p)
{
    return p.getByte4();
}

constexpr uint16_t getMidi2NoteAttributeData (const UniversalPacket& p)
{
    return uint16_t (p.data[1] & 0xffff);
}

constexpr uint8_t getPerNoteControllerIndex (const UniversalPacket& p)
{
    return p.getByte4();
}

constexpr bool isPitchBendSensitivityMessage (const UniversalPacket& p)
{
    return isRegisteredControllerMessage (p)
        && (p.getByte3() == 0)
        && (p.getByte4() == RegisteredParameterNumber::pitchBendSensitivity);
}

constexpr bool isPerNotePitchBendSensitivityMessage (const UniversalPacket& p)
{
    return isRegisteredControllerMessage (p)
        && (p.getByte3() == 0)
        && (p.getByte4() == RegisteredParameterNumber::perNotePitchBendSensitivity);
}

constexpr PitchBendSensitivity getPitchBendSensitivityValue (const UniversalPacket& p)
{
    return PitchBendSensitivity { p.data[1] & 0xfffc0000u };
}

constexpr PitchBendSensitivity getPerNotePitchBendSensitivityValue (const UniversalPacket& p)
{
    return PitchBendSensitivity { p.data[1] };
}

constexpr PitchBend getPerNotePitchBendValue (const UniversalPacket& p)
{
    return PitchBend { p.data[1] };
}

} // namespace yup::ump

#endif
