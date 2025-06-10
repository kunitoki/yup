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
class YUP_API Line;

//==============================================================================
/** Represents a 2D point with coordinates of a generic type.

    The Point class template provides a flexible representation of a two-dimensional
    point using any numeric type. It offers various methods for manipulating the point's
    position through arithmetic operations, scaling, rotation, and other geometric
    transformations. The class is designed to be efficient and used in contexts where
    points as mathematical concepts are needed, such as graphics, physics simulations,
    and vector calculations.

    @tparam ValueType The type of the coordinates, can be any numeric type (int, float, double, etc.).
*/
template <class ValueType>
class YUP_API Point
{
public:
    //==============================================================================
    /** Value type of the point. */
    using Type = ValueType;

    //==============================================================================
    /** Constructs a default point at the origin (0, 0).
    */
    constexpr Point() noexcept = default;

    /** Constructs a point with specified x and y coordinates.

        @param newX The x coordinate of the point.
        @param newY The y coordinate of the point.
    */
    constexpr Point (ValueType newX, ValueType newY) noexcept
        : x (newX)
        , y (newY)
    {
    }

    /** Constructs a point from another type, converting the coordinates to the ValueType.

        This constructor allows for points with different coordinate types to be converted into
        points with this instance's coordinate type, as long as the conversion is valid and does not
        narrow the type in a way that could lead to loss of data.

        @tparam T The type of the original coordinates (must be different from ValueType).
        @param newX The x coordinate of the original point.
        @param newY The y coordinate of the original point.
    */
    template <class T, std::enable_if_t<! std::is_same_v<T, ValueType>, int> = 0>
    constexpr Point (T newX, T newY) noexcept
        : x (static_cast<ValueType> (newX))
        , y (static_cast<ValueType> (newY))
    {
        static_assert (std::numeric_limits<ValueType>::max() >= std::numeric_limits<T>::max(), "Invalid narrow cast");
    }

    /** Constructs a point from another Point type, converting the coordinates to the ValueType.

        @tparam T The type of the original coordinates (must be different from ValueType).
        @param newPoint The original point to convert.
    */
    template <class T, std::enable_if_t<! std::is_same_v<T, ValueType>, int> = 0>
    constexpr Point (const Point<T> newPoint) noexcept
        : x (static_cast<ValueType> (newPoint.getX()))
        , y (static_cast<ValueType> (newPoint.getY()))
    {
        static_assert (std::numeric_limits<ValueType>::max() >= std::numeric_limits<T>::max(), "Invalid narrow cast");
    }

    //==============================================================================
    /** Copy and move constructors and assignment operators. */
    constexpr Point (const Point& other) noexcept = default;
    constexpr Point (Point&& other) noexcept = default;
    constexpr Point& operator= (const Point& other) noexcept = default;
    constexpr Point& operator= (Point&& other) noexcept = default;

    //==============================================================================
    /** Gets the x coordinate of this point.

        @return The x coordinate of the point.
    */
    [[nodiscard]] constexpr ValueType getX() const noexcept
    {
        return x;
    }

    // TODO - doxygen
    constexpr Point& setX (ValueType newX) noexcept
    {
        x = newX;
        return *this;
    }

    /** Returns a new point with the x coordinate changed to the specified value, keeping the y coordinate the same.

        @param newX The new x coordinate.

        @return A new point with the updated x coordinate.
    */
    [[nodiscard]] constexpr Point withX (ValueType newX) const noexcept
    {
        return { newX, y };
    }

    //==============================================================================
    /** Gets the y coordinate of this point.

        @return The y coordinate of the point.
    */
    [[nodiscard]] constexpr ValueType getY() const noexcept
    {
        return y;
    }

    // TODO - doxygen
    constexpr Point& setY (ValueType newY) noexcept
    {
        y = newY;
        return *this;
    }

    /** Returns a new point with the y coordinate changed to the specified value, keeping the x coordinate the same.

        @param newY The new y coordinate.

        @return A new point with the updated y coordinate.
    */
    [[nodiscard]] constexpr Point withY (ValueType newY) const noexcept
    {
        return { x, newY };
    }

    //==============================================================================
    /** Returns a new point with both x and y coordinates changed to the specified values.

        @param newX The new x coordinate.
        @param newY The new y coordinate.

        @return A new point with the updated coordinates.
    */
    [[nodiscard]] constexpr Point withXY (ValueType newX, ValueType newY) const noexcept
    {
        return { newX, newY };
    }

    //==============================================================================
    /** Checks if this point is located at the origin (0, 0).

        @return True if the point is at the origin, false otherwise.
    */
    [[nodiscard]] constexpr bool isOrigin() const noexcept
    {
        return isOnXAxis() && isOnYAxis();
    }

    /** Checks if this point is located on the X-axis (y == 0).

        @return True if the point is on the X-axis, false otherwise.
    */
    [[nodiscard]] constexpr bool isOnXAxis() const noexcept
    {
        return y == ValueType (0);
    }

    /** Checks if this point is located on the Y-axis (x == 0).

        @return True if the point is on the Y-axis, false otherwise.
    */
    [[nodiscard]] constexpr bool isOnYAxis() const noexcept
    {
        return x == ValueType (0);
    }

