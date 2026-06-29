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
    , texture (std::exchange (other.texture, {}))
    , renderCanvas (std::exchange (other.renderCanvas, {}))
{
}

Image& Image::operator= (const Image& other)
{
    if (this != &other)
    {
        bitmapData = other.bitmapData;
        texture = nullptr;
        renderCanvas = nullptr;
    }

    return *this;
}

Image& Image::operator= (Image&& other) noexcept
{
    if (this != &other)
    {
        bitmapData = std::exchange (other.bitmapData, {});
        texture = std::exchange (other.texture, {});
        renderCanvas = std::exchange (other.renderCanvas, {});
    }

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
Image Image::duplicate() const
{
    Image result;

    if (bitmapData != nullptr)
    {
        result.bitmapData = new BitmapData (
            bitmapData->getWidth(),
            bitmapData->getHeight(),
            bitmapData->getPixelFormat(),
            bitmapData->getRawData());
    }

    return result;
}

//==============================================================================

ResultValue<Image> Image::loadFromData (Span<const uint8> imageData)
{
    auto stream = std::make_unique<MemoryInputStream> (imageData.data(), imageData.size(), false);

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (stream.release());
    if (reader == nullptr || reader->width <= 0 || reader->height <= 0)
        return makeResultValueFail ("Unable to decode image");

    auto image = reader->readImage();
    if (! image.isValid())
        return makeResultValueFail ("Unable to decode image");

    return makeResultValueOk (image);
}

//==============================================================================
bool Image::createTextureIfNotPresent (GraphicsContext& context) const
{
    if (getTexture() != nullptr)
        return true;

    if (bitmapData == nullptr)
        return false;

    auto width = getWidth();
    auto height = getHeight();

    auto renderContext = context.renderContext();
    if (renderContext == nullptr || renderContext->impl() == nullptr)
        return false;

    const auto numPixels = static_cast<size_t> (width) * static_cast<size_t> (height);
    std::vector<uint8> texturePixels (numPixels * 4u);

    const auto sourceData = bitmapData->getRawData();
    const auto* source = sourceData.data();

    switch (bitmapData->getPixelFormat())
    {
        case PixelFormat::Grayscale:
            ColorVectorOperations::convertGrayscaleToRGBA (source, texturePixels.data(), static_cast<int> (numPixels));
            break;

        case PixelFormat::RGB:
            ColorVectorOperations::convertRGBToRGBA (source, texturePixels.data(), static_cast<int> (numPixels));
            break;

        case PixelFormat::RGBA:
            std::memcpy (texturePixels.data(), source, texturePixels.size());
            ColorVectorOperations::premultiplyRGBA (texturePixels.data(), static_cast<int> (numPixels));
            break;
    }

    texture = renderContext->impl()->makeImageTexture (
        width,
        height,
        rive::math::msb (width | height),
        rive::GPUTextureFormat::rgba32,
        texturePixels.data());

    renderCanvas = nullptr;

    return true;
}

void Image::invalidateTexture()
{
    texture = nullptr;
    renderCanvas = nullptr;
}

//==============================================================================
void Image::adoptTexture (rive::rcp<rive::gpu::Texture> t)
{
    texture = std::move (t);
    renderCanvas = nullptr;
}

void Image::adoptRenderCanvas (rive::rcp<rive::gpu::RenderCanvas> canvas)
{
    renderCanvas = std::move (canvas);
    texture = nullptr;
}

rive::rcp<rive::gpu::Texture> Image::getTexture() const
{
    if (renderCanvas != nullptr)
        return renderCanvas->renderImage()->refTexture();

    return texture;
}

rive::rcp<rive::gpu::RenderCanvas> Image::getRenderCanvas() const
{
    return renderCanvas;
}

rive::RenderImage* Image::getRenderImage() const
{
    if (renderCanvas != nullptr)
        return renderCanvas->renderImage();

    return nullptr;
}

} // namespace yup
