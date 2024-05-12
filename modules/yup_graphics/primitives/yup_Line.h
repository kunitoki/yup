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
class JUCE_API Line
{
public:
    //==============================================================================
    constexpr Line() noexcept = default;

    constexpr Line (ValueType x1, ValueType y1, ValueType x2, ValueType y2) noexcept
        : p1 (x1, y1)
        , p2 (x2, y2)
    {
    }

    constexpr Line (const Point<ValueType>& s, const Point<ValueType>& e) noexcept
        : p1 (s)
        , p2 (e)
    {
    }

    template <class T, std::enable_if_t<! std::is_same_v<T, ValueType>, int> = 0>
    constexpr Line (T x1, T y1, T x2, T y2) noexcept
        : p1 (static_cast<ValueType> (x1), static_cast<ValueType> (y1))
        , p2 (static_cast<ValueType> (x2), static_cast<ValueType> (y2))
    {
        static_assert (std::numeric_limits<ValueType>::max() >= std::numeric_limits<T>::max(), "Invalid narrow cast");
    }

    //==============================================================================
    constexpr Line (const Line& other) noexcept = default;
    constexpr Line (Line&& other) noexcept = default;
    constexpr Line& operator=(const Line& other) noexcept = default;
    constexpr Line& operator=(Line&& other) noexcept = default;

    //==============================================================================
    constexpr Point<ValueType> getStart() const noexcept
    {
        return p1;
    }

    constexpr Line& setStart (const Point<ValueType>& newStart) noexcept
    {
        p1 = newStart;
        return *this;
    }

    constexpr Line withStart (const Point<ValueType>& newStart) const noexcept
    {
        return { newStart, p2 };
    }

    //==============================================================================
    constexpr Point<ValueType> getEnd() const noexcept
    {
        return p2;
    }

    constexpr Line& setEnd (const Point<ValueType>& newEnd) noexcept
    {
        p2 = newEnd;
        return *this;
    }

    constexpr Line withEnd (const Point<ValueType>& newEnd) const noexcept
    {
        return { p1, newEnd };
    }

    //==============================================================================
    constexpr Line& reverse() noexcept
    {
        using std::swap;

        swap (p1, p2);

        return *this;
    }

    constexpr Line reversed() const noexcept
    {
        Line result (*this);
        result.reverse();
        return result;
    }

    //==============================================================================
    constexpr float length() const noexcept
    {
        return p1.distanceTo (p2);
    }

    //==============================================================================
    constexpr float slope() const noexcept
    {
        const float divisor = static_cast<float> (p2.getX() - p1.getX());
        if (divisor == 0.0f)
            return 0.0f;

        return (p2.getY() - p1.getY()) / divisor;
    }

    //==============================================================================
    constexpr bool contains (const Point<ValueType>& point) const noexcept
    {
        return contains (point, 1e-6f);
    }

    constexpr bool contains (const Point<ValueType>& point, float tolerance) const noexcept
    {
        return std::abs ((point.getY() - p1.getY()) * (p2.getX() - p1.getX()) - (point.getX() - p1.getX()) * (p2.getY() - p1.getY())) < tolerance;
    }

    //==============================================================================
    constexpr Point<ValueType> pointAlong (float proportionOfLength) const noexcept
    {
        return p1.lerp (p2, proportionOfLength);
    }

    //==============================================================================
    constexpr Line& translate (ValueType deltaX, ValueType deltaY) noexcept
    {
        p1.translate (deltaX, deltaY);
        p2.translate (deltaX, deltaY);
        return *this;
    }

    constexpr Line& translate (const Point<ValueType>& delta) noexcept
    {
        p1.translate (delta);
        p2.translate (delta);
        return *this;
    }

    constexpr Line translated (ValueType deltaX, ValueType deltaY) const noexcept
    {
        return { p1.translated (deltaX, deltaY), p2.translated (deltaX, deltaY) };
    }

    constexpr Line translated (const Point<ValueType>& delta) const noexcept
    {
        return { p1.translated (delta), p2.translated (delta) };
    }

    //==============================================================================
    constexpr Line& extend (ValueType length) noexcept
    {
        const float currentSlope = std::atan (slope());
        const float xAxisLength = static_cast<ValueType> (length * std::cos (currentSlope));
        const float yAxisLength = static_cast<ValueType> (length * std::sin (currentSlope));

        p1.setX (p1.getX() - xAxisLength);
        p1.setY (p1.getY() - yAxisLength);
        p2.setX (p2.getX() + xAxisLength);
        p2.setY (p2.getY() + yAxisLength);

        return *this;
    }