    //==============================================================================
    /** Checks if the coordinates of this point are finite numbers.

        This method is only available for floating-point coordinate types and checks if both coordinates
        are finite values as defined by the standard library function std::isfinite.

        @tparam T The type of the coordinates, constrained to floating-point types.

        @return True if both coordinates are finite, false otherwise.
    */
    template <class T = ValueType>
    [[nodiscard]] constexpr auto isFinite() const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, bool>
    {
        return std::isfinite (x) && std::isfinite (y);
    }

    //==============================================================================
    /** Calculates the Euclidean distance between this point and another point.

        @param other The point to calculate the distance to.

        @return The Euclidean distance as a floating-point value.
    */
    [[nodiscard]] constexpr float distanceTo (const Point& other) const noexcept
    {
        return static_cast<float> (std::sqrt (static_cast<float> (distanceToSquared (other))));
    }

    /** Calculates the squared Euclidean distance between this point and another point, avoiding the square root calculation.

        This method is useful for distance comparisons where the actual distance value is not needed,
        as it avoids the computationally expensive square root operation.

        @param other The point to calculate the squared distance to.

        @return The squared Euclidean distance as a value of type ValueType.
    */
    [[nodiscard]] constexpr ValueType distanceToSquared (const Point& other) const noexcept
    {
        return square (other.x - x) + square (other.y - y);
    }

    /** Calculates the horizontal distance between this point and another point.

        This method returns the difference in the x coordinates of this point and another point, representing
        the horizontal component of the distance between them.

        @param other The point to calculate the horizontal distance to.

        @return The horizontal distance as a value of type ValueType.
    */
    [[nodiscard]] constexpr ValueType horizontalDistanceTo (const Point& other) const noexcept
    {
        return other.x - x;
    }

    /** Calculates the vertical distance between this point and another point.

        This method returns the difference in the y coordinates of this point and another point, representing
        the vertical component of the distance between them.

        @param other The point to calculate the vertical distance to.

        @return The vertical distance as a value of type ValueType.
    */
    [[nodiscard]] constexpr ValueType verticalDistanceTo (const Point& other) const noexcept
    {
        return other.y - y;
    }

    /** Calculates the Manhattan distance between this point and another point.

        The Manhattan distance is the sum of the absolute differences of their Cartesian coordinates.
        It is often used in integrated circuits where wires only run parallel to the X or Y axis.

        @param other The point to calculate the Manhattan distance to.

        @return The Manhattan distance as a value of type ValueType.
    */
    [[nodiscard]] constexpr ValueType manhattanDistanceTo (const Point& other) const noexcept
    {
        return std::abs (x - other.x) + std::abs (y - other.y);
    }

    //==============================================================================
    /** Returns the magnitude (length) of this point interpreted as a vector from the origin.

        This method calculates the Euclidean norm of the vector represented by the point's coordinates.
        It is equivalent to the distance from the origin to this point.

        @return The magnitude of the vector as a floating-point value.
    */
    [[nodiscard]] constexpr float magnitude() const noexcept
    {
        return static_cast<float> (std::sqrt (static_cast<float> (square (x) + square (y))));
    }

    //==============================================================================
    /** Returns a new point located on the circumference of a circle centered at this point, given a radius and an angle.

        This method calculates the coordinates of a point located on the circumference of a circle with a specified
        radius and angle from the x-axis, using the polar coordinates transformation.

        @param radius The radius of the circle.
        @param angleRadians The angle in radians from the x-axis where the new point will be located.

        @return A new point on the specified circle.
    */
    [[nodiscard]] constexpr Point getPointOnCircumference (float radius, float angleRadians) const noexcept
    {
        return { x + (std::cos (angleRadians) * radius), y + std::sin (angleRadians) * radius };
    }

    /** Returns a new point located on the circumference of an ellipse centered at this point, given radii and an angle.

        This method is an extension of getPointOnCircumference that supports elliptical paths by specifying
        different radii for the x and y axes.

        @param radiusX The radius along the x-axis of the ellipse.
        @param radiusY The radius along the y-axis of the ellipse.
        @param angleRadians The angle in radians from the x-axis where the new point will be located.

        @return A new point on the specified ellipse.
    */
    [[nodiscard]] constexpr Point getPointOnCircumference (float radiusX, float radiusY, float angleRadians) const noexcept
    {
        return { x + (std::cos (angleRadians) * radiusX), y + std::sin (angleRadians) * radiusY };
    }

    //==============================================================================
    /** Translates this point by the specified changes in x and y coordinates.

        This method modifies this point's coordinates by adding the specified deltas to them.

        @param deltaX The change to apply to the x coordinate.
        @param deltaY The change to apply to the y coordinate.

        @return A reference to this point after the translation.
    */
    constexpr Point& translate (ValueType deltaX, ValueType deltaY) noexcept
    {
        x += deltaX;
        y += deltaY;
        return *this;
    }

