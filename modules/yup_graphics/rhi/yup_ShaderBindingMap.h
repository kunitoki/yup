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
/** Builds a GPU RSTB binding-map blob from shader reflection data.

    GpuPipeline (and the underlying GPU layer) require a pre-compiled RSTB
    binding-map sidecar for every shader stage. This helper converts a
    ShaderReflection (produced by ShaderTranspiler / ShaderBundle) into the
    binary blob format expected by GpuShaderSource::bindingMap.

    Uniform buffers, separate images (textures), separate samplers, and
    read/write storage buffers are all mapped to their corresponding GPU
    ResourceKind, carrying the reflected native backend slot for the given
    stage.

    @param reflection  The reflection data for a single shader stage.
    @param stage       Which stage the reflection belongs to (vertex/fragment).

    @returns A binary blob suitable for GpuShaderSource::bindingMap. The blob is
             empty only if the reflection declares no bindable resources.

    @see GpuShaderSource, GpuPipeline, ShaderReflection
*/
std::vector<uint8_t> makeShaderBindingMapBlob (const ShaderReflection& reflection,
                                               ShaderStage stage);

//==============================================================================
/** Builds a GPU program-link fixup blob from shader reflection data.

    OpenGL / OpenGL ES (GLES 3.00 / WebGL2) cannot express UBO block bindings or
    sampler texture units via @c layout(binding=) qualifiers, so the GL
    backend fixes them up by name after linking (@c glUniformBlockBinding /
    @c glUniform1i). This helper serialises the required name -> slot table for a
    single stage into the blob format consumed by @c ShaderModuleDesc::glFixupBytes.

    Uniform buffers contribute their block name + binding point; combined
    texture+sampler uniforms (see ShaderReflection::glCombinedSamplers) contribute
    their emitted uniform name + texture unit.

    @param reflection  The reflection data for a single shader stage (glsl/essl).

    @returns A binary blob suitable for GpuShaderSource::glFixup. Empty when the
             stage declares no uniform buffers or combined samplers.

    @see GpuShaderSource, GpuPipeline, ShaderReflection
*/
std::vector<uint8_t> makeGLFixupBlob (const ShaderReflection& reflection);

} // namespace yup
