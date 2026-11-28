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

DropShadowEffect::DropShadowEffect (Color shadowColor, float radiusPoints, Point<float> offsetPoints) noexcept
    : colorArgb (shadowColor.getARGB())
    , radius (jmax (0.0f, radiusPoints))
    , offsetX (offsetPoints.getX())
    , offsetY (offsetPoints.getY())
{
}

DropShadowEffect::~DropShadowEffect() = default;

void DropShadowEffect::setColor (Color shadowColor) noexcept
{
    const auto newColor = shadowColor.getARGB();

    if (colorArgb.exchange (newColor, std::memory_order_relaxed) != newColor)
        invalidate();
}

Color DropShadowEffect::getColor() const noexcept
{
    return Color (colorArgb.load (std::memory_order_relaxed));
}

void DropShadowEffect::setRadius (float radiusPoints) noexcept
{
    const auto newRadius = jmax (0.0f, radiusPoints);

    if (radius.exchange (newRadius, std::memory_order_relaxed) != newRadius)
        invalidate();
}

float DropShadowEffect::getRadius() const noexcept
{
    return radius.load (std::memory_order_relaxed);
}

void DropShadowEffect::setOffset (Point<float> offsetPoints) noexcept
{
    const bool movedX = offsetX.exchange (offsetPoints.getX(), std::memory_order_relaxed) != offsetPoints.getX();
    const bool movedY = offsetY.exchange (offsetPoints.getY(), std::memory_order_relaxed) != offsetPoints.getY();

    if (movedX || movedY)
        invalidate();
}

Point<float> DropShadowEffect::getOffset() const noexcept
{
    return { offsetX.load (std::memory_order_relaxed), offsetY.load (std::memory_order_relaxed) };
}

void DropShadowEffect::releaseResources()
{
    pipeline = nullptr;
    targetA = nullptr;
    targetB = nullptr;
    pipelineDevice = nullptr;
    compileAttempted = false;
}

void DropShadowEffect::apply (Graphics& g, GpuTexture::Ptr inputTexture, Rectangle<float> bounds)
{
    if (inputTexture == nullptr || ! inputTexture->isValid() || bounds.isEmpty())
        return;

    const Color shadowColor = getColor();
    const float radiusPoints = getRadius();
    const auto shadowOffset = getOffset();

    if (! shadowColor.isTransparent())
    {
        const float pixelScale = static_cast<float> (inputTexture->getWidth()) / bounds.getWidth();
        if (auto blurred = detail::runSeparableBlur (g.getGraphicsContext(),
                                                     pipelineDevice,
                                                     pipeline,
                                                     targetA,
                                                     targetB,
                                                     compileAttempted,
                                                     inputTexture,
                                                     radiusPoints * pixelScale,
                                                     true))
        {
            const auto saved = g.saveState();
            g.setTint (shadowColor.withAlpha (static_cast<uint8> (255)));
            g.setOpacity (g.getOpacity() * shadowColor.getAlphaFloat());
            g.drawTexture (blurred, bounds.translated (shadowOffset));
        }
    }

    g.drawTexture (inputTexture, bounds);
}

} // namespace yup
