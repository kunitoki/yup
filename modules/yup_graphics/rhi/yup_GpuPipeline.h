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
/** Identifies the shading language of a GpuShaderSource code block. */
enum class GpuShaderLanguage : uint8_t
{
    wgsl = 0, ///< WGSL (WebGPU Shading Language).
    glsl = 1, ///< GLSL (GLES 3.0+, GL path only).
    msl = 2,  ///< MSL (Metal Shading Language, Metal backend only).
    hlsl = 3, ///< HLSL (DirectX Shading Language, DirectX backend only).
};

//==============================================================================
/** Compiled shader source for one pipeline stage (vertex or fragment).

    The binding-map sidecar (@c bindingMap / @c bindingMapSize) is mandatory.
    It is produced offline by the Rive scripting-workspace RSTB toolchain and
    must accompany the shader code. GpuPipeline::compile() will assert and fail
    if the sidecar is missing.

    @see GpuPipeline
*/
struct GpuShaderSource
{
    GpuShaderSource() = default;

    /** Shading language of the source code. */
    GpuShaderLanguage language = GpuShaderLanguage::wgsl;

    /** Shader source code bytes. */
    const void* code = nullptr;

    /** Number of bytes in @c code. */
    uint32_t codeSize = 0;

    /** Mandatory pre-compiled RSTB binding-map sidecar blob. */
    const uint8_t* bindingMap = nullptr;

    /** Number of bytes in @c bindingMap. */
    uint32_t bindingMapSize = 0;

    /** Override the stage entry-point name. nullptr → "vs_main" / "fs_main". */
    const char* entryPoint = nullptr;
};

//==============================================================================
/** Per-vertex attribute data format. Mirrors the ore vertex formats. */
enum class GpuVertexFormat : uint8_t
{
    float1,   ///< One 32-bit float.
    float2,   ///< Two 32-bit floats.
    float3,   ///< Three 32-bit floats.
    float4,   ///< Four 32-bit floats.
    uint8x4,  ///< Four unsigned bytes (integer in shader).
    snorm8x4, ///< Four signed bytes normalised to [-1, 1].
    unorm8x4, ///< Four unsigned bytes normalised to [0, 1].
};

/** Vertex step mode: advance per vertex or per instance. */
enum class GpuVertexStepMode : uint8_t
{
    vertex,   ///< Attribute advances once per vertex.
    instance, ///< Attribute advances once per instance.
};

/** Primitive topology used to assemble vertices into primitives. */
enum class GpuPrimitiveTopology : uint8_t
{
    pointList,     ///< Each vertex is a point.
    lineList,      ///< Each pair of vertices is a line.
    lineStrip,     ///< Connected line strip.
    triangleList,  ///< Each triple of vertices is a triangle.
    triangleStrip, ///< Connected triangle strip.
};

/** Index buffer element format. */
enum class GpuIndexFormat : uint8_t
{
    none,   ///< No index buffer (non-indexed draw).
    uint16, ///< 16-bit indices.
    uint32, ///< 32-bit indices.
};

/** Face culling mode. */
enum class GpuCullMode : uint8_t
{
    none,  ///< No culling.
    front, ///< Cull front-facing triangles.
    back,  ///< Cull back-facing triangles.
};

/** Winding order that defines a front-facing triangle. */
enum class GpuFaceWinding : uint8_t
{
    clockwise,        ///< Clockwise winding is front-facing.
    counterClockwise, ///< Counter-clockwise winding is front-facing.
};

/** Depth/stencil comparison function. */
enum class GpuCompareFunction : uint8_t
{
    never,        ///< Never passes.
    less,         ///< Passes if new < stored.
    equal,        ///< Passes if new == stored.
    lessEqual,    ///< Passes if new <= stored.
    greater,      ///< Passes if new > stored.
    notEqual,     ///< Passes if new != stored.
    greaterEqual, ///< Passes if new >= stored.
    always,       ///< Always passes.
};

/** Stencil operation applied on test results. */
enum class GpuStencilOp : uint8_t
{
    keep,           ///< Keep the current value.
    zero,           ///< Set to zero.
    replace,        ///< Replace with reference value.
    incrementClamp, ///< Increment and clamp.
    decrementClamp, ///< Decrement and clamp.
    invert,         ///< Bitwise invert.
    incrementWrap,  ///< Increment and wrap.
    decrementWrap,  ///< Decrement and wrap.
};

