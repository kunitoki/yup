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
/** Class representing a 2D affine transformation.

    This class encapsulates a transformation matrix for performing various linear
    transformations such as translation, rotation, and scaling in 2D space. An affine
    transformation is a geometric operation that modifies the spatial relationships
    between points while preserving lines and parallelism (but not necessarily distances
    and angles). It allows for the transformation of coordinates and can be modified
    or combined with other transformations to execute complex changes in the geometry
    of graphics and other spatial data.
*/
class JUCE_API AffineTransform
{
public:
    //==============================================================================
    /** Default constructor

        Constructs an AffineTransform object initialized to the identity transformation, which does not modify any points that it is applied to.
    */
    constexpr AffineTransform() noexcept
        : AffineTransform (identity())
    {
    }

    /** Parameterized constructor

        Constructs an AffineTransform object with specified matrix components.

        @param scaleX Component that scales the x-coordinate.
        @param shearX Component that skews the x-coordinate.
        @param translateX Component that translates the x-coordinate.
        @param shearY Component that skews the y-coordinate.
        @param scaleY Component that scales the y-coordinate.
        @param translateY Component that translates the y-coordinate.
    */
    constexpr AffineTransform (
        float scaleX,
        float shearX,
        float translateX,
        float shearY,
        float scaleY,
        float translateY) noexcept
        : scaleX (scaleX)
        , shearX (shearX)
        , translateX (translateX)
        , shearY (shearY)
        , scaleY (scaleY)
        , translateY (translateY)
    {
    }

    //==============================================================================
    /** Copy and move constructors and assignment operators. */
    constexpr AffineTransform (const AffineTransform& other) noexcept = default;
    constexpr AffineTransform (AffineTransform&& other) noexcept = default;
    constexpr AffineTransform& operator= (const AffineTransform& other) noexcept = default;
    constexpr AffineTransform& operator= (AffineTransform&& other) noexcept = default;

    //==============================================================================
    /** Get scaleX component

        Returns the scaleX component of the AffineTransform object.

        @return The scaleX component of this AffineTransform.
    */
    constexpr float getScaleX() const noexcept
    {
        return scaleX;
    }

    /** Get shearX component

        Returns the shearX component of the AffineTransform object.

        @return The shearX component of this AffineTransform.
    */
    constexpr float getShearX() const noexcept
    {
        return shearX;
    }

    /** Get translateX component

        Returns the translateX component of the AffineTransform object.

        @return The translateX component of this AffineTransform.
    */
    constexpr float getTranslateX() const noexcept
    {
        return translateX;
    }

    /** Get shearY component

        Returns the shearY component of the AffineTransform object.

        @return The shearY component of this AffineTransform.
    */
    constexpr float getShearY() const noexcept
    {
        return shearY;
    }

    /** Get scaleY component

        Returns the scaleY component of the AffineTransform object.

        @return The scaleY component of this AffineTransform.
    */
    constexpr float getScaleY() const noexcept
    {
        return scaleY;
    }

    /** Get translateY component

        Returns the translateY component of the AffineTransform object.

        @return The translateY component of this AffineTransform.
    */
    constexpr float getTranslateY() const noexcept
    {
        return translateY;
    }

    //==============================================================================
    /** Get span of matrix array

        Returns a span of constant values of the underlying array representing the matrix components of the AffineTransform.

        @return A span to constant matrix components array.
    */
    [[nodiscard]] constexpr Span<const float> getMatrixPoints() const noexcept
    {
        return { std::addressof (m[0]), 6 };
    }

    //==============================================================================
    /** Check if the transformation is the identity matrix

        Checks if the AffineTransform object represents the identity transformation, which does not modify any points.

        @return True if this is the identity transformation, false otherwise.
    */
    constexpr bool isIdentity() const noexcept
    {
        return *this == AffineTransform();
    }

    /** Reset to identity transformation

        Resets the AffineTransform to the identity transformation, which does not modify any points that it is applied to.

        @return A reference to this reset AffineTransform object.
    */
    constexpr AffineTransform& resetToIdentity() noexcept
    {
        *this = identity();
        return *this;
    }

