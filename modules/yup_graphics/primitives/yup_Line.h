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

//==============================================================================
/** Defines a class for a geometric line in a 2D space.

    The `Line` class encapsulates a line defined by two points (start and end) in
    2D space. It provides constructors for creating lines from individual coordinates
    or from `Point` objects. The class supports operations such as getting and setting
    start/end points, checking containment of a point with tolerance, translating,
    extending, and rotating the line, and converting to another type.

    @tparam ValueType The data type of the coordinates (e.g., int, float, double).
*/
template <class ValueType>
class JUCE_API Line
{
public:
    //==============================================================================
    /** Default constructor which initializes a line where both points are at the origin. */
    constexpr Line() noexcept = default;

    /** Constructs a line from four coordinate values.

        This constructor initializes a line from two pairs of coordinates representing the start (x1, y1)
        and end (x2, y2) points of the line.

        @param x1 The x-coordinate of the start point.
        @param y1 The y-coordinate of the start point.
        @param x2 The x-coordinate of the end point.
        @param y2 The y-coordinate of the end point.
    */
    constexpr Line (ValueType x1, ValueType y1, ValueType x2, ValueType y2) noexcept
        : p1 (x1, y1)
        , p2 (x2, y2)
    {
    }

    /** Constructs a line from two points.

        This constructor creates a line defined by start point `s` and end point `e`.

        @param s The start point of the line as a `Point` object.
        @param e The end point of the line as a `Point` object.
    */
    constexpr Line (const Point<ValueType>& s, const Point<ValueType>& e) noexcept
        : p1 (s)
        , p2 (e)
    {
    }

    /** Constructs a line from four coordinate values of a different type, converting them to `ValueType`.

        This template constructor allows the creation of a line from coordinates of a different type `T`,
        which are then statically cast to `ValueType`. This constructor checks for possible overflow issues
        in the casting process.

        @param x1 The x-coordinate of the start point, of type `T`.
        @param y1 The y-coordinate of the start point, of type `T`.
        @param x2 The x-coordinate of the end point, of type `T`.
        @param y2 The y-coordinate of the end point, of type `T`.
    */
    template <class T, std::enable_if_t<! std::is_same_v<T, ValueType>, int> = 0>
    constexpr Line (T x1, T y1, T x2, T y2) noexcept
        : p1 (static_cast<ValueType> (x1), static_cast<ValueType> (y1))
        , p2 (static_cast<ValueType> (x2), static_cast<ValueType> (y2))
    {
        static_assert (std::numeric_limits<ValueType>::max() >= std::numeric_limits<T>::max(), "Invalid narrow cast");
    }

    //==============================================================================
    /** Copy and move constructors and assignment operators. */
    constexpr Line (const Line& other) noexcept = default;
    constexpr Line (Line&& other) noexcept = default;
    constexpr Line& operator=(const Line& other) noexcept = default;
    constexpr Line& operator=(Line&& other) noexcept = default;

    //==============================================================================
    /** Retrieves the start point of the line.

        @return The start point of the line as a `Point<ValueType>`.
    */
    constexpr Point<ValueType> getStart() const noexcept
    {
        return p1;
    }

    /** Sets the start point of the line.

        @param newStart The new start point of the line as a `Point<ValueType>`.

        @return A reference to this line after modification.
    */
    constexpr Line& setStart (const Point<ValueType>& newStart) noexcept
    {
        p1 = newStart;
        return *this;
    }

    /** Creates a new line with the specified start point while keeping the end point unchanged.

        @param newStart The new start point of the line as a `Point<ValueType>`.

        @return A new `Line` object with the updated start point.
    */
    constexpr Line withStart (const Point<ValueType>& newStart) const noexcept
    {
        return { newStart, p2 };
    }

    //==============================================================================
    /** Retrieves the end point of the line.

        @return The end point of the line as a `Point<ValueType>`.
    */
    constexpr Point<ValueType> getEnd() const noexcept
    {
        return p2;
    }

    /** Sets the end point of the line.

        @param newEnd The new end point of the line as a `Point<ValueType>`.

        @return A reference to this line after modification.
    */
    constexpr Line& setEnd (const Point<ValueType>& newEnd) noexcept
    {
        p2 = newEnd;
        return *this;
    }

