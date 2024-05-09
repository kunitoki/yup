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

    constexpr Point<ValueType> getEnd() const noexcept
    {
        return p2;
    }

    //==============================================================================
    constexpr Line withStart (const Point<ValueType>& newStart) const noexcept
    {
        return { newStart, p2 };
    }

    constexpr Line withEnd (const Point<ValueType>& newEnd) const noexcept
    {
        return { p1, newEnd };
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
    template <class T>
    constexpr Line keepOnlyStart (T proportionOfLength) noexcept
    {
        proportionOfLength = jlimit (0.0f, 1.0f, proportionOfLength);

        return { p1, { p1.getX() + (p2.getX() - p1.getX()) * proportionOfLength, p1.getY() + (p2.getY() - p1.getY()) * proportionOfLength } };
    }

    template <class T>
    constexpr Line keepOnlyEnd (T proportionOfLength) noexcept
    {
        proportionOfLength = jlimit (0.0f, 1.0f, proportionOfLength);

        return { { p1.getX() + (p2.getX() - p1.getX()) * proportionOfLength, p1.getY() + (p2.getY() - p1.getY()) * proportionOfLength }, p2 };
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

} // namespace juce

namespace std
{

template <class ValueType>
struct tuple_size<juce::Line<ValueType>>
{
    inline static constexpr std::size_t value = 4;
};

template <std::size_t I, class ValueType>
struct tuple_element<I, juce::Line<ValueType>>
{
    using type = ValueType;
};

} // namespace std