    constexpr Line extended (ValueType length) const noexcept
    {
        Line result (*this);
        result.extend (length);
        return result;
    }

    constexpr Line& extendBefore (ValueType length) noexcept
    {
        const float currentSlope = std::atan (slope());

        p1.setX (p1.getX() - static_cast<ValueType> (length * std::cos (currentSlope)));
        p1.setY (p1.getY() - static_cast<ValueType> (length * std::sin (currentSlope)));

        return *this;
    }

    constexpr Line extendedBefore (ValueType length) const noexcept
    {
        Line result (*this);
        result.extendBefore (length);
        return result;
    }

    constexpr Line& extendAfter (ValueType length) noexcept
    {
        const float currentSlope = std::atan (slope());

        p2.setX (p2.getX() + static_cast<ValueType> (length * std::cos (currentSlope)));
        p2.setY (p2.getY() + static_cast<ValueType> (length * std::sin (currentSlope)));

        return *this;
    }

    constexpr Line extendedAfter (ValueType length) const noexcept
    {
        Line result (*this);
        result.extendAfter (length);
        return result;
    }

    //==============================================================================
    constexpr Line keepOnlyStart (float proportionOfLength) noexcept
    {
        proportionOfLength = jlimit (0.0f, 1.0f, proportionOfLength);

        return { p1, {
            static_cast<ValueType> (p1.getX() + (p2.getX() - p1.getX()) * proportionOfLength),
            static_cast<ValueType> (p1.getY() + (p2.getY() - p1.getY()) * proportionOfLength)
        } };
    }

    constexpr Line keepOnlyEnd (float proportionOfLength) noexcept
    {
        proportionOfLength = jlimit (0.0f, 1.0f, proportionOfLength);

        return { {
            static_cast<ValueType> (p1.getX() + (p2.getX() - p1.getX()) * proportionOfLength),
            static_cast<ValueType> (p1.getY() + (p2.getY() - p1.getY()) * proportionOfLength) }, p2 };
    }

    //==============================================================================
    constexpr Line rotateAtPoint (const Point<ValueType>& point, float angleRadians) const noexcept
    {
        const float cosTheta = std::cos (angleRadians);
        const float sinTheta = std::sin (angleRadians);

        const auto pointFloat = point.template to<float>();

        auto xy1 = p1.translated (-point).template to<float>();
        xy1 = { xy1.getX() * cosTheta - xy1.getY() * sinTheta, xy1.getX() * sinTheta + xy1.getY() * cosTheta };
        xy1.translate (pointFloat);

        auto xy2 = p2.translated (-point).template to<float>();
        xy2 = { xy2.getX() * cosTheta - xy2.getY() * sinTheta, xy2.getX() * sinTheta + xy2.getY() * cosTheta };
        xy2.translate (pointFloat);

        return { xy1.template to<ValueType>(), xy2.template to<ValueType>() };
    }

    //==============================================================================
    template <class T>
    constexpr Line<T> to() const noexcept
    {
        return { p1.template to<T> (), p2.template to<T> () };
    }

    //==============================================================================
    constexpr Line operator- () const noexcept
    {
        return { -p1, -p2 };
    }

    //==============================================================================
    constexpr bool operator== (const Line& other) const noexcept
    {
        return p1 == other.p1 && p2 == other.p2;
    }

    constexpr bool operator!= (const Line& other) const noexcept
    {
        return !(*this == other);
    }

private:
    Point<ValueType> p1;
    Point<ValueType> p2;
};

template <std::size_t I, class ValueType>
ValueType get (const Line<ValueType>& line)
{
    if constexpr (I == 0)
        return line.getStart().getX();
    else if constexpr (I == 1)
        return line.getStart().getY();
    else if constexpr (I == 2)
        return line.getEnd().getX();
    else if constexpr (I == 3)
        return line.getEnd().getY();
    else
        return {}; // TODO - error
}

} // namespace yup

namespace std
{

template <class ValueType>
struct tuple_size<yup::Line<ValueType>>
{
    inline static constexpr std::size_t value = 4;
};

template <std::size_t I, class ValueType>
struct tuple_element<I, yup::Line<ValueType>>
{
    using type = ValueType;
};

} // namespace std