    /** Creates a new line with the specified end point while keeping the start point unchanged.

        @param newEnd The new end point of the line as a `Point<ValueType>`.

        @return A new `Line` object with the updated end point.
    */
    constexpr Line withEnd (const Point<ValueType>& newEnd) const noexcept
    {
        return { p1, newEnd };
    }

    //==============================================================================
    /** Reverses the direction of the line by swapping the start and end points.

        @return A reference to this line after the start and end points have been swapped.
    */
    constexpr Line& reverse() noexcept
    {
        using std::swap;

        swap (p1, p2);

        return *this;
    }

    /** Creates a new line that is a reversed version of this line.

        @return A new `Line` object with the start and end points swapped compared to this line.
    */
    constexpr Line reversed() const noexcept
    {
        Line result (*this);
        result.reverse();
        return result;
    }

    //==============================================================================
    /** Calculates the length of the line.

        @return The length of the line as a `float`.
    */
    constexpr float length() const noexcept
    {
        return p1.distanceTo (p2);
    }

    //==============================================================================
    /** Calculates the slope of the line.

        This function computes the slope as the ratio of the difference in y-coordinates to the difference
        in x-coordinates of the end and start points. If the line is vertical (divisor is zero), the function
        returns zero.

        @return The slope of the line as a `float`.
    */
    constexpr float slope() const noexcept
    {
        const float divisor = static_cast<float> (p2.getX() - p1.getX());
        if (divisor == 0.0f)
            return 0.0f;

        return (p2.getY() - p1.getY()) / divisor;
    }

    //==============================================================================
    /** Checks if a given point lies on the line, with a default very small tolerance.

        @param point The point to check as a `Point<ValueType>`.

        @return `true` if the point is on the line within the default tolerance; otherwise, `false`.
    */
    constexpr bool contains (const Point<ValueType>& point) const noexcept
    {
        return contains (point, 1e-6f);
    }

    /** Checks if a given point lies on the line, considering a specified tolerance.

        This function checks if the specified point is on the line segment within a given tolerance.
        The tolerance accounts for floating-point precision issues in calculations.

        @param point The point to check as a `Point<ValueType>`.
        @param tolerance The tolerance for considering a point to be on the line, as a `float`.

        @return `true` if the point is on the line within the specified tolerance; otherwise, `false`.
    */
    constexpr bool contains (const Point<ValueType>& point, float tolerance) const noexcept
    {
        return std::abs ((point.getY() - p1.getY()) * (p2.getX() - p1.getX()) - (point.getX() - p1.getX()) * (p2.getY() - p1.getY())) < tolerance;
    }

    //==============================================================================
    /** Finds a point along the line at a specified proportion of its length.

        This function calculates a point that lies a certain proportion along the line, from the start
        to the end. The proportion is specified as a fraction of the total line length.

        @param proportionOfLength The proportion of the line's total length, as a `float`.

        @return A `Point<ValueType>` that is located at the specified proportion along the line.
    */
    constexpr Point<ValueType> pointAlong (float proportionOfLength) const noexcept
    {
        return p1.lerp (p2, proportionOfLength);
    }

    //==============================================================================
    /** Translates the line by a specified amount in x and y directions.

        This method modifies both the start and end points of the line by adding the specified
        deltas to their coordinates.

        @param deltaX The amount to translate along the x-axis.
        @param deltaY The amount to translate along the y-axis.

        @return A reference to this line after translation.
    */
    constexpr Line& translate (ValueType deltaX, ValueType deltaY) noexcept
    {
        p1.translate (deltaX, deltaY);
        p2.translate (deltaX, deltaY);
        return *this;
    }

    /** Translates the line by a vector represented by a point.

        This method modifies both the start and end points of the line by adding the coordinates
        of the given `Point` to them, effectively translating the line by the vector represented
        by the point.

        @param delta The translation vector as a `Point<ValueType>`.

        @return A reference to this line after translation.
    */
    constexpr Line& translate (const Point<ValueType>& delta) noexcept
    {
        p1.translate (delta);
        p2.translate (delta);
        return *this;
    }

