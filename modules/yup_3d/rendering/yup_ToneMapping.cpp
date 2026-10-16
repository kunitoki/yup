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
// shaders/yup_ToneMapping.frag with: just yup3d_shaders
constexpr uint8_t toneMappingShaderBundle[] = {
#include "../shaders/yup_ToneMapping.inc"
};

constexpr uint8_t toneMappingRadianceShaderBundle[] = {
#include "../shaders/yup_ToneMapping_radiance_composite.inc"
};

// std140 mirror of ToneMappingBlock in yup_ToneMapping.frag
struct ToneMappingBlock
{
    float options[4];    // x 1 when the source is reversibly tone mapped, y 1 with a glow
    float background[4]; // the display-encoded background color
};

} // namespace

//==============================================================================
ToneMapping::ToneMapping() = default;

ToneMapping::~ToneMapping() = default;

//==============================================================================
Result ToneMapping::prepare (const GpuDevice::Ptr& device, GpuTextureFormat format, const FrameResources::Samplers* samplers, bool radiance)
{
    if (samplers == nullptr)
        return Result::fail ("Unable to create the tone mapping sampler");

    return radiance ? radiancePass.prepare (device, toneMappingRadianceShaderBundle, sizeof (toneMappingRadianceShaderBundle), format)
                    : pass.prepare (device, toneMappingShaderBundle, sizeof (toneMappingShaderBundle), format);
}

bool ToneMapping::render (const GpuDevice::Ptr& device, GpuFrame& frame, const ViewData& view, FrameResources& resources,
                          const GpuTexture::Ptr& source, bool sourceIsReversible, const GpuTexture::Ptr& glow,
                          const GpuTexture::Ptr& destination, Color background, const GpuTexture::Ptr& blurAmount)
{
    if (device == nullptr || source == nullptr || destination == nullptr)
        return false;

    const auto* samplers = resources.getSamplers();

    // Each pipeline is compiled once per destination format, remembering a failure instead of retrying every frame
    const auto prepareVariant = [&] (std::optional<Result>& variantPrepared, GpuTextureFormat& variantFormat, bool radiance) -> const Result&
    {
        if (! variantPrepared.has_value() || variantFormat != destination->getFormat())
        {
            variantFormat = destination->getFormat();
            variantPrepared = radiance && failRadianceComposite ? Result::fail ("The radiance composite failed for testing")
                                                                : prepare (device, variantFormat, samplers, radiance);
        }

        return *variantPrepared;
    };

    // Without the radiance composite, blurred frames still render, with the display space blend
    auto blurredCoverage = blurAmount != nullptr;
    if (blurredCoverage && prepareVariant (radiancePrepared, radiancePreparedFormat, true).failed())
    {
        if (! std::exchange (radianceFailureLogged, true))
            YUP_DBG ("ToneMapping: " << radiancePrepared->getErrorMessage() << ", blurred frames composite in display space");

        blurredCoverage = false;
    }

    if ((! blurredCoverage && prepareVariant (prepared, preparedFormat, false).failed()) || samplers == nullptr)
        return false;

    ToneMappingBlock block {};
    block.options[0] = sourceIsReversible ? 1.0f : 0.0f;
    block.options[1] = glow != nullptr ? 1.0f : 0.0f;
    block.background[0] = background.getRedFloat();
    block.background[1] = background.getGreenFloat();
    block.background[2] = background.getBlueFloat();
    block.background[3] = background.getAlphaFloat();

    // Every texel is written, so the previous contents are cleared rather than loaded
    return (blurredCoverage ? radiancePass : pass).draw (frame, destination, {}, [&] (GpuRenderPass& renderPass)
    {
        renderPass.setUniformBuffer (0, 0, &view.getBlock(), sizeof (ViewData::Block));
        renderPass.setSampler (0, 3, samplers->pointClamp);
        renderPass.setUniformBuffer (1, 0, &block, sizeof (block));
        renderPass.setTexture (1, 1, source);

        // Without a glow the shader doesn't read it, but every binding needs a texture
        renderPass.setTexture (1, 2, glow != nullptr ? glow : source);

        if (blurredCoverage)
            renderPass.setTexture (1, 3, blurAmount);
    }, GpuColor::transparentBlack());
}

} // namespace yup