    /** Translates this point by another point's coordinates.

        This method modifies the coordinates of this point by adding the coordinates of another point to its own coordinates.
        It effectively moves this point by the offset specified by the other point.

        @param delta Another Point object whose x and y values are used as the delta values for the translation.

        @return A reference to this Point object after the translation.
    */
    constexpr Point& translate (const Point& delta) noexcept
    {
        x += delta.x;
        y += delta.y;
        return *this;
    }

    /** Returns a new point translated by specified delta x and delta y values.

        This method creates a new Point object with coordinates that are the result of adding specified delta values
        to the coordinates of this point. It does not modify this point but returns a new point with the updated coordinates.

        @param deltaX The change to be applied to the x-coordinate of the new point.
        @param deltaY The change to be applied to the y-coordinate of the new point.

        @return A new Point object with the translated coordinates.
    */
    [[nodiscard]] constexpr Point translated (ValueType deltaX, ValueType deltaY) const noexcept
    {
        return { x + deltaX, y + deltaY };
    }

    /** Returns a new point translated by another point's coordinates.

        This method creates a new Point object with coordinates that are the result of adding the coordinates of another point
        to the coordinates of this point. It does not modify this point but returns a new point with the updated coordinates.

        @param delta Another Point object whose x and y values are used as the delta values for the translation of the new point.

        @return A new Point object with the translated coordinates.
    */
    [[nodiscard]] constexpr Point translated (const Point& delta) const noexcept
    {
        return { x + delta.x, y + delta.y };
    }

    //==============================================================================
    /** Scales this point's coordinates by a uniform factor.

        This method multiplies both x and y coordinates of this point by a single scale factor. It modifies this point's coordinates
        and is useful for resizing or scaling operations relative to the origin.

        @tparam T The numeric type of the scaling factor, constrained to floating-point types.

        @param factor The factor by which the point's coordinates are scaled.

        @return A reference to this Point object after scaling.
    */
    template <class T>
    constexpr auto scale (T factor) noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Point&>
    {
        scale (factor, factor);
        return *this;
    }

    /** Scales this point's coordinates by specified factors along the x and y axes.

        This method multiplies the x and y coordinates of this point by separate scale factors for the x and y axes respectively.
        It modifies this point's coordinates and is useful for asymmetric resizing or scaling operations relative to the origin.

        @tparam T The numeric type of the scaling factors, constrained to floating-point types.

        @param factorX The factor by which the x-coordinate of the point is scaled.
        @param factorY The factor by which the y-coordinate of the point is scaled.

        @return A reference to this Point object after scaling.
    */
    template <class T>
    constexpr auto scale (T factorX, T factorY) noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Point&>
    {
        x = static_cast<ValueType> (x * factorX);
        y = static_cast<ValueType> (y * factorY);
        return *this;
    }

    /** Returns a new point scaled by a uniform factor.

        This method creates a new Point object with coordinates that are the result of multiplying this point's coordinates
        by a single scale factor. It does not modify this point but returns a new point with the scaled coordinates.

        @tparam T The numeric type of the scaling factor, constrained to floating-point types.

        @param factor The factor by which the coordinates of the new point are scaled.

        @return A new Point object with the scaled coordinates.
    */
    template <class T>
    [[nodiscard]] constexpr auto scaled (T factor) const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Point>
    {
        Point result (*this);
        result.scale (factor);
        return result;
    }

    /** Returns a new point scaled by specified factors along the x and y axes.

        This method creates a new Point object with coordinates that are the result of multiplying this point's coordinates
        by separate scale factors for the x and y axes respectively. It does not modify this point but returns a new point with the scaled coordinates.

        @tparam T The numeric type of the scaling factors, constrained to floating-point types.

        @param factorX The factor by which the x-coordinate of the new point is scaled.
        @param factorY The factor by which the y-coordinate of the new point is scaled.

        @return A new Point object with the scaled coordinates.
    */
    template <class T>
    [[nodiscard]] constexpr auto scaled (T factorX, T factorY) const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Point>
    {
        Point result (*this);
        result.scale (factorX, factorY);
        return result;
    }

    //==============================================================================
    /** Rotates this point clockwise around the origin by a specified angle.

        This method modifies the coordinates of this point by rotating them clockwise around the origin (0,0).
        The rotation is defined by an angle in radians, using the standard mathematical rotation matrix.
        It is useful for operations that involve rotation or angular adjustments in the coordinate system.

        @param angleInRadians The angle in radians by which to rotate the point clockwise.

        @return A reference to this Point object after rotation.
    */
    constexpr Point& rotateClockwise (float angleInRadians) noexcept
    {
        const float cosTheta = std::cos (angleInRadians);
        const float sinTheta = std::sin (angleInRadians);

        const auto originalX = x;
        x = static_cast<ValueType> (originalX * cosTheta + y * sinTheta);
        y = static_cast<ValueType> (-originalX * sinTheta + y * cosTheta);

        return *this;
    }

