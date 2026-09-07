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

GpuSampler::Ptr GpuSampler::create (ReferenceCountedObjectPtr<GpuDevice> device, const GpuSamplerDesc& desc)
{
    if (device == nullptr)
        return nullptr;

    auto* oreCtx = device->getGpuContext();
    if (oreCtx == nullptr)
        return nullptr;

    jassert (desc.maxAnisotropy >= 1);
    jassert (desc.minLod <= desc.maxLod);

    const auto oreDesc = GpuPipelineHelpers::toOreSamplerDesc (desc);

    rive::rcp<rive::ore::Sampler> oreSampler;
    device->runOnGraphicsContext ([&]
    {
        oreSampler = oreCtx->makeSampler (oreDesc);
    });

    if (oreSampler == nullptr)
        return nullptr;

    GpuSampler::Ptr s = new GpuSampler();
    s->device = std::move (device);
    s->oreSampler = std::move (oreSampler);
    s->description = desc;
    return s;
}

GpuSampler::~GpuSampler()
{
    if (device == nullptr)
        return;

    device->runOnGraphicsContext ([this]
    {
        oreSampler = nullptr;
    });
}

//==============================================================================

bool GpuSampler::isValid() const noexcept
{
    return oreSampler != nullptr;
}

const GpuSamplerDesc& GpuSampler::getDescription() const noexcept
{
    return description;
}

} // namespace yup
