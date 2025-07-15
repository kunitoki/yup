/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

std::optional<MidiRPNMessage> MidiRPNDetector::tryParse (int midiChannel,
                                                         int controllerNumber,
                                                         int controllerValue)
{
    jassert (midiChannel > 0 && midiChannel <= 16);
    jassert (controllerNumber >= 0 && controllerNumber < 128);
    jassert (controllerValue >= 0 && controllerValue < 128);

    return states[midiChannel - 1].handleController (midiChannel, controllerNumber, controllerValue);
}

void MidiRPNDetector::reset() noexcept
{
    for (auto& state : states)
    {
        state.parameterMSB = 0xff;
        state.parameterLSB = 0xff;
        state.resetValue();
        state.isNRPN = false;
    }
}

//==============================================================================
std::optional<MidiRPNMessage> MidiRPNDetector::ChannelState::handleController (int channel,
                                                                               int controllerNumber,
                                                                               int value) noexcept
{
    switch (controllerNumber)
    {
        case 0x62:
            parameterLSB = uint8 (value);
            resetValue();
            isNRPN = true;
            break;
        case 0x63:
            parameterMSB = uint8 (value);
            resetValue();
            isNRPN = true;
            break;

        case 0x64:
            parameterLSB = uint8 (value);
            resetValue();
            isNRPN = false;
            break;
        case 0x65:
            parameterMSB = uint8 (value);
            resetValue();
            isNRPN = false;
            break;

        case 0x06:
            valueMSB = uint8 (value);
            valueLSB = 0xff;
            return sendIfReady (channel);
        case 0x26:
            valueLSB = uint8 (value);
            return sendIfReady (channel);
    }

    return {};
}

void MidiRPNDetector::ChannelState::resetValue() noexcept
{
    valueMSB = 0xff;
    valueLSB = 0xff;
}

//==============================================================================
std::optional<MidiRPNMessage> MidiRPNDetector::ChannelState::sendIfReady (int channel) noexcept
{
    if (parameterMSB >= 0x80 || parameterLSB >= 0x80 || valueMSB >= 0x80)
        return {};

    MidiRPNMessage result {};
    result.channel = channel;
    result.parameterNumber = (parameterMSB << 7) + parameterLSB;
    result.isNRPN = isNRPN;

    if (valueLSB < 0x80)
    {
        result.value = (valueMSB << 7) + valueLSB;
        result.is14BitValue = true;
    }
    else
    {
        result.value = valueMSB;
        result.is14BitValue = false;
    }

    return result;
}

//==============================================================================
MidiBuffer MidiRPNGenerator::generate (MidiRPNMessage message)
{
    return generate (message.channel,
                     message.parameterNumber,
                     message.value,
                     message.isNRPN,
                     message.is14BitValue);
}

MidiBuffer MidiRPNGenerator::generate (int midiChannel,
                                       int parameterNumber,
                                       int value,
                                       bool isNRPN,
                                       bool use14BitValue)
{
    jassert (midiChannel > 0 && midiChannel <= 16);
    jassert (parameterNumber >= 0 && parameterNumber < 16384);
    jassert (value >= 0 && value < (use14BitValue ? 16384 : 128));

    auto parameterLSB = uint8 (parameterNumber & 0x0000007f);
    auto parameterMSB = uint8 (parameterNumber >> 7);

    uint8 valueLSB = use14BitValue ? uint8 (value & 0x0000007f) : 0x00;
    uint8 valueMSB = use14BitValue ? uint8 (value >> 7) : uint8 (value);

    auto channelByte = uint8 (0xb0 + midiChannel - 1);

    MidiBuffer buffer;

    buffer.addEvent (MidiMessage (channelByte, isNRPN ? 0x62 : 0x64, parameterLSB), 0);
    buffer.addEvent (MidiMessage (channelByte, isNRPN ? 0x63 : 0x65, parameterMSB), 0);

    buffer.addEvent (MidiMessage (channelByte, 0x06, valueMSB), 0);

    // According to the MIDI spec, whenever a MSB is received, the corresponding LSB will
    // be reset. Therefore, the LSB should be sent after the MSB.
    if (use14BitValue)
        buffer.addEvent (MidiMessage (channelByte, 0x26, valueLSB), 0);

    return buffer;
}

} // namespace yup
