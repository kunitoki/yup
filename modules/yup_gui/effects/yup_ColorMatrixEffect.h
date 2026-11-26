/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#pragma once

namespace yup
{

//==============================================================================
/**
    A 4x5 color-matrix post-processing effect for a Component subtree.

    Transforms each straight RGBA pixel by four 4-component linear rows plus an RGBA offset:

    @code
    out.r = dot (row0, rgba) + offset.r
    out.g = dot (row1, rgba) + offset.g
    out.b = dot (row2, rgba) + offset.b
    out.a = dot (row3, rgba) + offset.a
    @endcode

    Built-in matrix builders cover the standard CSS/SVG color-matrix primitives (`grayscale`,
    `sepia`, `invert`, `saturate`, `brightness`, `contrast`, `hueRotate`, `opacity`).

    @see ComponentEffect, FilterEffect, Component::setComponentEffect

    @tags{UI}
*/
class YUP_API ColorMatrixEffect : public ComponentEffect
{
public:
    //==============================================================================
    /** 20-float row-major color matrix: 4 RGBA rows followed by 4 RGBA offsets. */
    using Matrix = std::array<float, 20>;

    //==============================================================================
    /** Creates an identity color-matrix effect. */
    ColorMatrixEffect() noexcept;

    /** Creates a color-matrix effect with the given matrix. */
    explicit ColorMatrixEffect (const Matrix& initialMatrix) noexcept;

    /** Destructor. */
    ~ColorMatrixEffect() override;

    //==============================================================================
    /** Returns the identity 4x5 color matrix. */
    static Matrix identityMatrix() noexcept;

    /** Returns a grayscale 4x5 color matrix (`amount` in [0, 1], 1 = full grayscale). */
    static Matrix grayscaleMatrix (float amount = 1.0f) noexcept;

    /** Returns a sepia 4x5 color matrix (`amount` in [0, 1], 1 = full sepia). */
    static Matrix sepiaMatrix (float amount = 1.0f) noexcept;

    /** Returns an RGB inversion 4x5 color matrix (`amount` in [0, 1], 1 = full invert). */
    static Matrix invertMatrix (float amount = 1.0f) noexcept;

    /** Returns a saturation 4x5 color matrix (`factor` = 0 grayscale, 1 identity, >1 saturated). */
    static Matrix saturateMatrix (float factor) noexcept;

    /** Returns a brightness 4x5 color matrix (`factor` = 1 identity). */
    static Matrix brightnessMatrix (float factor) noexcept;

    /** Returns a contrast 4x5 color matrix (`factor` = 1 identity). */
    static Matrix contrastMatrix (float factor) noexcept;

    /** Returns a hue-rotation 4x5 color matrix (`angleRadians` = 0 identity). */
    static Matrix hueRotateMatrix (float angleRadians) noexcept;

    /** Returns an opacity 4x5 color matrix (`amount` in [0, 1], 1 = unchanged). */
    static Matrix opacityMatrix (float amount = 1.0f) noexcept;

    //==============================================================================
    /** Creates a grayscale preset (`amount` in [0, 1], 1 = full grayscale). */
    static Ptr grayscale (float amount = 1.0f);

    /** Creates a sepia preset (`amount` in [0, 1], 1 = full sepia). */
    static Ptr sepia (float amount = 1.0f);

    /** Creates an RGB inversion preset (`amount` in [0, 1], 1 = full invert). */
    static Ptr invert (float amount = 1.0f);

    /** Creates a saturation preset (`factor` = 0 grayscale, 1 identity, >1 saturated). */
    static Ptr saturate (float factor);

    /** Creates a brightness preset (`factor` = 1 identity). */
    static Ptr brightness (float factor);

    /** Creates a contrast preset (`factor` = 1 identity). */
    static Ptr contrast (float factor);

    /** Creates a hue-rotation preset (`angleRadians` = 0 identity). */
    static Ptr hueRotate (float angleRadians);

    /** Creates an opacity preset (`amount` in [0, 1], 1 = unchanged). */
    static Ptr opacity (float amount = 1.0f);

    //==============================================================================
    /** Sets the 4x5 color matrix.

        Call it on the message thread, or with the message manager locked: a matrix that is not
        the one the effect has is shown by the components the effect is set on, which are drawn
        through it again without being painted again.
    */
    void setMatrix (const Matrix& newMatrix) noexcept;

    /** Returns the current 4x5 color matrix. */
    Matrix getMatrix() const noexcept;

    //==============================================================================
    /** @internal */
    void apply (Graphics& g, GpuTexture::Ptr inputTexture, Rectangle<float> bounds) override;

    /** @internal */
    void releaseResources() override;

private:
    Matrix matrix;
    mutable CriticalSection matrixLock;
    GpuDevice::Ptr pipelineDevice;
    GpuPipeline::Ptr pipeline;
    GpuTarget::Ptr target;
    bool compileAttempted = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColorMatrixEffect)
};

} // namespace yup
