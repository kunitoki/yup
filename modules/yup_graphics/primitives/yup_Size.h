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

template <class ValueType>
class YUP_API Rectangle;

//==============================================================================
/** Represents a 2D size with width and height.

    This template class encapsulates a 2-dimensional size defined by width and height. It provides methods
    to manipulate and query the size dimensions, such as getting and setting the width and height, checking
    if the size is zero, empty, square, or has specific orientation emptiness, and performing arithmetic
    operations to modify or compare sizes. It supports both copy and move operations efficiently with default
    constructors and assignment operators.

    @tparam ValueType The type of the width and height (typically numeric).
*/
template <class ValueType>
class YUP_API Size
{
public:
    //==============================================================================
    /** Constructs a default size object with zero width and height. */
    constexpr Size() noexcept = default;

    /** Constructs a size object with specified width and height.

        Initializes the size object to the given width and height values.

        @param newWidth The initial width of the size.
        @param newHeight The initial height of the size.
    */
    constexpr Size (ValueType newWidth, ValueType newHeight) noexcept
        : width (newWidth)
        , height (newHeight)
    {
    }

    //==============================================================================
    /** Copy and move constructors and assignment operators. */
    constexpr Size (const Size& other) noexcept = default;
    constexpr Size (Size&& other) noexcept = default;
    constexpr Size& operator= (const Size& other) noexcept = default;
    constexpr Size& operator= (Size&& other) noexcept = default;

    //==============================================================================
    /** Gets the width of the size.

        Returns the current width of the size object.

        @return The current width.
    */
    [[nodiscard]] constexpr ValueType getWidth() const noexcept
    {
        return width;
    }

    /** Sets the width of the size.

        Updates the width of the size object to the specified value and returns a reference to the size object.

        @param newWidth The new width to set.

        @return Reference to this size object.
    */
    constexpr Size& setWidth (ValueType newWidth) noexcept
    {
        width = newWidth;
        return *this;
    }

    /** Returns a new size object with the specified width.

        Creates a new size object with the given width while keeping the height of this size object unchanged.

        @param newWidth The new width for the created size object.

        @return A new size object with the specified width and current height.
    */
    [[nodiscard]] constexpr Size withWidth (ValueType newWidth) const noexcept
    {
        return { newWidth, height };
    }

    //==============================================================================
    /** Gets the height of the size.

        Returns the current height of the size object.

        @return The current height.
    */
    [[nodiscard]] constexpr ValueType getHeight() const noexcept
    {
        return height;
    }

    /** Sets the height of the size.

        Updates the height of the size object to the specified value and returns a reference to the size object.

        @param newHeight The new height to set.

        @return Reference to this size object.
    */
    constexpr Size& setHeight (ValueType newHeight) noexcept
    {
        height = newHeight;
        return *this;
    }

    /** Returns a new size object with the specified height.

        Creates a new size object with the given height while keeping the width of this size object unchanged.

        @param newHeight The new height for the created size object.

        @return A new size object with the current width and specified height.
    */
    [[nodiscard]] constexpr Size withHeight (ValueType newHeight) const noexcept
    {
        return { width, newHeight };
    }

    //==============================================================================
    /** Checks if both dimensions are zero.

        Determines if both the width and height of the size are zero, indicating a zero size.

        @return True if both width and height are zero, false otherwise.
    */
    [[nodiscard]] constexpr bool isZero() const noexcept
    {
        return width == ValueType (0) && height == ValueType (0);
    }

    /** Checks if either dimension is zero.

        Determines if either the width or height of the size is zero, indicating an empty dimension.

        @return True if either width or height is zero, false otherwise.
    */
    [[nodiscard]] constexpr bool isEmpty() const noexcept
    {
        return width == ValueType (0) || height == ValueType (0);
    }

    /** Checks if the height is zero but not the width.

        Determines if the height is zero while the width is not, indicating an empty vertical dimension.

        @return True if height is zero and width is not, false otherwise.
    */
    [[nodiscard]] constexpr bool isVerticallyEmpty() const noexcept
    {
        return width != ValueType (0) && height == ValueType (0);
    }

    /** Checks if the width is zero but not the height.

        Determines if the width is zero while the height is not, indicating an empty horizontal dimension.

        @return True if width is zero and height is not, false otherwise.
    */
    [[nodiscard]] constexpr bool isHorizontallyEmpty() const noexcept
    {
        return width == ValueType (0) && height != ValueType (0);
    }

    //==============================================================================
    /** Checks if the width and height are equal.

        Determines if the size is a square by checking if the width equals the height.

        @return True if the size is a square, false otherwise.
    */
    [[nodiscard]] constexpr bool isSquare() const noexcept
    {
        return width == height;
    }

