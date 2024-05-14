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
class JUCE_API Line;

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
    constexpr bool isOrigin() const noexcept
    {
        return isOnXAxis() && isOnYAxis();
    }

    constexpr bool isOnXAxis() const noexcept
    {
        return y == ValueType (0);
    }

    constexpr bool isOnYAxis() const noexcept
    {
        return x == ValueType (0);
    }

    //==============================================================================
    template <class T = ValueType>
    constexpr auto isFinite() const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, bool>
    {
        return std::isfinite (x) && std::isfinite (y);
    }

    //==============================================================================
    constexpr float distanceTo (const Point& other) const noexcept
    {
        return static_cast<float> (std::sqrt (static_cast<float> (distanceToSquared (other))));
    }

    constexpr ValueType distanceToSquared (const Point& other) const noexcept
    {
        return square (other.x - x) + square (other.y - y);
    }

    constexpr ValueType horizontalDistanceTo (const Point& other) const noexcept
    {
        return other.x - x;
    }

    constexpr ValueType verticalDistanceTo (const Point& other) const noexcept
    {
        return other.y - y;
    }

    constexpr ValueType manhattanDistanceTo (const Point& other) const noexcept
    {
        return std::abs (x - other.x) + std::abs (y - other.y);
    }

    //==============================================================================
    constexpr float magnitude() const noexcept
    {
        return static_cast<float> (std::sqrt (static_cast<float> (square (x) + square (y))));
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
    constexpr Point& translate (ValueType deltaX, ValueType deltaY) noexcept
    {
        x += deltaX;
        y += deltaY;
        return *this;
    }

    constexpr Point& translate (const Point& delta) noexcept
    {
        x += delta.x;
        y += delta.y;
        return *this;
    }

    constexpr Point translated (ValueType deltaX, ValueType deltaY) const noexcept
    {
        return { x + deltaX, y + deltaY };
    }

    constexpr Point translated (const Point& delta) const noexcept
    {
        return { x + delta.x, y + delta.y };
    }

    //==============================================================================
    template <class T>
    constexpr auto scale (T factor) noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Point&>
    {
        scale (factor, factor);
        return *this;
    }

    template <class T>
    constexpr auto scale (T factorX, T factorY) noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Point&>
    {
        x = static_cast<ValueType> (x * factorX);
        y = static_cast<ValueType> (y * factorY);
        return *this;
    }

    template <class T>
    constexpr auto scaled (float factor) const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Point>
    {
        Point result (*this);
        result.scale (factor);
        return result;
    }

    template <class T>
    constexpr auto scaled (float factorX, float factorY) const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Point>
    {
        Point result (*this);
        result.scale (factorX, factorY);
        return result;
    }

    //==============================================================================
    constexpr Point& rotateClockwise (float angleInRadians) noexcept
    {
        const float cosTheta = std::cos (angleInRadians);
        const float sinTheta = std::sin (angleInRadians);

        x = static_cast<ValueType> (x * cosTheta - y * sinTheta);
        y = static_cast<ValueType> (x * sinTheta + y * cosTheta);
        return *this;
    }

    constexpr Point rotatedClockwise (float angleInRadians) const noexcept
    {
        const float cosTheta = std::cos (angleInRadians);
        const float sinTheta = std::sin (angleInRadians);

        return { static_cast<ValueType> (x * cosTheta - y * sinTheta), static_cast<ValueType> (x * sinTheta + y * cosTheta) };
    }

    constexpr Point& rotateCounterClockwise (float angleInRadians) noexcept
    {
        *this = rotatedCounterClockwise (angleInRadians);
        return *this;
    }

    constexpr Point rotatedCounterClockwise (float angleInRadians) const noexcept
    {
        const float cosTheta = std::cos (angleInRadians);
        const float sinTheta = std::sin (angleInRadians);

        return { static_cast<ValueType> (x * cosTheta + y * sinTheta), static_cast<ValueType> (-x * sinTheta + y * cosTheta) };
    }

    //==============================================================================
    constexpr Point midpoint (const Point& other) const noexcept
    {
        return { (x + other.x) / ValueType (2), (y + other.y) / ValueType (2) };
    }

    constexpr Point pointBetween (const Point& other, float delta) const noexcept
    {
        delta = jlimit (0.0f, 1.0f, delta);

        return { static_cast<ValueType> (x + (other.x - x) * delta), static_cast<ValueType> (y + (other.y - y) * delta) };
    }

    //==============================================================================
    constexpr ValueType dotProduct (const Point& other) const noexcept
    {
        return x * other.x + y * other.y;
    }

    constexpr ValueType crossProduct (const Point& other) const noexcept
    {
        return x * other.y - y * other.x;
    }

    //==============================================================================
    constexpr float angleTo (const Point& other) const noexcept
    {
        const auto magProduct = magnitude() * other.magnitude();

        return magProduct == 0.0f ? 0.0f : std::acos (dotProduct (other) / magProduct);
    }

    //==============================================================================
    constexpr Point normalized() const noexcept
    {
        const auto mag = magnitude();

        return mag == 0.0f ? Point() : Point (static_cast<ValueType> (x / mag), static_cast<ValueType> (y / mag));
    }

    constexpr bool isNormalized() const noexcept
    {
        return magnitude() == ValueType (1);
    }

    //==============================================================================
    constexpr bool isCollinear (const Point& other) const noexcept
    {
        return crossProduct (other) == ValueType (0);
    }

    //==============================================================================
    constexpr bool isWithinCircle (const Point& center, float radius) const noexcept
    {
        return distanceTo (center) <= radius;
    }

    constexpr bool isWithinRectangle (const Point& topLeft, const Point& bottomRight) const noexcept
    {
        return x >= topLeft.x && x <= bottomRight.x_ && y >= topLeft.y && y <= bottomRight.y;
    }

    //==============================================================================
    constexpr Point& reflectOverXAxis() noexcept
    {
        y = -y;
        return *this;
    }

    constexpr Point reflectedOverXAxis() const noexcept
    {
        return { x, -y };
    }

    constexpr Point& reflectOverYAxis() noexcept
    {
        x = -x;
        return *this;
    }

    constexpr Point reflectedOverYAxis() const noexcept
    {
        return { -x, y };
    }

    constexpr Point& reflectOverOrigin() noexcept
    {
        x = -x;
        y = -y;
        return *this;
    }

    constexpr Point reflectedOverOrigin() const noexcept
    {
        return { -x, -y };
    }

    //==============================================================================
    constexpr Point min (const Point& other) const noexcept
    {
        return { std::min (x, other.x), std::min (y, other.y) };
    }

    constexpr Point max (const Point& other) const noexcept
    {
        return { std::max (x, other.x), std::max (y, other.y) };
    }

    constexpr Point abs() const noexcept
    {
        return { std::abs (x), std::abs(y) };
    }

    template <class T = ValueType>
    constexpr auto floor() const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Point>
    {
        return { std::floor (x), std::floor (y) };
    }

    template <class T = ValueType>
    constexpr auto ceil() const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Point>
    {
        return { std::ceil (x), std::ceil (y) };
    }

    //==============================================================================
    constexpr Point lerp (const Point& other, float delta) const noexcept
    {
        return { static_cast<ValueType> ((1.0f - delta) * x + delta * other.x), static_cast<ValueType> ((1.0f - delta) * y + delta * other.y) };
    }

    //==============================================================================
    template <class T>
    constexpr Point<T> to() const noexcept
    {
        return { static_cast<T> (x), static_cast<T> (y) };
    }

    //==============================================================================
    constexpr Point operator+ (const Point& other) const noexcept
    {
        Point result (*this);
        result += other;
        return result;
    }

    constexpr Point& operator+= (const Point& other) noexcept
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    constexpr Point operator+ (ValueType amount) const noexcept
    {
        Point result (*this);
        result += amount;
        return result;
    }

    constexpr Point& operator+= (ValueType amount) noexcept
    {
        x += amount;
        y += amount;
        return *this;
    }

    constexpr Point operator- (const Point& other) const noexcept
    {
        Point result (*this);
        result -= other;
        return result;
    }

    constexpr Point& operator-= (const Point& other) noexcept
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    constexpr Point operator- (ValueType amount) const noexcept
    {
        Point result (*this);
        result -= amount;
        return result;
    }

    constexpr Point& operator-= (ValueType amount) noexcept
    {
        x -= amount;
        y -= amount;
        return *this;
    }

    constexpr Point operator* (const Point& other) const noexcept
    {
        Point result (*this);
        result *= other;
        return result;
    }

    constexpr Point& operator*= (const Point& other) noexcept
    {
        x *= other.x;
        y *= other.y;
        return *this;
    }

    constexpr Point operator* (ValueType scale) const noexcept
    {
        Point result (*this);
        result *= scale;
        return result;
    }

    constexpr Point& operator*= (ValueType scale) noexcept
    {
        x *= scale;
        y *= scale;
        return *this;
    }

    constexpr Point operator/ (const Point& other) const noexcept
    {
        Point result (*this);
        result /= other;
        return result;
    }

    constexpr Point& operator/= (const Point& other) noexcept
    {
        if (other.x != ValueType (0))
            x /= other.x;

        if (other.y != ValueType (0))
            y /= other.y;

        return *this;
    }

    constexpr Point operator/ (ValueType scale) const noexcept
    {
        Point result (*this);
        result /= scale;
        return result;
    }

    constexpr Point& operator/= (ValueType scale) noexcept
    {
        if (scale != ValueType (0))
        {
            x /= scale;
            y /= scale;
        }

        return *this;
    }

    constexpr Point operator- () const noexcept
    {
        return reflectedOverOrigin();
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

template <class ValueType>
JUCE_API String& JUCE_CALLTYPE operator<< (String& string1, const Point<ValueType>& p)
{
    auto [x, y] = p;

    string1 << x << ", " << y;

    return string1;
}

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
