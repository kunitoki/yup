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

namespace yup
{

MPEValue::MPEValue() noexcept {}

MPEValue::MPEValue (int value)
    : normalisedValue (value)
{
}

//==============================================================================
MPEValue MPEValue::from7BitInt (int value) noexcept
{
    jassert (value >= 0 && value <= 127);

    auto valueAs14Bit = value <= 64 ? value << 7
                                    : int (jmap<float> (float (value - 64), 0.0f, 63.0f, 0.0f, 8191.0f)) + 8192;

    return { valueAs14Bit };
}

MPEValue MPEValue::from14BitInt (int value) noexcept
{
    jassert (value >= 0 && value <= 16383);
    return { value };
}

MPEValue MPEValue::fromUnsignedFloat (float value) noexcept
{
    jassert (0.0f <= value && value <= 1.0f);
    return { roundToInt (value * 16383.0f) };
}

MPEValue MPEValue::fromSignedFloat (float value) noexcept
{
    jassert (-1.0f <= value && value <= 1.0f);
    return { roundToInt (((value + 1.0f) * 16383.0f) / 2.0f) };
}

//==============================================================================
MPEValue MPEValue::minValue() noexcept { return MPEValue::from7BitInt (0); }

MPEValue MPEValue::centreValue() noexcept { return MPEValue::from7BitInt (64); }

MPEValue MPEValue::maxValue() noexcept { return MPEValue::from7BitInt (127); }

int MPEValue::as7BitInt() const noexcept
{
    return normalisedValue >> 7;
}

int MPEValue::as14BitInt() const noexcept
{
    return normalisedValue;
}

//==============================================================================
float MPEValue::asSignedFloat() const noexcept
{
    return (normalisedValue < 8192)
             ? jmap<float> (float (normalisedValue), 0.0f, 8192.0f, -1.0f, 0.0f)
             : jmap<float> (float (normalisedValue), 8192.0f, 16383.0f, 0.0f, 1.0f);
}

float MPEValue::asUnsignedFloat() const noexcept
{
    return jmap<float> (float (normalisedValue), 0.0f, 16383.0f, 0.0f, 1.0f);
}

//==============================================================================
bool MPEValue::operator== (const MPEValue& other) const noexcept
{
    return normalisedValue == other.normalisedValue;
}

bool MPEValue::operator!= (const MPEValue& other) const noexcept
{
    return ! operator== (other);
}

} // namespace yup
