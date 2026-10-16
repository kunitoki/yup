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

// Shader bundles built with the module, see shaderBundles in yup_renderer.h
constexpr uint8_t probeEquirectToCubeShaderBundle[] = {
#include "yup_renderer_shaders/yup_ProbeEquirectToCube.inc"
};

constexpr uint8_t probeCubeDownsampleShaderBundle[] = {
#include "yup_renderer_shaders/yup_ProbeCubeDownsample.inc"
};

constexpr uint8_t probePrefilterShaderBundle[] = {
#include "yup_renderer_shaders/yup_ProbePrefilter.inc"
};

constexpr uint8_t probeShProjectionShaderBundle[] = {
#include "yup_renderer_shaders/yup_ProbeShProjection.inc"
};

constexpr uint8_t probeVisibilityShaderBundle[] = {
#include "yup_renderer_shaders/yup_ProbeVisibility.inc"
};

// std140 mirror of ProbeEquirectBlock in yup_ProbeEquirectToCube.frag
struct ProbeEquirectBlock
{
    float params[4]; // x cube face, y 1 when the textures hold RGBM, z face size, w highest equirect mip
    float source[4]; // xy equirect size at mip 0
};

// std140 mirror of ProbeDownsampleBlock in yup_ProbeCubeDownsample.frag
struct ProbeDownsampleBlock
{
    float params[4]; // x cube face, y 1 when the cube holds RGBM, z face size of the mip written
};

// std140 mirror of ProbePrefilterBlock in yup_ProbePrefilter.frag
struct ProbePrefilterBlock
{
    float params[4]; // x output (0 a cube face, 1 an octahedral tile), y 1 when RGBM, z cube face, w perceptual roughness
    float sizes[4];  // x size written, y source face size at mip 0, z highest source mip, w samples (1 for a copy)
};

// std140 mirror of ProbeShBlock in yup_ProbeShProjection.frag
struct ProbeShBlock
{
    float params[4]; // x unused, y 1 when the textures hold RGBM, z face size of the mip read
};

// std140 mirror of ProbeVisibilityBlock in yup_ProbeVisibility.frag
struct ProbeVisibilityBlock
{
    float params[4]; // x tile size without its border, y distance scale, z samples, w cosine power
};

constexpr int numCubeFaces = 6;
constexpr int prefilterSamples = 64;
constexpr int irradianceFaceSize = 8;   // The cube mip summed by projectIrradiance(): 384 texels
constexpr int visibilitySamples = 64;
constexpr float visibilityPower = 50.0f; // McGuire et al. 2019

/** One mip of every face, as a cube: lod 0 of the shader is @p mip. */
GpuTextureViewDesc cubeMipView (uint32 mip)
{
    GpuTextureViewDesc view (mip, 0);
    view.dimension = GpuTextureViewDimension::cube;
    view.layerCount = numCubeFaces;
    return view;
}

float getRoughnessForMip (int mip, int numMips) noexcept
{
    return numMips > 1 ? static_cast<float> (mip) / static_cast<float> (numMips - 1) : 0.0f;
}

} // namespace

//==============================================================================
ProbeRenderer::ProbeRenderer (GpuDevice::Ptr newDevice)
    : device (std::move (newDevice))
{
}

ProbeRenderer::~ProbeRenderer() = default;

//==============================================================================
GpuTextureFormat ProbeRenderer::getRadianceFormat() const noexcept
{
    return usesRgbm() ? GpuTextureFormat::rgba8unorm : GpuTextureFormat::rgba16float;
}

bool ProbeRenderer::usesRgbm() const noexcept
{
    return forceRgbm || device == nullptr || ! device->isFormatRenderable (GpuTextureFormat::rgba16float);
}

void ProbeRenderer::setForceRgbmForTesting (bool shouldForceRgbm)
{
    if (forceRgbm == shouldForceRgbm)
        return;

    // The passes are compiled for the radiance format
    forceRgbm = shouldForceRgbm;
    prepared.reset();
}

