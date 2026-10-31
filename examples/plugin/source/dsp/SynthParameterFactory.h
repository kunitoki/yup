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

#pragma once

#include <yup_audio_processors/yup_audio_processors.h>

#include "SynthFormat.h"

#include <functional>

//==============================================================================
/** Builds the synthesiser's parameters with the flags every one of them shares.

    Every parameter takes a stable host ID, so automation survives reordering, and is
    modulatable, so CLAP hosts can modulate it; the processor honors those modulations
    itself because they never reach the parameter's own value.
*/
namespace SynthParameterFactory
{
/** Formats a real parameter value for display. */
using Formatter = std::function<yup::String (double)>;

/** Reads the text of a Formatter back into the real parameter value. */
using Parser = std::function<double (const yup::String&)>;

/** Creates a continuous parameter; the range carries any skew. */
yup::AudioParameter::Ptr createContinuous (const yup::String& id,
                                           const yup::String& name,
                                           yup::uint32 hostID,
                                           const yup::String& modulePath,
                                           yup::NormalisableRange<float> range,
                                           float defaultValue,
                                           Formatter format,
                                           Parser parse = SynthFormat::parse);

/** Creates an integer parameter stepping by one between minimum and maximum. */
yup::AudioParameter::Ptr createInteger (const yup::String& id,
                                        const yup::String& name,
                                        yup::uint32 hostID,
                                        const yup::String& modulePath,
                                        int minimum,
                                        int maximum,
                                        int defaultValue,
                                        Formatter format);

/** Creates an enumerated parameter whose values index the given names. */
yup::AudioParameter::Ptr createChoice (const yup::String& id,
                                       const yup::String& name,
                                       yup::uint32 hostID,
                                       const yup::String& modulePath,
                                       const yup::StringArray& choices,
                                       int defaultIndex);
} // namespace SynthParameterFactory

//==============================================================================
/** Reads parameter values either from their atomics or from a block's value table.

    The audio thread passes the table the processor keeps per block, which already holds
    host modulation; the user interface passes nothing and reads the atomics.
*/
struct SynthParameterReader
{
    /** One value per parameter index, or empty to read the parameters themselves. */
    yup::Span<const float> values;

    /** Returns the real value of a parameter. */
    float operator() (const yup::AudioParameter::Ptr& parameter) const noexcept
    {
        if (values.empty())
            return parameter->getValue();

        return values[static_cast<std::size_t> (parameter->getIndexInContainer())];
    }

    /** Returns the value of a stepped parameter, rounded to its nearest step. */
    int asInt (const yup::AudioParameter::Ptr& parameter) const noexcept
    {
        return yup::roundToInt ((*this) (parameter));
    }
};
