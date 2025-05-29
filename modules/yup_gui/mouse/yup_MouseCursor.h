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
/** Represents a mouse cursor image.

    This class is used to set the mouse cursor's appearance. You can create a custom
    cursor from an image, or use one of the standard system cursors.

    @see Component::setMouseCursor, Component::getMouseCursor
*/
class JUCE_API MouseCursor
{
public:
    /** The set of available standard system mouse cursors.

        @see MouseCursor::MouseCursor
    */
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
    /** Creates a default mouse cursor (an arrow). */
    MouseCursor() noexcept = default;

    /** Creates a standard system cursor.

        @param type    The type of cursor to create - this can be one of the values
                      from the Type enum.
        @see Type
    */
    MouseCursor (Type newType) noexcept;

    //==============================================================================
    /** Copy constructor. */
    MouseCursor (const MouseCursor& other) noexcept = default;

    /** Move constructor. */
    MouseCursor (MouseCursor&& other) noexcept = default;

    /** Copy assignment operator. */
    MouseCursor& operator= (const MouseCursor& other) noexcept = default;

    /** Move assignment operator. */
    MouseCursor& operator= (MouseCursor&& other) noexcept = default;

    //==============================================================================
    /** Checks if two cursors are the same.

        @param other    The other cursor to compare with
        @returns       True if the cursors are identical, false otherwise
    */
    bool operator== (const MouseCursor& other) const noexcept;

    /** Checks if two cursors are different.

        @param other    The other cursor to compare with
        @returns       True if the cursors are different, false if they're identical
    */
    bool operator!= (const MouseCursor& other) const noexcept;

    //==============================================================================
    /** Returns the type of this cursor.

        @returns    The cursor's type, as set when it was created.
        @see Type
    */
    Type getType() const noexcept { return currentType; }

private:
    Type currentType = Default;
};

} // namespace yup
