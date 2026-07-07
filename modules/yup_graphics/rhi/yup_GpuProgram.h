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
    must accompany the shader code. GpuProgram::compile() will assert and fail
    if the sidecar is missing.

    @see GpuProgram
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
/** Full pipeline configuration for a GpuProgram.

    Defaults reproduce the classic fullscreen-triangle post-process pipeline
    (no vertex buffers, no culling, single alpha-blended rgba8unorm target), so
    the two-argument GpuProgram::compile() overload behaves exactly as before.

    For custom geometry rendering supply vertex buffer layouts, an index format,
    culling / winding, and optionally depth-stencil state or extra color targets.

    @see GpuProgram::compile
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
/** Per-draw options controlling the render pass a draw call encodes. */
struct GpuRenderOptions
{
    constexpr GpuRenderOptions() = default;

    constexpr GpuRenderOptions (bool clear, Color clearColor)
        : clear (clear)
        , clearColor (clearColor)
    {
    }

    /** Whether to clear the target before drawing (LoadOp::clear). When false
        the existing contents are loaded (LoadOp::load). */
    bool clear = true;

    /** Clear color used when @c clear is true. */
    Color clearColor = Colors::transparentBlack;
};

class GraphicsContext;
class GpuTexture;
class GpuCanvas;
class GpuBuffer;

//==============================================================================
/** A compiled GPU render pipeline for custom shader dispatch.

    GpuProgram wraps an ore (Rive's backend-agnostic GPU layer) render pipeline
    consisting of a vertex shader and a fragment shader. It supports both
    fullscreen post-process effects and custom geometry rendering (indexed or
    non-indexed) with vertex buffers, culling, and depth-stencil state.

    Typical fullscreen post-process usage:
    @code
        auto prog = yup::GpuProgram::compile (ctx, vertSource, fragSource).getValue();

        auto canvas = yup::GpuCanvas::create (ctx, w, h);
        canvas->commit();                                 // commit Rive frame first
        prog->setTexture (0, 0, inputTexture);
        prog->setUniformBuffer (0, 1, &params, sizeof (params));
        prog->beginFrame();
        prog->dispatch (*canvas);                         // ore renders into canvas texture
        prog->endFrame();
        prog->waitForGPU();
        g.drawTexture (canvas->asTexture(), bounds);      // composite
    @endcode

    Typical custom geometry usage:
    @code
        yup::GpuPipelineOptions options;
        options.vertexBuffers = &layout;
        options.vertexBufferCount = 1;
        options.indexFormat = yup::GpuIndexFormat::uint16;
        options.cullMode = yup::GpuCullMode::back;
        auto prog = yup::GpuProgram::compile (ctx, vs, fs, options).getValue();

        auto vbo = yup::GpuBuffer::create (ctx, yup::GpuBufferType::vertex, verts, sizeof (verts));
        auto ibo = yup::GpuBuffer::create (ctx, yup::GpuBufferType::index, idx, sizeof (idx));

        prog->setVertexBuffer (0, vbo);
        prog->setIndexBuffer (ibo, yup::GpuIndexFormat::uint16);
        prog->setUniformBuffer (0, 0, &uniforms, sizeof (uniforms));
        prog->beginFrame();
        prog->drawIndexed (*canvas, indexCount, { true, yup::Colors::black });
        prog->endFrame();
        prog->waitForGPU();
    @endcode

    Requires the GraphicsContext to have been created with Options::enableOreContext = true.

    @see GpuCanvas, GpuBuffer, GpuShaderSource, GpuPipelineOptions, GraphicsContext::Options
*/
class YUP_API GpuProgram : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<GpuProgram>;

    //==============================================================================
    ~GpuProgram();

    //==============================================================================
    /** Binds a texture to the given (group, binding) slot.

        The texture may come from:
        - GpuCanvas::asTexture() (after commit) or from
        - Image::getTexture() (after Image::createTextureIfNotPresent()).
        
        If the same slot is set more than once the later call wins.
    */
    void setTexture (int group, int binding, GpuTexture::Ptr texture);

    /** Uploads raw uniform data to the given (group, binding) slot.

        The data is copied immediately; the caller need not keep it alive.
        If the same slot is set more than once the later call wins.
    */
    void setUniformBuffer (int group, int binding, const void* data, size_t byteSize);

    /** Binds a vertex buffer to the given slot for custom geometry rendering.

        The program must have been compiled with a matching vertex buffer layout
        in GpuPipelineOptions. The buffer is retained until replaced or the
        program is destroyed.
    */
    void setVertexBuffer (int slot, GpuBuffer::Ptr buffer);

    /** Binds an index buffer for indexed geometry rendering.

        Used by drawIndexed(). The format must match the pipeline's index format.
        The buffer is retained until replaced or the program is destroyed.
    */
    void setIndexBuffer (GpuIndexFormat format, GpuBuffer::Ptr buffer);

    //==============================================================================
    /** Begins an ore GPU frame.

        Must be called once before one or more dispatch()/draw()/drawIndexed()
        calls. Clears any GPU resources retained from the previous frame. Pair
        with endFrame().

        @return true on success; false if ore is unavailable.
    */
    bool beginFrame();

    /** Submits all render passes recorded since beginFrame().

        Must be called after all draw calls for this frame. Does not block the
        CPU - call waitForGPU() afterwards if you need results immediately.

        @return true on success; false if ore is unavailable.
    */
    bool endFrame();

    /** Blocks the calling thread until all submitted GPU work has completed. */
    void waitForGPU();

    //==============================================================================
    /** Encodes a fullscreen render pass into the committed GpuCanvas.

        Convenience for the classic fullscreen-triangle post-process pass:
        equivalent to draw (output, 3, { true, Colors::transparentBlack }).

        The canvas must already be committed. Must be called between beginFrame()
        and endFrame().

        @return true on success; false if invalid or the canvas isn't committed.
    */
    bool dispatch (GpuCanvas& output);

    /** Encodes a non-indexed draw of @c vertexCount vertices into the canvas.

        Binds any vertex buffers set via setVertexBuffer(). For fullscreen passes
        that generate vertices from the vertex index, pass vertexCount = 3 with no
        vertex buffers bound.

        @return true on success; false if invalid or the canvas isn't committed.
    */
    bool draw (GpuCanvas& output, uint32_t vertexCount, const GpuRenderOptions& options = {});

    /** Encodes an indexed draw of @c indexCount indices into the canvas.

        Binds the vertex buffers and index buffer set via setVertexBuffer() /
        setIndexBuffer().

        @return true on success; false if invalid, no index buffer is bound, or
                the canvas isn't committed.
    */
    bool drawIndexed (GpuCanvas& output, uint32_t indexCount, const GpuRenderOptions& options = {});

    //==============================================================================
    /** Compiles a fullscreen GpuProgram from vertex and fragment shader sources.

        Both shaders must supply pre-compiled RSTB binding-map blobs via
        GpuShaderSource::bindingMap. On failure the returned ResultValue holds a
        human-readable description of the failure.

        Requires ctx.gpuContext() != nullptr (enableOreContext = true).
    */
    static ResultValue<GpuProgram::Ptr> compile (GraphicsContext& ctx,
                                                 const GpuShaderSource& vertexShader,
                                                 const GpuShaderSource& fragmentShader);

    /** Compiles a GpuProgram with a full pipeline configuration.

        Use this overload to render custom geometry with vertex buffers, culling,
        depth-stencil state, or multiple color targets.

        @see GpuPipelineOptions
    */
    static ResultValue<GpuProgram::Ptr> compile (GraphicsContext& ctx,
                                                 const GpuShaderSource& vertexShader,
                                                 const GpuShaderSource& fragmentShader,
                                                 const GpuPipelineOptions& pipelineOptions);

    //==============================================================================
    /** Compiles a GpuProgram from a pre-built shader bundle.

        The bundle must contain both a vertex and a fragment shader stage. Picks
        the native shader variant matching the context's graphics API for each
        stage (Metal→MSL, Direct3D→HLSL, OpenGL(ES)→GLSL/ESSL, WebGPU→WGSL),
        derives the mandatory binding-map sidecar from the bundled reflection data,
        and compiles the pipeline. This is the recommended way to consume shaders
        loaded from .ysl files, and works without the shader transpiler.

        @param ctx              A GraphicsContext with enableOreContext = true.
        @param bundle           Bundle containing the vertex and fragment stages.
        @param pipelineOptions  Pipeline configuration.

        @returns A compiled program, or a failure with a human-readable description.

        @see ShaderBundle
    */
    static ResultValue<GpuProgram::Ptr> compileFromBundle (GraphicsContext& ctx,
                                                           const ShaderBundle& bundle,
                                                           const GpuPipelineOptions& pipelineOptions = {});

