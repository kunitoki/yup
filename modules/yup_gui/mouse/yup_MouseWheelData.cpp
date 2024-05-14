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

//==============================================================================

MouseWheelData::MouseWheelData (float deltaX, float deltaY) noexcept
    : deltaX (deltaX)
    , deltaY (deltaY)
{
}

//==============================================================================
float MouseWheelData::getDeltaX() const noexcept
{
    return deltaX;
}

MouseWheelData& MouseWheelData::setDeltaX (float newDeltaX) noexcept
{
    deltaX = newDeltaX;
    return *this;
}

MouseWheelData MouseWheelData::withDeltaX (float newDeltaX) const noexcept
{
    return { newDeltaX, deltaY };
}

float MouseWheelData::getDeltaY() const noexcept
{
    return deltaY;
}

MouseWheelData& MouseWheelData::setDeltaY (float newDeltaY) noexcept
{
    deltaY = newDeltaY;
    return *this;
}

MouseWheelData MouseWheelData::withDeltaY (float newDeltaY) const noexcept
{
    return { deltaX, newDeltaY };
}

//==============================================================================
bool MouseWheelData::operator== (const MouseWheelData& other) const noexcept
{
    return deltaX == other.deltaX && deltaY == other.deltaY;
}

bool MouseWheelData::operator!= (const MouseWheelData& other) const noexcept
{
    return !(*this == other);
}

} // namespace yup
