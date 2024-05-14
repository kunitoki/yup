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
class JUCE_API MouseWheelData
{
public:
    //==============================================================================
    constexpr MouseWheelData() noexcept = default;

    constexpr MouseWheelData (float deltaX, float deltaY) noexcept
        : deltaX (deltaX)
        , deltaY (deltaY)
    {
    }

    //==============================================================================
    constexpr MouseWheelData (const MouseWheelData& other) noexcept = default;
    constexpr MouseWheelData (MouseWheelData&& other) noexcept = default;
    constexpr MouseWheelData& operator= (const MouseWheelData& other) noexcept = default;
    constexpr MouseWheelData& operator= (MouseWheelData&& other) noexcept = default;

    //==============================================================================
    constexpr float getDeltaX() const noexcept
    {
        return deltaX;
    }

    constexpr MouseWheelData& setDeltaX (float newDeltaX) noexcept
    {
        deltaX = newDeltaX;
        return *this;
    }

    constexpr MouseWheelData withDeltaX (float newDeltaX) const noexcept
    {
        return { newDeltaX, deltaY };
    }

    //==============================================================================
    constexpr float getDeltaY() const noexcept
    {
        return deltaY;
    }

    constexpr MouseWheelData& setDeltaY (float newDeltaY) noexcept
    {
        deltaY = newDeltaY;
        return *this;
    }

    constexpr MouseWheelData withDeltaY (float newDeltaY) const noexcept
    {
        return { deltaX, newDeltaY };
    }

    //==============================================================================
    constexpr bool operator== (const MouseWheelData& other) const noexcept
    {
        return deltaX == other.deltaX && deltaY == other.deltaY;
    }

    constexpr bool operator!= (const MouseWheelData& other) const noexcept
    {
        return !(*this == other);
    }

private:
    float deltaX = 0.0f;
    float deltaY = 0.0f;
};

} // namespace yup
