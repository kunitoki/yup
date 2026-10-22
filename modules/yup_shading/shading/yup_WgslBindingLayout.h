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

#pragma once

namespace yup
{

//==============================================================================
/** Describes the bind group layouts a WGSL module needs, read from its module-scope
    resource declarations.

    WebGPU pipelines created with an explicit layout must list every resource the shader
    declares, with the exact texture sample type, storage format and access the shader
    uses. This reads them straight from the WGSL source, so it works for transpiled GLSL,
    shader bundles and hand-written WGSL alike, and has no dependency on WebGPU itself.

    Comments are ignored. A 32-bit float texture is described as filterable only when the
    shader samples it through a sampler (textureSample*() or textureGather(), directly or
    through the parameters of its own functions); otherwise it is unfilterable, as WebGPU
    derives it for a default layout.

    @see WgslTranspiler
*/
class YUP_API WgslBindingLayout final
{
public:
    //==============================================================================
    /** The kind of resource bound at a slot. */
    enum class ResourceKind
    {
        buffer,        ///< A uniform or storage buffer, see Entry::bufferType.
        sampler,       ///< A sampler, see Entry::samplerType.
        texture,       ///< A sampled texture, see Entry::sampleType.
        storageTexture ///< A storage texture, see Entry::storageFormat and Entry::storageAccess.
    };

    /** How a buffer is bound. */
    enum class BufferType
    {
        uniform,        ///< var<uniform>
        storage,        ///< var<storage, read_write>
        readOnlyStorage ///< var<storage> or var<storage, read>
    };

    /** How a sampler is bound. */
    enum class SamplerType
    {
        filtering, ///< sampler
        comparison ///< sampler_comparison
    };

    /** The sample type of a sampled texture. */
    enum class SampleType
    {
        floatFilterable,   ///< texture_*<f32> read through a sampler.
        floatUnfilterable, ///< texture_*<f32> only loaded, or multisampled.
        depth,             ///< texture_depth_*
        sint,              ///< texture_*<i32>
        uint               ///< texture_*<u32>
    };

    /** The view dimension of a sampled or storage texture. */
    enum class ViewDimension
    {
        texture1D,
        texture2D,
        array2D,
        cube,
        cubeArray,
        texture3D
    };

    /** The access of a storage texture. */
    enum class StorageAccess
    {
        readOnly,
        writeOnly,
        readWrite
    };

    /** One resource declared at module scope. Only the fields of its kind are meaningful. */
    struct Entry
    {
        String name;                                         ///< The WGSL variable name.
        uint32 group = 0;                                    ///< The @group index.
        uint32 binding = 0;                                  ///< The @binding index.
        ResourceKind kind = ResourceKind::buffer;            ///< What is bound.
        BufferType bufferType = BufferType::uniform;         ///< For buffers.
        SamplerType samplerType = SamplerType::filtering;    ///< For samplers.
        SampleType sampleType = SampleType::floatFilterable; ///< For textures.
        ViewDimension viewDimension = ViewDimension::texture2D; ///< For textures and storage textures.
        bool multisampled = false;                           ///< For textures.
        String storageFormat;                                ///< For storage textures, the WGSL texel format (e.g. "rgba16float").
        StorageAccess storageAccess = StorageAccess::writeOnly; ///< For storage textures.
    };

    //==============================================================================
    /** Lists every resource @p wgsl declares with @group and @binding.

        When two declarations share a slot, which the WGSL transpiler allows for a resource
        the shader never uses, the one the shader references is kept.

        @param wgsl  The WGSL module source.

        @returns The entries in declaration order, or a failure naming the first declaration
                 whose type cannot be bound in a bind group (such as texture_external).
    */
    static ResultValue<std::vector<Entry>> parse (const String& wgsl);
};

} // namespace yup