    //==============================================================================
    /** Calculate area of the Size

        Calculates and returns the area of the Size object.

        @return The area calculated as width multiplied by height.
    */
    [[nodiscard]] constexpr ValueType area() const noexcept
    {
        return width * height;
    }

    //==============================================================================
    /** Reverse dimensions

        Swaps the width and height of the Size object.

        @return A reference to this Size object with its dimensions reversed.
    */
    constexpr Size& reverse() noexcept
    {
        using std::swap;

        swap (width, height);

        return *this;
    }

    /** Create a reversed Size object

        Creates a new Size object with reversed dimensions compared to this Size object.

        @return A new Size object with reversed dimensions.
    */
    [[nodiscard]] constexpr Size reversed() const noexcept
    {
        Size result (*this);
        result.reverse();
        return result;
    }

    //==============================================================================
    /** Enlarge dimensions uniformly

        Increases both the width and height of the Size object by a uniform amount and returns a reference to this Size object.

        @param amount The amount to add to both width and height.
        @return A reference to this updated Size object.
    */
    constexpr Size& enlarge (ValueType amount) noexcept
    {
        width += amount;
        height += amount;
        return *this;
    }

    /** Enlarge dimensions separately

        Increases the width and height of the Size object by separate amounts and returns a reference to this Size object.

        @param widthAmount The amount to add to the width.
        @param heightAmount The amount to add to the height.
        @return A reference to this updated Size object.
    */
    constexpr Size& enlarge (ValueType widthAmount, ValueType heightAmount) noexcept
    {
        width += widthAmount;
        height += heightAmount;
        return *this;
    }

    /** Create an enlarged Size object uniformly

        Creates a new Size object with both dimensions increased by a uniform amount based on this Size object.

        @param amount The amount to add to both width and height.

        @return A new Size object with enlarged dimensions.
    */
    [[nodiscard]] constexpr Size enlarged (ValueType amount) const noexcept
    {
        Size result (*this);
        result.enlarge (amount);
        return result;
    }

    /** Create an enlarged Size object separately

        Creates a new Size object with dimensions increased by separate amounts based on this Size object.

        @param widthAmount The amount to add to the width.
        @param heightAmount The amount to add to the height.

        @return A new Size object with enlarged dimensions.
    */
    [[nodiscard]] constexpr Size enlarged (ValueType widthAmount, ValueType heightAmount) const noexcept
    {
        Size result (*this);
        result.enlarge (widthAmount, heightAmount);
        return result;
    }

    //==============================================================================
    /** Reduce dimensions uniformly

        Decreases both the width and height of the Size object by a uniform amount and returns a reference to this Size object.

        @param amount The amount to subtract from both width and height.

        @return A reference to this updated Size object.
    */
    constexpr Size& reduce (ValueType amount) noexcept
    {
        width -= amount;
        height -= amount;
        return *this;
    }

    /** Reduce dimensions separately

        Decreases the width and height of the Size object by separate amounts and returns a reference to this Size object.

        @param widthAmount The amount to subtract from the width.
        @param heightAmount The amount to subtract from the height.

        @return A reference to this updated Size object.
    */
    constexpr Size& reduce (ValueType widthAmount, ValueType heightAmount) noexcept
    {
        width -= widthAmount;
        height -= heightAmount;
        return *this;
    }

    /** Create a reduced Size object uniformly

        Creates a new Size object with both dimensions decreased by a uniform amount based on this Size object.

        @param amount The amount to subtract from both width and height.

        @return A new Size object with reduced dimensions.
    */
    [[nodiscard]] constexpr Size reduced (ValueType amount) const noexcept
    {
        Size result (*this);
        result.reduce (amount);
        return result;
    }

    /** Create a reduced Size object separately

        Creates a new Size object with dimensions decreased by separate amounts based on this Size object.

        @param widthAmount The amount to subtract from the width.
        @param heightAmount The amount to subtract from the height.

        @return A new Size object with reduced dimensions.
    */
    [[nodiscard]] constexpr Size reduced (ValueType widthAmount, ValueType heightAmount) const noexcept
    {
        Size result (*this);
        result.reduce (widthAmount, heightAmount);
        return result;
    }

    //==============================================================================
    /** Scale Size dimensions

        Scales the dimensions of the Size object by a uniform factor and returns a reference to this Size object.

        @param scaleFactor The uniform scale factor to apply to both width and height.

        @return A reference to this updated Size object.
    */
    template <class T>
    constexpr auto scale (T scaleFactor) noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Size&>
    {
        scale (scaleFactor, scaleFactor);
        return *this;
    }

