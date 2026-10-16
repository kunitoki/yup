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

//==============================================================================
// Precompiled bundles (do not edit the .inc by hand). Regenerate them after changing
// shaders/yup_Motion*.frag or shaders/lib/motion.glsl with: just yup3d_shaders

constexpr uint8_t motionTileMaxShaderBundle[] = {
#include "../shaders/yup_MotionTileMax.inc"
};
constexpr uint8_t motionNeighborMaxShaderBundle[] = {
#include "../shaders/yup_MotionNeighborMax.inc"
};
constexpr uint8_t motionReconstructShaderBundle[] = {
#include "../shaders/yup_MotionReconstruct.inc"
};

constexpr auto motionColorFormat = GpuTextureFormat::rgba16float;
constexpr auto motionTileFormat = GpuTextureFormat::rgba16float;
constexpr auto motionBlurAmountFormat = GpuTextureFormat::r8unorm;

// std140 mirror of MotionBlurBlock in lib/motion.glsl
struct MotionBlurBlock
{
    float options[4]; // shutter, 1 when the tiles pass reads the velocity, 1 when the blur amount input holds the depth of field's share
};

} // namespace

//==============================================================================
MotionBlur::MotionBlur (GpuDevice::Ptr deviceToUse)
    : device (std::move (deviceToUse))
{
}

MotionBlur::~MotionBlur() = default;

//==============================================================================
Result MotionBlur::prepare()
{
    if (prepared)
        return *prepared;

    prepared = [&]
    {
        if (device == nullptr)
            return Result::fail ("No device");

        const std::array<GpuTextureFormat, 2> reconstructFormats { motionColorFormat, motionBlurAmountFormat };

        if (auto result = tileMax.prepare (device, motionTileMaxShaderBundle, sizeof (motionTileMaxShaderBundle), motionTileFormat); result.failed())
            return Result::fail ("Unable to compile the tiles: " + result.getErrorMessage());

        if (auto result = neighborMaxPass.prepare (device, motionNeighborMaxShaderBundle, sizeof (motionNeighborMaxShaderBundle), motionTileFormat); result.failed())
            return Result::fail ("Unable to compile the neighborhood: " + result.getErrorMessage());

        if (auto result = reconstruct.prepare (device, motionReconstructShaderBundle, sizeof (motionReconstructShaderBundle), reconstructFormats); result.failed())
            return Result::fail ("Unable to compile the reconstruction: " + result.getErrorMessage());

        return Result::ok();
    }();

    return *prepared;
}