/** Blend factor for a color target. */
enum class GpuBlendFactor : uint8_t
{
    zero,             ///< 0.
    one,              ///< 1.
    srcColor,         ///< Source color.
    oneMinusSrcColor, ///< 1 - source color.
    srcAlpha,         ///< Source alpha.
    oneMinusSrcAlpha, ///< 1 - source alpha.
    dstColor,         ///< Destination color.
    oneMinusDstColor, ///< 1 - destination color.
    dstAlpha,         ///< Destination alpha.
    oneMinusDstAlpha, ///< 1 - destination alpha.
};

/** Blend equation for a color target. */
enum class GpuBlendOp : uint8_t
{
    add,             ///< src + dst.
    subtract,        ///< src - dst.
    reverseSubtract, ///< dst - src.
    min,             ///< min(src, dst).
    max,             ///< max(src, dst).
};

/** Color target pixel format. */
enum class GpuTextureFormat : uint8_t
{
    rgba8unorm,          ///< 8-bit RGBA, unsigned normalised.
    bgra8unorm,          ///< 8-bit BGRA, unsigned normalised.
    rgba16float,         ///< 16-bit float RGBA.
    depth24plusStencil8, ///< 24-bit depth + 8-bit stencil.
    depth32float,        ///< 32-bit float depth.
};

//==============================================================================
/** Describes a single vertex attribute within a vertex buffer layout. */
struct GpuVertexAttribute
{
    constexpr GpuVertexAttribute() = default;

    constexpr GpuVertexAttribute (GpuVertexFormat format, uint32_t offset, uint32_t shaderLocation)
        : format (format)
        , offset (offset)
        , shaderLocation (shaderLocation)
    {
    }

    GpuVertexFormat format = GpuVertexFormat::float4; ///< The attribute data format.
    uint32_t offset = 0;                              ///< Byte offset within the vertex.
    uint32_t shaderLocation = 0;                      ///< Shader @location index.
};

/** Describes the layout of one vertex buffer bound to a pipeline. */
struct GpuVertexBufferLayout
{
    constexpr GpuVertexBufferLayout() = default;

    constexpr GpuVertexBufferLayout (uint32_t stride, GpuVertexStepMode stepMode, const GpuVertexAttribute* attributes, uint32_t attributeCount)
        : stride (stride)
        , stepMode (stepMode)
        , attributes (attributes)
        , attributeCount (attributeCount)
    {
    }

    uint32_t stride = 0;                                    ///< Byte stride between vertices.
    GpuVertexStepMode stepMode = GpuVertexStepMode::vertex; ///< Per-vertex or per-instance.
    const GpuVertexAttribute* attributes = nullptr;         ///< Attribute array.
    uint32_t attributeCount = 0;                            ///< Number of attributes.
};

/** Blend state for a single color target. */
struct GpuBlendState
{
    constexpr GpuBlendState() = default;

    GpuBlendFactor srcColor = GpuBlendFactor::srcAlpha;
    GpuBlendFactor dstColor = GpuBlendFactor::oneMinusSrcAlpha;
    GpuBlendOp colorOp = GpuBlendOp::add;
    GpuBlendFactor srcAlpha = GpuBlendFactor::one;
    GpuBlendFactor dstAlpha = GpuBlendFactor::oneMinusSrcAlpha;
    GpuBlendOp alphaOp = GpuBlendOp::add;
};

/** State for a single color render target. */
struct GpuColorTarget
{
    constexpr GpuColorTarget() = default;

    GpuTextureFormat format = GpuTextureFormat::rgba8unorm; ///< Target pixel format.
    bool blendEnabled = true;                               ///< Enable alpha blending.
    GpuBlendState blend;                                    ///< Blend equation and factors.
};

/** Per-face stencil test state. */
struct GpuStencilFaceState
{
    constexpr GpuStencilFaceState() = default;

    GpuCompareFunction compare = GpuCompareFunction::always;
    GpuStencilOp failOp = GpuStencilOp::keep;
    GpuStencilOp depthFailOp = GpuStencilOp::keep;
    GpuStencilOp passOp = GpuStencilOp::keep;
};

/** Depth/stencil pipeline state.

    Leave @c enabled false (the default) for post-process passes that don't
    need a depth/stencil buffer. Setting it true attaches a depth/stencil
    target using @c format.
*/
struct GpuDepthStencilState
{
    constexpr GpuDepthStencilState() = default;