GpuTexture::Ptr ProbeRenderer::createCube (int faceSize, int numMips) const
{
    if (device == nullptr || faceSize < 1 || ! isPowerOfTwo (faceSize))
        return nullptr;

    const auto fullChain = static_cast<int> (std::log2 (faceSize)) + 1;
    if (numMips < 0 || numMips > fullChain)
        return nullptr;

    GpuTextureDesc desc (static_cast<uint32> (faceSize), static_cast<uint32> (faceSize), getRadianceFormat(), true);
    desc.type = GpuTextureType::cube;
    desc.depthOrArrayLayers = numCubeFaces;
    desc.mipLevels = static_cast<uint32> (numMips > 0 ? numMips : fullChain);
    desc.label = "ProbeRenderer cube";

    return GpuTexture::create (device, desc);
}

//==============================================================================
bool ProbeRenderer::equirectToCube (GpuFrame& frame, const GpuTexture::Ptr& equirect, const GpuTexture::Ptr& cube)
{
    if (equirect == nullptr
        || equirect->getType() != GpuTextureType::texture2D
        || equirect->getFormat() != getRadianceFormat()
        || ! isRadianceCube (cube)
        || ! cube->isRenderTarget()
        || ! prepare())
        return false;

    ProbeEquirectBlock block {};
    block.params[1] = usesRgbm() ? 1.0f : 0.0f;
    block.params[2] = static_cast<float> (cube->getWidth());
    block.params[3] = static_cast<float> (equirect->getMipLevels() - 1);
    block.source[0] = static_cast<float> (equirect->getWidth());
    block.source[1] = static_cast<float> (equirect->getHeight());

    auto drawn = true;

    for (int face = 0; face < numCubeFaces && drawn; ++face)
    {
        block.params[0] = static_cast<float> (face);

        drawn = equirectPass.draw (frame, cube, GpuTextureViewDesc (0, static_cast<uint32> (face)), [&] (GpuRenderPass& pass)
        {
            pass.setSampler (0, 1, equirectSampler);
            pass.setUniformBuffer (1, 0, &block, sizeof (block));
            pass.setTexture (1, 1, equirect);
        }, GpuColor::transparentBlack());
    }

    equirectPass.releaseTargets();
    return drawn;
}

bool ProbeRenderer::downsampleCube (GpuFrame& frame, const GpuTexture::Ptr& cube)
{
    if (! isRadianceCube (cube) || ! cube->isRenderTarget() || ! prepare())
        return false;

    ProbeDownsampleBlock block {};
    block.params[1] = usesRgbm() ? 1.0f : 0.0f;

    auto drawn = true;

    for (uint32 mip = 1; mip < cube->getMipLevels() && drawn; ++mip)
    {
        block.params[2] = static_cast<float> (jmax (1, cube->getWidth() >> mip));

        for (int face = 0; face < numCubeFaces && drawn; ++face)
        {
            block.params[0] = static_cast<float> (face);

            drawn = downsamplePass.draw (frame, cube, GpuTextureViewDesc (mip, static_cast<uint32> (face)), [&] (GpuRenderPass& pass)
            {
                pass.setSampler (0, 1, cubeSampler);
                pass.setUniformBuffer (1, 0, &block, sizeof (block));
                pass.setTexture (1, 1, cube, cubeMipView (mip - 1));
            }, GpuColor::transparentBlack());
        }
    }

    downsamplePass.releaseTargets();
    return drawn;
}

bool ProbeRenderer::prefilterToCube (GpuFrame& frame, const GpuTexture::Ptr& source, const GpuTexture::Ptr& destination)
{
    if (! isRadianceCube (source) || ! isRadianceCube (destination) || ! destination->isRenderTarget() || source == destination || ! prepare())
        return false;

    const auto numMips = static_cast<int> (destination->getMipLevels());

    ProbePrefilterBlock block {};
    block.params[1] = usesRgbm() ? 1.0f : 0.0f;
    block.sizes[1] = static_cast<float> (source->getWidth());
    block.sizes[2] = static_cast<float> (source->getMipLevels() - 1);

    auto drawn = true;

    for (int mip = 0; mip < numMips && drawn; ++mip)
    {
        block.params[3] = getRoughnessForMip (mip, numMips);
        block.sizes[0] = static_cast<float> (jmax (1, destination->getWidth() >> mip));
        block.sizes[3] = mip == 0 ? 1.0f : static_cast<float> (prefilterSamples);

        for (int face = 0; face < numCubeFaces && drawn; ++face)
        {
            block.params[2] = static_cast<float> (face);

            drawn = prefilterPass.draw (frame, destination, GpuTextureViewDesc (static_cast<uint32> (mip), static_cast<uint32> (face)), [&] (GpuRenderPass& pass)
            {
                pass.setSampler (0, 1, cubeSampler);
                pass.setUniformBuffer (1, 0, &block, sizeof (block));
                pass.setTexture (1, 1, source);
            }, GpuColor::transparentBlack());
        }
    }

    prefilterPass.releaseTargets();
    return drawn;
}

