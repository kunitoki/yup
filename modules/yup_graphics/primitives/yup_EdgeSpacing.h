/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#pragma once

namespace yup
{

//==============================================================================
/** Represents the spacing around the four edges of a rectangle.

    An EdgeSpacing holds independent left, top, right and bottom amounts, each
    measured inwards (or outwards) from the matching edge of a Rectangle. It is the
    value type used for margins, paddings, insets and borders: subtract it from a
    rectangle to get the content area inside it, add it to grow a rectangle by it.

    @code
    EdgeSpacing<float> padding (8.0f, 4.0f); // 8 left/right, 4 top/bottom

    auto content = padding.subtractedFrom (getLocalBounds());
    auto sameContent = getLocalBounds().reduced (padding);
    @endcode

    @tparam ValueType The numeric type of the four amounts (typically int or float).

    @see Rectangle::reduced, Rectangle::enlarged
*/
template <class ValueType>
class YUP_API EdgeSpacing
{
public:
    //==============================================================================
    /** Creates an EdgeSpacing with zero spacing on every edge. */
    constexpr EdgeSpacing() noexcept = default;

    /** Creates an EdgeSpacing with the same amount on every edge.

        @param amount The spacing applied to the left, top, right and bottom edges.
    */
    constexpr explicit EdgeSpacing (ValueType amount) noexcept
        : left (amount)
        , top (amount)
        , right (amount)
        , bottom (amount)
    {
    }

    /** Creates an EdgeSpacing with separate horizontal and vertical amounts.

        @param horizontal The spacing applied to the left and right edges.
        @param vertical The spacing applied to the top and bottom edges.
    */
    constexpr EdgeSpacing (ValueType horizontal, ValueType vertical) noexcept
        : left (horizontal)
        , top (vertical)
        , right (horizontal)
        , bottom (vertical)
    {
    }

    /** Creates an EdgeSpacing with an independent amount on each edge.

        @param newLeft The spacing applied to the left edge.
        @param newTop The spacing applied to the top edge.
        @param newRight The spacing applied to the right edge.
        @param newBottom The spacing applied to the bottom edge.
    */
    constexpr EdgeSpacing (ValueType newLeft, ValueType newTop, ValueType newRight, ValueType newBottom) noexcept
        : left (newLeft)
        , top (newTop)
        , right (newRight)
        , bottom (newBottom)
    {
    }

    /** Creates an EdgeSpacing converting the amounts of another value type.

        @param other The EdgeSpacing to convert.
    */
    template <class T, class = std::enable_if_t<! std::is_same_v<T, ValueType>>>
    constexpr explicit EdgeSpacing (const EdgeSpacing<T>& other) noexcept
        : left (static_cast<ValueType> (other.getLeft()))
        , top (static_cast<ValueType> (other.getTop()))
        , right (static_cast<ValueType> (other.getRight()))
        , bottom (static_cast<ValueType> (other.getBottom()))
    {
    }

    /** Copy and move constructors and assignment operators. */
    constexpr EdgeSpacing (const EdgeSpacing& other) noexcept = default;
    constexpr EdgeSpacing (EdgeSpacing&& other) noexcept = default;
    constexpr EdgeSpacing& operator= (const EdgeSpacing& other) noexcept = default;
    constexpr EdgeSpacing& operator= (EdgeSpacing&& other) noexcept = default;

    //==============================================================================
    /** Returns the spacing at the left edge. */
    [[nodiscard]] constexpr ValueType getLeft() const noexcept { return left; }

    /** Returns the spacing at the top edge. */
    [[nodiscard]] constexpr ValueType getTop() const noexcept { return top; }

    /** Returns the spacing at the right edge. */
    [[nodiscard]] constexpr ValueType getRight() const noexcept { return right; }

    /** Returns the spacing at the bottom edge. */
    [[nodiscard]] constexpr ValueType getBottom() const noexcept { return bottom; }

    /** Returns the total horizontal spacing, the sum of the left and right amounts. */
    [[nodiscard]] constexpr ValueType getHorizontal() const noexcept { return left + right; }

    /** Returns the total vertical spacing, the sum of the top and bottom amounts. */
    [[nodiscard]] constexpr ValueType getVertical() const noexcept { return top + bottom; }

    //==============================================================================
    /** Sets the spacing at the left edge.

        @return A reference to this EdgeSpacing to allow method chaining.
    */
    constexpr EdgeSpacing& setLeft (ValueType newLeft) noexcept
    {
        left = newLeft;
        return *this;
    }

    /** Sets the spacing at the top edge.

        @return A reference to this EdgeSpacing to allow method chaining.
    */
    constexpr EdgeSpacing& setTop (ValueType newTop) noexcept
    {
        top = newTop;
        return *this;
    }

