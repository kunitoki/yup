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
/** A class that manages a list of rectangles.

    This class provides a collection of rectangles and supports operations such as adding
    rectangles (with optional merging), removing rectangles, checking for containment and
    intersection, scaling, and offsetting. It also provides methods to get the bounding
    box of all rectangles, the number of rectangles, and access to individual rectangles.

    @code
      RectangleList<int> list;

      list.add ({ 0, 0, 10, 10 });
      list.add ({ 5, 5, 15, 15 });

      bool contains = list.contains (7, 7);
    @endcode
*/
template <class ValueType>
class JUCE_API RectangleList
{
public:
    using RectangleType = Rectangle<ValueType>;

    //==============================================================================
    /** Default constructor, initializes an empty list of rectangles. */
    RectangleList() = default;

    /** Construct from initializer list of rectangles. */
    RectangleList (std::initializer_list<RectangleType> rects)
        : rectangles (rects)
    {
    }

    /** Construct from initializer list of rectangles of different type. */
    template <class OtherRectangleType, class = std::enable_if_t<! std::is_same_v<OtherRectangleType, RectangleType>>>
    RectangleList (std::initializer_list<OtherRectangleType> rects)
    {
        for (auto& otherRect : rects)
            rectangles.add (otherRect.to<ValueType>());
    }

    //==============================================================================
    /** Copy and move constructors and assignment operators. */
    constexpr RectangleList (const RectangleList& other) noexcept = default;
    constexpr RectangleList (RectangleList&& other) noexcept = default;
    constexpr RectangleList& operator= (const RectangleList& other) noexcept = default;
    constexpr RectangleList& operator= (RectangleList&& other) noexcept = default;

    //==============================================================================
    /** Adds a rectangle to the list.

        @param rect The rectangle to add.

        @return A reference to the RectangleList object.
    */
    RectangleList& add (const RectangleType& newRect)
    {
        bool merged = false;

        for (auto& existingRect : rectangles)
        {
            if (existingRect.intersects (newRect))
            {
                existingRect = existingRect.smallestContainingRectangle (newRect);
                merged = true;
                break;
            }
        }

        if (! merged)
        {
            addWithoutMerge (newRect);
        }
        else
        {
            mergeRectangles();
        }

        return *this;
    }

    /** Adds a rectangle to the list without merging it with any existing rectangles.

        @param rect The rectangle to add.

        @return A reference to the RectangleList object.
    */
    RectangleList& addWithoutMerge (const RectangleType& newRect)
    {
        rectangles.addIfNotAlreadyThere (newRect);

        return *this;
    }

    /** Removes a rectangle from the list.

        @param rect The rectangle to remove.

        @return A reference to the RectangleList object.
    */
    RectangleList& remove (const RectangleType& rect)
    {
        rectangles.removeAllInstancesOf (rect);

        return *this;
    }