bool ProbeRenderer::prefilterToAtlas (GpuFrame& frame, const GpuTexture::Ptr& source, const GpuTexture::Ptr& atlas,
                                      Rectangle<int> tileAtMip0, int numMips)
{
    if (! isRadianceCube (source)
        || atlas == nullptr
        || atlas->getType() != GpuTextureType::texture2D
        || atlas->getFormat() != getRadianceFormat()
        || ! atlas->isRenderTarget()
        || numMips < 1
        || numMips > static_cast<int> (atlas->getMipLevels()))
        return false;

    // Mip m of the tile is the tile shifted right by m: it must stay on whole texels, inside the
    // atlas and at least two texels wide for the edge-to-edge octahedral mapping
    const auto alignment = 1 << (numMips - 1);
    const auto size = tileAtMip0.getWidth();

    if (size != tileAtMip0.getHeight()
        || tileAtMip0.getX() < 0
        || tileAtMip0.getY() < 0
        || tileAtMip0.getX() % alignment != 0
        || tileAtMip0.getY() % alignment != 0
        || size % alignment != 0
        || (size >> (numMips - 1)) < 2
        || tileAtMip0.getRight() > atlas->getWidth()
        || tileAtMip0.getBottom() > atlas->getHeight()
        || ! prepare())
        return false;

    ProbePrefilterBlock block {};
    block.params[0] = 1.0f;
    block.params[1] = usesRgbm() ? 1.0f : 0.0f;
    block.sizes[1] = static_cast<float> (source->getWidth());
    block.sizes[2] = static_cast<float> (source->getMipLevels() - 1);

    auto drawn = true;

    for (int mip = 0; mip < numMips && drawn; ++mip)
    {
        const auto x = tileAtMip0.getX() >> mip;
        const auto y = tileAtMip0.getY() >> mip;
        const auto tileSize = size >> mip;

        block.params[3] = getRoughnessForMip (mip, numMips);
        block.sizes[0] = static_cast<float> (tileSize);
        block.sizes[3] = mip == 0 ? 1.0f : static_cast<float> (prefilterSamples);

        // Loaded, not cleared, so the other tiles of the atlas survive
        drawn = prefilterPass.draw (frame, atlas, GpuTextureViewDesc (static_cast<uint32> (mip), 0), [&] (GpuRenderPass& pass)
        {
            pass.setViewport (static_cast<float> (x), static_cast<float> (y), static_cast<float> (tileSize), static_cast<float> (tileSize));
            pass.setScissorRect (x, y, tileSize, tileSize);
            pass.setSampler (0, 1, cubeSampler);
            pass.setUniformBuffer (1, 0, &block, sizeof (block));
            pass.setTexture (1, 1, source);
        });
    }

    prefilterPass.releaseTargets();
    return drawn;
}

