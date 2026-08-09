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

//==============================================================================
/** An immutable, compiled GPU render pipeline.

    GpuPipeline wraps an ore (Rive's backend-agnostic GPU layer) render pipeline
    consisting of a vertex shader and a fragment shader plus fixed pipeline
    state. It supports both fullscreen post-process effects and custom geometry
    rendering (indexed or non-indexed) with vertex buffers, culling, and
    depth-stencil state.

    A pipeline is immutable once compiled: mutable binding state and per-draw
    encoding live on GpuRenderPass. Compile a pipeline once (or fetch it from a
    GpuPipelineCache) and reuse it across frames and render passes.

    @warning Requires the GpuDevice with a GPU context available on this backend.

    @see GpuRenderPass, GpuFrame, GpuPipelineCache, GpuPipelineOptions
*/
class YUP_API GpuPipeline : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<GpuPipeline>;

    //==============================================================================
    ~GpuPipeline();

    //==============================================================================
    /** Compiles a GpuPipeline from vertex and fragment shader sources.

        Both shaders must supply pre-compiled RSTB binding-map blobs via
        GpuShaderSource::bindingMap. On failure the returned ResultValue holds a
        human-readable description of the failure.

        @param ctx              A GpuDevice with GPU context available.
        @param vertexShader     Vertex shader source and binding-map sidecar.
        @param fragmentShader   Fragment shader source and binding-map sidecar.
        @param pipelineOptions  Pipeline configuration.

        @returns A compiled pipeline, or a failure with a human-readable description.

        @warning Requires ctx.isGpuAvailable() (GPU context available on this backend).
    */
    static ResultValue<GpuPipeline::Ptr> compile (GpuDevice::Ptr ctx,
                                                  const GpuShaderSource& vertexShader,
                                                  const GpuShaderSource& fragmentShader,
                                                  const GpuPipelineOptions& pipelineOptions = {});

    /** Compiles a GpuPipeline from a pre-built shader bundle.

        The bundle must contain both a vertex and a fragment shader stage. Picks
        the native shader variant matching the context's graphics API for each
        stage (Metal→MSL, Direct3D→HLSL, OpenGL(ES)→GLSL/ESSL, WebGPU→WGSL),
        derives the mandatory binding-map sidecar from the bundled reflection data,
        and compiles the pipeline. This is the recommended way to consume shaders
        loaded from .ysl files, and works without the shader transpiler.

        @param ctx              A GpuDevice with GPU context available.
        @param bundle           Bundle containing the vertex and fragment stages.
        @param pipelineOptions  Pipeline configuration.

        @returns A compiled pipeline, or a failure with a human-readable description.

        @warning Requires ctx.isGpuAvailable() (GPU context available on this backend).

        @see ShaderBundle
    */
    static ResultValue<GpuPipeline::Ptr> compileFromBundle (GpuDevice::Ptr ctx,
                                                            const ShaderBundle& bundle,
                                                            const GpuPipelineOptions& pipelineOptions = {});

#if YUP_ENABLE_SHADER_TRANSPILER
    /** Compiles a GpuPipeline directly from GLSL 450 vertex and fragment sources.

        Convenience that transpiles the GLSL to the native language of the
        context's graphics API, derives the binding-map sidecar via reflection,
        and compiles the pipeline. Only available when the shader transpiler is
        compiled in (YUP_ENABLE_SHADER_TRANSPILER = 1).

        @param ctx              A GpuDevice with GPU context available.
        @param vertexGlsl       GLSL 450 vertex shader source.
        @param fragmentGlsl     GLSL 450 fragment shader source.
        @param pipelineOptions  Pipeline configuration.

        @returns A compiled pipeline, or a failure with a human-readable description.

        @warning Requires ctx.isGpuAvailable() (GPU context available on this backend).

    */
    static ResultValue<GpuPipeline::Ptr> compileFromGlsl (GpuDevice::Ptr ctx,
                                                          const String& vertexGlsl,
                                                          const String& fragmentGlsl,
                                                          const GpuPipelineOptions& pipelineOptions = {});
#endif

private:
    friend class GpuRenderPass;

    GpuPipeline() = default;

    struct Impl;
    Impl* getImpl() noexcept;
    const Impl* getImpl() const noexcept;

    static constexpr size_t ImplSizeBytes = 384;
    TypeErasedObject<ImplSizeBytes> impl;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuPipeline)
};

} // namespace yup