    /** Returns a new point rotated clockwise around the origin by a specified angle.

        This method creates a new Point object with coordinates that are the result of rotating this point's coordinates
        clockwise around the origin by a specified angle. It does not modify this point but returns a new point with the rotated coordinates.

        @param angleInRadians The angle in radians by which the new point is rotated clockwise.

        @return A new Point object with the rotated coordinates.
    */
    [[nodiscard]] constexpr Point rotatedClockwise (float angleInRadians) const noexcept
    {
        auto result = *this;
        result.rotateClockwise (angleInRadians);
        return result;
    }

    /** Rotates this point counterclockwise around the origin by a specified angle.

        This method modifies the coordinates of this point by rotating them counterclockwise around the origin (0,0).
        The rotation is defined by an angle in radians, using the standard mathematical rotation matrix reversed.
        It is useful for operations that involve rotation or angular adjustments in the coordinate system in the opposite direction of the clockwise rotation.

        @param angleInRadians The angle in radians by which to rotate the point counterclockwise.

        @return A reference to this Point object after rotation.
    */
    constexpr Point& rotateCounterClockwise (float angleInRadians) noexcept
    {
        const float cosTheta = std::cos (angleInRadians);
        const float sinTheta = std::sin (angleInRadians);

        const auto originalX = x;
        x = static_cast<ValueType> (originalX * cosTheta - y * sinTheta);
        y = static_cast<ValueType> (originalX * sinTheta + y * cosTheta);

        return *this;
    }

    /** Returns a new point rotated counterclockwise around the origin by a specified angle.

        This method creates a new Point object with coordinates that are the result of rotating this point's coordinates
        counterclockwise around the origin by a specified angle. It does not modify this point but returns a new point with the rotated coordinates.

        @param angleInRadians The angle in radians by which the new point is rotated counterclockwise.

        @return A new Point object with the rotated coordinates.
    */
    [[nodiscard]] constexpr Point rotatedCounterClockwise (float angleInRadians) const noexcept
    {
        auto result = *this;
        result.rotateCounterClockwise (angleInRadians);
        return result;
    }

    //==============================================================================
    /** Calculates the midpoint between this point and another point.

        This method computes the coordinates of the midpoint between this point and another specified point.
        The midpoint is calculated by averaging the coordinates of the two points.
        It is useful in geometry for finding the center point between two points or dividing line segments into equal parts.

        @param other Another Point object with which to calculate the midpoint.

        @return A new Point object representing the midpoint between this point and the other point.
    */
    [[nodiscard]] constexpr Point midpoint (const Point& other) const noexcept
    {
        return { (x + other.x) / ValueType (2), (y + other.y) / ValueType (2) };
    }

    /** Calculates a point linearly interpolated between this point and another point by a specified factor.

        This method computes the coordinates of a point that lies a certain way between this point and another point, based on a specified interpolation factor.
        The interpolation factor should be between 0 and 1, where 0 represents this point and 1 represents the other point.
        It is useful for smooth transitions or animations between points.

        @param other Another Point object to interpolate towards.
        @param delta The interpolation factor, typically between 0.0 and 1.0, representing the position between the two points.

        @return A new Point object representing the interpolated position between this point and the other point.
    */
    [[nodiscard]] constexpr Point pointBetween (const Point& other, float delta) const noexcept
    {
        delta = jlimit (0.0f, 1.0f, delta);

        return { static_cast<ValueType> (x + (other.x - x) * delta), static_cast<ValueType> (y + (other.y - y) * delta) };
    }

    //==============================================================================
    /** Calculates the dot product of this point with another point.

        This method computes the dot product of this point's vector with another point's vector.
        The dot product is a scalar representation of the vector alignment and length, and is often used in physics and graphics computations.

        @param other Another Point object with which to calculate the dot product.

        @return The dot product of this point with the other point as a ValueType.
    */
    [[nodiscard]] constexpr ValueType dotProduct (const Point& other) const noexcept
    {
        return x * other.x + y * other.y;
    }

    /** Calculates the cross product of this point with another point.

        This method computes the cross product of this point's vector with another point's vector.
        The cross product is a vector perpendicular to the plane of the vectors, and its magnitude represents the area of the parallelogram that the vectors span.
        It is commonly used in physics, engineering, and computer graphics to determine orientation and area.

        @param other Another Point object with which to calculate the cross product.

        @return The cross product of this point with the other point as a ValueType.
    */
    [[nodiscard]] constexpr ValueType crossProduct (const Point& other) const noexcept
    {
        return x * other.y - y * other.x;
    }

    //==============================================================================
    /** Calculates the angle to another point in radians.

        This method computes the angle in radians between this point's vector and another point's vector.
        The angle is calculated based on the cosine of the angle derived from the dot product and magnitudes of the vectors.
        It is useful for determining the relative orientation of points or vectors in 2D space.

        @param other Another Point object to which the angle is calculated.

        @return The angle in radians between this point and the other point.
    */
    [[nodiscard]] constexpr float angleTo (const Point& other) const noexcept
    {
        const auto magProduct = magnitude() * other.magnitude();

        return magProduct == 0.0f ? 0.0f : std::acos (dotProduct (other) / magProduct);
    }

