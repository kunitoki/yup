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
    MouseWheelData() noexcept = default;
    MouseWheelData (float deltaX, float deltaY) noexcept;

    //==============================================================================
    MouseWheelData (const MouseWheelData& other) noexcept = default;
    MouseWheelData (MouseWheelData&& other) noexcept = default;
    MouseWheelData& operator= (const MouseWheelData& other) noexcept = default;
    MouseWheelData& operator= (MouseWheelData&& other) noexcept = default;

    //==============================================================================
    float getDeltaX() const noexcept;
    MouseWheelData& setDeltaX (float newDeltaX) noexcept;
    MouseWheelData withDeltaX (float newDeltaX) const noexcept;

    float getDeltaY() const noexcept;
    MouseWheelData& setDeltaY (float newDeltaY) noexcept;
    MouseWheelData withDeltaY (float newDeltaY) const noexcept;

    //==============================================================================
    bool operator== (const MouseWheelData& other) const noexcept;
    bool operator!= (const MouseWheelData& other) const noexcept;

private:
    float deltaX = 0.0f;
    float deltaY = 0.0f;
};

} // namespace yup