    bool enabled = false;                                            ///< Enable depth/stencil testing.
    GpuTextureFormat format = GpuTextureFormat::depth24plusStencil8; ///< Depth/stencil format.
    GpuCompareFunction depthCompare = GpuCompareFunction::less;      ///< Depth comparison function.
    bool depthWriteEnabled = true;                                   ///< Enable depth writes.
};

//==============================================================================
/** Full pipeline configuration for a GpuPipeline.

    Defaults reproduce the classic fullscreen-triangle post-process pipeline
    (no vertex buffers, no culling, single alpha-blended rgba8unorm target), so
    the two-shader GpuPipeline::compile() overload behaves as a fullscreen pass.

    For custom geometry rendering supply vertex buffer layouts, an index format,
    culling / winding, and optionally depth-stencil state or extra color targets.

    @see GpuPipeline::compile
*/
struct GpuPipelineOptions
{
    GpuPipelineOptions() = default;

    /** Vertex buffer layouts. Leave null/zero for fullscreen passes that
        generate vertices from the vertex index. */
    const GpuVertexBufferLayout* vertexBuffers = nullptr;
    uint32_t vertexBufferCount = 0;

    GpuPrimitiveTopology topology = GpuPrimitiveTopology::triangleList;
    GpuIndexFormat indexFormat = GpuIndexFormat::none;
    GpuCullMode cullMode = GpuCullMode::none;
    GpuFaceWinding winding = GpuFaceWinding::counterClockwise;

    /** Color targets. When @c colorTargetCount is zero a single default
        alpha-blended rgba8unorm target is used. Up to four are supported. */
    GpuColorTarget colorTargets[4] = {};
    uint32_t colorTargetCount = 0;

    GpuDepthStencilState depthStencil;
    GpuStencilFaceState stencilFront;
    GpuStencilFaceState stencilBack;
    uint8_t stencilReadMask = 0xFF;
    uint8_t stencilWriteMask = 0xFF;

    uint32_t sampleCount = 1; ///< MSAA sample count.
};

//==============================================================================
class GraphicsContext;

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

    Requires the GraphicsContext to have been created with
    Options::enableOreContext = true.

    @see GpuRenderPass, GpuFrame, GpuCanvas, GpuPipelineCache, GpuPipelineOptions
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

        Requires ctx.isGpuAvailable() (enableOreContext = true).
    */
    static ResultValue<GpuPipeline::Ptr> compile (GraphicsContext& ctx,
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

        @param ctx              A GraphicsContext with enableOreContext = true.
        @param bundle           Bundle containing the vertex and fragment stages.
        @param pipelineOptions  Pipeline configuration.

        @returns A compiled pipeline, or a failure with a human-readable description.

        @see ShaderBundle
    */
    static ResultValue<GpuPipeline::Ptr> compileFromBundle (GraphicsContext& ctx,
                                                            const ShaderBundle& bundle,
                                                            const GpuPipelineOptions& pipelineOptions = {});

#if YUP_ENABLE_SHADER_TRANSPILER
    /** Compiles a GpuPipeline directly from GLSL 450 vertex and fragment sources.

        Convenience that transpiles the GLSL to the native language of the
        context's graphics API, derives the binding-map sidecar via reflection,
        and compiles the pipeline. Only available when the shader transpiler is
        compiled in (YUP_ENABLE_SHADER_TRANSPILER = 1).

        @param ctx              A GraphicsContext with enableOreContext = true.
        @param vertexGlsl       GLSL 450 vertex shader source.
        @param fragmentGlsl     GLSL 450 fragment shader source.
        @param pipelineOptions  Pipeline configuration.

        @returns A compiled pipeline, or a failure with a human-readable description.
    */
    static ResultValue<GpuPipeline::Ptr> compileFromGlsl (GraphicsContext& ctx,
                                                          const String& vertexGlsl,
                                                          const String& fragmentGlsl,
                                                          const GpuPipelineOptions& pipelineOptions = {});
#endif

private:
    friend class GpuRenderPass;

    GpuPipeline() = default;

    static constexpr size_t kImplSize = 384;

    struct Impl;
    TypeErasedObject<kImplSize> impl;

    Impl* getImpl() noexcept;
    const Impl* getImpl() const noexcept;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuPipeline)
};

} // namespace yup