    //==============================================================================
    /** Normalizes the coordinates of this point to a unit vector.

        This method modifies the coordinates of this point so that its magnitude (distance from the origin) becomes 1, turning it into a unit vector.
        It is useful in physics and graphics where unit vectors are needed to represent directions irrespective of length.

        @return A reference to this Point object after normalization.
    */
    constexpr Point& normalize() noexcept
    {
        const auto mag = magnitude();
        if (mag != 0.0f)
        {
            x = static_cast<ValueType> (x / mag);
            y = static_cast<ValueType> (y / mag);
        }

        return *this;
    }

    /** Return a normalized point to a unit vector.

        This method returns a new point so that its magnitude (distance from the origin) becomes 1, turning it into a unit vector.
        It is useful in physics and graphics where unit vectors are needed to represent directions irrespective of length.

        @return A new Point object representing the normalized version of this point.
    */
    [[nodiscard]] constexpr Point normalized() const noexcept
    {
        Point result (*this);
        result.normalize();
        return result;
    }

    /** Checks if the coordinates of this point represent a unit vector.

        This method determines whether the magnitude of this point is exactly 1, indicating that the point is a unit vector.
        Unit vectors are often used to represent directions in vector space, where only the direction and not the magnitude is of interest.

        @return True if this point is a unit vector, false otherwise.
    */
    [[nodiscard]] constexpr bool isNormalized() const noexcept
    {
        return magnitude() == ValueType (1);
    }

    //==============================================================================
    /** Checks if this point is collinear with another point.

        This method determines whether this point and another point lie on a single straight line (i.e., are collinear).
        Collinearity is checked using the cross product, which is zero if the points are collinear.

        @param other Another Point object to check for collinearity.

        @return True if this point is collinear with the other point, false otherwise.
    */
    [[nodiscard]] constexpr bool isCollinear (const Point& other) const noexcept
    {
        return juce_abs (crossProduct (other)) == ValueType (0);
    }

    //==============================================================================
    /** Checks if this point is within a circle defined by a center point and radius.

        This method determines whether this point lies within or on the boundary of a circle defined by a center point and a specified radius.
        The check is performed using the Euclidean distance from this point to the center point, comparing it with the radius.

        @param center The center point of the circle.
        @param radius The radius of the circle.

        @return True if this point is within the specified circle, false otherwise.
    */
    [[nodiscard]] constexpr bool isWithinCircle (const Point& center, float radius) const noexcept
    {
        return distanceTo (center) <= radius;
    }

    /** Checks if this point is within a rectangular area defined by two corner points.

        This method determines whether this point lies within or on the boundary of a rectangle defined by its top-left and bottom-right corner points.
        The check involves comparing this point's coordinates with the coordinates of the corners to ensure it lies within the defined bounds.

        @param topLeft The top-left corner point of the rectangle.
        @param bottomRight The bottom-right corner point of the rectangle.

        @return True if this point is within the specified rectangle, false otherwise.
    */
    [[nodiscard]] constexpr bool isWithinRectangle (const Point& topLeft, const Point& bottomRight) const noexcept
    {
        return x >= topLeft.x && x <= bottomRight.x && y >= topLeft.y && y <= bottomRight.y;
    }

    //==============================================================================
    /** Reflects this point over the X-axis.

        This method modifies the y-coordinate of this point by negating it, effectively reflecting the point over the X-axis.
        It is useful for mirroring points in graphical or geometrical transformations.

        @return A reference to this Point object after reflection over the X-axis.
    */
    constexpr Point& reflectOverXAxis() noexcept
    {
        y = -y;
        return *this;
    }

    /** Returns a new point reflected over the X-axis.

        This method creates a new Point object with the y-coordinate negated, effectively representing this point reflected over the X-axis.
        It does not modify this point but returns a new point with the reflected coordinates.

        @return A new Point object representing this point reflected over the X-axis.
    */
    [[nodiscard]] constexpr Point reflectedOverXAxis() const noexcept
    {
        return { x, -y };
    }

    /** Reflects this point over the Y-axis.

        This method modifies the x-coordinate of this point by negating it, effectively reflecting the point over the Y-axis.
        It is useful for mirroring points in graphical or geometrical transformations.

        @return A reference to this Point object after reflection over the Y-axis.
    */
    constexpr Point& reflectOverYAxis() noexcept
    {
        x = -x;
        return *this;
    }

    /** Returns a new point reflected over the Y-axis.

        This method creates a new Point object with the x-coordinate negated, effectively representing this point reflected over the Y-axis.
        It does not modify this point but returns a new point with the reflected coordinates.

        @return A new Point object representing this point reflected over the Y-axis.
    */
    [[nodiscard]] constexpr Point reflectedOverYAxis() const noexcept
    {
        return { -x, y };
    }

    /** Reflects this point over the origin.

        This method modifies both the x and y coordinates of this point by negating them, effectively reflecting the point over the origin.
        It is useful for mirroring points across both axes in graphical or geometrical transformations.

        @return A reference to this Point object after reflection over the origin.
    */
    constexpr Point& reflectOverOrigin() noexcept
    {
        x = -x;
        y = -y;
        return *this;
    }