    /** Creates a new line that is a translated version of this line by specified x and y values.

        @param deltaX The x-value to translate by.
        @param deltaY The y-value to translate by.

        @return A new `Line` object representing the translated line.
    */
    constexpr Line translated (ValueType deltaX, ValueType deltaY) const noexcept
    {
        return { p1.translated (deltaX, deltaY), p2.translated (deltaX, deltaY) };
    }

    /** Creates a new line that is a translated version of this line by a vector represented by a point.

        @param delta The translation vector as a `Point<ValueType>`.

        @return A new `Line` object representing the translated line.
    */
    constexpr Line translated (const Point<ValueType>& delta) const noexcept
    {
        return { p1.translated (delta), p2.translated (delta) };
    }

    //==============================================================================
    /** Extends the line symmetrically before and after by a specified length.

        This function extends the line by moving the start and end points away from each other
        along the line's current direction, by the specified length. The extension is divided equally
        between the start and end points.

        @param length The total length by which to extend the line, as a `ValueType`.

        @return A reference to this line after extension.
    */
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

    /** Creates a new line that is an extended version of this line by a specified length.

        @param length The total length by which to extend the line, as a `ValueType`.

        @return A new `Line` object representing the extended line.
    */
    constexpr Line extended (ValueType length) const noexcept
    {
        Line result (*this);
        result.extend (length);
        return result;
    }

    /** Extends the line before the start point by a specified length.

        This method extends the line by increasing the distance of the start point from the end
        point by the specified length, effectively elongating the line before its start point.

        @param length The length by which to extend the line before the start point, as a `ValueType`.

        @return A reference to this line after extension.
    */
    constexpr Line& extendBefore (ValueType length) noexcept
    {
        const float currentSlope = std::atan (slope());

        p1.setX (p1.getX() - static_cast<ValueType> (length * std::cos (currentSlope)));
        p1.setY (p1.getY() - static_cast<ValueType> (length * std::sin (currentSlope)));

        return *this;
    }

    /** Creates a new line that is an extended version of this line before the start point by a specified length.

        @param length The length by which to extend the line before the start point, as a `ValueType`.

        @return A new `Line` object representing the extended line.
    */
    constexpr Line extendedBefore (ValueType length) const noexcept
    {
        Line result (*this);
        result.extendBefore (length);
        return result;
    }

    /** Extends the line after the end point by a specified length.

        This method extends the line by increasing the distance of the end point from the start
        point by the specified length, effectively elongating the line after its end point.

        @param length The length by which to extend the line after the end point, as a `ValueType`.

        @return A reference to this line after extension.
    */
    constexpr Line& extendAfter (ValueType length) noexcept
    {
        const float currentSlope = std::atan (slope());

        p2.setX (p2.getX() + static_cast<ValueType> (length * std::cos (currentSlope)));
        p2.setY (p2.getY() + static_cast<ValueType> (length * std::sin (currentSlope)));

        return *this;
    }

    /** Creates a new line that is an extended version of this line after the end point by a specified length.

        @param length The length by which to extend the line after the end point, as a `ValueType`.

        @return A new `Line` object representing the extended line.
    */
    constexpr Line extendedAfter (ValueType length) const noexcept
    {
        Line result (*this);
        result.extendAfter (length);
        return result;
    }

    //==============================================================================
    /** Creates a new line segment from the start point to a specified proportion along the line.

        This method computes a new end point along the line at a given proportion of the total line length,
        starting from the original start point. The proportion is clamped between 0.0 and 1.0 to ensure the new end
        point lies within the original line segment.

        @param proportionOfLength The proportion of the total line length where the new end point will be located,
        as a float. This value is clamped to the range [0.0, 1.0].

        @return A new `Line` object representing the segment from the start point to the calculated end point.
    */
    constexpr Line keepOnlyStart (float proportionOfLength) noexcept
    {
        proportionOfLength = jlimit (0.0f, 1.0f, proportionOfLength);

        return { p1, {
            static_cast<ValueType> (p1.getX() + (p2.getX() - p1.getX()) * proportionOfLength),
            static_cast<ValueType> (p1.getY() + (p2.getY() - p1.getY()) * proportionOfLength)
        } };
    }

