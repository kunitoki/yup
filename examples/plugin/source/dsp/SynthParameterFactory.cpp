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

#include "SynthParameterFactory.h"

namespace SynthParameterFactory
{
namespace
{
yup::AudioParameterBuilder makeBuilder (const yup::String& id,
                                        const yup::String& name,
                                        yup::uint32 hostID,
                                        const yup::String& modulePath)
{
    yup::AudioParameterBuilder builder;
    builder.withID (id)
        .withName (name)
        .withHostID (hostID)
        .withModulePath (modulePath)
        .withModulatable (true);

    return builder;
}
} // namespace

yup::AudioParameter::Ptr createContinuous (const yup::String& id,
                                           const yup::String& name,
                                           yup::uint32 hostID,
                                           const yup::String& modulePath,
                                           yup::NormalisableRange<float> range,
                                           float defaultValue,
                                           Formatter format,
                                           Parser parse)
{
    return makeBuilder (id, name, hostID, modulePath)
        .withRange (range)
        .withDefault (defaultValue)
        .withValueToString ([format = std::move (format)] (float value) { return format (value); })
        .withStringToValue ([parse = std::move (parse)] (const yup::String& text) { return static_cast<float> (parse (text)); })
        .build();
}

yup::AudioParameter::Ptr createInteger (const yup::String& id,
                                        const yup::String& name,
                                        yup::uint32 hostID,
                                        const yup::String& modulePath,
                                        int minimum,
                                        int maximum,
                                        int defaultValue,
                                        Formatter format)
{
    return makeBuilder (id, name, hostID, modulePath)
        .withRange (yup::NormalisableRange<float> (static_cast<float> (minimum), static_cast<float> (maximum), 1.0f))
        .withDefault (static_cast<float> (defaultValue))
        .withStepped (true)
        .withValueToString ([format = std::move (format)] (float value) { return format (value); })
        .withStringToValue ([] (const yup::String& text) { return static_cast<float> (SynthFormat::parse (text)); })
        .build();
}

yup::AudioParameter::Ptr createChoice (const yup::String& id,
                                       const yup::String& name,
                                       yup::uint32 hostID,
                                       const yup::String& modulePath,
                                       const yup::StringArray& choices,
                                       int defaultIndex)
{
    return makeBuilder (id, name, hostID, modulePath)
        .withRange (yup::NormalisableRange<float> (0.0f, static_cast<float> (choices.size() - 1), 1.0f))
        .withDefault (static_cast<float> (defaultIndex))
        .withStepped (true)
        .withEnum (true)
        .withValueToString ([choices] (float value) { return choices[yup::roundToInt (value)]; })
        .withStringToValue ([choices] (const yup::String& text) { return static_cast<float> (yup::jmax (0, choices.indexOf (text))); })
        .build();
}
} // namespace SynthParameterFactory
