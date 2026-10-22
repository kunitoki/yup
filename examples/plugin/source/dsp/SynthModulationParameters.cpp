/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#include "SynthModulationParameters.h"
#include "SynthFormat.h"

SynthModulationParameters::SynthModulationParameters()
{
    using namespace SynthParameterFactory;

    const auto sourceNames = getSynthModulationSourceNames();
    const auto destinationNames = getSynthModulationDestinationNames();

    for (std::size_t index = 0; index < slots.size(); ++index)
    {
        const auto prefix = "mod" + yup::String (static_cast<int> (index) + 1) + ".";
        const auto module = "Mod " + yup::String (static_cast<int> (index) + 1);
        const auto hostBase = static_cast<yup::uint32> (1000 + 10 * index);

        auto& slot = slots[index];
        slot.source = createChoice (prefix + "source", module + " Source", hostBase + 0, "Modulation", sourceNames, 0);
        slot.destination = createChoice (prefix + "destination", module + " Destination", hostBase + 1, "Modulation", destinationNames, 0);
        slot.depth = createContinuous (prefix + "depth", module + " Depth", hostBase + 2, "Modulation", { -1.0f, 1.0f }, 0.0f, SynthFormat::bipolar);
    }
}

void SynthModulationParameters::addTo (yup::AudioProcessor& processor) const
{
    for (const auto& slot : slots)
        for (const auto& parameter : { slot.source, slot.destination, slot.depth })
            processor.addParameter (parameter);
}

SynthModulationValues SynthModulationParameters::read (SynthParameterReader reader) const noexcept
{
    SynthModulationValues values;

    for (std::size_t index = 0; index < slots.size(); ++index)
        values.routes[index] = { static_cast<SynthModulationSource> (reader.asInt (slots[index].source)),
                                 static_cast<SynthModulationDestination> (reader.asInt (slots[index].destination)),
                                 reader (slots[index].depth) };

    return values;
}
