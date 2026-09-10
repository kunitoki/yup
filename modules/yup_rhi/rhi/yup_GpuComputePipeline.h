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
/** An immutable, compiled GPU compute pipeline.

    GpuComputePipeline holds a single compute shader stage compiled for the
    current GPU backend. It is immutable once compiled — bind resources and
    dispatch workgroups via GpuComputePass.

    Compile a pipeline once and reuse it across frames and compute passes.

    @warning Requires GpuDevice::isComputeAvailable() (Metal, D3D11, WebGPU,
             or OpenGL ≥4.3 / GLES ≥3.1).

    @see GpuComputePass, GpuDevice, GpuWorkgroupSize
*/
class YUP_API GpuComputePipeline : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<GpuComputePipeline>;

    //==============================================================================
    /** Compiles a compute pipeline from a native shader source.

        Provide the source code in the shading language matching your target
        platform (MSL for Metal, HLSL for Direct3D, WGSL for WebGPU, GLSL for
        OpenGL). On failure the returned ResultValue holds a human-readable
        description.

        @param ctx            A GpuDevice with compute shader support.
        @param source         Compute shader source and language.
        @param workgroupSize  The local workgroup size declared in the shader.

        @returns A compiled compute pipeline, or a failure description.

        @warning Requires ctx->isComputeAvailable().
    */
    static ResultValue<Ptr> compile (GpuDevice::Ptr ctx,
                                     const GpuShaderSource& source,
                                     const GpuWorkgroupSize& workgroupSize);

    /** Compiles a compute pipeline from a pre-built shader bundle (.ysl).

        The bundle must contain a compute shader stage. Picks the native variant
        matching the device's graphics API and compiles the pipeline. This is the
        recommended path for shaders built offline — no transpiler needed at
        runtime.

        @param ctx            A GpuDevice with compute shader support.
        @param bundle         Bundle containing a compute shader stage.
        @param workgroupSize  Overrides the bundle's reflected workgroup size
                              when non-zero.

        @returns A compiled compute pipeline, or a failure description.

        @see ShaderBundle
    */
    static ResultValue<Ptr> compileFromBundle (GpuDevice::Ptr ctx,
                                               const ShaderBundle& bundle,
                                               const GpuWorkgroupSize& workgroupSize = {});

#if YUP_ENABLE_SHADER_TRANSPILER
    /** Compiles a compute pipeline from GLSL 450 source (online transpilation).

        Transpiles the GLSL to the native shading language, reflects the
        workgroup size, and compiles the pipeline. Only available when the shader
        transpiler is enabled at build time.

        @param ctx            A GpuDevice with compute shader support.
        @param glsl           GLSL 450 compute shader source.
        @param workgroupSize  Overrides the reflected workgroup size when non-zero.

        @returns A compiled compute pipeline, or a failure description.
    */
    static ResultValue<Ptr> compileFromGlsl (GpuDevice::Ptr ctx,
                                             const String& glsl,
                                             const GpuWorkgroupSize& workgroupSize = {});
#endif

    //==============================================================================
    /** Returns the local workgroup size this pipeline was compiled with. */
    virtual GpuWorkgroupSize getWorkgroupSize() const noexcept = 0;

protected:
    GpuComputePipeline() = default;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuComputePipeline)
};

} // namespace yup