GpuTexture::Ptr MotionBlur::render (GpuFrame& frame, const ViewData& view, const RenderSettings& settings,
                                    FrameResources& resources, const Inputs& inputs)
{
    output = nullptr;
    blurAmount = nullptr;
    neighborMax = nullptr;

    // A closed shutter or a missing input skips the frame without counting as a failure
    const auto* samplers = resources.getSamplers();
    if (givenUp || ! (settings.motionBlurShutter > 0.0f) || inputs.color == nullptr || inputs.depth == nullptr
        || inputs.velocity == nullptr || samplers == nullptr)
        return nullptr;

    String failure;
    auto result = [&]() -> GpuTexture::Ptr
    {
        if (failRenders > 0)
        {
            --failRenders;
            failure = "the renders failed for testing";
            return nullptr;
        }

        if (auto prepareResult = prepare(); prepareResult.failed())
        {
            failure = prepareResult.getErrorMessage();
            return nullptr;
        }

        const auto acquire = [&] (int width, int height, GpuTextureFormat format, const char* label)
        {
            GpuTextureDesc desc (static_cast<uint32_t> (width), static_cast<uint32_t> (height), format, true);
            desc.label = label;
            return resources.acquire (desc);
        };

        // The tiles follow the color: a tile is tileSize x tileSize of its pixels
        const auto width = inputs.color->getWidth();
        const auto height = inputs.color->getHeight();
        const auto quarterWidth = (width + 3) / 4;
        const auto quarterHeight = (height + 3) / 4;
        const auto tileWidth = (quarterWidth + 3) / 4;
        const auto tileHeight = (quarterHeight + 3) / 4;

        const auto quarterTiles = acquire (quarterWidth, quarterHeight, motionTileFormat, "MotionBlur quarter tiles");
        const auto tiles = acquire (tileWidth, tileHeight, motionTileFormat, "MotionBlur tiles");
        auto neighbors = acquire (tileWidth, tileHeight, motionTileFormat, "MotionBlur neighbor tiles");
        auto smeared = acquire (width, height, motionColorFormat, "MotionBlur output");
        auto amount = acquire (width, height, motionBlurAmountFormat, "MotionBlur blur amount");

        if (quarterTiles == nullptr || tiles == nullptr || neighbors == nullptr || smeared == nullptr || amount == nullptr)
        {
            failure = "unable to create the targets";
            return nullptr;
        }

        MotionBlurBlock block {};
        block.options[0] = jmin (settings.motionBlurShutter, 1.0f);
        block.options[2] = inputs.blurAmount != nullptr ? 1.0f : 0.0f;

        const auto bindCommon = [&] (GpuRenderPass& pass, const MotionBlurBlock& passBlock)
        {
            pass.setUniformBuffer (0, 0, &view.getBlock(), sizeof (ViewData::Block));
            pass.setSampler (0, 3, samplers->pointClamp);
            pass.setUniformBuffer (1, 0, &passBlock, sizeof (passBlock));
        };

        // The tiles by steps of 4 x 4 (one pass looping over a whole tile per texel is slow, as
        // DepthOfField measured): the velocity per pixel of the color, then the quarter tiles
        const auto reduce = [&] (const GpuTexture::Ptr& source, const GpuTexture::Ptr& target, bool readsVelocity)
        {
            auto tilesBlock = block;
            tilesBlock.options[1] = readsVelocity ? 1.0f : 0.0f;

            return tileMax.draw (frame, target, {}, [&] (GpuRenderPass& pass)
            {
                bindCommon (pass, tilesBlock);
                pass.setTexture (1, 1, source);
                pass.setTexture (1, 2, inputs.color);
            }, GpuColor::transparentBlack());
        };

        // Every draw writes every texel: the targets are cleared rather than loaded, the extra one neither
        const GpuRenderOptions writesEveryTexel (GpuLoadOp::dontCare, GpuStoreOp::store);
        const auto clear = GpuColor::transparentBlack();

        // Without the depth of field's share, the color stands in for it: the pass never reads it then
        const auto spread = inputs.blurAmount != nullptr ? inputs.blurAmount : inputs.color;

        const auto drawn = reduce (inputs.velocity, quarterTiles, true)
                        && reduce (quarterTiles, tiles, false)
                        && neighborMaxPass.draw (frame, neighbors, {}, [&] (GpuRenderPass& pass)
                           {
                               bindCommon (pass, block);
                               pass.setTexture (1, 1, tiles);
                           }, clear)
                        && reconstruct.draw (frame, smeared, {}, [&] (GpuRenderPass& pass)
                           {
                               pass.setColorAttachment (1, amount, writesEveryTexel);
                               bindCommon (pass, block);
                               pass.setTexture (1, 1, inputs.color);
                               pass.setTexture (1, 2, inputs.depth);
                               pass.setTexture (1, 3, inputs.velocity);
                               pass.setTexture (1, 4, neighbors);
                               pass.setTexture (1, 5, spread);
                           }, clear);

        if (! drawn)
        {
            failure = "unable to record a draw";
            return nullptr;
        }

        neighborMax = neighbors;
        blurAmount = amount;
        return smeared;
    }();

    if (result != nullptr)
    {
        consecutiveFailures = 0;
        output = result;
        return output;
    }

    // A failure leaves the frame without the smear; the second in a row gives up for good
    if (++consecutiveFailures >= 2 && ! std::exchange (givenUp, true))
    {
        giveUpReason = failure;
        YUP_DBG ("MotionBlur: " << failure << ", the frames render without motion blur");
    }

    return nullptr;
}

void MotionBlur::trimUnusedTargets()
{
    tileMax.trimUnusedTargets();
    neighborMaxPass.trimUnusedTargets();
    reconstruct.trimUnusedTargets();
}

void MotionBlur::releaseTargets() noexcept
{
    tileMax.releaseTargets();
    neighborMaxPass.releaseTargets();
    reconstruct.releaseTargets();
    output = nullptr;
    blurAmount = nullptr;
    neighborMax = nullptr;
}

size_t MotionBlur::getNumCachedTargets() const noexcept
{
    return tileMax.getNumCachedTargets() + neighborMaxPass.getNumCachedTargets() + reconstruct.getNumCachedTargets();
}

} // namespace yup
