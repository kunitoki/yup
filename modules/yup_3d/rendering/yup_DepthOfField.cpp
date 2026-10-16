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
// shaders/yup_Dof*.frag or shaders/lib/dof.glsl with: just yup3d_shaders

constexpr uint8_t dofSetupShaderBundle[] = {
#include "../shaders/yup_DofSetup.inc"
};
constexpr uint8_t dofTilesShaderBundle[] = {
#include "../shaders/yup_DofTiles.inc"
};
constexpr uint8_t dofGatherShaderBundle[] = {
#include "../shaders/yup_DofGather.inc"
};
constexpr uint8_t dofCompositeShaderBundle[] = {
#include "../shaders/yup_DofComposite.inc"
};

constexpr auto dofColorFormat = GpuTextureFormat::rgba16float;
constexpr auto dofScalarFormat = GpuTextureFormat::r16float;
constexpr auto dofBlurAmountFormat = GpuTextureFormat::r8unorm;

// std140 mirror of DofBlock in lib/dof.glsl
struct DofBlock
{
    float lens[4];                                    // CoC at infinity, focus distance, largest CoC, 1 when the color is reversible
    float tiles[4];                                   // tile size (the tiles pass: source texels per texel), the tiles pass reach and source sign
    float samples[DepthOfField::numBokehSamples][4]; // offset xy, its length
};

static_assert (sizeof (DofBlock) == 16 * (2 + DepthOfField::numBokehSamples));

} // namespace

//==============================================================================
std::vector<Point<float>> DepthOfField::makeBokehSamples (int rings, int blades, float rotation)
{
    std::vector<Point<float>> result { Point<float>() };

    constexpr auto pi = MathConstants<float>::pi;
    const auto side = blades >= 3 ? MathConstants<float>::twoPi / static_cast<float> (blades) : 0.0f;

    for (int ring = 1; ring <= rings; ++ring)
    {
        const auto count = 8 * ring;
        const auto radius = static_cast<float> (ring) / static_cast<float> (rings);

        for (int k = 0; k < count; ++k)
        {
            const auto angle = rotation + MathConstants<float>::twoPi * (static_cast<float> (k) + 0.5f * static_cast<float> (ring % 2)) / static_cast<float> (count);
            auto scale = radius;

            // The boundary of the polygon along this ray, its vertices on the unit circle (Abadie 2018)
            if (side > 0.0f)
            {
                const auto withinSide = std::fmod (angle - rotation + MathConstants<float>::twoPi, side);
                scale *= std::cos (pi / static_cast<float> (blades)) / std::cos (withinSide - side * 0.5f);
            }

            result.emplace_back (scale * std::cos (angle), scale * std::sin (angle));
        }
    }

    return result;
}

DepthOfField::Lens DepthOfField::getLens (const CameraNode* camera, float height) noexcept
{
    if (camera == nullptr || camera->projection != CameraNode::Projection::perspective || ! (camera->fStop > 0.0f)
        || ! (camera->yFov > 0.0f) || ! (height > 0.0f))
        return {};

    // CameraNode::getCircleOfConfusion(), halved into a radius: aperture * f * (1 - focus / d) / (focus - f) on the sensor
    const auto focalLength = CameraNode::sensorHeight / (2.0f * std::tan (camera->yFov * 0.5f));
    const auto focus = jmax (camera->focusDistance, focalLength * 1.001f);
    const auto aperture = focalLength / camera->fStop;

    return { aperture * focalLength / (focus - focalLength) / CameraNode::sensorHeight * height * 0.5f, focus };
}

bool DepthOfField::canBlur (const CameraNode* camera, const RenderSettings& settings) noexcept
{
    return getLens (camera, 1.0f).scale > 0.0f && settings.depthOfFieldMaxSize > 0.0f;
}

//==============================================================================
DepthOfField::DepthOfField (GpuDevice::Ptr deviceToUse)
    : device (std::move (deviceToUse))
{
}

DepthOfField::~DepthOfField() = default;

