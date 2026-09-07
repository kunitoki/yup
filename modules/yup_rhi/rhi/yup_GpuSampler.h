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

class GpuDevice;
class GpuRenderPass;

//==============================================================================
/** An immutable GPU sampler describing how a shader reads a texture.

    Every sampler binding a pipeline declares is given a linear / clamp-to-edge
    sampler by default, so pipelines that do not care about filtering need no
    sampler object at all. Create one and bind it with
    GpuRenderPass::setSampler() when a slot needs different filtering, wrapping,
    a mip filter, an explicit LOD range or anisotropy.

    A LOD-clamped sampler is also the portable way to read one specific mip level
    of a mip chain: a mip-narrowed texture *view* is honoured for attachments but
    silently degrades to mip 0 when sampled on OpenGL / OpenGL ES.

    @code
        yup::GpuSamplerDesc desc;
        desc.minFilter = desc.magFilter = desc.mipmapFilter = yup::GpuFilter::linear;
        desc.wrapU = desc.wrapV = yup::GpuWrapMode::repeat;
        desc.maxLod = 4.0f;

        auto sampler = yup::GpuSampler::create (device, desc);
        pass.setSampler (0, 1, sampler);
    @endcode

    @see GpuSamplerDesc, GpuRenderPass::setSampler, GpuTexture
*/
class YUP_API GpuSampler : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<GpuSampler>;

    //==============================================================================
    /** Creates a sampler from the given description.

        @param device  A GpuDevice with a GPU context available.
        @param desc    The sampler state to create.

        @returns A GpuSampler, or nullptr if the backend could not create it.

        @warning Requires device->isGpuAvailable(). Setting maxAnisotropy above 1
                 requires GpuDevice::isAnisotropicFilteringAvailable().
    */
    static GpuSampler::Ptr create (ReferenceCountedObjectPtr<GpuDevice> device, const GpuSamplerDesc& desc);

    /** Destructor. Releases the GPU resources with the rendering context current. */
    ~GpuSampler() override;

    //==============================================================================
    /** Returns true if this sampler holds a valid GPU resource. */
    bool isValid() const noexcept;

    /** Returns the description this sampler was created from. */
    const GpuSamplerDesc& getDescription() const noexcept;

private:
    //==============================================================================
    friend class GpuRenderPass;

    GpuSampler() = default;

    /** Returns the underlying ore sampler. */
    rive::rcp<rive::ore::Sampler> getOreSampler() const noexcept { return oreSampler; }

    //==============================================================================
    GpuSamplerDesc description;

    ReferenceCountedObjectPtr<GpuDevice> device;
    rive::rcp<rive::ore::Sampler> oreSampler;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuSampler)
};

} // namespace yup
