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
/**
 * @brief Class representing a 2D affine transformation.
 *
 * This class encapsulates a transformation matrix for performing various linear transformations such as
 * translation, rotation, and scaling in 2D space. An affine transformation is a geometric operation that
 * modifies the spatial relationships between points while preserving lines and parallelism (but not
 * necessarily distances and angles). It allows for the transformation of coordinates and can be modified
 * or combined with other transformations to execute complex changes in the geometry of graphics and
 * other spatial data.
 */
class JUCE_API AffineTransform
{
public:
    //==============================================================================
    /**
     * @brief Default constructor
     *
     * Constructs an AffineTransform object initialized to the identity transformation, which does not modify any points that it is applied to.
     */
    constexpr AffineTransform() noexcept
        : AffineTransform (1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f)
    {
    }

    /**
     * @brief Parameterized constructor
     *
     * Constructs an AffineTransform object with specified matrix components.
     *
     * @param scaleX Component that scales the x-coordinate.
     * @param shearX Component that skews the x-coordinate.
     * @param translateX Component that translates the x-coordinate.
     * @param shearY Component that skews the y-coordinate.
     * @param scaleY Component that scales the y-coordinate.
     * @param translateY Component that translates the y-coordinate.
     */
    constexpr AffineTransform (
            float scaleX, float shearX, float translateX,
            float shearY, float scaleY, float translateY) noexcept
        : scaleX (scaleX)
        , shearX (shearX)
        , translateX (translateX)
        , shearY (shearY)
        , scaleY (scaleY)
        , translateY (translateY)
    {
    }

    //==============================================================================
    /**
     * @brief Copy and move constructors and assignment operators.
     */
    constexpr AffineTransform (const AffineTransform& other) noexcept = default;
    constexpr AffineTransform (AffineTransform&& other) noexcept = default;
    constexpr AffineTransform& operator=(const AffineTransform& other) noexcept = default;
    constexpr AffineTransform& operator=(AffineTransform&& other) noexcept = default;

    //==============================================================================
    /**
     * @brief Get scaleX component
     *
     * Returns the scaleX component of the AffineTransform object.
     *
     * @return The scaleX component of this AffineTransform.
     */
    constexpr float getScaleX() const noexcept
    {
        return scaleX;
    }

    /**
     * @brief Get shearX component
     *
     * Returns the shearX component of the AffineTransform object.
     *
     * @return The shearX component of this AffineTransform.
     */
    constexpr float getShearX() const noexcept
    {
        return shearX;
    }

    /**
     * @brief Get translateX component
     *
     * Returns the translateX component of the AffineTransform object.
     *
     * @return The translateX component of this AffineTransform.
     */
    constexpr float getTranslateX() const noexcept
    {
        return translateX;
    }

    /**
     * @brief Get shearY component
     *
     * Returns the shearY component of the AffineTransform object.
     *
     * @return The shearY component of this AffineTransform.
     */
    constexpr float getShearY() const noexcept
    {
        return shearY;
    }

    /**
     * @brief Get scaleY component
     *
     * Returns the scaleY component of the AffineTransform object.
     *
     * @return The scaleY component of this AffineTransform.
     */
    constexpr float getScaleY() const noexcept
    {
        return scaleY;
    }

    /**
     * @brief Get translateY component
     *
     * Returns the translateY component of the AffineTransform object.
     *
     * @return The translateY component of this AffineTransform.
     */
    constexpr float getTranslateY() const noexcept
    {
        return translateY;
    }

    //==============================================================================
    /**
     * @brief Get span of matrix array
     *
     * Returns a span of constant values of the underlying array representing the matrix components of the AffineTransform.
     *
     * @return A span to constant matrix components array.
     */
    constexpr Span<const float> getMatrixPoints() const noexcept
    {
        return { std::addressof (m[0]), 6 };
    }

    //==============================================================================
    /**
     * @brief Check if the transformation is the identity matrix
     *
     * Checks if the AffineTransform object represents the identity transformation, which does not modify any points.
     *
     * @return True if this is the identity transformation, false otherwise.
     */
    constexpr bool isIdentity() const noexcept
    {
        return *this == AffineTransform();
    }

