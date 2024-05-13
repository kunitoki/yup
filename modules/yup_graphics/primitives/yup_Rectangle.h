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
class JUCE_API Rectangle
{
public:
    //==============================================================================
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
        : xy (other.getPosition().template to<ValueType>())
        , size (other.getSize().template to<ValueType>())
    {
        static_assert (std::numeric_limits<ValueType>::max() >= std::numeric_limits<T>::max(), "Invalid narrow cast");
    }

    //==============================================================================
    constexpr Rectangle (const Rectangle& other) noexcept = default;
    constexpr Rectangle (Rectangle&& other) noexcept = default;
    constexpr Rectangle& operator=(const Rectangle& other) noexcept = default;
    constexpr Rectangle& operator=(Rectangle&& other) noexcept = default;

    //==============================================================================
    constexpr ValueType getX() const noexcept
    {
        return xy.getX();
    }

    constexpr ValueType getY() const noexcept
    {
        return xy.getY();
    }

    //==============================================================================
    constexpr ValueType getWidth() const noexcept
    {
        return size.getWidth();
    }

    constexpr ValueType getHeight() const noexcept
    {
        return size.getHeight();
    }

    //==============================================================================
    constexpr Point<ValueType> getPosition() const noexcept
    {
        return xy;
    }

    constexpr Rectangle& setPosition (const Point<ValueType>& newPosition) noexcept
    {
        xy = newPosition;

        return *this;
    }

    template <class T>
    constexpr Rectangle withPosition (const Point<T>& newPosition) const noexcept
    {
        static_assert (std::numeric_limits<ValueType>::max() >= std::numeric_limits<T>::max(), "Invalid narrow cast");

        return { newPosition.template to<ValueType>(), size };
    }

    template <class T>
    constexpr Rectangle withPosition (T x, T y) const noexcept
    {
        static_assert (std::numeric_limits<ValueType>::max() >= std::numeric_limits<T>::max(), "Invalid narrow cast");

        return { static_cast<ValueType> (x), static_cast<ValueType> (y), size };
    }

    constexpr Rectangle withZeroPosition() const noexcept
    {
        return { 0, 0, size };
    }

    //==============================================================================
    constexpr Point<ValueType> getTopLeft() const noexcept
    {
        return xy;
    }

    constexpr Point<ValueType> getTopRight() const noexcept
    {
        return xy.translated (getWidth(), ValueType (0));
    }

    constexpr Point<ValueType> getBottomLeft() const noexcept
    {
        return xy.translated (ValueType (0), getHeight());
    }

    constexpr Point<ValueType> getBottomRight() const noexcept
    {
        return xy.translated (getWidth(), getHeight());
    }

    //==============================================================================
    constexpr Size<ValueType> getSize() const noexcept
    {
        return size;
    }

    constexpr Rectangle& setSize (const Size<ValueType>& newSize) noexcept
    {
        size = newSize;

        return *this;
    }

    template <class T>
    constexpr Rectangle withSize (const Size<T>& newSize) const noexcept
    {
        static_assert (std::numeric_limits<ValueType>::max() >= std::numeric_limits<T>::max(), "Invalid narrow cast");

        return { xy, newSize.template to<ValueType>() };
    }

    template <class T>
    constexpr Rectangle withSize (T width, T height) const noexcept
    {
        static_assert (std::numeric_limits<ValueType>::max() >= std::numeric_limits<T>::max(), "Invalid narrow cast");

        return { xy, static_cast<ValueType> (width), static_cast<ValueType> (height) };
    }

    template <class T>
    constexpr auto withScaledSize (T scaleFactor) const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Rectangle>
    {
        return withSize (size * scaleFactor);
    }

    constexpr Rectangle withZeroSize() const noexcept
    {
        return { xy, 0, 0 };
    }

    //==============================================================================
    constexpr Rectangle& setBounds (int x, int y, int width, int height) noexcept
    {
        xy = { x, y };
        size = { width, height };

        return *this;
    }

    //==============================================================================
    constexpr Point<ValueType> getCenter() const noexcept
    {
        return { xy.getX() + size.getWidth() / 2.0f, xy.getY() + size.getHeight() / 2.0f };
    }