//==============================================================================
Result DepthOfField::prepare()
{
    if (prepared)
        return *prepared;

    prepared = [&]
    {
        if (device == nullptr)
            return Result::fail ("No device");

        const std::array<GpuTextureFormat, 2> setupFormats { dofColorFormat, dofScalarFormat };
        const std::array<GpuTextureFormat, 3> gatherFormats { dofColorFormat, dofColorFormat, dofScalarFormat };
        const std::array<GpuTextureFormat, 2> compositeFormats { dofColorFormat, dofBlurAmountFormat };

        if (auto result = setup.prepare (device, dofSetupShaderBundle, sizeof (dofSetupShaderBundle), setupFormats); result.failed())
            return Result::fail ("Unable to compile the setup: " + result.getErrorMessage());

        if (auto result = tiles.prepare (device, dofTilesShaderBundle, sizeof (dofTilesShaderBundle), dofScalarFormat); result.failed())
            return Result::fail ("Unable to compile the tiles: " + result.getErrorMessage());

        if (auto result = gather.prepare (device, dofGatherShaderBundle, sizeof (dofGatherShaderBundle), gatherFormats); result.failed())
            return Result::fail ("Unable to compile the gather: " + result.getErrorMessage());

        if (auto result = composite.prepare (device, dofCompositeShaderBundle, sizeof (dofCompositeShaderBundle), compositeFormats); result.failed())
            return Result::fail ("Unable to compile the composite: " + result.getErrorMessage());

        return Result::ok();
    }();

    return *prepared;
}