    /**
     * @brief Reset to identity transformation
     *
     * Resets the AffineTransform to the identity transformation, which does not modify any points that it is applied to.
     *
     * @return A reference to this reset AffineTransform object.
     */
    constexpr AffineTransform& resetToIdentity() noexcept
    {
        *this = identity();
        return *this;
    }

    /**
     * @brief Create identity transformation
     *
     * Creates an AffineTransform object representing the identity transformation, which does not modify any points.
     *
     * @return An AffineTransform object initialized as the identity transformation.
     */
    static constexpr AffineTransform identity() noexcept
    {
        return { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
    }

    //==============================================================================
    /**
     * @brief Translate the transformation
     *
     * Modifies this AffineTransform by translating it by specified amounts in the x and y directions.
     *
     * @param tx The amount to translate in the x-direction.
     * @param ty The amount to translate in the y-direction.
     *
     * @return A reference to this translated AffineTransform object.
     */
    constexpr AffineTransform& translate (float tx, float ty) noexcept
    {
        translateX += tx;
        translateY += ty;
        return *this;
    }

    /**
     * @brief Create a translated transformation
     *
     * Creates a new AffineTransform object that represents this transformation translated by specified amounts in the x and y directions.
     *
     * @param tx The amount to translate in the x-direction.
     * @param ty The amount to translate in the y-direction.
     *
     * @return A new AffineTransform object representing the translated transformation.
     */
    constexpr AffineTransform translated (float tx, float ty) const noexcept
    {
        AffineTransform result (*this);
        result.translated (tx, ty);
        return result;
    }

    /**
     * @brief Create a translation transformation
     *
     * Creates an AffineTransform object representing a translation by specified amounts in the x and y directions.
     *
     * @param tx The amount to translate in the x-direction.
     * @param ty The amount to translate in the y-direction.
     *
     * @return An AffineTransform object representing the specified translation.
     */
    static constexpr AffineTransform translation (float tx, float ty) noexcept
    {
        return AffineTransform (1.0f, 0.0f, tx, 0.0f, 1.0f, ty);
    }

    //==============================================================================
    /**
     * @brief Rotate the transformation
     *
     * Modifies this AffineTransform by rotating it by a specified angle around the origin.
     *
     * @param angleInRadians The angle in radians by which to rotate.
     *
     * @return A reference to this rotated AffineTransform object.
     */
    constexpr AffineTransform& rotate (float angleInRadians) noexcept
    {
        const float cosTheta = std::cosf (angleInRadians);
        const float sinTheta = std::sinf (angleInRadians);

        scaleX = cosTheta * scaleX - sinTheta * shearX;
        shearX = cosTheta * shearX - sinTheta * scaleY;
        translateX = cosTheta * translateX - sinTheta * translateY;
        shearY = sinTheta * scaleX + cosTheta * shearX;
        scaleY = sinTheta * shearX + cosTheta * scaleY;
        translateY = sinTheta * translateX + cosTheta * translateY;
        return *this;
    }

    /**
     * @brief Rotate the transformation around a point
     *
     * Modifies this AffineTransform by rotating it by a specified angle around a specified point.
     *
     * @param angleInRadians The angle in radians by which to rotate.
     * @param centerX The x-coordinate of the point around which to rotate.
     * @param centerY The y-coordinate of the point around which to rotate.
     *
     * @return A reference to this rotated AffineTransform object.
     */
    constexpr AffineTransform& rotate (float angleInRadians, float centerX, float centerY) noexcept
    {
        followBy (rotation (angleInRadians, centerX, centerY));
        return *this;
    }

    /**
     * @brief Create a rotated transformation
     *
     * Creates a new AffineTransform object that represents this transformation rotated by a specified angle around the origin.
     *
     * @param angleInRadians The angle in radians by which to rotate.
     *
     * @return A new AffineTransform object representing the rotated transformation.
     */
    constexpr AffineTransform rotated (float angleInRadians) const noexcept
    {
        AffineTransform result (*this);
        result.rotate (angleInRadians);
        return result;
    }

    /**
     * @brief Create a rotated transformation around a point
     *
     * Creates a new AffineTransform object that represents this transformation rotated by a specified angle around a specified point.
     *
     * @param angleInRadians The angle in radians by which to rotate.
     * @param centerX The x-coordinate of the point around which to rotate.
     * @param centerY The y-coordinate of the point around which to rotate.
     *
     * @return A new AffineTransform object representing the rotated transformation.
     */
    constexpr AffineTransform rotated (float angleInRadians, float centerX, float centerY) const noexcept
    {
        AffineTransform result (*this);
        result.rotate (angleInRadians, centerX, centerY);
        return result;
    }

    /**
     * @brief Create a rotation transformation
     *
     * Creates an AffineTransform object representing a rotation by a specified angle around the origin.
     *
     * @param angleInRadians The angle in radians by which to rotate.
     *
     * @return An AffineTransform object representing the specified rotation.
     */
    static constexpr AffineTransform rotation (float angleInRadians) noexcept
    {
        const float cosTheta = std::cosf (angleInRadians);
        const float sinTheta = std::sinf (angleInRadians);

        return AffineTransform (cosTheta, -sinTheta, 0.0f, sinTheta, cosTheta, 0.0f);
    }

    /**
     * @brief Create a rotation transformation around a point
     *
     * Creates an AffineTransform object representing a rotation by a specified angle around a specified point.
     *
     * @param angleInRadians The angle in radians by which to rotate.
     * @param centerX The x-coordinate of the point around which to rotate.
     * @param centerY The y-coordinate of the point around which to rotate.
     *
     * @return An AffineTransform object representing the specified rotation around the point.
     */
    static constexpr AffineTransform rotation (float angleInRadians, float centerX, float centerY) noexcept
    {
        const float cosTheta = std::cosf (angleInRadians);
        const float sinTheta = std::sinf (angleInRadians);

        return AffineTransform (
            cosTheta,
            -sinTheta,
            centerX - centerX * cosTheta + centerY * sinTheta,
            sinTheta,
            cosTheta,
            centerY - centerX * sinTheta - centerY * cosTheta);
    }

    //==============================================================================
    /**
     * @brief Scale the transformation
     *
     * Modifies this AffineTransform by scaling it uniformly by a specified factor.
     *
     * @param factor The uniform scale factor to apply.
     *
     * @return A reference to this scaled AffineTransform object.
     */
    constexpr AffineTransform& scale (float factor) noexcept
    {
        scaleX *= factor;
        shearX *= factor;
        translateX *= factor;
        shearY *= factor;
        scaleY *= factor;
        translateY *= factor;
        return *this;
    }

    /**
     * @brief Scale the transformation non-uniformly
     *
     * Modifies this AffineTransform by scaling it by specified factors along the x and y axes.
     *
     * @param factorX The scale factor to apply to the x-axis.
     * @param factorY The scale factor to apply to the y-axis.
     *
     * @return A reference to this scaled AffineTransform object.
     */
    constexpr AffineTransform& scale (float factorX, float factorY) noexcept
    {
        scaleX *= factorX;
        shearX *= factorX;
        translateX *= factorX;
        shearY *= factorY;
        scaleY *= factorY;
        translateY *= factorY;
        return *this;
    }

    /**
     * @brief Scale the transformation non-uniformly around a point
     *
     * Modifies this AffineTransform by scaling it by specified factors along the x and y axes around a specified point.
     *
     * @param factorX The scale factor to apply to the x-axis.
     * @param factorY The scale factor to apply to the y-axis.
     * @param centerX The x-coordinate of the center point for scaling.
     * @param centerY The y-coordinate of the center point for scaling.
     *
     * @return A reference to this scaled AffineTransform object.
     */
    constexpr AffineTransform& scale (float factorX, float factorY, float centerX, float centerY) noexcept
    {
        scaleX *= factorX;
        shearX *= factorX;
        translateX = translateX * factorX + centerX * (1.0f - factorX);
        shearY *= factorY;
        scaleY *= factorY;
        translateY = translateY * factorY + centerY * (1.0f - factorY);
        return *this;
    }

    /**
     * @brief Create a scaled transformation
     *
     * Creates a new AffineTransform object that represents this transformation scaled uniformly by a specified factor.
     *
     * @param factor The uniform scale factor to apply.
     *
     * @return A new AffineTransform object representing the scaled transformation.
     */
    constexpr AffineTransform scaled (float factor) const noexcept
    {
        AffineTransform result (*this);
        result.scale (factor);
        return result;
    }

    /**
     * @brief Create a scaled transformation non-uniformly
     *
     * Creates a new AffineTransform object that represents this transformation scaled by specified factors along the x and y axes.
     *
     * @param factorX The scale factor to apply to the x-axis.
     * @param factorY The scale factor to apply to the y-axis.
     *
     * @return A new AffineTransform object representing the non-uniformly scaled transformation.
     */
    constexpr AffineTransform scaled (float factorX, float factorY) const noexcept
    {
        AffineTransform result (*this);
        result.scale (factorX, factorY);
        return result;
    }

    /**
     * @brief Create a scaled transformation non-uniformly around a point
     *
     * Creates a new AffineTransform object that represents this transformation scaled by specified factors along the x and y axes around a specified point.
     *
     * @param factorX The scale factor to apply to the x-axis.
     * @param factorY The scale factor to apply to the y-axis.
     * @param centerX The x-coordinate of the center point for scaling.
     * @param centerY The y-coordinate of the center point for scaling.
     *
     * @return A new AffineTransform object representing the non-uniformly scaled transformation around the point.
     */
    constexpr AffineTransform scaled (float factorX, float factorY, float centerX, float centerY) const noexcept
    {
        AffineTransform result (*this);
        result.scale (factorX, factorY, centerX, centerY);
        return result;
    }

    /**
     * @brief Create a scaling transformation
     *
     * Creates an AffineTransform object representing a uniform scaling by a specified factor.
     *
     * @param factor The uniform scale factor to apply.
     *
     * @return An AffineTransform object representing the specified scaling.
     */
    static constexpr AffineTransform scaling (float factor) noexcept
    {
        return { factor, 0.0f, 0.0f, 0.0f, factor, 0.0f };
    }

    /**
     * @brief Create a scaling transformation non-uniformly
     *
     * Creates an AffineTransform object representing a non-uniform scaling by specified factors along the x and y axes.
     *
     * @param factorX The scale factor to apply to the x-axis.
     * @param factorY The scale factor to apply to the y-axis.
     *
     * @return An AffineTransform object representing the specified non-uniform scaling.
     */
    static constexpr AffineTransform scaling (float factorX, float factorY) noexcept
    {
        return { factorX, 0.0f, 0.0f, 0.0f, factorY, 0.0f };
    }

    /**
     * @brief Create a scaling transformation non-uniformly around a point
     *
     * Creates an AffineTransform object representing a non-uniform scaling by specified factors along the x and y axes around a specified point.
     *
     * @param factorX The scale factor to apply to the x-axis.
     * @param factorY The scale factor to apply to the y-axis.
     * @param centerX The x-coordinate of the center point for scaling.
     * @param centerY The y-coordinate of the center point for scaling.
     *
     * @return An AffineTransform object representing the specified non-uniform scaling around the point.
     */
    static constexpr AffineTransform scaling (float factorX, float factorY, float centerX, float centerY) noexcept
    {
        return { factorX, 0.0f, centerX * (1.0f - factorX), 0.0f, factorY, centerY * (1.0f - factorY) };
    }

    //==============================================================================
    /**
     * @brief Shear the transformation
     *
     * Modifies this AffineTransform by shearing it by specified factors along the x and y axes.
     *
     * @param factorX The shear factor to apply to the x-axis.
     * @param factorY The shear factor to apply to the y-axis.
     *
     * @return A reference to this sheared AffineTransform object.
     */
    constexpr AffineTransform& shear (float factorX, float factorY) noexcept
    {
        scaleX += factorX * shearY;
        shearX += factorX * scaleY;
        translateX += factorX * translateY;
        shearY += factorY * scaleX;
        scaleY += factorY * shearX;
        translateY += factorY * translateX;
        return *this;
    }

    /**
     * @brief Create a sheared transformation
     *
     * Creates a new AffineTransform object that represents this transformation sheared by specified factors along the x and y axes.
     *
     * @param factorX The shear factor to apply to the x-axis.
     * @param factorY The shear factor to apply to the y-axis.
     *
     * @return A new AffineTransform object representing the sheared transformation.
     */
    constexpr AffineTransform sheared (float factorX, float factorY) const noexcept
    {
        AffineTransform result (*this);
        result.shear (factorX, factorY);
        return result;
    }

    /**
     * @brief Create a shearing transformation
     *
     * Creates an AffineTransform object representing a shearing by specified factors along the x and y axes.
     *
     * @param factorX The shear factor to apply to the x-axis.
     * @param factorY The shear factor to apply to the y-axis.
     *
     * @return An AffineTransform object representing the specified shearing.
     */
    static constexpr AffineTransform shearing (float factorX, float factorY) noexcept
    {
        return { 1.0f, factorX, 0.0f, factorY, 1.0f, 0.0f };
    }

    //==============================================================================
    /**
     * @brief Apply another transformation to this one
     *
     * Modifies this AffineTransform by applying another AffineTransform to it, combining their effects in sequence.
     *
     * @param other The AffineTransform to apply to this one.
     *
     * @return A reference to this updated AffineTransform object.
     */
    constexpr AffineTransform& followBy (const AffineTransform& other) noexcept
    {
        scaleX = scaleX * other.scaleX + shearX * other.shearY;
        shearX = scaleX * other.shearX + shearX * other.scaleY;
        translateX = scaleX * other.translateX + shearX * other.translateY + translateX;
        shearY = shearY * other.scaleX + scaleY * other.shearY;
        scaleY = shearY * other.shearX + scaleY * other.scaleY;
        translateY = shearY * other.translateX + scaleY * other.translateY + translateY;
        return *this;
    }

    /**
     * @brief Create a transformation that follows another
     *
     * Creates a new AffineTransform object that represents this transformation followed by another specified AffineTransform.
     *
     * @param other The AffineTransform to follow this one.
     *
     * @return A new AffineTransform object representing the combined transformation.
     */
    constexpr AffineTransform followedBy (const AffineTransform& other) const noexcept
    {
        AffineTransform result (*this);
        result.followBy (other);
        return result;
    }

    //==============================================================================
    /**
     * @brief Transform a point
     *
     * Applies the transformation to a point represented by its x and y coordinates, modifying the coordinates in place.
     *
     * @param x A reference to the x-coordinate of the point.
     * @param y A reference to the y-coordinate of the point.
     */
    template <class T>
    constexpr void transformPoint (T& x, T& y) const noexcept
    {
        const T originalX = x;
        x = static_cast<T> (scaleX * originalX + shearX * y + translateX);
        y = static_cast<T> (shearY * originalX + scaleY * y + translateY);
    }

    /**
     * @brief Transform multiple points
     *
     * Applies the transformation to multiple points, modifying each point's coordinates in place. This method handles pairs of coordinates and can process multiple pairs at once.
     *
     * @param x A reference to the x-coordinate of the first point.
     * @param y A reference to the y-coordinate of the first point.
     * @param args Additional pairs of coordinates to be transformed.
     *
     * @return A tuple representing the transformed coordinates of the last point processed.
     */
    template <class T, class... Args>
    constexpr std::tuple<T, T> transformPoints (T& x, T& y, Args&&... args) const noexcept
    {
        transformPoint (x, y);

        if constexpr (sizeof... (Args) > 0)
            transformPoints (std::forward<Args> (args)...);
    }

    //==============================================================================
    /**
     * @brief Equality operator
     *
     * Compares this AffineTransform with another AffineTransform for equality.
     *
     * @param other A reference to the other AffineTransform to compare with.
     *
     * @return True if all corresponding matrix components are equal, false otherwise.
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

    /**
     * @brief Inequality operator
     *
     * Compares this AffineTransform with another AffineTransform for inequality.
     *
     * @param other A reference to the other AffineTransform to compare with.
     *
     * @return True if any corresponding matrix components are not equal, false otherwise.
     */
    constexpr bool operator!= (const AffineTransform& other) const noexcept
    {
        return !(*this == other);
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
float get (const AffineTransform& transform)
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
        return {}; // TODO - error
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
