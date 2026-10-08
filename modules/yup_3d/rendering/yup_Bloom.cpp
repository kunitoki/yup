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

namespace
{

// Precompiled bundles (do not edit the .inc by hand). Regenerate them after changing
// shaders/yup_BloomDownsample.frag or shaders/yup_BloomUpsample.frag with: just yup3d_shaders
constexpr uint8_t bloomDownsampleShaderBundle[] = {
#include "../shaders/yup_BloomDownsample.inc"
};

constexpr uint8_t bloomUpsampleShaderBundle[] = {
#include "../shaders/yup_BloomUpsample.inc"
};

// std140 mirror of BloomDownsampleBlock in yup_BloomDownsample.frag
struct BloomDownsampleBlock
{
    float params[4]; // x 1 on the first level, y 1 when the source is reversible, z threshold, w knee
    float texel[4];  // xy the size of a source texel in uv, zw the size of the source in texels
};

// std140 mirror of BloomUpsampleBlock in yup_BloomUpsample.frag
struct BloomUpsampleBlock
{
    float params[4]; // xy the tent step in uv, z the scale of the result
};

constexpr int bloomMaxLevels = 6;
constexpr int bloomSmallestLevel = 8;

} // namespace

//==============================================================================
Bloom::Bloom() = default;

Bloom::~Bloom() = default;

//==============================================================================
Result Bloom::prepare (const GpuDevice::Ptr& device)
{
    linearClampSampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::linear, GpuWrapMode::clampToEdge));
    if (linearClampSampler == nullptr)
        return Result::fail ("Unable to create the bloom sampler");

    if (auto result = downsample.prepare (device, bloomDownsampleShaderBundle, sizeof (bloomDownsampleShaderBundle), GpuTextureFormat::rgba16float); result.failed())
        return result;

    // Each upsampled level is added onto the downsampled one already there
    return upsample.prepare (device, bloomUpsampleShaderBundle, sizeof (bloomUpsampleShaderBundle), GpuTextureFormat::rgba16float, true);
}

GpuTexture::Ptr Bloom::render (const GpuDevice::Ptr& device, GpuFrame& frame, FrameResources& resources,
                               const RenderSettings& settings, const GpuTexture::Ptr& color, bool colorIsReversible)
{
    if (device == nullptr || color == nullptr)
        return nullptr;

    // Compiled once, remembering a failure instead of retrying every frame
    if (! prepared.has_value())
        prepared = prepare (device);

    if (prepared->failed())
        return nullptr;

    const auto acquire = [&] (int width, int height, const char* label)
    {
        GpuTextureDesc desc (static_cast<uint32_t> (width), static_cast<uint32_t> (height), GpuTextureFormat::rgba16float, true);
        desc.label = label;
        return resources.acquire (desc);
    };

    // One texture per level: halving down to 8 pixels or 6 levels, and at least one level
    std::array<GpuTexture::Ptr, bloomMaxLevels> levels;
    int numLevels = 0;
    auto width = color->getWidth();
    auto height = color->getHeight();

    do
    {
        width = jmax (1, width / 2);
        height = jmax (1, height / 2);

        auto& level = levels[static_cast<size_t> (numLevels++)];
        level = acquire (width, height, "Bloom level");
        if (level == nullptr)
            return nullptr;
    } while (numLevels < bloomMaxLevels && jmin (width, height) > bloomSmallestLevel);

    const auto getTexel = [] (const GpuTexture::Ptr& texture)
    {
        return std::array<float, 2> { 1.0f / static_cast<float> (texture->getWidth()), 1.0f / static_cast<float> (texture->getHeight()) };
    };

    const auto threshold = jmax (0.0f, settings.bloomThreshold);

    for (int i = 0; i < numLevels; ++i)
    {
        const auto& source = i == 0 ? color : levels[static_cast<size_t> (i - 1)];
        const auto texel = getTexel (source);

        BloomDownsampleBlock block {};
        block.params[0] = i == 0 ? 1.0f : 0.0f;
        block.params[1] = i == 0 && colorIsReversible ? 1.0f : 0.0f;
        block.params[2] = threshold;
        block.params[3] = threshold * 0.5f;
        block.texel[0] = texel[0];
        block.texel[1] = texel[1];
        block.texel[2] = static_cast<float> (source->getWidth());
        block.texel[3] = static_cast<float> (source->getHeight());

        if (! downsample.draw (frame, levels[static_cast<size_t> (i)], {}, [&] (GpuRenderPass& renderPass)
            {
                renderPass.setSampler (0, 1, linearClampSampler);
                renderPass.setUniformBuffer (1, 0, &block, sizeof (block));
                renderPass.setTexture (1, 1, source);
            }, GpuColor::transparentBlack()))
            return nullptr;
    }

    const auto radius = jmap (jlimit (0.0f, 1.0f, settings.bloomRadius), 0.5f, 1.5f);

    // The blend state adds the upsampled glow onto the target: loaded within the chain, cleared for the output
    const auto drawUpsample = [&] (const GpuTexture::Ptr& source, const GpuTexture::Ptr& target, float scale, std::optional<GpuColor> clear)
    {
        const auto texel = getTexel (source);

        BloomUpsampleBlock block {};
        block.params[0] = texel[0] * radius;
        block.params[1] = texel[1] * radius;
        block.params[2] = scale;

        return upsample.draw (frame, target, {}, [&] (GpuRenderPass& renderPass)
        {
            renderPass.setSampler (0, 1, linearClampSampler);
            renderPass.setUniformBuffer (1, 0, &block, sizeof (block));
            renderPass.setTexture (1, 1, source);
        }, clear);
    };

    for (int i = numLevels - 2; i >= 0; --i)
    {
        if (! drawUpsample (levels[static_cast<size_t> (i + 1)], levels[static_cast<size_t> (i)], 1.0f, {}))
            return nullptr;
    }

    auto output = acquire (color->getWidth(), color->getHeight(), "Bloom glow");
    if (output == nullptr || ! drawUpsample (levels[0], output, jmax (0.0f, settings.bloomIntensity), GpuColor::transparentBlack()))
        return nullptr;

    return output;
}

} // namespace yup
