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

GlowEffect::GlowEffect (float radiusPoints, float glowIntensity, Color tintColor) noexcept
    : colorArgb (tintColor.getARGB())
    , radius (jmax (0.0f, radiusPoints))
    , intensity (jlimit (0.0f, 1.0f, glowIntensity))
{
}

GlowEffect::~GlowEffect() = default;

void GlowEffect::setRadius (float radiusPoints) noexcept
{
    radius.store (jmax (0.0f, radiusPoints), std::memory_order_relaxed);
}

float GlowEffect::getRadius() const noexcept
{
    return radius.load (std::memory_order_relaxed);
}

void GlowEffect::setIntensity (float glowIntensity) noexcept
{
    intensity.store (jlimit (0.0f, 1.0f, glowIntensity), std::memory_order_relaxed);
}

float GlowEffect::getIntensity() const noexcept
{
    return intensity.load (std::memory_order_relaxed);
}

void GlowEffect::setColor (Color tintColor) noexcept
{
    colorArgb.store (tintColor.getARGB(), std::memory_order_relaxed);
}

Color GlowEffect::getColor() const noexcept
{
    return Color (colorArgb.load (std::memory_order_relaxed));
}

void GlowEffect::apply (Graphics& g, GpuTexture::Ptr inputTexture, Rectangle<float> bounds)
{
    if (inputTexture == nullptr || ! inputTexture->isValid() || bounds.isEmpty())
        return;

    g.drawTexture (inputTexture, bounds);

    const float radiusPoints = getRadius();
    const float glowIntensity = getIntensity();
    const Color tintColor = getColor();

    if (radiusPoints <= 0.0001f || glowIntensity <= 0.0001f || tintColor.isTransparent())
        return;

    const float pixelScale = static_cast<float> (inputTexture->getWidth()) / bounds.getWidth();
    if (auto blurred = detail::runSeparableBlur (g.getGraphicsContext(),
                                                 pipelineDevice,
                                                 pipeline,
                                                 targetA,
                                                 targetB,
                                                 compileAttempted,
                                                 inputTexture,
                                                 radiusPoints * pixelScale,
                                                 false))
    {
        const auto saved = g.saveState();
        g.setTint (tintColor.withAlpha (static_cast<uint8> (255)));
        g.setOpacity (g.getOpacity() * tintColor.getAlphaFloat() * glowIntensity);
        g.setBlendMode (BlendMode::Additive);
        g.setAdditiveAmount (1.0f);
        g.drawTexture (blurred, bounds);
    }
}

} // namespace yup
