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
class JUCE_API Point
{
public:
    //==============================================================================
    constexpr Point() noexcept = default;

    constexpr Point (ValueType newX, ValueType newY) noexcept
        : x (newX)
        , y (newY)
    {
    }

    template <class T, std::enable_if_t<! std::is_same_v<T, ValueType>, int> = 0>
    constexpr Point (T newX, T newY) noexcept
        : x (static_cast<ValueType> (newX))
        , y (static_cast<ValueType> (newY))
    {
        static_assert (std::numeric_limits<ValueType>::max() >= std::numeric_limits<T>::max(), "Invalid narrow cast");
    }

    //==============================================================================
    constexpr Point (const Point& other) noexcept = default;
    constexpr Point (Point&& other) noexcept = default;
    constexpr Point& operator=(const Point& other) noexcept = default;
    constexpr Point& operator=(Point&& other) noexcept = default;

    //==============================================================================
    constexpr ValueType getX() const noexcept
    {
        return x;
    }

    constexpr ValueType getY() const noexcept
    {
        return y;
    }

    //==============================================================================
    constexpr Point withX (ValueType newX) const noexcept
    {
        return { newX, y };
    }

    constexpr Point withY (ValueType newY) const noexcept
    {
        return { x, newY };
    }

    //==============================================================================
    constexpr Point withXY (ValueType newX, ValueType newY) const noexcept
    {
        return { newX, newY };
    }

    //==============================================================================
    constexpr Point& translate (ValueType deltaX, ValueType deltaY) noexcept
    {
        x += deltaX;
        y += deltaY;
        return *this;
    }

    constexpr Point& translate (const Point<ValueType>& delta) noexcept
    {
        x += delta.x;
        y += delta.y;
        return *this;
    }

    constexpr Point translated (ValueType deltaX, ValueType deltaY) const noexcept
    {
        return { x + deltaX, y + deltaY };
    }

    constexpr Point translated (const Point<ValueType>& delta) const noexcept
    {
        return { x + delta.x, y + delta.y };
    }

    //==============================================================================
    constexpr float distance (Point<ValueType> other) const noexcept
    {
        return std::sqrt (distanceSquared (other));
    }

    constexpr float distanceSquared (Point<ValueType> other) const noexcept
    {
        return square (other.x - x) + square (other.y - y);
    }

    constexpr float distanceX (Point<ValueType> other) const noexcept
    {
        return other.x - x;
    }

    constexpr float distanceY (Point<ValueType> other) const noexcept
    {
        return other.y - y;
    }

    //==============================================================================
    constexpr Point getPointOnCircumference (float radius, float angleRadians) const noexcept
    {
        return { x + (std::cos (angleRadians) * radius), y + std::sin (angleRadians) * radius };
    }

    constexpr Point getPointOnCircumference (float radiusX, float radiusY, float angleRadians) const noexcept
    {
        return { x + (std::cos (angleRadians) * radiusX), y + std::sin (angleRadians) * radiusY };
    }

    //==============================================================================
    template <class T>
    constexpr Point<T> to() const noexcept
    {
        return { static_cast<T> (x), static_cast<T> (y) };
    }

    //==============================================================================
    constexpr Point operator- () const noexcept
    {
        return { -x, -y };
    }

    //==============================================================================
    constexpr bool operator== (const Point& other) const noexcept
    {
        return x == other.x && y == other.y;
    }

    constexpr bool operator!= (const Point& other) const noexcept
    {
        return !(*this == other);
    }

private:
    ValueType x = 0;
    ValueType y = 0;
};

template <std::size_t I, class ValueType>
ValueType get (const Point<ValueType>& point)
{
    if constexpr (I == 0)
        return point.getX();
    else if constexpr (I == 1)
        return point.getY();
    else
        return {}; // TODO - error
}

} // namespace yup

namespace std
{

template <class ValueType>
struct tuple_size<yup::Point<ValueType>>
{
    inline static constexpr std::size_t value = 2;
};

template <std::size_t I, class ValueType>
struct tuple_element<I, yup::Point<ValueType>>
{
    using type = ValueType;
};

} // namespace std
