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

namespace yup
{

namespace detail
{

constexpr uint8 kComponentColorMatrixShaderBundle[] = {
#include "yup_gui_shaders/yup_ComponentColorMatrixEffect.inc"
};

struct ColorMatrixPassUniforms
{
    ColorMatrixEffect::Matrix matrix;
    float invWidth;
    float invHeight;
    float pad0;
    float pad1;
};

inline GpuTexture::Ptr runColorMatrix (GraphicsContext& context,
                                       GpuDevice::Ptr& pipelineDevice,
                                       GpuPipeline::Ptr& pipeline,
                                       GpuTarget::Ptr& target,
                                       bool& compileAttempted,
                                       const GpuTexture::Ptr& input,
                                       const ColorMatrixEffect::Matrix& matrix)
{
    const int width = input->getWidth();
    const int height = input->getHeight();
    if (width <= 0 || height <= 0)
        return nullptr;

    if (! ensureComponentEffectPipeline (context,
                                         pipelineDevice,
                                         pipeline,
                                         &target,
                                         nullptr,
                                         compileAttempted,
                                         kComponentColorMatrixShaderBundle,
                                         sizeof (kComponentColorMatrixShaderBundle))
        || ! ensureComponentEffectTarget (context.getGpuDevice(), target, width, height))
    {
        return nullptr;
    }

    auto frame = GpuFrame::begin (context.getGpuDevice());
    if (! frame.isValid())
        return nullptr;

    const ColorMatrixPassUniforms uniforms {
        matrix,
        1.0f / static_cast<float> (width),
        1.0f / static_cast<float> (height),
        0.0f,
        0.0f
    };

    auto pass = target->beginRenderPass (frame, { true, Colors::transparentBlack });
    pass.setPipeline (pipeline);
    pass.setTexture (0, 0, input);
    pass.setUniformBuffer (0, 2, &uniforms, sizeof (uniforms));
    pass.draw (3);
    pass.finish();

    auto outputTexture = target->asTexture();
    frame.submit();
    return outputTexture;
}

} // namespace detail

namespace
{

ColorMatrixEffect::Matrix lerpColorMatrix (const ColorMatrixEffect::Matrix& a,
                                           const ColorMatrixEffect::Matrix& b,
                                           float t) noexcept
{
    ColorMatrixEffect::Matrix out {};
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = a[i] + (b[i] - a[i]) * t;
    return out;
}

} // namespace

//==============================================================================

ColorMatrixEffect::ColorMatrixEffect() noexcept
    : matrix (identityMatrix())
{
}

ColorMatrixEffect::ColorMatrixEffect (const Matrix& initialMatrix) noexcept
    : matrix (initialMatrix)
{
}

ColorMatrixEffect::~ColorMatrixEffect() = default;

ColorMatrixEffect::Matrix ColorMatrixEffect::identityMatrix() noexcept
{
    return {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    };
}

ColorMatrixEffect::Matrix ColorMatrixEffect::grayscaleMatrix (float amount) noexcept
{
    return saturateMatrix (1.0f - jlimit (0.0f, 1.0f, amount));
}

ColorMatrixEffect::Matrix ColorMatrixEffect::sepiaMatrix (float amount) noexcept
{
    static constexpr Matrix fullSepia = {
        0.393f, 0.769f, 0.189f, 0.0f,
        0.349f, 0.686f, 0.168f, 0.0f,
        0.272f, 0.534f, 0.131f, 0.0f,
        0.0f,   0.0f,   0.0f,   1.0f,
        0.0f,   0.0f,   0.0f,   0.0f
    };

    return lerpColorMatrix (identityMatrix(), fullSepia, jlimit (0.0f, 1.0f, amount));
}

ColorMatrixEffect::Matrix ColorMatrixEffect::invertMatrix (float amount) noexcept
{
    const float t = jlimit (0.0f, 1.0f, amount);
    const float scale = 1.0f - 2.0f * t;

    return {
        scale, 0.0f,  0.0f,  0.0f,
        0.0f,  scale, 0.0f,  0.0f,
        0.0f,  0.0f,  scale, 0.0f,
        0.0f,  0.0f,  0.0f,  1.0f,
        t,     t,     t,     0.0f
    };
}

ColorMatrixEffect::Matrix ColorMatrixEffect::saturateMatrix (float factor) noexcept
{
    const float s = jmax (0.0f, factor);
    const float lr = 0.2126f * (1.0f - s);
    const float lg = 0.7152f * (1.0f - s);
    const float lb = 0.0722f * (1.0f - s);

    return {
        lr + s, lg,     lb,     0.0f,
        lr,     lg + s, lb,     0.0f,
        lr,     lg,     lb + s, 0.0f,
        0.0f,   0.0f,   0.0f,   1.0f,
        0.0f,   0.0f,   0.0f,   0.0f
    };
}

ColorMatrixEffect::Matrix ColorMatrixEffect::brightnessMatrix (float factor) noexcept
{
    const float b = jmax (0.0f, factor);

    return {
        b,    0.0f, 0.0f, 0.0f,
        0.0f, b,    0.0f, 0.0f,
        0.0f, 0.0f, b,    0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    };
}

ColorMatrixEffect::Matrix ColorMatrixEffect::contrastMatrix (float factor) noexcept
{
    const float c = jmax (0.0f, factor);
    const float intercept = 0.5f * (1.0f - c);

    return {
        c,         0.0f,      0.0f,      0.0f,
        0.0f,      c,         0.0f,      0.0f,
        0.0f,      0.0f,      c,         0.0f,
        0.0f,      0.0f,      0.0f,      1.0f,
        intercept, intercept, intercept, 0.0f
    };
}

ColorMatrixEffect::Matrix ColorMatrixEffect::hueRotateMatrix (float angleRadians) noexcept
{
    const float c = std::cos (angleRadians);
    const float s = std::sin (angleRadians);

    return {
        0.213f + 0.787f * c - 0.213f * s, 0.715f - 0.715f * c - 0.715f * s, 0.072f - 0.072f * c + 0.928f * s, 0.0f,
        0.213f - 0.213f * c + 0.143f * s, 0.715f + 0.285f * c + 0.140f * s, 0.072f - 0.072f * c - 0.283f * s, 0.0f,
        0.213f - 0.213f * c - 0.787f * s, 0.715f - 0.715f * c + 0.715f * s, 0.072f + 0.928f * c + 0.072f * s, 0.0f,
        0.0f,                             0.0f,                             0.0f,                             1.0f,
        0.0f,                             0.0f,                             0.0f,                             0.0f
    };
}

ColorMatrixEffect::Matrix ColorMatrixEffect::opacityMatrix (float amount) noexcept
{
    const float a = jlimit (0.0f, 1.0f, amount);

    return {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, a,
        0.0f, 0.0f, 0.0f, 0.0f
    };
}

ComponentEffect::Ptr ColorMatrixEffect::grayscale (float amount)
{
    return Ptr (new ColorMatrixEffect (grayscaleMatrix (amount)));
}

ComponentEffect::Ptr ColorMatrixEffect::sepia (float amount)
{
    return Ptr (new ColorMatrixEffect (sepiaMatrix (amount)));
}

ComponentEffect::Ptr ColorMatrixEffect::invert (float amount)
{
    return Ptr (new ColorMatrixEffect (invertMatrix (amount)));
}

ComponentEffect::Ptr ColorMatrixEffect::saturate (float factor)
{
    return Ptr (new ColorMatrixEffect (saturateMatrix (factor)));
}

ComponentEffect::Ptr ColorMatrixEffect::brightness (float factor)
{
    return Ptr (new ColorMatrixEffect (brightnessMatrix (factor)));
}

ComponentEffect::Ptr ColorMatrixEffect::contrast (float factor)
{
    return Ptr (new ColorMatrixEffect (contrastMatrix (factor)));
}

ComponentEffect::Ptr ColorMatrixEffect::hueRotate (float angleRadians)
{
    return Ptr (new ColorMatrixEffect (hueRotateMatrix (angleRadians)));
}

ComponentEffect::Ptr ColorMatrixEffect::opacity (float amount)
{
    return Ptr (new ColorMatrixEffect (opacityMatrix (amount)));
}

void ColorMatrixEffect::setMatrix (const Matrix& newMatrix) noexcept
{
    const ScopedLock sl (matrixLock);
    matrix = newMatrix;
}

ColorMatrixEffect::Matrix ColorMatrixEffect::getMatrix() const noexcept
{
    const ScopedLock sl (matrixLock);
    return matrix;
}

void ColorMatrixEffect::apply (Graphics& g, GpuTexture::Ptr inputTexture, Rectangle<float> bounds)
{
    if (inputTexture == nullptr || ! inputTexture->isValid() || bounds.isEmpty())
        return;

    if (auto outputTexture = detail::runColorMatrix (g.getGraphicsContext(),
                                                     pipelineDevice,
                                                     pipeline,
                                                     target,
                                                     compileAttempted,
                                                     inputTexture,
                                                     getMatrix()))
    {
        g.drawTexture (outputTexture, bounds);
    }
    else
    {
        g.drawTexture (inputTexture, bounds);
    }
}

} // namespace yup