    /** Returns a new point reflected over the origin.

        This method creates a new Point object with both coordinates negated, effectively representing this point reflected over the origin.
        It does not modify this point but returns a new point with the reflected coordinates.

        @return A new Point object representing this point reflected over the origin.
    */
    [[nodiscard]] constexpr Point reflectedOverOrigin() const noexcept
    {
        return { -x, -y };
    }

    //==============================================================================
    /** Computes the minimum of this point and another point based on each coordinate.

        This method compares the coordinates of this point with those of another point and returns a new Point object
        whose coordinates are the lesser values for each coordinate. It is useful for bounding calculations or reducing values to constraints.

        @param other Another Point object to compare against.

        @return A new Point object with the minimum x and y coordinates from this point and the other point.
    */
    [[nodiscard]] constexpr Point min (const Point& other) const noexcept
    {
        return { std::min (x, other.x), std::min (y, other.y) };
    }

    /** Computes the maximum of this point and another point based on each coordinate.

        This method compares the coordinates of this point with those of another point and returns a new Point object
        whose coordinates are the greater values for each coordinate. It is useful for bounding calculations or extending values to constraints.

        @param other Another Point object to compare against.

        @return A new Point object with the maximum x and y coordinates from this point and the other point.
    */
    [[nodiscard]] constexpr Point max (const Point& other) const noexcept
    {
        return { std::max (x, other.x), std::max (y, other.y) };
    }

    /** Returns a new point with the absolute values of the coordinates of this point.

        This method creates a new Point object whose coordinates are the absolute values of this point's coordinates.
        It is useful for ensuring non-negative coordinate values in calculations and transformations.

        @return A new Point object with the absolute values of the x and y coordinates.
    */
    [[nodiscard]] constexpr Point abs() const noexcept
    {
        return { std::abs (x), std::abs (y) };
    }

    /** Floors the coordinates of this point.

        This method modifies the coordinates of this point by applying the floor function to each coordinate,
        which rounds down to the nearest integer less than or equal to the original coordinate.
        It is useful for coordinate calculations that require integer values or for aligning points to a grid.

        @tparam T The numeric type of the coordinates, constrained to floating-point types.

        @return A new Point object with the floored coordinates.
    */
    template <class T = ValueType>
    [[nodiscard]] constexpr auto floor() const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Point>
    {
        return { std::floor (x), std::floor (y) };
    }

    /** Ceils the coordinates of this point.

        This method modifies the coordinates of this point by applying the ceil function to each coordinate,
        which rounds up to the nearest integer greater than or equal to the original coordinate.
        It is useful for coordinate calculations that require integer values or for aligning points to a grid.

        @tparam T The numeric type of the coordinates, constrained to floating-point types.

        @return A new Point object with the ceiled coordinates.
    */
    template <class T = ValueType>
    [[nodiscard]] constexpr auto ceil() const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Point>
    {
        return { std::ceil (x), std::ceil (y) };
    }

    //==============================================================================
    /** Linearly interpolates between this point and another point by a specified factor.

        This method computes the coordinates of a point that lies a certain way between this point and another point, based on a specified interpolation factor.
        The interpolation factor should be between 0 and 1, where 0 represents this point and 1 represents the other point.
        It is useful for smooth transitions or animations between points.

        @param other Another Point object to interpolate towards.
        @param delta The interpolation factor, typically between 0.0 and 1.0, representing the position between the two points.

        @return A new Point object representing the interpolated position between this point and the other point.
    */
    [[nodiscard]] constexpr Point lerp (const Point& other, float delta) const noexcept
    {
        return {
            static_cast<ValueType> ((1.0f - delta) * x + delta * other.x),
            static_cast<ValueType> ((1.0f - delta) * y + delta * other.y)
        };
    }

    //==============================================================================
    // TODO - doxygen
    constexpr Point& transform (const AffineTransform& t) noexcept
    {
        t.transformPoints (x, y);
        return *this;
    }

    // TODO - doxygen
    [[nodiscard]] constexpr Point transformed (const AffineTransform& t) const noexcept
    {
        Point result (*this);
        result.transform (t);
        return result;
    }

    //==============================================================================
    /** Converts the coordinates of this point to another numeric type.

        This method creates a new Point object with coordinates that are the result of casting this point's coordinates to another specified numeric type.
        It is useful when working with different numeric types across various calculations or APIs that require specific types.

        @tparam T The target numeric type to which the coordinates will be converted.

        @return A new Point<T> object with the coordinates converted to type T.
    */
    template <class T>
    [[nodiscard]] constexpr Point<T> to() const noexcept
    {
        return { static_cast<T> (x), static_cast<T> (y) };
    }

    template <class T = ValueType>
    [[nodiscard]] constexpr auto roundToInt() const noexcept
        -> std::enable_if_t<std::is_floating_point_v<T>, Point<int>>
    {
        return { yup::roundToInt (x), yup::roundToInt (y) };
    }

    //==============================================================================
    /** Adds the coordinates of another point to this point and returns the result.

        This method creates a new Point object whose coordinates are the result of adding the coordinates of another point to this point's coordinates.
        It is useful for vector addition or combining shifts in coordinates.

        @param other Another Point object whose coordinates are added to this point's coordinates.

        @return A new Point object representing the sum of this point and the other point.
    */
    constexpr Point operator+ (const Point& other) const noexcept
    {
        Point result (*this);
        result += other;
        return result;
    }

