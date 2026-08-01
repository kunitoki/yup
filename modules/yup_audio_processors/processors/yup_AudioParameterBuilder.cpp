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

AudioParameterBuilder::AudioParameterBuilder() = default;

//==============================================================================

AudioParameterBuilder& AudioParameterBuilder::withID (const String& paramID)
{
    jassert (paramID.isNotEmpty());

    id = paramID;
    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withName (const String& paramName)
{
    jassert (paramName.isNotEmpty());

    metadata.name = paramName;
    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withHostID (uint32 hostParameterID)
{
    jassert (hostParameterID <= AudioParameter::maximumHostParameterID);

    metadata.hostParameterID = hostParameterID;
    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withRange (float minValue, float maxValue)
{
    metadata.valueRange = { minValue, maxValue };
    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withRange (NormalisableRange<float> valueRange)
{
    metadata.valueRange = std::move (valueRange);
    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withDefault (float defaultValue)
{
    metadata.defaultValue = defaultValue;
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
    if (smoothingTimeMs > 0.0f)
    {
        metadata.setSmoothingEnabled (true);
        metadata.smoothingTimeMs = smoothingTimeMs;
    }
    else
    {
        metadata.setSmoothingEnabled (false);
        metadata.smoothingTimeMs = 0.0f;
    }

    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withAutomatable (bool shouldBeAutomatable)
{
    if (shouldBeAutomatable && metadata.isReadOnly())
        metadata.setReadOnly (false);

    metadata.setAutomatable (shouldBeAutomatable);

    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withReadOnly (bool shouldBeReadOnly)
{
    if (shouldBeReadOnly && metadata.isAutomatable())
        metadata.setAutomatable (false);

    metadata.setReadOnly (shouldBeReadOnly);

    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withStepped (bool shouldBeStepped)
{
    if (! shouldBeStepped && metadata.isEnum())
        metadata.setEnum (false);

    metadata.setStepped (shouldBeStepped);

    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withEnum (bool shouldBeEnum)
{
    if (shouldBeEnum)
        metadata.setStepped (true);

    metadata.setEnum (shouldBeEnum);

    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withModulatable (bool shouldBeModulatable)
{
    metadata.setModulatable (shouldBeModulatable);
    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withPerNoteModulatable (bool shouldBePerNoteModulatable)
{
    if (shouldBePerNoteModulatable)
        metadata.setModulatable (true);

    metadata.setPerNoteModulatable (shouldBePerNoteModulatable);

    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withModulePath (const String& modulePath)
{
    metadata.modulePath = modulePath;
    return *this;
}

AudioParameterBuilder& AudioParameterBuilder::withUnit (AudioParameter::ParameterUnit parameterUnit,
                                                        const String& parameterUnitName)
{
    metadata.unit = parameterUnit;
    metadata.unitName = parameterUnitName;
    return *this;
}

//==============================================================================

AudioParameter::Ptr AudioParameterBuilder::build() const
{
    jassert (! id.isEmpty() && ! metadata.name.isEmpty());

    auto parameterMetadata = metadata;
    parameterMetadata.defaultValue = parameterMetadata.valueRange.snapToLegalValue (parameterMetadata.defaultValue);

    return AudioParameter::Ptr (new AudioParameter (id,
                                                    std::move (parameterMetadata),
                                                    valueToString,
                                                    stringToValue));
}

} // namespace yup