    /** Sets the spacing at the right edge.

        @return A reference to this EdgeSpacing to allow method chaining.
    */
    constexpr EdgeSpacing& setRight (ValueType newRight) noexcept
    {
        right = newRight;
        return *this;
    }

    /** Sets the spacing at the bottom edge.

        @return A reference to this EdgeSpacing to allow method chaining.
    */
    constexpr EdgeSpacing& setBottom (ValueType newBottom) noexcept
    {
        bottom = newBottom;
        return *this;
    }

    //==============================================================================
    /** Returns a copy of this EdgeSpacing with a different left amount. */
    [[nodiscard]] constexpr EdgeSpacing withLeft (ValueType newLeft) const noexcept
    {
        return { newLeft, top, right, bottom };
    }

    /** Returns a copy of this EdgeSpacing with a different top amount. */
    [[nodiscard]] constexpr EdgeSpacing withTop (ValueType newTop) const noexcept
    {
        return { left, newTop, right, bottom };
    }

    /** Returns a copy of this EdgeSpacing with a different right amount. */
    [[nodiscard]] constexpr EdgeSpacing withRight (ValueType newRight) const noexcept
    {
        return { left, top, newRight, bottom };
    }

    /** Returns a copy of this EdgeSpacing with a different bottom amount. */
    [[nodiscard]] constexpr EdgeSpacing withBottom (ValueType newBottom) const noexcept
    {
        return { left, top, right, newBottom };
    }

    //==============================================================================
    /** Returns true if every edge has zero spacing. */
    [[nodiscard]] constexpr bool isZero() const noexcept
    {
        return left == ValueType (0) && top == ValueType (0) && right == ValueType (0) && bottom == ValueType (0);
    }

    /** Returns true if every edge has the same spacing. */
    [[nodiscard]] constexpr bool isUniform() const noexcept
    {
        return left == top && top == right && right == bottom;
    }

    //==============================================================================
    /** Shrinks a rectangle in place by this spacing.

        The width and height are clamped to zero if the spacing exceeds them.

        @param rectangle The rectangle to shrink.
    */
    constexpr void subtractFrom (Rectangle<ValueType>& rectangle) const noexcept
    {
        rectangle.reduce (left, top, right, bottom);
    }

    /** Returns a rectangle shrunk by this spacing, the area inside the edges.

        The width and height are clamped to zero if the spacing exceeds them.

        @param rectangle The rectangle to shrink.

        @return The shrunk rectangle.
    */
    [[nodiscard]] constexpr Rectangle<ValueType> subtractedFrom (const Rectangle<ValueType>& rectangle) const noexcept
    {
        return rectangle.reduced (left, top, right, bottom);
    }

    /** Grows a rectangle in place by this spacing.

        @param rectangle The rectangle to grow.
    */
    constexpr void addTo (Rectangle<ValueType>& rectangle) const noexcept
    {
        rectangle.enlarge (left, top, right, bottom);
    }

    /** Returns a rectangle grown by this spacing.

        @param rectangle The rectangle to grow.

        @return The grown rectangle.
    */
    [[nodiscard]] constexpr Rectangle<ValueType> addedTo (const Rectangle<ValueType>& rectangle) const noexcept
    {
        auto result = rectangle;
        addTo (result);
        return result;
    }

    //==============================================================================
    /** Converts the amounts to another value type. */
    template <class T>
    [[nodiscard]] constexpr EdgeSpacing<T> to() const noexcept
    {
        return { static_cast<T> (left), static_cast<T> (top), static_cast<T> (right), static_cast<T> (bottom) };
    }

    /** Rounds every amount to the nearest integer. */
    template <class T = ValueType>
    [[nodiscard]] auto roundToInt() const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, EdgeSpacing<int>>
    {
        return { yup::roundToInt (left), yup::roundToInt (top), yup::roundToInt (right), yup::roundToInt (bottom) };
    }

    //==============================================================================
    /** Returns the edge-wise sum of two spacings. */
    [[nodiscard]] constexpr EdgeSpacing operator+ (const EdgeSpacing& other) const noexcept
    {
        return { left + other.left, top + other.top, right + other.right, bottom + other.bottom };
    }

    /** Adds another spacing to this one, edge by edge. */
    constexpr EdgeSpacing& operator+= (const EdgeSpacing& other) noexcept
    {
        return *this = *this + other;
    }

    /** Returns the edge-wise difference of two spacings. */
    [[nodiscard]] constexpr EdgeSpacing operator- (const EdgeSpacing& other) const noexcept
    {
        return { left - other.left, top - other.top, right - other.right, bottom - other.bottom };
    }

