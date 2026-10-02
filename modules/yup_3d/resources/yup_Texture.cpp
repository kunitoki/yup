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

//==============================================================================

Texture::Texture (Image sourceImage, const GpuSamplerDesc& sampler, bool isSrgb)
    : image (std::move (sourceImage))
    , samplerDesc (sampler)
    , srgb (isSrgb)
{
}

Texture::Texture (GpuTexture::Ptr texture, const GpuSamplerDesc& sampler, bool isSrgb)
    : samplerDesc (sampler)
    , srgb (isSrgb)
    , gpuOnly (true)
    , gpuTexture (std::move (texture))
{
}

Texture::~Texture() = default;

void Texture::setGpuTexture (GpuTexture::Ptr newTexture)
{
    jassert (gpuOnly); // Only textures created from a GpuTexture can be replaced
    gpuTexture = std::move (newTexture);
}

//==============================================================================

std::vector<std::vector<uint8>> Texture::createMipChain (const Image& image)
{
    std::vector<std::vector<uint8>> levels;

    if (! image.isValid() || image.getWidth() <= 0 || image.getHeight() <= 0)
        return levels;

    auto width = image.getWidth();
    auto height = image.getHeight();

    levels.push_back (image.getPixelData().toRGBA (false));

    while (width > 1 || height > 1)
    {
        const auto& source = levels.back();
        const auto sourceWidth = width;
        const auto sourceHeight = height;

        width = jmax (1, width / 2);
        height = jmax (1, height / 2);

        std::vector<uint8> level (static_cast<size_t> (width * height * 4));

        for (int y = 0; y < height; ++y)
        {
            const auto y0 = jmin (y * 2, sourceHeight - 1);
            const auto y1 = jmin (y * 2 + 1, sourceHeight - 1);

            for (int x = 0; x < width; ++x)
            {
                const auto x0 = jmin (x * 2, sourceWidth - 1);
                const auto x1 = jmin (x * 2 + 1, sourceWidth - 1);

                for (int channel = 0; channel < 4; ++channel)
                {
                    const auto sum = source[static_cast<size_t> ((y0 * sourceWidth + x0) * 4 + channel)]
                                   + source[static_cast<size_t> ((y0 * sourceWidth + x1) * 4 + channel)]
                                   + source[static_cast<size_t> ((y1 * sourceWidth + x0) * 4 + channel)]
                                   + source[static_cast<size_t> ((y1 * sourceWidth + x1) * 4 + channel)];

                    level[static_cast<size_t> ((y * width + x) * 4 + channel)] = static_cast<uint8> ((sum + 2) / 4);
                }
            }
        }

        levels.push_back (std::move (level));
    }

    return levels;
}

//==============================================================================

GpuTexture::Ptr Texture::getGpuTexture (const GpuDevice::Ptr& device)
{
    if (device == nullptr)
        return nullptr;

    useDevice (device);

    if (gpuTexture != nullptr || gpuOnly)
        return gpuTexture;

    const auto levels = createMipChain (image);
    if (levels.empty())
        return nullptr;

    GpuTextureDesc desc (static_cast<uint32_t> (image.getWidth()), static_cast<uint32_t> (image.getHeight()), GpuTextureFormat::rgba8unorm);
    desc.mipLevels = static_cast<uint32_t> (levels.size());

    auto texture = GpuTexture::create (device, desc);
    if (texture == nullptr)
        return nullptr;

    for (size_t level = 0; level < levels.size(); ++level)
    {
        GpuTextureDataDesc data;
        data.data = levels[level].data();
        data.mipLevel = static_cast<uint32_t> (level);

        if (! texture->upload (data))
            return nullptr;
    }

    gpuTexture = std::move (texture);
    return gpuTexture;
}

GpuSampler::Ptr Texture::getGpuSampler (const GpuDevice::Ptr& device)
{
    if (device == nullptr)
        return nullptr;

    useDevice (device);

    if (gpuSampler == nullptr)
    {
        // Anisotropy needs device support, and linear filtering everywhere on some backends
        auto desc = samplerDesc;
        const auto allLinear = desc.minFilter == GpuFilter::linear && desc.magFilter == GpuFilter::linear && desc.mipmapFilter == GpuFilter::linear;

        if (! allLinear || ! device->isAnisotropicFilteringAvailable())
            desc.maxAnisotropy = 1;

        gpuSampler = GpuSampler::create (device, desc);
    }

    return gpuSampler;
}

void Texture::useDevice (const GpuDevice::Ptr& device)
{
    if (device == cachedDevice)
        return;

    cachedDevice = device;
    gpuSampler = nullptr;

    if (! gpuOnly)
        gpuTexture = nullptr;
}

} // namespace yup
