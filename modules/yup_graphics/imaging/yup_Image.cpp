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

namespace
{
uint8 premultiplyComponent (uint8 component, uint8 alpha) noexcept
{
    return static_cast<uint8> ((static_cast<uint32> (component) * static_cast<uint32> (alpha) + 127u) / 255u);
}

uint8 unpremultiplyComponent (uint8 component, uint8 alpha) noexcept
{
    if (alpha == 0)
        return 0;

    const auto value = (static_cast<uint32> (component) * 255u + static_cast<uint32> (alpha) / 2u) / static_cast<uint32> (alpha);
    return static_cast<uint8> (value > 255u ? 255u : value);
}

void writeARGBAsRGBAPremultiplied (uint32 argb, uint8* destination) noexcept
{
    const auto alpha = static_cast<uint8> ((argb >> 24) & 0xFF);

    destination[0] = premultiplyComponent (static_cast<uint8> ((argb >> 16) & 0xFF), alpha);
    destination[1] = premultiplyComponent (static_cast<uint8> ((argb >> 8) & 0xFF), alpha);
    destination[2] = premultiplyComponent (static_cast<uint8> (argb & 0xFF), alpha);
    destination[3] = alpha;
}

std::unique_ptr<const uint8[]> copyRGBAPremultipliedAsRGBA (const rive::Bitmap& bitmap)
{
    const auto numPixels = static_cast<size_t> (bitmap.width()) * static_cast<size_t> (bitmap.height());
    auto destination = std::make_unique<uint8[]> (numPixels * 4u);

    const auto* source = bitmap.bytes();
    auto* target = destination.get();

    for (size_t i = 0; i < numPixels; ++i)
    {
        const auto alpha = source[3];

        target[0] = unpremultiplyComponent (source[0], alpha);
        target[1] = unpremultiplyComponent (source[1], alpha);
        target[2] = unpremultiplyComponent (source[2], alpha);
        target[3] = alpha;

        source += 4;
        target += 4;
    }

    return std::unique_ptr<const uint8[]> (destination.release());
}
} // namespace

//==============================================================================
void BitmapData::setPixelColor (int x, int y, Color color)
{
    setPixel (x, y, color.getARGB());
}

Color BitmapData::getPixelColor (int x, int y) const
{
    return Color (getPixel (x, y));
}

void BitmapData::fillColor (Color color)
{
    fill (color.getARGB());
}

//==============================================================================
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
bool Image::isValid() const noexcept
{
    return bitmapData != nullptr;
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
void Image::setPixel (int x, int y, uint32 color)
{
    jassert (bitmapData != nullptr);

    bitmapData->setPixel (x, y, color);
}

void Image::setPixelColor (int x, int y, Color color)
{
    jassert (bitmapData != nullptr);

    bitmapData->setPixelColor (x, y, color);
}

uint32 Image::getPixel (int x, int y) const
{
    jassert (bitmapData != nullptr);

    return bitmapData->getPixel (x, y);
}

Color Image::getPixelColor (int x, int y) const
{
    jassert (bitmapData != nullptr);

    return bitmapData->getPixelColor (x, y);
}

void Image::fill (uint32 color)
{
    bitmapData->fill (color);
}

void Image::fillColor (Color color)
{
    bitmapData->fillColor (color);
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

    auto renderContext = context.renderContext();
    if (renderContext == nullptr || renderContext->impl() == nullptr)
        return false;

    std::vector<uint8> texturePixels (static_cast<size_t> (width) * static_cast<size_t> (height) * 4u);
    auto* destination = texturePixels.data();

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            writeARGBAsRGBAPremultiplied (bitmapData->getPixel (x, y), destination);
            destination += 4;
        }
    }

    texture = renderContext->impl()->makeImageTexture (
        width,
        height,
        rive::math::msb (width | height),
        rive::GPUTextureFormat::rgba32,
        texturePixels.data());

    return true;
}

void Image::invalidateTexture()
{
    texture = nullptr;
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
        return makeResultValueFail ("Unable to decode image");

    Image result;

    const auto pixelFormat = bitmap->pixelFormat();
    const auto imagePixelFormat = pixelFormat == rive::Bitmap::PixelFormat::RGB
                                    ? PixelFormat::RGB
                                    : PixelFormat::RGBA;
    auto pixelData = pixelFormat == rive::Bitmap::PixelFormat::RGBAPremul
                       ? copyRGBAPremultipliedAsRGBA (*bitmap)
                       : bitmap->detachBytes();

    result.bitmapData = new BitmapData (bitmap->width(), bitmap->height(), imagePixelFormat, std::move (pixelData));

    return makeResultValueOk (result);
}

} // namespace yup
