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

Image::Image (int w, int h, PixelFormat fmt)
    : bitmapData (new BitmapData (w, h, fmt))
{
}

Image::Image (const Image& other)
    : bitmapData (other.bitmapData)
{
}

Image::Image (Image&& other) noexcept
    : bitmapData (std::exchange (other.bitmapData, {}))
{
}

Image& Image::operator= (const Image& other)
{
    if (this != &other)
        bitmapData = other.bitmapData;

    return *this;
}

Image& Image::operator= (Image&& other) noexcept
{
    if (this != &other)
        bitmapData = std::exchange (other.bitmapData, {});

    return *this;
}

//==============================================================================
int Image::getWidth() const noexcept
{
    jassert (bitmapData != nullptr);

    return bitmapData->getWidth();
}

int Image::getHeight() const noexcept
{
    jassert (bitmapData != nullptr);

    return bitmapData->getHeight();
}

PixelFormat Image::getPixelFormat() const noexcept
{
    jassert (bitmapData != nullptr);

    return bitmapData->getPixelFormat();
}

int Image::getPixelStride() const noexcept
{
    jassert (bitmapData != nullptr);

    return bitmapData->getPixelStride();
}

//==============================================================================
void Image::setPixel (int x, int y, uint32_t color)
{
    jassert (bitmapData != nullptr);

    bitmapData->setPixel (x, y, color);
}

uint32_t Image::getPixel (int x, int y) const
{
    jassert (bitmapData != nullptr);

    return bitmapData->getPixel (x, y);
}

void Image::fill (uint32_t color)
{
    bitmapData->fill (color);
}

void Image::clear()
{
    bitmapData->clear();
}

const BitmapData& Image::getBitmapData() const noexcept
{
    jassert (bitmapData != nullptr);

    return *bitmapData;
}

BitmapData& Image::getBitmapData() noexcept
{
    jassert (bitmapData != nullptr);

    return *bitmapData;
}

Span<const uint8> Image::getRawData() const noexcept
{
    jassert (bitmapData != nullptr);

    return bitmapData->getRawData();
}

Span<uint8> Image::getRawData() noexcept
{
    jassert (bitmapData != nullptr);

    return bitmapData->getRawData();
}

//==============================================================================
/*
Image Image::duplicate() const
{
    Image result;

    if (bitmapData != nullptr)
        result.bitmapData = new BitmapData (*bitmapData);

    return result;
}
*/

bool Image::createTextureIfNotPresent (GraphicsContext& context) const
{
    if (texture != nullptr)
        return true;

    if (bitmapData == nullptr)
        return false;

    auto width = getWidth();
    auto height = getHeight();

    auto renderContext = context.renderContextOrNull();
    if (renderContext == nullptr || renderContext->impl() == nullptr)
        return false;

    texture = renderContext->impl()->makeImageTexture (
        width,
        height,
        rive::math::msb (width | height),
        getRawData().data());

    return true;
}

rive::rcp<rive::gpu::Texture> Image::getTexture() const
{
    return texture;
}

//==============================================================================

ResultValue<Image> Image::loadFromData (Span<const uint8> imageData)
{
    auto bitmap = rive::Bitmap::decode (imageData.data(), imageData.size());
    if (bitmap == nullptr)
        return ResultValue<Image>::fail ("Unable to decode image");

    Image result;

    result.bitmapData = new BitmapData (
        bitmap->width(),
        bitmap->height(),
        (bitmap->pixelFormat() == rive::Bitmap::PixelFormat::RGB)
            ? PixelFormat::RGB
            : PixelFormat::RGBA,
        bitmap->detachBytes());

    return ResultValue<Image>::ok (result);
}

} // namespace yup
