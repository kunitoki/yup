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
class JUCE_API MouseCursor
{
public:
    enum Type
    {
        None = 0,
        Default,
        Arrow = Default,
        Text,
        Wait,
        WaitArrow,
        Hand,
        Crosshair,
        Crossbones,
        ResizeLeftRight,
        ResizeUpDown,
        ResizeTopLeftRightBottom,
        ResizeBottomLeftRightTop,
        ResizeAll
    };

    //==============================================================================
    MouseCursor() noexcept = default;
    MouseCursor (Type newType) noexcept;

    //==============================================================================
    MouseCursor (const MouseCursor& other) noexcept = default;
    MouseCursor (MouseCursor&& other) noexcept = default;
    MouseCursor& operator= (const MouseCursor& other) noexcept = default;
    MouseCursor& operator= (MouseCursor&& other) noexcept = default;

    //==============================================================================
    bool operator== (const MouseCursor& other) const noexcept;
    bool operator!= (const MouseCursor& other) const noexcept;

    //==============================================================================
    Type getType() const noexcept { return currentType; }

private:
    Type currentType = Default;
};

} // namespace yup
