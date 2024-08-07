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

namespace juce::universal_midi_packets
{

uint32_t SysEx7::getNumPacketsRequiredForDataSize (uint32_t size)
{
    constexpr auto denom = 6;
    return (size / denom) + ((size % denom) != 0);
}

SysEx7::PacketBytes SysEx7::getDataBytes (const PacketX2& packet)
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

} // namespace juce::universal_midi_packets
