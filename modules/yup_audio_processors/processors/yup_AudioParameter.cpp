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
                                Metadata metadata,
                                ValueToString valueToString,
                                StringToValue stringToValue)
    : paramID (id)
    , metadata (std::move (metadata))
    , valueToString (valueToString ? valueToString : defaultToString)
    , stringToValue (stringToValue ? stringToValue : defaultFromString)
{
    jassert (this->metadata.hostParameterID == invalidHostParameterID || this->metadata.hostParameterID <= maximumHostParameterID);

    setValue (this->metadata.defaultValue);
}

AudioParameter::~AudioParameter()
{
    jassert (isInsideGesture == 0); // Unbalanced calls to begin and end change gesture found!
}

//==============================================================================

void AudioParameter::beginChangeGesture()
{
    const auto newGestureDepth = isInsideGesture.fetch_add (1) + 1;

    if (newGestureDepth == 1)
        listeners.call (&Listener::parameterGestureBegin, this, paramIndex);
}

void AudioParameter::endChangeGesture()
{
    const auto currentGestureDepth = isInsideGesture.load();

    jassert (currentGestureDepth > 0); // Unbalanced calls to begin and end change gesture found!
    if (currentGestureDepth <= 0)
        return;

    const auto newGestureDepth = isInsideGesture.fetch_sub (1) - 1;

    if (newGestureDepth == 0)
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
