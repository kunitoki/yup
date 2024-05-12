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
class JUCE_API Size
{
public:
    //==============================================================================
    constexpr Size() noexcept = default;

    constexpr Size (ValueType newWidth, ValueType newHeight) noexcept
        : width (newWidth)
        , height (newHeight)
    {
    }

    //==============================================================================
    constexpr Size (const Size& other) noexcept = default;
    constexpr Size (Size&& other) noexcept = default;
    constexpr Size& operator=(const Size& other) noexcept = default;
    constexpr Size& operator=(Size&& other) noexcept = default;

    //==============================================================================
    constexpr ValueType getWidth() const noexcept
    {
        return width;
    }

    constexpr Size& setWidth (ValueType newWidth) noexcept
    {
        width = newWidth;
        return *this;
    }

    constexpr Size withWidth (ValueType newWidth) const noexcept
    {
        return { newWidth, height };
    }

    //==============================================================================
    constexpr ValueType getHeight() const noexcept
    {
        return height;
    }

    constexpr Size& setHeight (ValueType newHeight) noexcept
    {
        height = newHeight;
        return *this;
    }

    constexpr Size withHeight (ValueType newHeight) const noexcept
    {
        return { width, newHeight };
    }

    //==============================================================================
    constexpr bool isZero() const noexcept
    {
        return width == ValueType (0) && height == ValueType (0);
    }

    constexpr bool isEmpty() const noexcept
    {
        return width == ValueType (0) || height == ValueType (0);
    }

    constexpr bool isVerticallyEmpty() const noexcept
    {
        return width == ValueType (0) && height != ValueType (0);
    }

    constexpr bool isHorizontallyEmpty() const noexcept
    {
        return width != ValueType (0) && height == ValueType (0);
    }

    //==============================================================================
    constexpr bool isSquare() const noexcept
    {
        return width == height;
    }

    //==============================================================================
    constexpr ValueType area() const noexcept
    {
        return width * height;
    }

    //==============================================================================
    constexpr Size& reverse() noexcept
    {
        using std::swap;

        swap (width, height);

        return *this;
    }

    constexpr Size reversed() const noexcept
    {
        Size result (*this);
        result.reverse();
        return result;
    }

    //==============================================================================
    constexpr Size& enlarge (ValueType amount) noexcept
    {
        width += amount;
        height += amount;
        return *this;
    }

    constexpr Size& enlarge (ValueType widthAmount, ValueType heightAmount) noexcept
    {
        width += widthAmount;
        height += heightAmount;
        return *this;
    }

    constexpr Size enlarged (ValueType amount) const noexcept
    {
        Size result (*this);
        result.enlarge (amount);
        return result;
    }

    constexpr Size enlarged (ValueType widthAmount, ValueType heightAmount) const noexcept
    {
        Size result (*this);
        result.enlarge (widthAmount, heightAmount);
        return result;
    }

    //==============================================================================
    constexpr Size& reduce (ValueType amount) noexcept
    {
        width -= amount;
        height -= amount;
        return *this;
    }

    constexpr Size& reduce (ValueType widthAmount, ValueType heightAmount) noexcept
    {
        width -= widthAmount;
        height -= heightAmount;
        return *this;
    }

    constexpr Size reduced (ValueType amount) const noexcept
    {
        Size result (*this);
        result.reduce (amount);
        return result;
    }

    constexpr Size reduced (ValueType widthAmount, ValueType heightAmount) const noexcept
    {
        Size result (*this);
        result.reduce (widthAmount, heightAmount);
        return result;
    }

    //==============================================================================
    template <class T>
    constexpr auto scale (T scaleFactor) noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Size&>
    {
        scale (scaleFactor, scaleFactor);
        return *this;
    }

    template <class T>
    constexpr auto scale (T scaleFactorX, T scaleFactorY) noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Size&>
    {
        width = static_cast<ValueType> (width * scaleFactorX);
        height = static_cast<ValueType> (height * scaleFactorY);
        return *this;
    }

    template <class T>
    constexpr auto scaled (T scaleFactor) const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Size>
    {
        Size result (*this);
        result.scale (scaleFactor);
        return result;
    }

    template <class T>
    constexpr auto scaled (T scaleFactorX, T scaleFactorY) const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Size>
    {
        Size result (*this);
        result.scale (scaleFactorX, scaleFactorY);
        return result;
    }

    //==============================================================================
    template <class T>
    constexpr Size<T> to() const noexcept
    {
        return { static_cast<T> (width), static_cast<T> (height) };
    }

    //==============================================================================
    template <class T>
    constexpr auto operator* (T scaleFactor) const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Size>
    {
        Size result (*this);
        result *= scaleFactor;
        return result;
    }

    template <class T>
    constexpr auto operator*= (T scaleFactor) noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Size&>
    {
        width = static_cast<ValueType> (width * scaleFactor);
        height = static_cast<ValueType> (height * scaleFactor);
        return *this;
    }

    template <class T>
    constexpr auto operator/ (T scaleFactor) const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Size>
    {
        Size result (*this);
        result /= scaleFactor;
        return result;
    }

    template <class T>
    constexpr auto operator/= (T scaleFactor) noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Size&>
    {
        width = static_cast<ValueType> (width / scaleFactor);
        height = static_cast<ValueType> (height / scaleFactor);
        return *this;
    }

    //==============================================================================
    constexpr bool operator== (const Size& other) const noexcept
    {
        return width == other.width && height == other.height;
    }

    constexpr bool operator!= (const Size& other) const noexcept
    {
        return !(*this == other);
    }

private:
    ValueType width = 0;
    ValueType height = 0;
};

template <std::size_t I, class ValueType>
ValueType get (const Size<ValueType>& point)
{
    if constexpr (I == 0)
        return point.getWidth();
    else if constexpr (I == 1)
        return point.getHeight();
    else
        return {}; // TODO - error
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
