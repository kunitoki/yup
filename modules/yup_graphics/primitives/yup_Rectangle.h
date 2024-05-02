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

namespace juce
{

template <class ValueType>
class JUCE_API Rectangle
{
public:
    constexpr Rectangle() noexcept = default;

    constexpr Rectangle (ValueType x, ValueType y, ValueType width, ValueType height) noexcept
        : xy (x, y)
        , size (width, height)
    {
    }

    constexpr Rectangle (ValueType x, ValueType y, const Size<ValueType>& size) noexcept
        : xy (x, y)
        , size (size)
    {
    }

    constexpr Rectangle (const Point<ValueType>& xy, ValueType width, ValueType height) noexcept
        : xy (xy)
        , size (width, height)
    {
    }

    constexpr Rectangle (const Point<ValueType>& xy, const Size<ValueType>& size) noexcept
        : xy (xy)
        , size (size)
    {
    }

    template <class T, class = std::enable_if_t<!std::is_same_v<T, ValueType>>>
    constexpr Rectangle (const Rectangle<T>& other) noexcept
        : xy (other.getTopLeft().template to<ValueType>())
        , size (other.getSize().template to<ValueType>())
    {
        static_assert (std::numeric_limits<ValueType>::max() >= std::numeric_limits<T>::max(), "Invalid narrow cast");
    }

    constexpr Rectangle (const Rectangle& other) noexcept = default;
    constexpr Rectangle (Rectangle&& other) noexcept = default;
    constexpr Rectangle& operator=(const Rectangle& other) noexcept = default;
    constexpr Rectangle& operator=(Rectangle&& other) noexcept = default;

    constexpr ValueType getX() const noexcept
    {
        return xy.getX();
    }

    constexpr ValueType getY() const noexcept
    {
        return xy.getY();
    }

    constexpr ValueType getWidth() const noexcept
    {
        return size.getWidth();
    }

    constexpr ValueType getHeight() const noexcept
    {
        return size.getHeight();
    }

    constexpr Point<ValueType> getTopLeft() const noexcept
    {
        return xy;
    }

    constexpr Size<ValueType> getSize() const noexcept
    {
        return size;
    }

    template <class T>
    constexpr Rectangle withPosition (const Point<T>& newPosition) noexcept
    {
        static_assert (std::numeric_limits<ValueType>::max() >= std::numeric_limits<T>::max(), "Invalid narrow cast");

        return { newPosition.template to<ValueType>(), size };
    }

    template <class T>
    constexpr Rectangle withSize (const Size<T>& newSize) noexcept
    {
        static_assert (std::numeric_limits<ValueType>::max() >= std::numeric_limits<T>::max(), "Invalid narrow cast");

        return { xy, newSize.template to<ValueType>() };
    }

    constexpr Rectangle& translate (ValueType deltaX, ValueType deltaY) noexcept
    {
        xy.translate (deltaX, deltaY);
        return *this;
    }

    constexpr Rectangle& translate (Point<ValueType> delta) noexcept
    {
        xy.translate (delta);
        return *this;
    }

    constexpr Rectangle<ValueType> translated (Point<ValueType> delta) const noexcept
    {
        return { xy.translated (delta), size };
    }

    template <class T>
    constexpr Rectangle<T> to() const noexcept
    {
        return { xy.template to<T>(), size.template to<T>() };
    }

    constexpr bool operator== (const Rectangle& other) const noexcept
    {
        return xy == other.xy && size == other.size;
    }

    constexpr bool operator!= (const Rectangle& other) const noexcept
    {
        return !(*this == other);
    }

private:
    Point<ValueType> xy;
    Size<ValueType> size;
};

template <std::size_t I, class ValueType>
ValueType get (const Rectangle<ValueType>& point)
{
    if constexpr (I == 0)
        return point.getX();
    else if constexpr (I == 1)
        return point.getY();
    else if constexpr (I == 2)
        return point.getWidth();
    else if constexpr (I == 3)
        return point.getHeight();
    else
        return {}; // TODO - error
}

} // namespace juce

namespace std
{

template <class ValueType>
struct tuple_size<juce::Rectangle<ValueType>>
{
    inline static constexpr std::size_t value = 4;
};

template <std::size_t I, class ValueType>
struct tuple_element<I, juce::Rectangle<ValueType>>
{
    using type = ValueType;
};

} // namespace std
