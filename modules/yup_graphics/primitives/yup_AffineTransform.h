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

class JUCE_API AffineTransform
{
public:
    //==============================================================================
    constexpr AffineTransform() noexcept
        : AffineTransform (1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f)
    {
    }

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
    constexpr AffineTransform (const AffineTransform& other) noexcept = default;
    constexpr AffineTransform (AffineTransform&& other) noexcept = default;
    constexpr AffineTransform& operator=(const AffineTransform& other) noexcept = default;
    constexpr AffineTransform& operator=(AffineTransform&& other) noexcept = default;

    //==============================================================================
    constexpr float getScaleX() const noexcept
    {
        return scaleX;
    }

    constexpr float getShearX() const noexcept
    {
        return shearX;
    }

    constexpr float getTranslateX() const noexcept
    {
        return translateX;
    }

    constexpr float getShearY() const noexcept
    {
        return shearY;
    }

    constexpr float getScaleY() const noexcept
    {
        return scaleY;
    }

    constexpr float getTranslateY() const noexcept
    {
        return translateY;
    }

    //==============================================================================
    constexpr const float* getMatrixPoints() const noexcept
    {
        return std::addressof (m[0]);
    }

    //==============================================================================
    constexpr bool isIdentity() const noexcept
    {
        return *this == AffineTransform();
    }

    constexpr AffineTransform& resetToIdentity() noexcept
    {
        *this = identity();
        return *this;
    }

    static constexpr AffineTransform identity() noexcept
    {
        return { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
    }

    //==============================================================================
    constexpr AffineTransform& translate (float tx, float ty) noexcept
    {
        translateX += tx;
        translateY += ty;
        return *this;
    }

    constexpr AffineTransform translated (float tx, float ty) const noexcept
    {
        AffineTransform result (*this);
        result.translated (tx, ty);
        return result;
    }

    static constexpr AffineTransform translation (float tx, float ty) noexcept
    {
        return AffineTransform (1.0f, 0.0f, tx, 0.0f, 1.0f, ty);
    }

    //==============================================================================
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

    constexpr AffineTransform& rotate (float angleInRadians, float centerX, float centerY) noexcept
    {
        followBy (rotation (angleInRadians, centerX, centerY));
        return *this;
    }

    constexpr AffineTransform rotated (float angleInRadians) const noexcept
    {
        AffineTransform result (*this);
        result.rotate (angleInRadians);
        return result;
    }

    constexpr AffineTransform rotated (float angleInRadians, float centerX, float centerY) const noexcept
    {
        AffineTransform result (*this);
        result.rotate (angleInRadians, centerX, centerY);
        return result;
    }

    static constexpr AffineTransform rotation (float angleInRadians) noexcept
    {
        const float cosTheta = std::cosf (angleInRadians);
        const float sinTheta = std::sinf (angleInRadians);

        return AffineTransform (cosTheta, -sinTheta, 0.0f, sinTheta, cosTheta, 0.0f);
    }

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

    constexpr AffineTransform scaled (float factor) const noexcept
    {
        AffineTransform result (*this);
        result.scale (factor);
        return result;
    }

    constexpr AffineTransform scaled (float factorX, float factorY) const noexcept
    {
        AffineTransform result (*this);
        result.scale (factorX, factorY);
        return result;
    }

    constexpr AffineTransform scaled (float factorX, float factorY, float centerX, float centerY) const noexcept
    {
        AffineTransform result (*this);
        result.scale (factorX, factorY, centerX, centerY);
        return result;
    }

    static constexpr AffineTransform scaling (float factor) noexcept
    {
        return { factor, 0.0f, 0.0f, 0.0f, factor, 0.0f };
    }

    static constexpr AffineTransform scaling (float factorX, float factorY) noexcept
    {
        return { factorX, 0.0f, 0.0f, 0.0f, factorY, 0.0f };
    }

    static constexpr AffineTransform scaling (float factorX, float factorY, float centerX, float centerY) noexcept
    {
        return { factorX, 0.0f, centerX * (1.0f - factorX), 0.0f, factorY, centerY * (1.0f - factorY) };
    }

    //==============================================================================
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

    constexpr AffineTransform sheared (float factorX, float factorY) const noexcept
    {
        AffineTransform result (*this);
        result.shear (factorX, factorY);
        return result;
    }

    static constexpr AffineTransform shearing (float factorX, float factorY) noexcept
    {
        return { 1.0f, factorX, 0.0f, factorY, 1.0f, 0.0f };
    }

    //==============================================================================
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

    constexpr AffineTransform followedBy (const AffineTransform& other) const noexcept
    {
        AffineTransform result (*this);
        result.followBy (other);
        return result;
    }

    //==============================================================================
    template <class T>
    constexpr void transformPoint (T& x, T& y) const noexcept
    {
        const T originalX = x;
        x = static_cast<T> (scaleX * originalX + shearX * y + translateX);
        y = static_cast<T> (shearY * originalX + scaleY * y + translateY);
    }

    template <class T, class... Args>
    constexpr std::tuple<T, T> transformPoints (T& x, T& y, Args&&... args) const noexcept
    {
        transformPoint (x, y);

        if constexpr (sizeof... (Args) > 0)
            transformPoints (std::forward<Args> (args)...);
    }

    //==============================================================================
    constexpr bool operator== (const AffineTransform& other) const noexcept
    {
        return m[0] == other.m[0]
            && m[1] == other.m[1]
            && m[2] == other.m[2]
            && m[3] == other.m[3]
            && m[4] == other.m[4]
            && m[5] == other.m[5];
    }

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