    //==============================================================================
    /** Returns true if the list is empty.

        @return True if the list is empty, otherwise false.
    */
    [[nodiscard]] bool isEmpty() const
    {
        return rectangles.isEmpty();
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
    [[nodiscard]] bool contains (ValueType x, ValueType y, ValueType width, ValueType height) const
    {
        return rectangles.contains (x, y, width, height);
    }

    [[nodiscard]] bool contains (const RectangleType& rect) const
    {
        return rectangles.contains (rect);
    }

    /** Checks if the given point (x, y) is contained within any of the rectangles in the list.

        @param x The x-coordinate of the point to check.
        @param y The y-coordinate of the point to check.

        @return true if the point is contained within any rectangle in the list, false otherwise.
    */
    [[nodiscard]] bool contains (ValueType x, ValueType y) const
    {
        for (const auto& rect : rectangles)
        {
            if (rect.contains (x, y))
                return true;
        }

        return false;
    }


    [[nodiscard]] bool contains (const Point<ValueType>& point) const
    {
        return contains (point.getX(), point.getY());
    }

    //==============================================================================
    /** Checks if the list intersects a specified rectangle.

        @param rect The rectangle to check for.

        @return True if the rectangle intersects the list, otherwise false.
    */
    [[nodiscard]] bool intersects (ValueType x, ValueType y, ValueType width, ValueType height) const
    {
        for (const auto& rect : rectangles)
        {
            if (rect.intersects (x, y, width, height))
                return true;
        }

        return false;
    }

    [[nodiscard]] bool intersects (const RectangleType& rect) const
    {
        return intersects (rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    }

    //==============================================================================
    /** Returns the number of rectangles in the list.

        @return The number of rectangles.
    */
    [[nodiscard]] int getNumRectangles() const
    {
        return rectangles.size();
    }

    //==============================================================================
    /** Returns one of the rectangles at a specified index.

        @param index The index of the rectangle to return.

        @return A new instance of the requested rectangle.
    */
    [[nodiscard]] RectangleType getRectangle (int index) const
    {
        jassert (isPositiveAndBelow (index, rectangles.size()));

        return rectangles[index];
    }

    //==============================================================================
    /** Returns the list of rectangles.

        @return A const reference to the vector of rectangles.
    */
    [[nodiscard]] Span<const RectangleType> getRectangles() const
    {
        return rectangles;
    }

    //==============================================================================
    /** Returns the bounding box of the whole list of rectangles.

        @return A new instance of the requested rectangle.
    */
    [[nodiscard]] RectangleType getBoundingBox() const
    {
        ValueType minX = std::numeric_limits<ValueType>::max();
        ValueType maxX = std::numeric_limits<ValueType>::min();
        ValueType minY = std::numeric_limits<ValueType>::max();
        ValueType maxY = std::numeric_limits<ValueType>::min();

        for (const auto& rect : rectangles)
        {
            if (minX > rect.getX())
                minX = rect.getX();
            if (minY > rect.getY())
                minY = rect.getY();
            if (maxX > rect.getX() + rect.getWidth())
                maxX = rect.getX() + rect.getWidth();
            if (maxY > rect.getY() + rect.getHeight())
                maxY = rect.getY() + rect.getHeight();
        }

        return { minX, minY, maxX - minX, maxY - minY };
    }

    //==============================================================================
    /** Offset all rectangles in the list by the specified x and y offsets.

        @param deltaX The amount to add to the x-coordinates.
        @param deltaY The amount to add to the y-coordinates.
    */
    RectangleList& offset (ValueType deltaX, ValueType deltaY)
    {
        for (auto& rect : rectangles)
            rect.translate (deltaX, deltaY);

        return *this;
    }

    /** Offset all rectangles in the list by the specified point.

        @param delta The point containing the amounts to add to the x and y coordinates.
    */
    RectangleList& offset (const Point<ValueType>& delta)
    {
        for (auto& rect : rectangles)
            rect.translate (delta);

        return *this;
    }

    //==============================================================================
    /** Scales all rectangles in the list by the specified factor.

        @param factor The scaling factor.
    */
    RectangleList& scale (float factor)
    {
        for (auto& rect : rectangles)
            rect.scale (factor);

        return *this;
    }

    /** Scales all rectangles in the list by the specified factor.

        @param factor The scaling factor.
    */
    RectangleList& scale (float factorX, float factorY)
    {
        for (auto& rect : rectangles)
            rect.scale (factorX, factorY);

        return *this;
    }

    //==============================================================================
    const Rectangle<ValueType>* begin() const
    {
        return rectangles.begin();
    }

    const Rectangle<ValueType>* end() const
    {
        return rectangles.end();
    }

    Rectangle<ValueType>* begin()
    {
        return rectangles.begin();
    }

    Rectangle<ValueType>* end()
    {
        return rectangles.end();
    }

private:
    void mergeRectangles()
    {
        for (auto it = rectangles.begin(); it != rectangles.end();)
        {
            bool furtherMerged = false;

            for (auto& existingRect : rectangles)
            {
                if (it != rectangles.end() && it->intersects (existingRect) && std::addressof (existingRect) != std::addressof (*it))
                {
                    existingRect = existingRect.smallestContainingRectangle (*it);

                    it = rectangles.erase (it);

                    furtherMerged = true;
                    break;
                }
            }

            if (! furtherMerged)
                ++it;
        }
    }

    Array<Rectangle<ValueType>> rectangles;
};

} // namespace yup
