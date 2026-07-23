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
Image::Image (int w, int h, PixelFormat fmt)
    : pixelData (new ImagePixelData (w, h, fmt))
{
}

Image::Image (const Image& other)
    : pixelData (other.pixelData)
    , metadata (other.metadata)
{
}

Image::Image (Image&& other) noexcept
    : pixelData (std::exchange (other.pixelData, {}))
    , gpuTexture (std::exchange (other.gpuTexture, {}))
    , metadata (std::exchange (other.metadata, {}))
{
}

Image& Image::operator= (const Image& other)
{
    if (this != &other)
    {
        pixelData = other.pixelData;
        gpuTexture = nullptr;
        metadata = other.metadata;
    }

    return *this;
}

Image& Image::operator= (Image&& other) noexcept
{
    if (this != &other)
    {
        pixelData = std::exchange (other.pixelData, {});
        gpuTexture = std::exchange (other.gpuTexture, {});
        metadata = std::exchange (other.metadata, {});
    }

    return *this;
}

//==============================================================================
bool Image::isValid() const noexcept
{
    return pixelData != nullptr;
}

//==============================================================================
int Image::getWidth() const noexcept
{
    jassert (pixelData != nullptr);

    return pixelData->getWidth();
}

int Image::getHeight() const noexcept
{
    jassert (pixelData != nullptr);

    return pixelData->getHeight();
}

PixelFormat Image::getPixelFormat() const noexcept
{
    jassert (pixelData != nullptr);

    return pixelData->getPixelFormat();
}

int Image::getPixelStride() const noexcept
{
    jassert (pixelData != nullptr);

    return pixelData->getPixelStride();
}

//==============================================================================
void Image::setPixel (int x, int y, uint32 color)
{
    jassert (pixelData != nullptr);

    pixelData->setPixel (x, y, color);
}

void Image::setPixelColor (int x, int y, Color color)
{
    jassert (pixelData != nullptr);

    pixelData->setPixelColor (x, y, color);
}

uint32 Image::getPixel (int x, int y) const
{
    jassert (pixelData != nullptr);

    return pixelData->getPixel (x, y);
}

Color Image::getPixelColor (int x, int y) const
{
    jassert (pixelData != nullptr);

    return pixelData->getPixelColor (x, y);
}

void Image::fill (uint32 color)
{
    pixelData->fill (color);
}

void Image::fillColor (Color color)
{
    pixelData->fillColor (color);
}

void Image::clear()
{
    pixelData->clear();
}

const ImagePixelData& Image::getPixelData() const noexcept
{
    jassert (pixelData != nullptr);

    return *pixelData;
}

ImagePixelData& Image::getPixelData() noexcept
{
    jassert (pixelData != nullptr);

    return *pixelData;
}

Span<const uint8> Image::getRawData() const noexcept
{
    jassert (pixelData != nullptr);

    return pixelData->getRawData();
}

Span<uint8> Image::getRawData() noexcept
{
    jassert (pixelData != nullptr);

    return pixelData->getRawData();
}

//==============================================================================
Image Image::duplicate() const
{
    Image result;

    if (pixelData != nullptr)
    {
        result.pixelData = new ImagePixelData (
            pixelData->getWidth(),
            pixelData->getHeight(),
            pixelData->getPixelFormat(),
            pixelData->getRawData());
    }

    result.metadata = metadata;
    return result;
}

//==============================================================================

Image Image::fromTexture (GpuTexture::Ptr tex)
{
    if (tex == nullptr || ! tex->isValid())
        return {};

    Image image (tex->getWidth(), tex->getHeight());
    image.gpuTexture = std::move (tex);
    return image;
}

//==============================================================================

ResultValue<Image> Image::loadFromData (Span<const uint8> imageData,
                                        const ImageFormat::Options& options)
{
    auto stream = std::make_unique<MemoryInputStream> (imageData.data(), imageData.size(), false);

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (stream.release(), options);
    if (reader == nullptr || reader->width <= 0 || reader->height <= 0)
        return makeResultValueFail ("Unable to decode image");

    auto image = reader->readImage();
    if (! image.isValid())
        return makeResultValueFail ("Unable to decode image");

    image.metadata = reader->metadata;
    return makeResultValueOk (image);
}

//==============================================================================
bool Image::createTextureIfNotPresent (GraphicsContext& context) const
{
    if (getTexture() != nullptr)
        return true;

    if (pixelData == nullptr)
        return false;

    auto width = getWidth();
    auto height = getHeight();

    auto renderContext = context.renderContext();
    if (renderContext == nullptr || renderContext->impl() == nullptr)
        return false;

    const auto texturePixels = pixelData->toRGBA (true);

    auto riveTex = renderContext->impl()->makeImageTexture (
        width,
        height,
        rive::math::msb (width | height),
        rive::GPUTextureFormat::rgba32,
        texturePixels.data(),
        1,     /* blockWidth */
        1,     /* blockHeight */
        false, /* srgb */
        true); /* generateRemainingMips */

    if (riveTex == nullptr)
        return false;

    gpuTexture = GpuTexture::fromGpuTexture (std::move (riveTex), width, height);
    return true;
}

void Image::invalidateTexture()
{
    gpuTexture = nullptr;
}

//==============================================================================

void Image::setGpuTexture (GpuTexture::Ptr tex)
{
    gpuTexture = std::move (tex);
}

GpuTexture::Ptr Image::getGpuTexture() const
{
    return gpuTexture;
}

//==============================================================================

rive::rcp<rive::gpu::Texture> Image::getTexture() const
{
    if (gpuTexture != nullptr)
        return gpuTexture->getOrAdoptGpuTexture();

    return nullptr;
}

} // namespace yup
