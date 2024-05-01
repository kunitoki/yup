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

    constexpr Rectangle (const Point<ValueType>& xy, const Size<ValueType>& size) noexcept
        : xy (xy)
        , size (size)
    {
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
        return xy.getX();
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

    constexpr Rectangle withSize (const Size<ValueType>& newSize) noexcept
    {
        return { xy, newSize };
    }

    template <class T>
    constexpr Rectangle withSize (const Size<T>& newSize) noexcept
    {
        return { xy, newSize };
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