    /** Scale Size dimensions separately

        Scales the dimensions of the Size object by separate factors for width and height and returns a reference to this Size object.

        @param scaleFactorX The scale factor to apply to the width.
        @param scaleFactorY The scale factor to apply to the height.

        @return A reference to this updated Size object.
    */
    template <class T>
    constexpr auto scale (T scaleFactorX, T scaleFactorY) noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Size&>
    {
        width = static_cast<ValueType> (width * scaleFactorX);
        height = static_cast<ValueType> (height * scaleFactorY);
        return *this;
    }

    /** Create a scaled Size object

        Creates a new Size object with dimensions scaled by a uniform factor based on this Size object.

        @param scaleFactor The uniform scale factor to apply to both width and height.

        @return A new Size object with scaled dimensions.
    */
    template <class T>
    [[nodiscard]] constexpr auto scaled (T scaleFactor) const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Size>
    {
        Size result (*this);
        result.scale (scaleFactor);
        return result;
    }

    /** Create a scaled Size object separately

        Creates a new Size object with dimensions scaled by separate factors for width and height based on this Size object.

        @param scaleFactorX The scale factor to apply to the width.
        @param scaleFactorY The scale factor to apply to the height.

        @return A new Size object with scaled dimensions.
    */
    template <class T>
    [[nodiscard]] constexpr auto scaled (T scaleFactorX, T scaleFactorY) const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Size>
    {
        Size result (*this);
        result.scale (scaleFactorX, scaleFactorY);
        return result;
    }

    //==============================================================================
    /** Convert Size to another type

        Converts the dimensions of this Size object to another numeric type and returns a new Size object of that type.

        @tparam T The type to convert the dimensions to.

        @return A new Size object with dimensions converted to type T.
    */
    template <class T>
    [[nodiscard]] constexpr Size<T> to() const noexcept
    {
        return { static_cast<T> (width), static_cast<T> (height) };
    }

    /** Round the dimensions of this Size object to integers

        Rounds the dimensions of this Size object to integers and returns a new Size object with the rounded dimensions.

        @tparam T The type of the dimensions, constrained to floating-point types.

        @return A new Size object with the rounded dimensions.
    */
    template <class T = ValueType>
    [[nodiscard]] auto roundToInt() const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Size<int>>
    {
        return { yup::roundToInt (width), yup::roundToInt (height) };
    }

    //==============================================================================
    /** Convert Size to Point

        Converts the dimensions of this Size object to a Point object and returns a new Point object with the converted dimensions.

        @tparam T The type of the dimensions, constrained to numeric types.

        @return A new Point object with the converted dimensions.
    */
    template <class T>
    [[nodiscard]] constexpr Point<T> toPoint() const noexcept
    {
        return { static_cast<T> (width), static_cast<T> (height) };
    }

    /** Convert Size to Rectangle

        Converts the dimensions of this Size object to a Rectangle object and returns a new Rectangle object with the converted dimensions.

        @tparam T The type of the dimensions, constrained to numeric types.

        @return A new Rectangle object with the converted dimensions.
    */
    template <class T>
    [[nodiscard]] constexpr Rectangle<T> toRectangle() const noexcept;

    /** Convert Size to Rectangle with specified coordinates

        Converts the dimensions of this Size object to a Rectangle object with specified coordinates and returns a new Rectangle object with the converted dimensions.

        @tparam T The type of the dimensions, constrained to numeric types.

        @param x The x-coordinate of the top-left corner of the Rectangle.
        @param y The y-coordinate of the top-left corner of the Rectangle.

        @return A new Rectangle object with the converted dimensions.
    */
    template <class T>
    [[nodiscard]] constexpr Rectangle<T> toRectangle (T x, T y) const noexcept;

    /** Convert Size to Rectangle with specified coordinates

        Converts the dimensions of this Size object to a Rectangle object with specified coordinates and returns a new Rectangle object with the converted dimensions.

        @tparam T The type of the dimensions, constrained to numeric types.

        @param xy The Point object containing the x and y coordinates of the top-left corner of the Rectangle.

        @return A new Rectangle object with the converted dimensions.
    */
    template <class T>
    [[nodiscard]] constexpr Rectangle<T> toRectangle (Point<T> xy) const noexcept;

    //==============================================================================
    /** Multiplication operator

        Multiplies the dimensions of this Size object by a scale factor and returns a new Size object with the scaled dimensions.

        @tparam T The type of the scale factor.

        @param scaleFactor The scale factor to multiply the dimensions by.

        @return A new Size object with dimensions scaled by the given factor.
    */
    template <class T>
    [[nodiscard]] constexpr Size operator* (T scaleFactor) const noexcept
    {
        Size result (*this);
        result *= scaleFactor;
        return result;
    }

    /** Multiplication assignment operator

        Multiplies the dimensions of this Size object by a scale factor and updates this Size object.

        @tparam T The type of the scale factor.

        @param scaleFactor The scale factor to multiply the dimensions by.

        @return A reference to this updated Size object.
    */
    template <class T>
    constexpr auto operator*= (T scaleFactor) noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Size&>
    {
        width = static_cast<ValueType> (width * scaleFactor);
        height = static_cast<ValueType> (height * scaleFactor);
        return *this;
    }

