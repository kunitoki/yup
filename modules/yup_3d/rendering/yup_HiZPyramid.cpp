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
// shaders/yup_HiZCopy.frag or shaders/yup_HiZDownsample.frag with: just yup3d_shaders

constexpr uint8_t hiZCopyShaderBundle[] = {
#include "../shaders/yup_HiZCopy.inc"
};
constexpr uint8_t hiZDownsampleShaderBundle[] = {
#include "../shaders/yup_HiZDownsample.inc"
};

constexpr auto hiZFormat = GpuTextureFormat::r32float;

// std140 mirror of HiZBlock in yup_HiZDownsample.frag
struct HiZBlock
{
    float params[4]; // xy the extent of the mip read
};

static_assert (sizeof (HiZBlock) == 16);

} // namespace

//==============================================================================
HiZPyramid::HiZPyramid (GpuDevice::Ptr deviceToUse)
    : device (std::move (deviceToUse))
{
}

HiZPyramid::~HiZPyramid() = default;

//==============================================================================
Result HiZPyramid::prepare()
{
    if (prepared)
        return *prepared;

    prepared = [&]
    {
        if (device == nullptr)
            return Result::fail ("No device");

        if (! device->isFormatRenderable (hiZFormat))
            return Result::fail ("r32float is not renderable");

        if (auto result = copy.prepare (device, hiZCopyShaderBundle, sizeof (hiZCopyShaderBundle), hiZFormat); result.failed())
            return Result::fail ("Unable to compile the depth copy: " + result.getErrorMessage());

        if (auto result = downsample.prepare (device, hiZDownsampleShaderBundle, sizeof (hiZDownsampleShaderBundle), hiZFormat); result.failed())
            return Result::fail ("Unable to compile the downsample: " + result.getErrorMessage());

        return Result::ok();
    }();

    if (prepared->failed())
        YUP_DBG ("HiZPyramid: " << prepared->getErrorMessage() << ", no pyramid is built");

    return *prepared;
}

GpuTexture::Ptr HiZPyramid::fail (const String& reason)
{
    if (! std::exchange (failureLogged, true))
        YUP_DBG ("HiZPyramid: " << reason << ", no pyramid is built");

    return nullptr;
}

GpuTexture::Ptr HiZPyramid::build (GpuFrame& frame, const GpuTexture::Ptr& depth, FrameResources& resources)
{
    // Logged once by prepare()
    if (prepare().failed())
        return nullptr;

    if (depth == nullptr || depth->getFormat() != GpuTextureFormat::depth32float || depth->getSampleCount() != 1)
        return fail ("no single-sampled depth32float depth");

    const auto* samplers = resources.getSamplers();
    if (samplers == nullptr)
        return fail ("no samplers");

    GpuTextureDesc desc (static_cast<uint32> (depth->getWidth()), static_cast<uint32> (depth->getHeight()), hiZFormat, true);
    desc.mipLevels = static_cast<uint32> (std::bit_width (jmax (desc.width, desc.height)));
    desc.label = "HiZPyramid";

    auto pyramid = resources.acquire (desc);
    if (pyramid == nullptr)
        return fail ("unable to create the pyramid");

    // Every draw writes every texel: the clear only spares the load
    const auto copied = copy.draw (frame, pyramid, GpuTextureViewDesc (0, 0), [&] (GpuRenderPass& pass)
    {
        pass.setSampler (0, 3, samplers->pointClamp);
        pass.setTexture (1, 0, depth);
    }, GpuColor::transparentBlack());

    if (! copied)
        return fail ("unable to record a draw");

    HiZBlock block {};

    for (uint32 mip = 1; mip < desc.mipLevels; ++mip)
    {
        block.params[0] = static_cast<float> (jmax (1u, desc.width >> (mip - 1)));
        block.params[1] = static_cast<float> (jmax (1u, desc.height >> (mip - 1)));

        const auto drawn = downsample.draw (frame, pyramid, GpuTextureViewDesc (mip, 0), [&] (GpuRenderPass& pass)
        {
            pass.setSampler (0, 3, samplers->pointClamp);
            pass.setUniformBuffer (1, 0, &block, sizeof (block));
            pass.setTexture (1, 1, pyramid, GpuTextureViewDesc (mip - 1, 0));
        }, GpuColor::transparentBlack());

        if (! drawn)
            return fail ("unable to record a draw");
    }

    failureLogged = false;
    return pyramid;
}

void HiZPyramid::trimUnusedTargets()
{
    copy.trimUnusedTargets();
    downsample.trimUnusedTargets();
}

void HiZPyramid::releaseTargets() noexcept
{
    copy.releaseTargets();
    downsample.releaseTargets();
}

} // namespace yup
