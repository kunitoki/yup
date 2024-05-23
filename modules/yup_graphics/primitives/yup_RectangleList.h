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

template <class ValueType>
class JUCE_API RectangleList
{
public:
    //==============================================================================
    /** Default constructor, initializes an empty list of rectangles. */
    RectangleList() = default;

    //==============================================================================
    /** Adds a rectangle to the list.

        @param rect The rectangle to add.
    */
    void add (const Rectangle<ValueType>& rect)
    {
        rectangles.addIfNotAlreadyThere (rect);
    }

    /** Removes a rectangle from the list.

        @param rect The rectangle to remove.
    */
    void remove (const Rectangle<ValueType>& rect)
    {
        rectangles.removeAllInstancesOf (rect);
    }

    //==============================================================================
    /** Clears all rectangles from the list. */
    void clear()
    {
        rectangles.clear();
    }

    //==============================================================================
    /** Checks if the list contains a specified rectangle.

        @param rect The rectangle to check for.

        @return True if the rectangle is in the list, otherwise false.
    */
    bool contains (const Rectangle<ValueType>& rect) const
    {
        return rectangles.contains (rect);
    }

    bool contains (ValueType x, ValueType y) const
    {
        for (const auto& rect : rectangles)
        {
            if (rect.contains (x, y))
                return true;
        }

        return false;
    }

    bool contains (const Point<ValueType>& point) const
    {
        return contains (point.getX(), point.getY());
    }

    //==============================================================================
    /** Returns the number of rectangles in the list.

        @return The number of rectangles.
    */
    int size() const
    {
        return rectangles.size();
    }

    //==============================================================================
    /** Returns true if the list is empty.

        @return True if the list is empty, otherwise false.
    */
    bool isEmpty() const
    {
        return rectangles.isEmpty();
    }

    /** Returns a reference to the rectangle at a specified index.

        @param index The index of the rectangle to return.

        @return A reference to the rectangle.
    */
    Rectangle<ValueType>& operator[] (int index)
    {
        return rectangles[index];
    }

    /** Returns a const reference to the rectangle at a specified index.

        @param index The index of the rectangle to return.

        @return A const reference to the rectangle.
    */
    const Rectangle<ValueType>& operator[] (int index) const
    {
        return rectangles[index];
    }

    /** Returns the list of rectangles.

        @return A const reference to the vector of rectangles.
    */
    const Span<Rectangle<ValueType>>& getRectangles() const
    {
        return rectangles;
    }

    /** Translates all rectangles in the list by the specified x and y offsets.

        @param deltaX The amount to add to the x-coordinates.
        @param deltaY The amount to add to the y-coordinates.
    */
    void translateAll (ValueType deltaX, ValueType deltaY)
    {
        for (auto& rect : rectangles)
            rect.translate (deltaX, deltaY);
    }

    /** Scales all rectangles in the list by the specified factor.

        @param factor The scaling factor.
    */
    void scaleAll (float factor)
    {
        for (auto& rect : rectangles)
            rect.scale (factor);
    }

private:
    Array<Rectangle<ValueType>> rectangles;
};

} // namespace yup
