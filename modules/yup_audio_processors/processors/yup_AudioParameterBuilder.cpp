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

namespace yup
{

//==============================================================================

AudioParameterBuilder& AudioParameterBuilder::withID (const String& paramID)
{
    id = paramID;
    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withName (const String& paramName)
{
    name = paramName;
    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withRange (float minValue, float maxValue)
{
    valueRange = { minValue, maxValue };
    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withRange (NormalisableRange<float> valueRange)
{
    this->valueRange = std::move (valueRange);
    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withDefault (float defaultValue)
{
    this->defaultValue = defaultValue;
    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withValueToString (AudioParameter::ValueToString fn)
{
    valueToString = std::move (fn);
    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withStringToValue (AudioParameter::StringToValue fn)
{
    stringToValue = std::move (fn);
    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withSmoothing (float smoothingTimeMs)
{
    smoothingEnabled = true;
    this->smoothingTimeMs = smoothingTimeMs;
    return *this;
}

//==============================================================================

AudioParameter::Ptr AudioParameterBuilder::build() const
{
    jassert (!id.isEmpty() && !name.isEmpty());

    return AudioParameter::Ptr (new AudioParameter(
        id,
        name,
        valueRange,
        valueRange.snapToLegalValue (defaultValue),
        std::move (valueToString),
        std::move (stringToValue),
        smoothingEnabled,
        smoothingTimeMs));
}

} // namespace yup