    /** Adds the coordinates of another point to this point.

        This operator modifies this point's coordinates by adding the coordinates of another point to it.
        It is useful for vector addition or combining shifts in coordinates directly on this point.

        @param other Another Point object whose coordinates are added to this point's coordinates.

        @return A reference to this Point object after the coordinates have been added.
    */
    constexpr Point& operator+= (const Point& other) noexcept
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    /** Adds a scalar value to both coordinates of this point and returns the result.

        This method creates a new Point object whose coordinates are the result of adding a scalar value to both coordinates of this point.
        It is useful for uniform shifts in both x and y coordinates.

        @param amount The scalar value to add to both coordinates.

        @return A new Point object representing the coordinates of this point plus the scalar amount.
    */
    constexpr Point operator+ (ValueType amount) const noexcept
    {
        Point result (*this);
        result += amount;
        return result;
    }

    /** Adds a scalar value to both coordinates of this point.

        This operator modifies this point's coordinates by adding a scalar value to both coordinates.
        It is useful for uniform shifts in both x and y coordinates directly on this point.

        @param amount The scalar value to add to both coordinates.

        @return A reference to this Point object after the scalar amount has been added.
    */
    constexpr Point& operator+= (ValueType amount) noexcept
    {
        x += amount;
        y += amount;
        return *this;
    }

    /** Subtracts the coordinates of another point from this point and returns the result.

        This method creates a new Point object whose coordinates are the result of subtracting the coordinates of another point from this point's coordinates.
        It is useful for vector subtraction or determining the relative position between points.

        @param other Another Point object whose coordinates are subtracted from this point's coordinates.

        @return A new Point object representing the difference between this point and the other point.
    */
    constexpr Point operator- (const Point& other) const noexcept
    {
        Point result (*this);
        result -= other;
        return result;
    }

    /** Subtracts the coordinates of another point from this point.

        This operator modifies this point's coordinates by subtracting the coordinates of another point from it.
        It is useful for vector subtraction or adjusting positions based on relative movements.

        @param other Another Point object whose coordinates are subtracted from this point's coordinates.

        @return A reference to this Point object after the coordinates have been subtracted.
    */
    constexpr Point& operator-= (const Point& other) noexcept
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    /** Subtracts a scalar value from both coordinates of this point and returns the result.

        This method creates a new Point object whose coordinates are the result of subtracting a scalar value from both coordinates of this point.
        It is useful for uniform reductions in both x and y coordinates.

        @param amount The scalar value to subtract from both coordinates.

        @return A new Point object representing the coordinates of this point minus the scalar amount.
    */
    constexpr Point operator- (ValueType amount) const noexcept
    {
        Point result (*this);
        result -= amount;
        return result;
    }

    /** Subtracts a scalar value from both coordinates of this point.

        This operator modifies this point's coordinates by subtracting a scalar value from both coordinates.
        It is useful for uniform reductions in both x and y coordinates directly on this point.

        @param amount The scalar value to subtract from both coordinates.

        @return A reference to this Point object after the scalar amount has been subtracted.
    */
    constexpr Point& operator-= (ValueType amount) noexcept
    {
        x -= amount;
        y -= amount;
        return *this;
    }

    /** Multiplies the coordinates of this point by the coordinates of another point and returns the result.

        This method creates a new Point object whose coordinates are the result of multiplying the coordinates of this point by those of another point.
        It is useful for element-wise multiplication of coordinates, often used in scaling or transforming points non-uniformly.

        @param other Another Point object whose coordinates are multiplied by this point's coordinates.

        @return A new Point object representing the product of the coordinates of this point and the other point.
    */
    constexpr Point operator* (const Point& other) const noexcept
    {
        Point result (*this);
        result *= other;
        return result;
    }

    /** Multiplies the coordinates of this point by the coordinates of another point.

        This operator modifies this point's coordinates by multiplying them by the coordinates of another point.
        It is useful for element-wise multiplication of coordinates directly on this point, often used in scaling or transforming points non-uniformly.

        @param other Another Point object whose coordinates are multiplied by this point's coordinates.

        @return A reference to this Point object after the coordinates have been multiplied.
    */
    constexpr Point& operator*= (const Point& other) noexcept
    {
        x *= other.x;
        y *= other.y;
        return *this;
    }

    /** Multiplies both coordinates of this point by a scalar value and returns the result.

        This method creates a new Point object whose coordinates are the result of multiplying both coordinates of this point by a scalar value.
        It is useful for uniform scaling of points, often used in resizing or adjusting distances.

        @param scale The scalar value by which to multiply both coordinates.

        @return A new Point object representing the scaled coordinates of this point.
    */
    constexpr Point operator* (ValueType scale) const noexcept
    {
        Point result (*this);
        result *= scale;
        return result;
    }

