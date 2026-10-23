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

    for (std::size_t index = 0; index < depths.size(); ++index)
    {
        const auto number = yup::String (static_cast<int> (index) + 1);

        depths[index] = createContinuous ("route." + number + ".depth", "Route " + number + " Depth",
                                          static_cast<yup::uint32> (6000 + index), "Modulation",
                                          { -1.0f, 1.0f }, 0.0f, SynthFormat::bipolar);
    }
}

void SynthModulationParameters::addTo (yup::AudioProcessor& processor) const
{
    for (const auto& parameter : depths)
        processor.addParameter (parameter);
}