    /** Multiplication assignment operator

        Multiplies the dimensions of this Size object by a scale factor and updates this Size object.

        @tparam T The type of the scale factor.

        @param scaleFactor The scale factor to multiply the dimensions by.

        @return A reference to this updated Size object.
    */
    template <class T>
    constexpr auto operator*= (T scaleFactor) noexcept
        -> std::enable_if_t<std::is_integral_v<T>, Size&>
    {
        width = static_cast<ValueType> (width * static_cast<ValueType> (scaleFactor));
        height = static_cast<ValueType> (height * static_cast<ValueType> (scaleFactor));
        return *this;
    }

    /** Division operator

        Divides the dimensions of this Size object by a scale factor and returns a new Size object with the scaled dimensions.

        @tparam T The type of the scale factor.

        @param scaleFactor The scale factor to divide the dimensions by.

        @return A new Size object with dimensions scaled down by the given factor.
    */
    template <class T>
    [[nodiscard]] constexpr Size operator/ (T scaleFactor) const noexcept
    {
        Size result (*this);
        result /= scaleFactor;
        return result;
    }

    /** Division assignment operator

        Divides the dimensions of this Size object by a scale factor and updates this Size object.

        @tparam T The type of the scale factor.

        @param scaleFactor The scale factor to divide the dimensions by.

        @return A reference to this updated Size object.
    */
    template <class T>
    constexpr auto operator/= (T scaleFactor) noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Size&>
    {
        width = static_cast<ValueType> (width / scaleFactor);
        height = static_cast<ValueType> (height / scaleFactor);
        return *this;
    }

    /** Division assignment operator

        Divides the dimensions of this Size object by a scale factor and updates this Size object.

        @tparam T The type of the scale factor.

        @param scaleFactor The scale factor to divide the dimensions by.
    */
    template <class T>
    constexpr auto operator/= (T scaleFactor) noexcept
        -> std::enable_if_t<std::is_integral_v<T>, Size&>
    {
        width = static_cast<ValueType> (width / static_cast<float> (scaleFactor));
        height = static_cast<ValueType> (height / static_cast<float> (scaleFactor));
        return *this;
    }

    //==============================================================================
    /** Returns true if the two sizes are approximately equal. */
    constexpr bool approximatelyEqualTo (const Size& other) const noexcept
    {
        if constexpr (std::is_floating_point_v<ValueType>)
        {
            return approximatelyEqual (width, other.width)
                && approximatelyEqual (height, other.height);
        }
        else
        {
            return *this == other;
        }
    }

    //==============================================================================
    /** Equality operator

        Compares this Size object with another Size object for equality.

        @param other A reference to the other Size object to compare with.

        @return True if both the width and height are equal, false otherwise.
    */
    constexpr bool operator== (const Size& other) const noexcept
    {
        return width == other.width && height == other.height;
    }

    /** Inequality operator

        Compares this Size object with another Size object for inequality.

        @param other A reference to the other Size object to compare with.

        @return True if either the width or height are not equal, false otherwise.
    */
    constexpr bool operator!= (const Size& other) const noexcept
    {
        return ! (*this == other);
    }

private:
    ValueType width = 0;
    ValueType height = 0;
};

/** Stream insertion operator

    Inserts the width and height of the Size object into the provided String object, formatting them as a comma-separated pair.

    @param string1 The String object to which the width and height will be appended.
    @param s The Size object whose dimensions are to be inserted into the String.

    @return A reference to the updated String object containing the formatted dimensions.
*/
template <class ValueType>
YUP_API String& YUP_CALLTYPE operator<< (String& string1, const Size<ValueType>& s)
{
    auto [w, h] = s;

    string1 << w << ", " << h;

    return string1;
}

/** Get the coordinate at the specified index

    Returns the coordinate at the specified index.

    @param point The Point to get the coordinate from.
    @param I The index of the coordinate to get.

    @return The coordinate at the specified index.
*/
template <std::size_t I, class ValueType>
constexpr ValueType get (const Size<ValueType>& point) noexcept
{
    if constexpr (I == 0)
        return point.getWidth();
    else if constexpr (I == 1)
        return point.getHeight();
    else
        static_assert (dependentFalse<I>);
}

} // namespace yup

namespace std
{

template <class ValueType>
struct tuple_size<yup::Size<ValueType>>
{
    inline static constexpr std::size_t value = 2;
};

template <std::size_t I, class ValueType>
struct tuple_element<I, yup::Size<ValueType>>
{
    using type = ValueType;
};

} // namespace std