//==============================================================================
bool ProbeRenderer::projectIrradiance (GpuFrame& frame, const GpuTexture::Ptr& cube, const GpuTexture::Ptr& atlas, Point<int> texel)
{
    if (! isRadianceCube (cube)
        || atlas == nullptr
        || atlas->getType() != GpuTextureType::texture2D
        || atlas->getFormat() != getRadianceFormat()
        || ! atlas->isRenderTarget()
        || texel.getX() < 0
        || texel.getY() < 0
        || texel.getX() >= static_cast<int> (atlas->getWidth())
        || texel.getY() + 4 > static_cast<int> (atlas->getHeight())
        || ! prepare())
        return false;

    auto* pass = prepareVolumePass (usesRgbm() ? irradianceRgbmPass : irradiancePass, probeShProjectionShaderBundle, sizeof (probeShProjectionShaderBundle), getRadianceFormat());
    if (pass == nullptr)
        return false;

    // The mip with faces of irradianceFaceSize texels, or mip 0 of a smaller cube
    auto mip = 0;
    while (mip + 1 < static_cast<int> (cube->getMipLevels()) && (cube->getWidth() >> mip) > irradianceFaceSize)
        ++mip;

    ProbeShBlock block {};
    block.params[1] = usesRgbm() ? 1.0f : 0.0f;
    block.params[2] = static_cast<float> (jmax (1, cube->getWidth() >> mip));

    // Loaded, not cleared, so the other texels of the atlas survive
    const auto drawn = pass->draw (frame, atlas, GpuTextureViewDesc (0, 0), [&] (GpuRenderPass& renderPass)
    {
        renderPass.setViewport (static_cast<float> (texel.getX()), static_cast<float> (texel.getY()), 1.0f, 4.0f);
        renderPass.setScissorRect (texel.getX(), texel.getY(), 1, 4);
        renderPass.setSampler (0, 1, cubeSampler);
        renderPass.setUniformBuffer (1, 0, &block, sizeof (block));
        renderPass.setTexture (1, 1, cube, cubeMipView (static_cast<uint32> (mip)));
    });

    pass->releaseTargets();
    return drawn;
}

bool ProbeRenderer::canRenderVisibility() const noexcept
{
    return device != nullptr && ! usesRgbm() && device->isFormatRenderable (visibilityFormat);
}

bool ProbeRenderer::renderVisibility (GpuFrame& frame, const GpuTexture::Ptr& distanceCube, const GpuTexture::Ptr& atlas,
                                      Point<int> origin, int tileSize, float distanceScale)
{
    const auto bordered = tileSize + 2;

    if (! canRenderVisibility()
        || distanceCube == nullptr
        || distanceCube->getType() != GpuTextureType::cube
        || distanceCube->getDepthOrArrayLayers() != numCubeFaces
        || distanceCube->getFormat() != visibilityFormat
        || atlas == nullptr
        || atlas->getType() != GpuTextureType::texture2D
        || atlas->getFormat() != visibilityFormat
        || ! atlas->isRenderTarget()
        || tileSize < 2
        || origin.getX() < 0
        || origin.getY() < 0
        || origin.getX() + bordered > static_cast<int> (atlas->getWidth())
        || origin.getY() + bordered > static_cast<int> (atlas->getHeight()))
        return false;

    // The distances are half floats, always filtered
    if (distanceSampler == nullptr)
        distanceSampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::linear, GpuWrapMode::clampToEdge));

    auto* pass = prepareVolumePass (visibilityPass, probeVisibilityShaderBundle, sizeof (probeVisibilityShaderBundle), visibilityFormat);
    if (pass == nullptr || distanceSampler == nullptr)
        return false;

    const ProbeVisibilityBlock block { { static_cast<float> (tileSize), distanceScale, static_cast<float> (visibilitySamples), visibilityPower } };

    // Loaded, not cleared, so the other tiles of the atlas survive
    const auto drawn = pass->draw (frame, atlas, GpuTextureViewDesc (0, 0), [&] (GpuRenderPass& renderPass)
    {
        renderPass.setViewport (static_cast<float> (origin.getX()), static_cast<float> (origin.getY()), static_cast<float> (bordered), static_cast<float> (bordered));
        renderPass.setScissorRect (origin.getX(), origin.getY(), bordered, bordered);
        renderPass.setSampler (0, 1, distanceSampler);
        renderPass.setUniformBuffer (1, 0, &block, sizeof (block));
        renderPass.setTexture (1, 1, distanceCube);
    });

    pass->releaseTargets();
    return drawn;
}

FullscreenPass* ProbeRenderer::prepareVolumePass (VolumePass& volumePass, const uint8* bundle, size_t bundleSize, GpuTextureFormat format)
{
    // Compiled on first use, apart from the environment passes, and kept: frames in flight may use it
    if (! volumePass.prepared.has_value())
        volumePass.prepared = volumePass.pass.prepare (device, bundle, bundleSize, format);

    return volumePass.prepared->wasOk() ? &volumePass.pass : nullptr;
}