    /** Multiplies both coordinates of this point by a scalar value.

        This operator modifies this point's coordinates by multiplying them by a scalar value.
        It is useful for uniform scaling of points directly on this point, often used in resizing or adjusting distances.

        @param scale The scalar value by which to multiply both coordinates.

        @return A reference to this Point object after the coordinates have been scaled.
    */
    constexpr Point& operator*= (ValueType scale) noexcept
    {
        x *= scale;
        y *= scale;
        return *this;
    }

    /** Divides the coordinates of this point by the coordinates of another point and returns the result.

        This method creates a new Point object whose coordinates are the result of dividing the coordinates of this point by those of another point.
        It is useful for element-wise division of coordinates, often used in proportional adjustments or creating ratios.

        @param other Another Point object whose coordinates are used as divisors for this point's coordinates.

        @return A new Point object representing the quotient of the coordinates of this point and the other point.
    */
    constexpr Point operator/ (const Point& other) const noexcept
    {
        Point result (*this);
        result /= other;
        return result;
    }

    /** Divides the coordinates of this point by the coordinates of another point.

        This operator modifies this point's coordinates by dividing them by the coordinates of another point.
        It is useful for element-wise division of coordinates directly on this point, often used in proportional adjustments or creating ratios.

        @param other Another Point object whose coordinates are used as divisors for this point's coordinates.

        @return A reference to this Point object after the coordinates have been divided.
    */
    constexpr Point& operator/= (const Point& other) noexcept
    {
        if (other.x != ValueType (0))
            x /= other.x;

        if (other.y != ValueType (0))
            y /= other.y;

        return *this;
    }

    /** Divides both coordinates of this point by a scalar value and returns the result.

        This method creates a new Point object whose coordinates are the result of dividing both coordinates of this point by a scalar value.
        It is useful for uniform scaling down of points, often used in resizing or adjusting distances inversely.

        @param scale The scalar value by which to divide both coordinates.

        @return A new Point object representing the scaled-down coordinates of this point.
    */
    constexpr Point operator/ (ValueType scale) const noexcept
    {
        Point result (*this);
        result /= scale;
        return result;
    }

    /** Divides both coordinates of this point by a scalar value.

        This operator modifies this point's coordinates by dividing them by a scalar value.
        It is useful for uniform scaling down of points directly on this point, often used in resizing or adjusting distances inversely.

        @param scale The scalar value by which to divide both coordinates.

        @return A reference to this Point object after the coordinates have been scaled down.
    */
    constexpr Point& operator/= (ValueType scale) noexcept
    {
        if (scale != ValueType (0))
        {
            x /= scale;
            y /= scale;
        }

        return *this;
    }

    /** Returns a new point with negated coordinates.

        This operator creates a new Point object with both coordinates negated, effectively representing this point reflected over the origin.
        It is useful for creating opposite or inverted points in coordinate transformations.

        @return A new Point object with negated coordinates.
    */
    constexpr Point operator-() const noexcept
    {
        return reflectedOverOrigin();
    }

    //==============================================================================
    /** Returns true if the two points are approximately equal. */
    constexpr bool approximatelyEqualTo (const Point& other) const noexcept
    {
        if constexpr (std::is_floating_point_v<ValueType>)
        {
            return approximatelyEqual (x, other.x)
                && approximatelyEqual (y, other.y);
        }
        else
        {
            return *this == other;
        }
    }

    //==============================================================================
    /** Checks for equality between this point and another point based on their coordinates.

        This operator determines if both the x and y coordinates of this point are exactly equal to those of another point.
        It is used for comparing positions or points in 2D space.

        @param other Another Point object to compare against.

        @return True if the coordinates of both points are equal, false otherwise.
    */
    constexpr bool operator== (const Point& other) const noexcept
    {
        return x == other.x && y == other.y;
    }

    /** Checks for inequality between this point and another point based on their coordinates.

        This operator determines if either the x or y coordinates of this point are not equal to those of another point.
        It is used for distinguishing positions or points in 2D space.

        @param other Another Point object to compare against.

        @return True if the coordinates of the points are not equal, false if they are equal.
    */
    constexpr bool operator!= (const Point& other) const noexcept
    {
        return ! (*this == other);
    }

private:
    ValueType x = 0;
    ValueType y = 0;
};

/** Stream output operator for Point objects.

    This operator allows Point objects to be written to output streams in a formatted manner.
    It is useful for logging, debugging, or any other scenario where points need to be output to streams.

    @tparam ValueType The numeric type of the point's coordinates.

    @param string1 The output stream to which the point's coordinates are written.
    @param p The Point object whose coordinates are written to the stream.

    @return A reference to the output stream after the point's coordinates have been written.
*/
template <class ValueType>
YUP_API String& YUP_CALLTYPE operator<< (String& string1, const Point<ValueType>& p)
{
    auto [x, y] = p;

    string1 << x << ", " << y;

    return string1;
}

template <std::size_t I, class ValueType>
[[nodiscard]] constexpr ValueType get (const Point<ValueType>& point) noexcept
{
    if constexpr (I == 0)
        return point.getX();
    else if constexpr (I == 1)
        return point.getY();
    else
        static_assert (dependentFalse<I>);
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
