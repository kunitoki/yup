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
*/

namespace yup
{

namespace
{

//==============================================================================

String defaultToString (float value)
{
    return String (value, 2);
}

float defaultFromString (const String& string)
{
    return string.getFloatValue();
}

} // namespace

//==============================================================================

AudioParameter::AudioParameter (const String& id,
                                const String& name,
                                float minValue,
                                float maxValue,
                                float defaultValue,
                                ValueToString valueToString,
                                StringToValue stringToValue,
                                bool smoothingEnabled,
                                float smoothingTimeMs)
    : paramID (id)
    , paramName (name)
    , valueRange (minValue, maxValue)
    , defaultValue (defaultValue)
    , valueToString (valueToString ? valueToString : defaultToString)
    , stringToValue (stringToValue ? stringToValue : defaultFromString)
    , smoothingEnabled (smoothingEnabled)
    , smoothingTimeMs (smoothingTimeMs)
{
    setValue (defaultValue);
}

AudioParameter::AudioParameter (const String& id,
                                const String& name,
                                NormalisableRange<float> valueRange,
                                float defaultValue,
                                ValueToString valueToString,
                                StringToValue stringToValue,
                                bool smoothingEnabled,
                                float smoothingTimeMs)
    : paramID (id)
    , paramName (name)
    , valueRange (std::move (valueRange))
    , defaultValue (defaultValue)
    , valueToString (valueToString ? valueToString : defaultToString)
    , stringToValue (stringToValue ? stringToValue : defaultFromString)
    , smoothingEnabled (smoothingEnabled)
    , smoothingTimeMs (smoothingTimeMs)
{
    setValue (defaultValue);
}

AudioParameter::~AudioParameter()
{
    jassert (isInsideGesture == 0); // Unbalanced calls to begin and end change gesture found!
}

//==============================================================================

void AudioParameter::beginChangeGesture()
{
    ++isInsideGesture;

    if (isInsideGesture == 1)
        listeners.call (&Listener::parameterGestureBegin, this, paramIndex);
}

void AudioParameter::endChangeGesture()
{
    jassert (isInsideGesture > 0); // Unbalanced calls to begin and end change gesture found!

    --isInsideGesture;

    if (isInsideGesture == 0)
        listeners.call (&Listener::parameterGestureEnd, this, paramIndex);
}

//==============================================================================

void AudioParameter::setValueNotifyingHost (float value)
{
    setValue (value);

    listeners.call (&Listener::parameterValueChanged, this, paramIndex);
}

//==============================================================================

void AudioParameter::addListener (Listener* listener)
{
    listeners.add (listener);
}

void AudioParameter::removeListener (Listener* listener)
{
    listeners.remove (listener);
}

} // namespace yup