//==============================================================================
bool ProbeRenderer::prepare()
{
    if (prepared.has_value())
        return *prepared;

    prepared = false;

    if (device == nullptr)
        return false;

    const auto format = getRadianceFormat();

    if (equirectPass.prepare (device, probeEquirectToCubeShaderBundle, sizeof (probeEquirectToCubeShaderBundle), format).failed()
        || downsamplePass.prepare (device, probeCubeDownsampleShaderBundle, sizeof (probeCubeDownsampleShaderBundle), format).failed()
        || prefilterPass.prepare (device, probePrefilterShaderBundle, sizeof (probePrefilterShaderBundle), format).failed())
        return false;

    // RGBM texels are read one at a time: filtering them would mix their multipliers
    const auto filter = usesRgbm() ? GpuFilter::nearest : GpuFilter::linear;

    GpuSamplerDesc cubeDesc (filter, GpuWrapMode::clampToEdge);
    cubeDesc.mipmapFilter = filter;
    cubeSampler = GpuSampler::create (device, cubeDesc);

    // Repeating around the vertical axis, clamped at the poles
    GpuSamplerDesc equirectDesc (filter, GpuWrapMode::repeat);
    equirectDesc.mipmapFilter = filter;
    equirectDesc.wrapV = GpuWrapMode::clampToEdge;
    equirectSampler = GpuSampler::create (device, equirectDesc);

    prepared = cubeSampler != nullptr && equirectSampler != nullptr;
    return *prepared;
}

bool ProbeRenderer::isRadianceCube (const GpuTexture::Ptr& texture) const noexcept
{
    return texture != nullptr
        && texture->getType() == GpuTextureType::cube
        && texture->getDepthOrArrayLayers() == numCubeFaces
        && texture->getFormat() == getRadianceFormat()
        && isPowerOfTwo (texture->getWidth());
}

//==============================================================================

ProbeRenderer::CubeBuild ProbeRenderer::buildEnvironmentCube (const EnvironmentMap& environment)
{
    if (device == nullptr)
        return {};

    auto frame = GpuFrame::begin (device);
    if (! frame.isValid())
        return {};

    const auto format = getRadianceFormat();
    const auto rgbm = format == GpuTextureFormat::rgba8unorm;
    const auto width = environment.getWidth();
    const auto height = width / 2;
    const auto& pixels = environment.getPixels();

    // An RGBM image is sampled at mip 0 only: averaging RGBM texels would mix their multipliers
    GpuTextureDesc imageDesc (static_cast<uint32> (width), static_cast<uint32> (height), format, ! rgbm);
    imageDesc.mipLevels = rgbm ? 1u : static_cast<uint32> (std::log2 (width)) + 1;
    imageDesc.label = "EnvironmentMap image";

    // The recorded passes keep these textures until the GPU is done with them, but not the
    // generateMips() pipeline the image caches: the caller keeps the image
    auto image = GpuTexture::create (device, imageDesc);
    auto source = createCube (environment.getFaceSize());
    auto cube = createCube (environment.getFaceSize(), environment.getNumLevels());

    std::vector<uint8> encoded;
    GpuTextureDataDesc data;
    data.data = pixels.data();

    if (rgbm)
    {
        encoded.resize (pixels.size());
        for (size_t i = 0; i < pixels.size(); i += 4)
        {
            const float rgb[] = { halfToFloat (pixels[i]), halfToFloat (pixels[i + 1]), halfToFloat (pixels[i + 2]) };
            encodeRgbm (rgb, encoded.data() + i);
        }

        data.data = encoded.data();
    }

    if (image == nullptr || source == nullptr || cube == nullptr || ! image->upload (data))
    {
        YUP_DBG ("ProbeRenderer: unable to create the textures of the environment cube");
        return { nullptr, frame.getStatus() };
    }

    const auto recorded = (rgbm || image->generateMips (frame))
                       && equirectToCube (frame, image, source)
                       && downsampleCube (frame, source)
                       && prefilterToCube (frame, source, cube);

    CubeBuild build { recorded ? cube : nullptr, frame.getStatus(), rgbm ? nullptr : image };
    frame.submit();

    if (! recorded)
        YUP_DBG ("ProbeRenderer: unable to record the passes building the environment cube");

    return build;
}

} // namespace yup