    /** Subtracts another spacing from this one, edge by edge. */
    constexpr EdgeSpacing& operator-= (const EdgeSpacing& other) noexcept
    {
        return *this = *this - other;
    }

    /** Returns a spacing with every amount negated. */
    [[nodiscard]] constexpr EdgeSpacing operator-() const noexcept
    {
        return { -left, -top, -right, -bottom };
    }

    /** Returns a spacing with every amount multiplied by a scale factor. */
    template <class T>
    [[nodiscard]] constexpr auto operator* (T scaleFactor) const noexcept
        -> std::enable_if_t<std::is_arithmetic_v<T>, EdgeSpacing>
    {
        return { static_cast<ValueType> (left * scaleFactor),
                 static_cast<ValueType> (top * scaleFactor),
                 static_cast<ValueType> (right * scaleFactor),
                 static_cast<ValueType> (bottom * scaleFactor) };
    }

    /** Multiplies every amount by a scale factor. */
    template <class T>
    constexpr auto operator*= (T scaleFactor) noexcept
        -> std::enable_if_t<std::is_arithmetic_v<T>, EdgeSpacing&>
    {
        return *this = *this * scaleFactor;
    }

    /** Returns a spacing with every amount divided by a scale factor.

        Dividing by zero is a programming error and returns the spacing unchanged.
    */
    template <class T>
    [[nodiscard]] constexpr auto operator/ (T scaleFactor) const noexcept
        -> std::enable_if_t<std::is_arithmetic_v<T>, EdgeSpacing>
    {
        jassert (scaleFactor != T (0));
        if (scaleFactor == T (0))
            return *this;

        return { static_cast<ValueType> (left / scaleFactor),
                 static_cast<ValueType> (top / scaleFactor),
                 static_cast<ValueType> (right / scaleFactor),
                 static_cast<ValueType> (bottom / scaleFactor) };
    }

    /** Divides every amount by a scale factor.

        Dividing by zero is a programming error and leaves the spacing unchanged.
    */
    template <class T>
    constexpr auto operator/= (T scaleFactor) noexcept
        -> std::enable_if_t<std::is_arithmetic_v<T>, EdgeSpacing&>
    {
        return *this = *this / scaleFactor;
    }

    //==============================================================================
    /** Returns the amounts as "left, top, right, bottom". */
    String toString() const
    {
        String result;
        result << left << ", " << top << ", " << right << ", " << bottom;
        return result;
    }

    //==============================================================================
    /** Returns true if the two spacings are approximately equal. */
    constexpr bool approximatelyEqualTo (const EdgeSpacing& other) const noexcept
    {
        if constexpr (std::is_floating_point_v<ValueType>)
        {
            return approximatelyEqual (left, other.left)
                && approximatelyEqual (top, other.top)
                && approximatelyEqual (right, other.right)
                && approximatelyEqual (bottom, other.bottom);
        }
        else
        {
            return *this == other;
        }
    }

    /** Returns true if every edge of the two spacings is equal. */
    constexpr bool operator== (const EdgeSpacing& other) const noexcept
    {
        return left == other.left && top == other.top && right == other.right && bottom == other.bottom;
    }

    /** Returns true if any edge of the two spacings differs. */
    constexpr bool operator!= (const EdgeSpacing& other) const noexcept
    {
        return ! (*this == other);
    }

private:
    ValueType left = 0;
    ValueType top = 0;
    ValueType right = 0;
    ValueType bottom = 0;
};

/** Appends the amounts of an EdgeSpacing to a String as "left, top, right, bottom". */
template <class ValueType>
YUP_API String& YUP_CALLTYPE operator<< (String& string1, const EdgeSpacing<ValueType>& s)
{
    return string1 << s.toString();
}

/** Returns the amount at the given index, in left, top, right, bottom order. */
template <std::size_t I, class ValueType>
constexpr ValueType get (const EdgeSpacing<ValueType>& s) noexcept
{
    if constexpr (I == 0)
        return s.getLeft();
    else if constexpr (I == 1)
        return s.getTop();
    else if constexpr (I == 2)
        return s.getRight();
    else if constexpr (I == 3)
        return s.getBottom();
    else
        static_assert (dependentFalse<I>);
}

} // namespace yup

#ifndef DOXYGEN
namespace std
{

template <class ValueType>
struct tuple_size<yup::EdgeSpacing<ValueType>>
{
    inline static constexpr std::size_t value = 4;
};

template <std::size_t I, class ValueType>
struct tuple_element<I, yup::EdgeSpacing<ValueType>>
{
    using type = ValueType;
};

} // namespace std
#endif
