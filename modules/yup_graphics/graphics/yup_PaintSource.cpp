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

PaintSource::PaintSource() = default;

PaintSource::~PaintSource() = default;

//==============================================================================
void PaintSource::markDirty() noexcept
{
    dirty = true;
}

void PaintSource::releaseResources()
{
    canvas = nullptr;
    context = nullptr;
    shrinkStreak = 0;
    dirty = true;
}

//==============================================================================
GpuTexture::Ptr PaintSource::rasterize (GraphicsContext& targetContext, const AffineTransform& contentToDevice)
{
    if (rasterizing)
        return nullptr;

    const auto contentSize = getContentSize();
    const auto contentWidth = contentSize.getWidth();
    const auto contentHeight = contentSize.getHeight();
    if (! (contentWidth > 0.0f) || ! (contentHeight > 0.0f) || ! std::isfinite (contentWidth) || ! std::isfinite (contentHeight))
        return nullptr;

    if (context != std::addressof (targetContext))
        releaseResources();

    context = std::addressof (targetContext);

    const auto device = targetContext.getGpuDevice();
    if (device == nullptr)
        return nullptr;

    const auto maxDim = jmin (rive::offscreen::kMaxDim, device->getMaximumTextureSize (GpuTextureType::texture2D));
    if (maxDim == 0)
        return nullptr;

    // Device pixels per content unit along an axis, quantized up to sixteenths so
    // tiny transform changes keep the same raster, then the texels the extent needs
    const auto texelsFor = [] (float unitLength, float extent)
    {
        if (! (unitLength > 0.0f) || ! std::isfinite (unitLength))
            return 1.0f;

        const auto perUnit = std::ceil (unitLength * 16.0f) / 16.0f;
        return jmax (1.0f, std::ceil (extent * perUnit));
    };

    auto wantWidth = texelsFor (std::hypot (contentToDevice.getScaleX(), contentToDevice.getShearY()), contentWidth);
    auto wantHeight = texelsFor (std::hypot (contentToDevice.getShearX(), contentToDevice.getScaleY()), contentHeight);

    // Capped proportionally, so a capped raster keeps the content's aspect
    const auto largest = jmax (wantWidth, wantHeight);
    if (largest > static_cast<float> (maxDim))
    {
        const auto shrink = static_cast<float> (maxDim) / largest;
        wantWidth = jmin (wantWidth * shrink, static_cast<float> (maxDim));
        wantHeight = jmin (wantHeight * shrink, static_cast<float> (maxDim));
    }

    const auto haveWidth = canvas != nullptr ? static_cast<uint32> (canvas->getWidth()) : 0u;
    const auto haveHeight = canvas != nullptr ? static_cast<uint32> (canvas->getHeight()) : 0u;

    const auto resize = rive::offscreen::decideRasterResize (jmax (1u, static_cast<uint32> (wantWidth)),
                                                             jmax (1u, static_cast<uint32> (wantHeight)),
                                                             haveWidth,
                                                             haveHeight,
                                                             &shrinkStreak);

    if (canvas == nullptr || resize.reallocate)
    {
        // A new canvas, so draws already recorded keep sampling the previous texture
        canvas = GpuCanvas::create (targetContext,
                                    static_cast<int> (jmin (resize.widthPx, maxDim)),
                                    static_cast<int> (jmin (resize.heightPx, maxDim)));
        dirty = true;

        if (canvas == nullptr)
            return nullptr;
    }

    if (dirty)
    {
        const ScopedValueSetter<bool> guard (rasterizing, true);
        dirty = false;

        // Stretched over the whole canvas, so tiling has no slack at the far edges
        auto& g = canvas->beginDraw();
        g.setTransform (AffineTransform::scaling (static_cast<float> (canvas->getWidth()) / contentWidth,
                                                  static_cast<float> (canvas->getHeight()) / contentHeight));
        paintContent (g);

        if (! canvas->commit())
        {
            canvas = nullptr;
            dirty = true;
            return nullptr;
        }
    }

    return canvas->asTexture();
}

} // namespace yup