#if YUP_ENABLE_SHADER_TRANSPILER
    /** Compiles a GpuProgram directly from GLSL 450 vertex and fragment sources.

        Convenience that transpiles the GLSL to the native language of the
        context's graphics API, derives the binding-map sidecar via reflection,
        and compiles the pipeline. Only available when the shader transpiler is
        compiled in (YUP_ENABLE_SHADER_TRANSPILER = 1).

        @param ctx              A GraphicsContext with enableOreContext = true.
        @param vertexGlsl       GLSL 450 vertex shader source.
        @param fragmentGlsl     GLSL 450 fragment shader source.
        @param pipelineOptions  Pipeline configuration.

        @returns A compiled program, or a failure with a human-readable description.
    */
    static ResultValue<GpuProgram::Ptr> compileFromGlsl (GraphicsContext& ctx,
                                                         const String& vertexGlsl,
                                                         const String& fragmentGlsl,
                                                         const GpuPipelineOptions& pipelineOptions = {});
#endif

    //==============================================================================
    /** @internal Returns the ore Context used to compile this program, or nullptr. */
    rive::ore::Context* oreContext() const noexcept;

    /** @internal Returns the compiled ore Pipeline for advanced vertex / 3D draw calls. */
    rive::ore::Pipeline* orePipeline() const noexcept;

private:
    GpuProgram() = default;

    static constexpr size_t kImplSize = 384;

    struct Impl;
    TypeErasedObject<kImplSize> impl;

    Impl* getImpl() noexcept;
    const Impl* getImpl() const noexcept;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuProgram)
};

} // namespace yup