    constexpr Rectangle& setCenter (ValueType centerX, ValueType centerY) noexcept
    {
        xy = { centerX - size.getWidth() / static_cast<ValueType> (2), centerY - size.getHeight() / static_cast<ValueType> (2) };

        return *this;
    }

    constexpr Rectangle& setCenter (const Point<ValueType>& center) noexcept
    {
        setCenter (center.getX(), center.getY());

        return *this;
    }

    constexpr Rectangle withCenter (ValueType centerX, ValueType centerY) noexcept
    {
        Rectangle result = *this;
        result.setCenter (centerX, centerY);
        return result;
    }

    constexpr Rectangle withCenter (const Point<ValueType>& center) noexcept
    {
        Rectangle result = *this;
        result.setCenter (center);
        return result;
    }

    //==============================================================================
    constexpr bool isPoint() const noexcept
    {
        return size.isZero();
    }

    constexpr bool isLine() const noexcept
    {
        return isVerticalLine() || isHorizontalLine();
    }

    constexpr bool isVerticalLine() const noexcept
    {
        return size.isHorizontallyEmpty();
    }

    constexpr bool isHorizontalLine() const noexcept
    {
        return size.isVerticallyEmpty();
    }

    //==============================================================================
    constexpr Line<ValueType> leftSide() const noexcept
    {
        return { xy, xy.translated (ValueType (0), getHeight()) };
    }

    constexpr Line<ValueType> topSide() const noexcept
    {
        return { xy, xy.translated (getWidth(), ValueType (0)) };
    }

    constexpr Line<ValueType> rightSide() const noexcept
    {
        return { xy.translated (getWidth(), ValueType (0)), xy.translated (getWidth(), getHeight()) };
    }

    constexpr Line<ValueType> bottomSide() const noexcept
    {
        return { xy.translated (ValueType (0), getHeight()), xy.translated (getWidth(), getHeight()) };
    }

    constexpr Line<ValueType> diagonalTopToBottom() const noexcept
    {
        return { xy, xy.translated (getWidth(), getHeight()) };
    }

    constexpr Line<ValueType> diagonalBottomToTop() const noexcept
    {
        return { xy.translated (ValueType (0), getHeight()), xy.translated (getWidth(), ValueType (0)) };
    }

    //==============================================================================
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

    //==============================================================================
    constexpr Rectangle& scale (float deltaX, float deltaY) noexcept
    {
        size.scale (deltaX, deltaY);
        return *this;
    }

    constexpr Rectangle<ValueType> scaled (float deltaX, float deltaY) const noexcept
    {
        return { xy, size.scaled (deltaX, deltaY) };
    }

    //==============================================================================
    constexpr Rectangle removeFromTop (ValueType delta) noexcept
    {
        const Rectangle result { xy, size.withHeight (jmax (0, delta)) };

        xy = xy.withY (xy.getY() + delta);
        size = size.withHeight (jmax (0, size.getHeight() - delta));

        return result;
    }

    constexpr Rectangle removeFromLeft (ValueType delta) noexcept
    {
        const Rectangle result { xy, size.withWidth (jmax (0, delta)) };

        xy = xy.withX (xy.getX() + delta);
        size = size.withWidth (jmax (0, size.getWidth() - delta));

        return result;
    }

    constexpr Rectangle removeFromBottom (ValueType delta) noexcept
    {
        const Rectangle result { xy.withY (jmax (0, xy.getY() + size.getHeight() - delta)), size.withHeight (jmax (0, delta)) };

        size = size.withHeight (jmax (0, size.getHeight() - delta));

        return result;
    }

    constexpr Rectangle removeFromRight (ValueType delta) noexcept
    {
        const Rectangle result { xy.withX (jmax (0, xy.getX() + size.getWidth() - delta)), size.withWidth (jmax (0, delta)) };

        size = size.withWidth (jmax (0, size.getWidth() - delta));

        return result;
    }