    /** Creates a new line segment from a specified proportion along the line to the original end point.

        This method computes a new start point along the line at a given proportion of the total line length,
        ending at the original end point. The proportion is clamped between 0.0 and 1.0 to ensure the new start
        point lies within the original line segment.

        @param proportionOfLength The proportion of the total line length where the new start point will be located,
        as a float. This value is clamped to the range [0.0, 1.0].
        @return A new `Line` object representing the segment from the calculated start point to the end point.
    */
    constexpr Line keepOnlyEnd (float proportionOfLength) noexcept
    {
        proportionOfLength = jlimit (0.0f, 1.0f, proportionOfLength);

        return { {
            static_cast<ValueType> (p1.getX() + (p2.getX() - p1.getX()) * proportionOfLength),
            static_cast<ValueType> (p1.getY() + (p2.getY() - p1.getY()) * proportionOfLength) }, p2 };
    }

    //==============================================================================
    /** Rotates the line around a specified point by a given angle in radians.

        This function rotates both the start and end points of the line around a given point by the specified
        angle, measured in radians. The rotation follows the right-hand rule (counter-clockwise direction).

        @param point The center point around which to rotate the line, as a `Point<ValueType>`.
        @param angleRadians The angle by which to rotate the line, in radians.

        @return A new `Line` object representing the rotated line.
    */
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
    // TODO - doxygen
    constexpr Line& transform (const AffineTransform& t) noexcept
    {
        auto x1 = static_cast<float> (p1.x);
        auto y1 = static_cast<float> (p1.y);
        auto x2 = static_cast<float> (p2.x);
        auto y2 = static_cast<float> (p2.y);

        t.transformPoints (x1, y1, x2, y2);

        p1.x = static_cast<ValueType> (x1);
        p1.y = static_cast<ValueType> (y1);
        p2.x = static_cast<ValueType> (x2);
        p2.y = static_cast<ValueType> (y2);

        return *this;
    }

    // TODO - doxygen
    constexpr Line transformed (const AffineTransform& t) const noexcept
    {
        Line result (*this);
        result.transform (t);
        return result;
    }

    //==============================================================================
    /** Converts the line to another type.

        This template function converts both the start and end points of the line to another specified type `T`,
        effectively creating a new `Line<T>` with points of the new type.

        @tparam T The new type to which the line's points should be converted.

        @return A new `Line<T>` object with points of type `T`.
    */
    template <class T>
    constexpr Line<T> to() const noexcept
    {
        return { p1.template to<T> (), p2.template to<T> () };
    }

    //==============================================================================
    /** Unary minus operator to negate both points of the line.

        This operator creates a new line where both the start and end points have their coordinates negated,
        effectively reflecting the line across the origin.

        @return A new `Line` object with both points negated.
    */
    constexpr Line operator- () const noexcept
    {
        return { -p1, -p2 };
    }

    //==============================================================================
    /** Equality comparison operator.

        This operator compares two lines for equality, based on the equality of their start and end points.

        @param other The other line to compare against.

        @return `true` if the lines are equal (both start and end points match); otherwise, `false`.
    */
    constexpr bool operator== (const Line& other) const noexcept
    {
        return p1 == other.p1 && p2 == other.p2;
    }

    /** Inequality comparison operator.

        This operator checks if two lines are not equal by comparing their start and end points.

        @param other The other line to compare against.

        @return `true` if the lines are not equal (either start or end points do not match); otherwise, `false`.
    */
    constexpr bool operator!= (const Line& other) const noexcept
    {
        return !(*this == other);
    }

private:
    Point<ValueType> p1;
    Point<ValueType> p2;
};

/** Stream output operator for Line objects.

    This operator allows Line objects to be written to output streams in a formatted manner.
    It is useful for logging, debugging, or any other scenario where points need to be output to streams.

    @tparam ValueType The numeric type of the line's coordinates.

    @param string1 The output stream to which the line's coordinates are written.
    @param ñ The Line object whose coordinates are written to the stream.

    @return A reference to the output stream after the line's coordinates have been written.
*/
template <class ValueType>
JUCE_API String& JUCE_CALLTYPE operator<< (String& string1, const Line<ValueType>& l)
{
    auto [x1, y1, x2, y2] = l;

    string1 << x1 << ", " << y1 << ", " << x2 << ", " << y2;

    return string1;
}

template <std::size_t I, class ValueType>
constexpr ValueType get (const Line<ValueType>& line) noexcept
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
        static_assert (dependentFalse<I>);
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
