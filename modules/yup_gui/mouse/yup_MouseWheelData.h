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
/** Data associated with mouse wheel events.

    This class encapsulates information about the movement of the mouse wheel,
    including the amount of scrolling in both the horizontal and vertical directions.

    @see MouseListener::mouseWheel
*/
class YUP_API MouseWheelData
{
public:
    //==============================================================================
    /** Default constructor. */
    constexpr MouseWheelData() noexcept = default;

    /** Constructor with delta values. */
    constexpr MouseWheelData (float deltaX, float deltaY) noexcept
        : deltaX (deltaX)
        , deltaY (deltaY)
    {
    }

    //==============================================================================
    /** Copy constructor and assignment operators. */
    constexpr MouseWheelData (const MouseWheelData& other) noexcept = default;
    constexpr MouseWheelData (MouseWheelData&& other) noexcept = default;
    constexpr MouseWheelData& operator= (const MouseWheelData& other) noexcept = default;
    constexpr MouseWheelData& operator= (MouseWheelData&& other) noexcept = default;

    //==============================================================================
    /** Returns the horizontal scroll amount. */
    constexpr float getDeltaX() const noexcept
    {
        return deltaX;
    }

    /** Sets the horizontal scroll amount. 
    
        @param newDeltaX The new horizontal scroll amount.
        @return A reference to this MouseWheelData object.
    */
    constexpr MouseWheelData& setDeltaX (float newDeltaX) noexcept
    {
        deltaX = newDeltaX;
        return *this;
    }

    /** Returns a new MouseWheelData object with the specified horizontal scroll amount. 
    
        @param newDeltaX The new horizontal scroll amount.
        @return A new MouseWheelData object with the updated horizontal scroll amount.
    */
    constexpr MouseWheelData withDeltaX (float newDeltaX) const noexcept
    {
        return { newDeltaX, deltaY };
    }

    //==============================================================================
    /** Returns the vertical scroll amount. */
    constexpr float getDeltaY() const noexcept
    {
        return deltaY;
    }

    /** Sets the vertical scroll amount. 
    
        @param newDeltaY The new vertical scroll amount.
        @return A reference to this MouseWheelData object.
    */
    constexpr MouseWheelData& setDeltaY (float newDeltaY) noexcept
    {
        deltaY = newDeltaY;
        return *this;
    }

    /** Returns a new MouseWheelData object with the specified vertical scroll amount. 
    
        @param newDeltaY The new vertical scroll amount.
        @return A new MouseWheelData object with the updated vertical scroll amount.
    */
    constexpr MouseWheelData withDeltaY (float newDeltaY) const noexcept
    {
        return { deltaX, newDeltaY };
    }

    //==============================================================================
    /** Compares two MouseWheelData objects for equality.
    
        @return True if both objects have the same deltaX and deltaY values, false otherwise.
    */
    constexpr bool operator== (const MouseWheelData& other) const noexcept
    {
        return deltaX == other.deltaX && deltaY == other.deltaY;
    }

    /** Compares two MouseWheelData objects for inequality.
    
        @return True if the objects have different deltaX or deltaY values, false otherwise.
    */
    constexpr bool operator!= (const MouseWheelData& other) const noexcept
    {
        return ! (*this == other);
    }

private:
    float deltaX = 0.0f;
    float deltaY = 0.0f;
};

} // namespace yup