    //==============================================================================
    constexpr Rectangle& reduce (ValueType delta) noexcept
    {
        xy = { xy.getX() + delta, xy.getY() + delta };
        size = { jmax (0, size.getWidth () - 2 * delta), jmax (0, size.getHeight () - 2 * delta) };

        return *this;
    }

    constexpr Rectangle& reduce (ValueType deltaX, ValueType deltaY) noexcept
    {
        xy = { xy.getX() + deltaX, xy.getY() + deltaY };
        size = { jmax (0, size.getWidth () - 2 * deltaX), jmax (0, size.getHeight () - 2 * deltaY) };

        return *this;
    }

    constexpr Rectangle reduced (ValueType delta) noexcept
    {
        Rectangle result = *this;
        result.reduce (delta);
        return result;
    }

    constexpr Rectangle reduced (ValueType deltaX, ValueType deltaY) noexcept
    {
        Rectangle result = *this;
        result.reduce (deltaX, deltaY);
        return result;
    }

    //==============================================================================
    constexpr bool contains (ValueType x, ValueType y) const noexcept
    {
        return
            x >= xy.getX()
            && y >= xy.getY()
            && x <= (xy.getX() + size.getWidth())
            && y <= (xy.getY() + size.getHeight());
    }

    constexpr bool contains (const Point<ValueType>& p) const noexcept
    {
        return contains (p.getX(), p.getY());
    }

    //==============================================================================
    constexpr ValueType area() const noexcept
    {
        return size.area();
    }

    //==============================================================================
    constexpr bool intersects (const Rectangle& other) const noexcept
    {
        const auto bottomRight = getBottomRight();
        const auto otherBottomRight = other.getBottomRight();

        return !(getX() > otherBottomRight.getX() || bottomRight.getX() < other.getX() ||
                 getY() > otherBottomRight.getY() || bottomRight.getY() < other.getY());
    }

    //==============================================================================
    constexpr Rectangle largestFittingSquare() const noexcept
    {
        if (getWidth() == getHeight())
            return *this;

        if (getWidth() > getHeight())
        {
            const auto newPosX = static_cast<ValueType> ((getWidth() - getHeight()) / 2.0f);
            return { xy.getX() + newPosX, xy.getY(), getHeight(), getHeight() };
        }
        else
        {
            const auto newPosY = static_cast<ValueType> ((getHeight() - getWidth()) / 2.0f);
            return { xy.getX(), xy.getY() + newPosY, getWidth(), getWidth() };
        }
    }

    //==============================================================================
    template <class T>
    constexpr Rectangle<T> to() const noexcept
    {
        return { xy.template to<T>(), size.template to<T>() };
    }

    //==============================================================================
    template <class T>
    constexpr auto operator* (T scaleFactor) const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Rectangle>
    {
        Rectangle r (*this);
        r *= scaleFactor;
        return r;
    }

    template <class T>
    constexpr auto operator*= (T scaleFactor) noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Rectangle&>
    {
        xy = { static_cast<ValueType> (xy.getX() * scaleFactor), static_cast<ValueType> (xy.getY() * scaleFactor) };
        size = { static_cast<ValueType> (size.getWidth() * scaleFactor), static_cast<ValueType> (size.getHeight() * scaleFactor) };
        return *this;
    }

    template <class T>
    constexpr auto operator/ (T scaleFactor) const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Rectangle>
    {
        Rectangle r (*this);
        r /= scaleFactor;
        return r;
    }

    template <class T>
    constexpr auto operator/= (T scaleFactor) noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Rectangle&>
    {
        xy = { static_cast<ValueType> (xy.getX() / scaleFactor), static_cast<ValueType> (xy.getY() / scaleFactor) };
        size = { static_cast<ValueType> (size.getWidth() / scaleFactor), static_cast<ValueType> (size.getHeight() / scaleFactor) };
        return *this;
    }

    //==============================================================================
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

} // namespace yup

namespace std
{

template <class ValueType>
struct tuple_size<yup::Rectangle<ValueType>>
{
    inline static constexpr std::size_t value = 4;
};

template <std::size_t I, class ValueType>
struct tuple_element<I, yup::Rectangle<ValueType>>
{
    using type = ValueType;
};

} // namespace std