GpuTexture::Ptr DepthOfField::render (GpuFrame& frame, const ViewData& view, const RenderSettings& settings,
                                      FrameResources& resources, const Inputs& inputs)
{
    output = nullptr;
    blurAmount = nullptr;
    circleOfConfusion = nullptr;
    maxCircleOfConfusion = 0.0f;

    // A missing input, like a raster prepass that could not be drawn, skips the frame without counting as a failure
    const auto* samplers = resources.getSamplers();
    if (givenUp || ! canBlur (inputs.camera, settings) || inputs.color == nullptr || inputs.depth == nullptr || samplers == nullptr)
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

        // The intermediates follow the depth, the result the color
        const auto halfWidth = (inputs.depth->getWidth() + 1) / 2;
        const auto halfHeight = (inputs.depth->getHeight() + 1) / 2;
        const auto quarterWidth = (halfWidth + 3) / 4;
        const auto quarterHeight = (halfHeight + 3) / 4;
        const auto tileWidth = (quarterWidth + 3) / 4;
        const auto tileHeight = (quarterHeight + 3) / 4;

        const auto halfColor = acquire (halfWidth, halfHeight, dofColorFormat, "DepthOfField half color");
        const auto coc = acquire (halfWidth, halfHeight, dofScalarFormat, "DepthOfField circle of confusion");
        const auto quarterCoc = acquire (quarterWidth, quarterHeight, dofScalarFormat, "DepthOfField quarter tiles");
        const auto tileCoc = acquire (tileWidth, tileHeight, dofScalarFormat, "DepthOfField tiles");
        const auto dilatedCoc = acquire (tileWidth, tileHeight, dofScalarFormat, "DepthOfField dilated tiles");
        const auto far = acquire (halfWidth, halfHeight, dofColorFormat, "DepthOfField far field");
        const auto near = acquire (halfWidth, halfHeight, dofColorFormat, "DepthOfField near field");
        const auto coverage = acquire (halfWidth, halfHeight, dofScalarFormat, "DepthOfField near coverage");
        auto blurred = acquire (inputs.color->getWidth(), inputs.color->getHeight(), dofColorFormat, "DepthOfField output");
        auto amount = acquire (inputs.color->getWidth(), inputs.color->getHeight(), dofBlurAmountFormat, "DepthOfField blur amount");

        if (halfColor == nullptr || coc == nullptr || quarterCoc == nullptr || tileCoc == nullptr || dilatedCoc == nullptr
            || far == nullptr || near == nullptr || coverage == nullptr || blurred == nullptr || amount == nullptr)
        {
            failure = "unable to create the targets";
            return nullptr;
        }

        if (samples.empty() || sampleBlades != inputs.camera->apertureBlades)
        {
            sampleBlades = inputs.camera->apertureBlades;
            samples = makeBokehSamples (bokehRings, sampleBlades, 0.0f);
        }

        // The largest blur is a diameter in pixels of the color: a radius in CoC texels
        const auto lens = getLens (inputs.camera, static_cast<float> (halfHeight));
        const auto maxRadius = jmin (settings.depthOfFieldMaxSize, maxBlurSize) * 0.5f * static_cast<float> (halfHeight) / static_cast<float> (inputs.color->getHeight());

        DofBlock block {};
        block.lens[0] = lens.scale;
        block.lens[1] = lens.focus;
        block.lens[2] = maxRadius;
        block.lens[3] = inputs.colorIsReversible ? 1.0f : 0.0f;
        block.tiles[0] = static_cast<float> (tileSize);

        // z is the radius of the ring before the polygon moved the sample: a source covers it when its
        // CoC reaches that far, which keeps the polygon of its own CoC when that is below the radius
        for (int i = 0, ring = 0; i < numBokehSamples; ++i)
        {
            if (i > 4 * ring * (ring + 1))
                ++ring;

            block.samples[i][0] = samples[static_cast<size_t> (i)].getX();
            block.samples[i][1] = samples[static_cast<size_t> (i)].getY();
            block.samples[i][2] = static_cast<float> (ring) / static_cast<float> (bokehRings);
        }

        const auto bindCommon = [&] (GpuRenderPass& pass, const DofBlock& passBlock)
        {
            pass.setUniformBuffer (0, 0, &view.getBlock(), sizeof (ViewData::Block));
            pass.setSampler (0, 1, samplers->linearClamp);
            pass.setSampler (0, 3, samplers->pointClamp);
            pass.setUniformBuffer (1, 0, &passBlock, sizeof (passBlock));
        };

        // The tiles by steps of few texels each (one pass looping over a tile and its neighbors per
        // texel took over 1 ms at 1280 x 800): 4 x 4 of signed CoC, 4 x 4 again, then the neighbors
        // within the largest radius, one tile away since maxBlurSize keeps it within a tile
        const auto reduce = [&] (const GpuTexture::Ptr& source, const GpuTexture::Ptr& target, float scale, float reach, float sign)
        {
            auto tilesBlock = block;
            tilesBlock.tiles[0] = scale;
            tilesBlock.tiles[1] = reach;
            tilesBlock.tiles[2] = sign;

            return tiles.draw (frame, target, {}, [&] (GpuRenderPass& pass)
            {
                bindCommon (pass, tilesBlock);
                pass.setTexture (1, 1, source);
            }, GpuColor::transparentBlack());
        };

        // Every draw writes every texel: the targets are cleared rather than loaded, the extra ones neither
        const GpuRenderOptions writesEveryTexel (GpuLoadOp::dontCare, GpuStoreOp::store);
        const auto clear = GpuColor::transparentBlack();

        const auto drawn = setup.draw (frame, halfColor, {}, [&] (GpuRenderPass& pass)
                           {
                               pass.setColorAttachment (1, coc, writesEveryTexel);
                               bindCommon (pass, block);
                               pass.setTexture (1, 1, inputs.color);
                               pass.setTexture (1, 2, inputs.depth);
                           }, clear)
                        && reduce (coc, quarterCoc, 4.0f, 0.0f, -1.0f)
                        && reduce (quarterCoc, tileCoc, 4.0f, 0.0f, 1.0f)
                        && reduce (tileCoc, dilatedCoc, 1.0f, std::ceil (maxRadius / static_cast<float> (tileSize)), 1.0f)
                        && gather.draw (frame, far, {}, [&] (GpuRenderPass& pass)
                           {
                               pass.setColorAttachment (1, near, writesEveryTexel);
                               pass.setColorAttachment (2, coverage, writesEveryTexel);
                               bindCommon (pass, block);
                               pass.setTexture (1, 1, halfColor);
                               pass.setTexture (1, 2, coc);
                               pass.setTexture (1, 3, dilatedCoc);
                           }, clear)
                        && composite.draw (frame, blurred, {}, [&] (GpuRenderPass& pass)
                           {
                               pass.setColorAttachment (1, amount, writesEveryTexel);
                               bindCommon (pass, block);
                               pass.setTexture (1, 1, inputs.color);
                               pass.setTexture (1, 2, inputs.depth);
                               pass.setTexture (1, 3, far);
                               pass.setTexture (1, 4, near);
                               pass.setTexture (1, 5, coverage);
                           }, clear);

        if (! drawn)
        {
            failure = "unable to record a draw";
            return nullptr;
        }

        circleOfConfusion = coc;
        maxCircleOfConfusion = maxRadius;
        blurAmount = amount;
        return blurred;
    }();

    if (result != nullptr)
    {
        consecutiveFailures = 0;
        output = result;
        return output;
    }

    // A failure leaves the frame without the blur; the second in a row gives up for good
    if (++consecutiveFailures >= 2 && ! std::exchange (givenUp, true))
    {
        giveUpReason = failure;
        YUP_DBG ("DepthOfField: " << failure << ", the frames render without depth of field");
    }

    return nullptr;
}

void DepthOfField::trimUnusedTargets()
{
    setup.trimUnusedTargets();
    tiles.trimUnusedTargets();
    gather.trimUnusedTargets();
    composite.trimUnusedTargets();
}

void DepthOfField::releaseTargets() noexcept
{
    setup.releaseTargets();
    tiles.releaseTargets();
    gather.releaseTargets();
    composite.releaseTargets();
    output = nullptr;
    blurAmount = nullptr;
    circleOfConfusion = nullptr;
    maxCircleOfConfusion = 0.0f;
}

size_t DepthOfField::getNumCachedTargets() const noexcept
{
    return setup.getNumCachedTargets() + tiles.getNumCachedTargets() + gather.getNumCachedTargets() + composite.getNumCachedTargets();
}

} // namespace yup