    /** Create identity transformation

        Creates an AffineTransform object representing the identity transformation, which does not modify any points.

        @return An AffineTransform object initialized as the identity transformation.
    */
    [[nodiscard]] static constexpr AffineTransform identity() noexcept
    {
        return { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
    }

    //==============================================================================
    /** Create an inverted transformation

        Creates a new AffineTransform object that represents this transformation inverted, or the identity.

        @return A new AffineTransform object representing the inverted transformation.
    */
    [[nodiscard]] constexpr AffineTransform inverted() const noexcept
    {
	    double determinant = getDeterminant();

	    if (! approximatelyEqual (determinant, 0.0))
	    {
	        determinant = 1.0 / determinant;

            auto dst00 = static_cast<float> ( m[4] * determinant);
	        auto dst10 = static_cast<float> (-m[3] * determinant);
	        auto dst01 = static_cast<float> (-m[1] * determinant);
	        auto dst11 = static_cast<float> ( m[0] * determinant);

	        return {
                dst00, dst01, -m[2] * dst00 - m[5] * dst01,
                dst10, dst11, -m[2] * dst10 - m[5] * dst11
            };
	    }

	    // singularity..
	    return *this;
    }

    //==============================================================================
    /** Create a translated transformation

        Creates a new AffineTransform object that represents this transformation translated by specified amounts in the x and y directions.

        @param tx The amount to translate in the x-direction.
        @param ty The amount to translate in the y-direction.

        @return A new AffineTransform object representing the translated transformation.
    */
    [[nodiscard]] constexpr AffineTransform translated (float tx, float ty) const noexcept
    {
        return { scaleX, shearX, translateX + tx, shearY, scaleY, translateY + ty };
    }

    /** Create a translation transformation

        Creates an AffineTransform object representing a translation by specified amounts in the x and y directions.

        @param tx The amount to translate in the x-direction.
        @param ty The amount to translate in the y-direction.

        @return An AffineTransform object representing the specified translation.
    */
    [[nodiscard]] static constexpr AffineTransform translation (float tx, float ty) noexcept
    {
        return AffineTransform (1.0f, 0.0f, tx, 0.0f, 1.0f, ty);
    }

    //==============================================================================
    /** Create a rotated transformation

        Creates a new AffineTransform object that represents this transformation rotated by a specified angle around the origin.

        @param angleInRadians The angle in radians by which to rotate.

        @return A new AffineTransform object representing the rotated transformation.
    */
    [[nodiscard]] constexpr AffineTransform rotated (float angleInRadians) const noexcept
    {
        if (angleInRadians == 0.0f)
            return identity();

        const float cosTheta = std::cosf (angleInRadians);
        const float sinTheta = std::sinf (angleInRadians);

        return {
            cosTheta * scaleX - sinTheta * shearY,
            cosTheta * shearX - sinTheta * scaleY,
            cosTheta * translateX - sinTheta * translateY,
            sinTheta * scaleX + cosTheta * shearY,
            sinTheta * shearX + cosTheta * scaleY,
            sinTheta * translateX + cosTheta * translateY
        };
    }

    /** Create a rotated transformation around a point

        Creates a new AffineTransform object that represents this transformation rotated by a specified angle around a specified point.

        @param angleInRadians The angle in radians by which to rotate.
        @param centerX The x-coordinate of the point around which to rotate.
        @param centerY The y-coordinate of the point around which to rotate.

        @return A new AffineTransform object representing the rotated transformation.
    */
    [[nodiscard]] constexpr AffineTransform rotated (float angleInRadians, float centerX, float centerY) const noexcept
    {
        return followedBy (rotation (angleInRadians, centerX, centerY));
    }

    /** Create a rotation transformation

        Creates an AffineTransform object representing a rotation by a specified angle around the origin.

        @param angleInRadians The angle in radians by which to rotate.

        @return An AffineTransform object representing the specified rotation.
    */
    [[nodiscard]] static constexpr AffineTransform rotation (float angleInRadians) noexcept
    {
        if (angleInRadians == 0.0f)
            return identity();

        const float cosTheta = std::cosf (angleInRadians);
        const float sinTheta = std::sinf (angleInRadians);

        return AffineTransform (cosTheta, -sinTheta, 0.0f, sinTheta, cosTheta, 0.0f);
    }

    /** Create a rotation transformation around a point

        Creates an AffineTransform object representing a rotation by a specified angle around a specified point.

        @param angleInRadians The angle in radians by which to rotate.
        @param centerX The x-coordinate of the point around which to rotate.
        @param centerY The y-coordinate of the point around which to rotate.

        @return An AffineTransform object representing the specified rotation around the point.
    */
    [[nodiscard]] static constexpr AffineTransform rotation (float angleInRadians, float centerX, float centerY) noexcept
    {
        if (angleInRadians == 0.0f)
            return identity();

        const float cosTheta = std::cosf (angleInRadians);
        const float sinTheta = std::sinf (angleInRadians);

        return AffineTransform (
            cosTheta,
            -sinTheta,
            -cosTheta * centerX + sinTheta * centerY + centerY,
            sinTheta,
            cosTheta,
            -sinTheta * centerX + -cosTheta * centerY + centerY);
    }

    //==============================================================================
    /** Create a scaled transformation

        Creates a new AffineTransform object that represents this transformation scaled uniformly by a specified factor.

        @param factor The uniform scale factor to apply.

        @return A new AffineTransform object representing the scaled transformation.
    */
    [[nodiscard]] constexpr AffineTransform scaled (float factor) const noexcept
    {
        return {
            factor * scaleX,
            factor * shearX,
            factor * translateX,
            factor * shearY,
            factor * scaleY,
            factor * translateY
        };
    }

    /** Create a scaled transformation non-uniformly

        Creates a new AffineTransform object that represents this transformation scaled by specified factors along the x and y axes.

        @param factorX The scale factor to apply to the x-axis.
        @param factorY The scale factor to apply to the y-axis.

        @return A new AffineTransform object representing the non-uniformly scaled transformation.
    */
    [[nodiscard]] constexpr AffineTransform scaled (float factorX, float factorY) const noexcept
    {
        return {
            factorX * scaleX,
            factorX * shearX,
            factorX * translateX,
            factorY * shearY,
            factorY * scaleY,
            factorY * translateY
        };
    }

    /** Create a scaled transformation non-uniformly around a point

        Creates a new AffineTransform object that represents this transformation scaled by specified factors along the x and y axes around a specified point.

        @param factorX The scale factor to apply to the x-axis.
        @param factorY The scale factor to apply to the y-axis.
        @param centerX The x-coordinate of the center point for scaling.
        @param centerY The y-coordinate of the center point for scaling.

        @return A new AffineTransform object representing the non-uniformly scaled transformation around the point.
    */
    [[nodiscard]] constexpr AffineTransform scaled (float factorX, float factorY, float centerX, float centerY) const noexcept
    {
        return {
            factorX * scaleX,
            factorX * shearX,
            factorX * translateX + centerX * (1.0f - factorX),
            factorY * shearY,
            factorY * scaleY,
            factorY * translateY + centerY * (1.0f - factorY)
        };
    }

    /** Create a scaling transformation

        Creates an AffineTransform object representing a uniform scaling by a specified factor.

        @param factor The uniform scale factor to apply.

        @return An AffineTransform object representing the specified scaling.
    */
    [[nodiscard]] static constexpr AffineTransform scaling (float factor) noexcept
    {
        return { factor, 0.0f, 0.0f, 0.0f, factor, 0.0f };
    }

    /** Create a scaling transformation non-uniformly

        Creates an AffineTransform object representing a non-uniform scaling by specified factors along the x and y axes.

        @param factorX The scale factor to apply to the x-axis.
        @param factorY The scale factor to apply to the y-axis.

        @return An AffineTransform object representing the specified non-uniform scaling.
    */
    [[nodiscard]] static constexpr AffineTransform scaling (float factorX, float factorY) noexcept
    {
        return { factorX, 0.0f, 0.0f, 0.0f, factorY, 0.0f };
    }

    /** Create a scaling transformation non-uniformly around a point

        Creates an AffineTransform object representing a non-uniform scaling by specified factors along the x and y axes around a specified point.

        @param factorX The scale factor to apply to the x-axis.
        @param factorY The scale factor to apply to the y-axis.
        @param centerX The x-coordinate of the center point for scaling.
        @param centerY The y-coordinate of the center point for scaling.

        @return An AffineTransform object representing the specified non-uniform scaling around the point.
    */
    [[nodiscard]] static constexpr AffineTransform scaling (float factorX, float factorY, float centerX, float centerY) noexcept
    {
        return { factorX, 0.0f, centerX * (1.0f - factorX), 0.0f, factorY, centerY * (1.0f - factorY) };
    }

    //==============================================================================
    /** Create a sheared transformation

        Creates a new AffineTransform object that represents this transformation sheared by specified factors along the x and y axes.

        @param factorX The shear factor to apply to the x-axis.
        @param factorY The shear factor to apply to the y-axis.

        @return A new AffineTransform object representing the sheared transformation.
    */
    [[nodiscard]] constexpr AffineTransform sheared (float factorX, float factorY) const noexcept
    {
        return {
            scaleX + factorX * shearY,
            shearX + factorX * scaleY,
            translateX,
            shearY + factorY * scaleX,
            scaleY + factorY * shearX,
            translateY
        };
    }

    /** Create a shearing transformation

        Creates an AffineTransform object representing a shearing by specified factors along the x and y axes.

        @param factorX The shear factor to apply to the x-axis.
        @param factorY The shear factor to apply to the y-axis.

        @return An AffineTransform object representing the specified shearing.
    */
    [[nodiscard]] static constexpr AffineTransform shearing (float factorX, float factorY) noexcept
    {
        return { 1.0f, factorX, 0.0f, factorY, 1.0f, 0.0f };
    }

    //==============================================================================
    /** Create a transformation that follows another

        Creates a new AffineTransform object that represents this transformation followed by another specified AffineTransform.

        @param other The AffineTransform to follow this one.

        @return A new AffineTransform object representing the combined transformation.
    */
    [[nodiscard]] constexpr AffineTransform followedBy (const AffineTransform& other) const noexcept
    {
        return {
            scaleX * other.scaleX + shearY * other.shearX,
            shearX * other.scaleX + scaleY * other.shearX,
            translateX * other.scaleX + translateY * other.shearX + other.translateX,
            scaleX * other.shearY + shearY * other.scaleY,
            shearX * other.shearY + scaleY * other.scaleY,
            translateX * other.shearY + translateY * other.scaleY + other.translateY
        };
    }

    //==============================================================================
    // TODO - doxygen
    [[nodiscard]] constexpr float getDeterminant() const noexcept
    {
        return (scaleX * scaleY) - (shearX * shearY);
    }

    // TODO - doxygen
    [[nodiscard]] constexpr float getScaleFactor() const noexcept
    {
        return (juce_abs (scaleX) + juce_abs (scaleY)) / 2.0f;
    }

    //==============================================================================
    /** Transform a point

        Applies the transformation to a point represented by its x and y coordinates, modifying the coordinates in place.

        @param x A reference to the x-coordinate of the point.
        @param y A reference to the y-coordinate of the point.
    */
    template <class T>
    constexpr void transformPoint (T& x, T& y) const noexcept
    {
        const T originalX = x;
        x = static_cast<T> (scaleX * originalX + shearX * y + translateX);
        y = static_cast<T> (shearY * originalX + scaleY * y + translateY);
    }

    /** Transform multiple points

        Applies the transformation to multiple points, modifying each point's coordinates in place. This method handles pairs of coordinates and can process multiple pairs at once.

        @param x A reference to the x-coordinate of the first point.
        @param y A reference to the y-coordinate of the first point.
        @param args Additional pairs of coordinates to be transformed.

        @return A tuple representing the transformed coordinates of the last point processed.
    */
    template <class T, class... Args>
    constexpr void transformPoints (T& x, T& y, Args&&... args) const noexcept
    {
        transformPoint (x, y);

        if constexpr (sizeof...(Args) > 0)
            transformPoints (std::forward<Args> (args)...);
    }

    //==============================================================================
    /** Equality operator

        Compares this AffineTransform with another AffineTransform for equality.

        @param other A reference to the other AffineTransform to compare with.

        @return True if all corresponding matrix components are equal, false otherwise.
    */
    constexpr bool operator== (const AffineTransform& other) const noexcept
    {
        return m[0] == other.m[0]
            && m[1] == other.m[1]
            && m[2] == other.m[2]
            && m[3] == other.m[3]
            && m[4] == other.m[4]
            && m[5] == other.m[5];
    }

    /** Inequality operator

        Compares this AffineTransform with another AffineTransform for inequality.

        @param other A reference to the other AffineTransform to compare with.

        @return True if any corresponding matrix components are not equal, false otherwise.
    */
    constexpr bool operator!= (const AffineTransform& other) const noexcept
    {
        return ! (*this == other);
    }

private:
    union
    {
        float m[6];

        struct
        {
            float scaleX;
            float shearX;
            float translateX;
            float shearY;
            float scaleY;
            float translateY;
        };
    };
};

template <std::size_t I>
constexpr float get (const AffineTransform& transform) noexcept
{
    if constexpr (I == 0)
        return transform.getScaleX();
    else if constexpr (I == 1)
        return transform.getShearX();
    else if constexpr (I == 2)
        return transform.getTranslateX();
    else if constexpr (I == 3)
        return transform.getShearY();
    else if constexpr (I == 4)
        return transform.getScaleY();
    else if constexpr (I == 5)
        return transform.getTranslateY();
    else
        static_assert (dependentFalse<I>);
}

} // namespace yup

namespace std
{

template <>
struct tuple_size<yup::AffineTransform>
{
    inline static constexpr std::size_t value = 6;
};

template <std::size_t I>
struct tuple_element<I, yup::AffineTransform>
{
    using type = float;
};

} // namespace std
