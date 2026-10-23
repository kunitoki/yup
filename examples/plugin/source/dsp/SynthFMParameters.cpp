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

#include "SynthFMParameters.h"
#include "SynthFormat.h"

SynthFMParameters::SynthFMParameters()
{
    using namespace SynthParameterFactory;

    for (int carrier = 0; carrier < SynthExample::oscillatorCount; ++carrier)
    {
        for (int modulator = 0; modulator < SynthExample::oscillatorCount; ++modulator)
        {
            const auto digits = yup::String (carrier + 1) + yup::String (modulator + 1);
            const auto hostID = static_cast<yup::uint32> (2000 + 10 * carrier + modulator);

            auto& parameter = amounts[static_cast<std::size_t> (carrier)][static_cast<std::size_t> (modulator)];

            if (carrier == modulator)
            {
                parameter = createContinuous ("fm." + digits, "Feedback " + yup::String (carrier + 1), hostID, "FM",
                                              yup::NormalisableRange<float> (0.0f, 1.0f), 0.0f, SynthFormat::percent);
            }
            else
            {
                yup::NormalisableRange<float> range (0.0f, maximumAmount);
                range.setSkewForCentre (0.5f);

                parameter = createContinuous ("fm." + digits, "FM " + yup::String (modulator + 1) + " > " + yup::String (carrier + 1), hostID, "FM",
                                              range, 0.0f, [] (double value) { return SynthFormat::plain (value, 2); });
            }
        }
    }
}

void SynthFMParameters::addTo (yup::AudioProcessor& processor) const
{
    for (const auto& row : amounts)
        for (const auto& parameter : row)
            processor.addParameter (parameter);
}

SynthFMValues SynthFMParameters::read (SynthParameterReader reader) const noexcept
{
    SynthFMValues values;

    for (std::size_t carrier = 0; carrier < amounts.size(); ++carrier)
        for (std::size_t modulator = 0; modulator < amounts.size(); ++modulator)
            values.amounts[carrier][modulator] = reader (amounts[carrier][modulator]);

    return values;
}
