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

namespace {

/** Converts a GpuLoadOp to the matching rive::gpu::LoadAction. */
rive::gpu::LoadAction toRiveLoadAction (GpuLoadOp op) noexcept
{
    switch (op)
    {
        case GpuLoadOp::load:
            return rive::gpu::LoadAction::preserveRenderTarget;
        case GpuLoadOp::dontCare:
            return rive::gpu::LoadAction::dontCare;
        case GpuLoadOp::clear:
        default:
            return rive::gpu::LoadAction::clear;
    }
}

/** Converts a GpuDitherMode to the matching rive::gpu::DitherMode. */
rive::gpu::DitherMode toRiveDitherMode (GpuDitherMode mode) noexcept
{
    switch (mode)
    {
        case GpuDitherMode::none:
            return rive::gpu::DitherMode::none;
        case GpuDitherMode::interleavedGradientNoise:
        default:
            return rive::gpu::DitherMode::interleavedGradientNoise;
    }
}

/** Converts a GpuFrameDescriptor to the rive::gpu::RenderContext::FrameDescriptor Rive expects. */
rive::gpu::RenderContext::FrameDescriptor toRiveFrameDescriptor (const GpuFrameDescriptor& desc) noexcept
{
    rive::gpu::RenderContext::FrameDescriptor frameDesc;
    frameDesc.renderTargetWidth = desc.renderTargetWidth;
    frameDesc.renderTargetHeight = desc.renderTargetHeight;
    frameDesc.loadAction = toRiveLoadAction (desc.loadOp);
    frameDesc.clearColor = rive::colorARGB (static_cast<int> (desc.clearColor.alpha * 255.0f + 0.5f),
                                            static_cast<int> (desc.clearColor.red * 255.0f + 0.5f),
                                            static_cast<int> (desc.clearColor.green * 255.0f + 0.5f),
                                            static_cast<int> (desc.clearColor.blue * 255.0f + 0.5f));
    frameDesc.msaaSampleCount = desc.msaaSampleCount;
    frameDesc.disableRasterOrdering = desc.disableRasterOrdering;
    frameDesc.ditherMode = toRiveDitherMode (desc.ditherMode);
    frameDesc.virtualTileWidth = desc.virtualTileWidth;
    frameDesc.virtualTileHeight = desc.virtualTileHeight;
    frameDesc.wireframe = desc.wireframe;
    frameDesc.fillsDisabled = desc.fillsDisabled;
    frameDesc.strokesDisabled = desc.strokesDisabled;
    frameDesc.clockwiseFillOverride = desc.clockwiseFillOverride;
    return frameDesc;
}

} // namespace

} // namespace yup
