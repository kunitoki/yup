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
FullscreenPass::FullscreenPass() = default;

FullscreenPass::~FullscreenPass() = default;

//==============================================================================
Result FullscreenPass::prepare (const GpuDevice::Ptr& newDevice, const uint8* bundleData, size_t bundleSize,
                                GpuTextureFormat outputFormat, Blend blend, int colorTargets)
{
    return prepare (newDevice, bundleData, bundleSize, std::vector<GpuTextureFormat> (static_cast<size_t> (jmax (1, colorTargets)), outputFormat), blend);
}

Result FullscreenPass::prepare (const GpuDevice::Ptr& newDevice, const uint8* bundleData, size_t bundleSize,
                                Span<const GpuTextureFormat> outputFormats, Blend blend,
                                const GpuDepthStencilState& depthStencil, GpuColorWriteMask writeMask)
{
    device = nullptr;
    pipeline = nullptr;
    targets.clear();

    if (newDevice == nullptr || bundleData == nullptr || bundleSize == 0 || outputFormats.empty())
        return Result::fail ("No device, shader bundle or output format");

    auto bundle = ShaderBundle::loadFromData (bundleData, bundleSize);
    if (bundle.failed())
        return Result::fail ("Unable to load the fullscreen shader bundle: " + bundle.getErrorMessage());

    GpuPipelineOptions options;
    options.topology = GpuPrimitiveTopology::triangleList;
    options.cullMode = GpuCullMode::none;
    options.depthStencil = depthStencil;

    for (const auto format : outputFormats)
    {
        auto& target = options.colorTargets.emplace_back();
        target.format = format;
        target.writeMask = writeMask;
        const auto destination = blend == Blend::premultipliedOver ? GpuBlendFactor::oneMinusSrcAlpha : GpuBlendFactor::one;
        target.blendEnabled = blend != Blend::replace;
        target.blend.srcColor = GpuBlendFactor::one;
        target.blend.dstColor = destination;
        target.blend.srcAlpha = GpuBlendFactor::one;
        target.blend.dstAlpha = destination;
    }

    auto compiled = GpuPipeline::compileFromBundle (newDevice, bundle.getReference(), options);
    if (compiled.failed())
        return Result::fail ("Unable to compile the fullscreen pipeline: " + compiled.getErrorMessage());

    device = newDevice;
    pipeline = compiled.getValue();
    return Result::ok();
}

bool FullscreenPass::draw (GpuFrame& frame, const GpuTexture::Ptr& target, const GpuTextureViewDesc& view,
                           const std::function<void (GpuRenderPass&)>& bind, std::optional<GpuColor> clear)
{
    auto pass = begin (frame, target, view, clear);
    if (! pass.has_value() || ! pass->isValid())
        return false;

    const auto drawn = record (*pass, bind);
    return pass->finish() && drawn;
}

std::optional<GpuRenderPass> FullscreenPass::begin (GpuFrame& frame, const GpuTexture::Ptr& target, const GpuTextureViewDesc& view, std::optional<GpuColor> clear)
{
    if (! isPrepared() || target == nullptr)
        return std::nullopt;

    auto cached = std::find_if (targets.begin(), targets.end(), [&] (const CachedTarget& t)
    {
        return t.texture == target && t.view == view;
    });

    if (cached == targets.end())
    {
        auto surface = GpuTarget::createFromTexture (device, target, view);
        if (surface == nullptr)
            return std::nullopt;

        targets.push_back ({ target, view, std::move (surface) });
        cached = std::prev (targets.end());
    }

    cached->used = true;

    const auto options = clear.has_value() ? GpuRenderOptions (true, *clear)
                                           : GpuRenderOptions (GpuLoadOp::load, GpuStoreOp::store);

    return cached->target->beginRenderPass (frame, options);
}

bool FullscreenPass::record (GpuRenderPass& pass, const std::function<void (GpuRenderPass&)>& bind)
{
    if (! isPrepared() || ! pass.isValid())
        return false;

    pass.setPipeline (pipeline);

    if (bind)
        bind (pass);

    return pass.draw (3);
}

void FullscreenPass::trimUnusedTargets()
{
    std::erase_if (targets, [] (const CachedTarget& cached)
    {
        return ! cached.used;
    });

    for (auto& cached : targets)
        cached.used = false;
}

bool FullscreenPass::isPrepared() const noexcept
{
    return pipeline != nullptr;
}

} // namespace yup
