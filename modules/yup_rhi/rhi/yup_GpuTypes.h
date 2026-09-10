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
/** Enumerates supported GPU platforms / backends. */
enum class GpuPlatform
{
    Headless, ///< Specifies the use of a headless context for rendering.
    OpenGL,   ///< Specifies the use of desktop OpenGL for rendering.
    OpenGLES, ///< Specifies the use of OpenGL ES (GLES 3.0+) for rendering (Android, WASM).
    Direct3D, ///< Specifies the use of Direct3D for rendering.
    Metal,    ///< Specifies the use of Metal for rendering.
    WebGPU    ///< Specifies the use of WebGPU (native browser WebGPU on Emscripten, Dawn elsewhere).
};

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
/** Copies a NUL-terminated shader source string into the byte blob GpuShaderSource
    expects, excluding the terminating NUL.
*/
inline std::vector<uint8> gpuShaderSourceBytes (const char* text)
{
    if (text == nullptr)
        return {};

    auto* bytes = reinterpret_cast<const uint8*> (text);
    return std::vector<uint8> (bytes, bytes + std::strlen (text));
}

/** Copies a String's UTF-8 bytes into the byte blob GpuShaderSource expects. */
inline std::vector<uint8> gpuShaderSourceBytes (const String& text)
{
    auto* bytes = reinterpret_cast<const uint8*> (text.toRawUTF8());
    return std::vector<uint8> (bytes, bytes + text.getNumBytesAsUTF8());
}

//==============================================================================
/** Compiled shader source for one pipeline stage (vertex, fragment, or compute).

    The binding-map sidecar (@c bindingMap) is mandatory for vertex/fragment
    stages. Compute shaders may omit it when using the native compute path
    (GpuComputePipeline).

    Each blob field owns its data, so a descriptor built from temporaries stays
    valid for as long as the descriptor does. Use gpuShaderSourceBytes() to
    build them from source text.

    @see GpuPipeline, GpuComputePipeline, gpuShaderSourceBytes
*/
struct GpuShaderSource
{
    GpuShaderSource() = default;

    /** Shading language of the source code. */
    GpuShaderLanguage language = GpuShaderLanguage::wgsl;

    /** Shader source code bytes. */
    std::vector<uint8> code;

    /** Mandatory pre-compiled RSTB binding-map sidecar blob (render pipelines). */
    std::vector<uint8> bindingMap;

    /** Optional GL program-link fixup blob (GLSL/ESSL only).

        On the OpenGL / OpenGL ES backend, UBO block bindings and sampler
        texture units are assigned by name after linking rather than via
        @c layout(binding=) qualifiers (which GLES 3.00 / WebGL2 cannot use).
        This blob carries the name→slot table; it is ignored by every non-GL
        backend. Produced by makeGLFixupBlob(). */
    std::vector<uint8> glFixup;

    /** Override the stage entry-point name. Empty → "vs_main" / "fs_main". */
    String entryPoint;
};

//==============================================================================
/** Identifies the intended usage of a GpuBuffer. */
enum class GpuBufferType : uint8_t
{
    vertex,  ///< Per-vertex attribute data, bound via GpuRenderPass::setVertexBuffer().
    index,   ///< Index data, bound via GpuRenderPass::setIndexBuffer().
    uniform, ///< Uniform (constant) data.
    storage, ///< Storage buffer (read-write SSBO, for compute shaders).
};

//==============================================================================
/** Per-vertex attribute data format. Mirrors the ore vertex formats. */
enum class GpuVertexFormat : uint8_t
{
    float1,    ///< One 32-bit float.
    float2,    ///< Two 32-bit floats.
    float3,    ///< Three 32-bit floats.
    float4,    ///< Four 32-bit floats.
    uint8x4,   ///< Four unsigned bytes (integer in shader).
    sint8x4,   ///< Four signed bytes (integer in shader).
    snorm8x4,  ///< Four signed bytes normalised to [-1, 1].
    unorm8x4,  ///< Four unsigned bytes normalised to [0, 1].
    uint16x2,  ///< Two unsigned shorts (integer in shader).
    sint16x2,  ///< Two signed shorts (integer in shader).
    unorm16x2, ///< Two unsigned shorts normalised to [0, 1].
    snorm16x2, ///< Two signed shorts normalised to [-1, 1].
    uint16x4,  ///< Four unsigned shorts (integer in shader).
    sint16x4,  ///< Four signed shorts (integer in shader).
    float16x2, ///< Two 16-bit floats.
    float16x4, ///< Four 16-bit floats.
    uint32,    ///< One 32-bit unsigned integer.
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
    dstAlpha,           ///< Destination alpha.
    oneMinusDstAlpha,   ///< 1 - destination alpha.
    srcAlphaSaturated,  ///< min(srcAlpha, 1 - dstAlpha).
    blendColor,         ///< The constant blend color set via GpuRenderPass::setBlendColor().
    oneMinusBlendColor, ///< 1 - the constant blend color.
};

/** Per-channel color write mask for a color target. Values are bit flags. */
enum class GpuColorWriteMask : uint8_t
{
    none = 0,      ///< Write no channels.
    red = 1 << 0,  ///< Write the red channel.
    green = 1 << 1, ///< Write the green channel.
    blue = 1 << 2, ///< Write the blue channel.
    alpha = 1 << 3, ///< Write the alpha channel.
    all = 0xF,     ///< Write every channel.
};

/** Bitwise OR of two color write masks. */
constexpr GpuColorWriteMask operator| (GpuColorWriteMask a, GpuColorWriteMask b)
{
    return static_cast<GpuColorWriteMask> (static_cast<uint8_t> (a) | static_cast<uint8_t> (b));
}

/** Bitwise AND of two color write masks. */
constexpr GpuColorWriteMask operator& (GpuColorWriteMask a, GpuColorWriteMask b)
{
    return static_cast<GpuColorWriteMask> (static_cast<uint8_t> (a) & static_cast<uint8_t> (b));
}

/** Blend equation for a color target. */
enum class GpuBlendOp : uint8_t
{
    add,             ///< src + dst.
    subtract,        ///< src - dst.
    reverseSubtract, ///< dst - src.
    min,             ///< min(src, dst).
    max,             ///< max(src, dst).
};

/** Texture / color target pixel format.

    Mirrors the full ore format set. Block-compressed formats are only usable for
    uploaded textures (never as render targets), and their availability varies by
    device - probe with GpuDevice::isFormatSupported() before creating one.

    Float render targets are extension-gated on OpenGL ES / WebGL2; probe with
    GpuDevice::isFormatRenderable() and degrade instead of rendering black.
*/
enum class GpuTextureFormat : uint8_t
{
    // 8-bit
    r8unorm,    ///< 8-bit red, unsigned normalised.
    rg8unorm,   ///< 8-bit red+green, unsigned normalised.
    rgba8unorm, ///< 8-bit RGBA, unsigned normalised.
    rgba8snorm, ///< 8-bit RGBA, signed normalised.
    bgra8unorm, ///< 8-bit BGRA, unsigned normalised.

    // 16-bit float
    rgba16float, ///< 16-bit float RGBA.
    rg16float,   ///< 16-bit float red+green.
    r16float,    ///< 16-bit float red.

    // 32-bit float
    rgba32float, ///< 32-bit float RGBA.
    rg32float,   ///< 32-bit float red+green.
    r32float,    ///< 32-bit float red.

    // Packed
    rgb10a2unorm,   ///< 10-bit RGB + 2-bit alpha, unsigned normalised.
    r11g11b10float, ///< Packed 11/11/10-bit float RGB.

    // Depth/stencil
    depth16unorm,         ///< 16-bit unsigned normalised depth.
    depth24plusStencil8,  ///< 24-bit depth + 8-bit stencil.
    depth32float,         ///< 32-bit float depth.
    depth32floatStencil8, ///< 32-bit float depth + 8-bit stencil.

    // Block compressed (upload only, runtime support varies)
    bc1unorm,  ///< BC1 (DXT1) compressed RGBA.
    bc3unorm,  ///< BC3 (DXT5) compressed RGBA.
    bc7unorm,  ///< BC7 compressed RGBA.
    etc2rgb8,  ///< ETC2 compressed RGB.
    etc2rgba8, ///< ETC2 compressed RGBA.
    astc4x4,   ///< ASTC 4x4 block compressed.
    astc6x6,   ///< ASTC 6x6 block compressed.
    astc8x8,   ///< ASTC 8x8 block compressed.
};

/** Returns true if the given format is a depth and/or stencil format. */
constexpr bool isDepthStencilFormat (GpuTextureFormat format) noexcept
{
    return format == GpuTextureFormat::depth16unorm
        || format == GpuTextureFormat::depth24plusStencil8
        || format == GpuTextureFormat::depth32float
        || format == GpuTextureFormat::depth32floatStencil8;
}

/** The shape of a texture's storage. */
enum class GpuTextureType : uint8_t
{
    texture2D, ///< A single 2D image.
    cube,      ///< Six square 2D faces forming a cube map.
    texture3D, ///< A volume of depthOrArrayLayers slices.
    array2D,   ///< An array of depthOrArrayLayers 2D images.
};

/** How a texture is interpreted when bound to a shader or an attachment. */
enum class GpuTextureViewDimension : uint8_t
{
    texture2D, ///< A single 2D image.
    cube,      ///< A cube map.
    texture3D, ///< A 3D volume.
    array2D,   ///< A 2D array.
    cubeArray, ///< An array of cube maps.
};

/** Which planes of a depth/stencil texture a view exposes. */
enum class GpuTextureAspect : uint8_t
{
    all,          ///< Every plane the format has.
    depthOnly,    ///< The depth plane only.
    stencilOnly,  ///< The stencil plane only.
};

/** What happens to an attachment's existing contents when a render pass begins. */
enum class GpuLoadOp : uint8_t
{
    clear,    ///< Fill the attachment with the clear value.
    load,     ///< Preserve the existing contents.
    dontCare, ///< Contents are undefined; the fastest option when the pass writes every pixel.
};

/** What happens to an attachment's contents when a render pass ends. */
enum class GpuStoreOp : uint8_t
{
    store,   ///< Write the results back to the attachment.
    discard, ///< Throw the results away (e.g. a transient depth buffer).
};

/** Dithering applied to reduce banding in gradients. Mirrors rive::gpu::DitherMode. */
enum class GpuDitherMode : uint8_t
{
    none,                     ///< No dithering.
    interleavedGradientNoise, ///< Interleaved gradient noise dithering.
};

/** Texture minification / magnification / mipmap filter. */
enum class GpuFilter : uint8_t
{
    nearest, ///< Nearest-neighbour sampling.
    linear,  ///< Linear interpolation.
};

/** Texture coordinate addressing mode outside [0, 1]. */
enum class GpuWrapMode : uint8_t
{
    repeat,       ///< Tile the texture.
    mirrorRepeat, ///< Tile the texture, mirroring every other repetition.
    clampToEdge,  ///< Clamp to the edge texel.
};

//==============================================================================
/** Describes a GPU texture to allocate.

    @see GpuTexture::create, GpuTarget::create
*/
struct GpuTextureDesc
{
    GpuTextureDesc() = default;

    /** Convenience constructor for the common 2D case. */
    GpuTextureDesc (uint32_t width, uint32_t height, GpuTextureFormat format, bool renderTarget = false)
        : width (width)
        , height (height)
        , format (format)
        , renderTarget (renderTarget)
    {
    }

    uint32_t width = 0;  ///< Width in texels (must be > 0).
    uint32_t height = 0; ///< Height in texels (must be > 0).

    /** Slice count for texture3D, layer count for array2D. Must be 1 for
        texture2D, and 6 for cube (each face is one layer). */
    uint32_t depthOrArrayLayers = 1;

    GpuTextureFormat format = GpuTextureFormat::rgba8unorm; ///< Texel format.
    GpuTextureType type = GpuTextureType::texture2D;        ///< Storage shape.

    /** Whether the texture may be used as a render pass attachment. */
    bool renderTarget = false;

    /** Number of mip levels to allocate. Storage is allocated for all of them;
        nothing generates their contents - render or upload each level. */
    uint32_t mipLevels = 1;

    /** MSAA sample count. Values above 1 require renderTarget = true.

        @warning On OpenGL / OpenGL ES an MSAA color texture is allocated as a
                 renderbuffer, so it can be resolved but never sampled. Resolve
                 into a separate single-sampled texture and sample that. */
    uint32_t sampleCount = 1;

    /** Optional debug label passed through to the native API. */
    String label;
};

//==============================================================================
/** Describes a CPU-to-GPU texture upload of one mip level of one layer.

    @see GpuTexture::upload
*/
struct GpuTextureDataDesc
{
    constexpr GpuTextureDataDesc() = default;

    const void* data = nullptr; ///< Source pixels (must be non-null).

    /** Bytes between consecutive rows of @c data. Zero means tightly packed
        (width * bytes-per-texel). */
    uint32_t bytesPerRow = 0;

    /** Rows between consecutive depth slices of @c data. Zero means @c height. */
    uint32_t rowsPerImage = 0;

    uint32_t mipLevel = 0; ///< Destination mip level.
    uint32_t layer = 0;    ///< Destination array layer, or cube face (0..5).

    uint32_t x = 0; ///< Destination x origin in texels.
    uint32_t y = 0; ///< Destination y origin in texels.
    uint32_t z = 0; ///< Destination z origin in texels (3D textures).

    uint32_t width = 0;  ///< Region width in texels. Zero means the whole mip level.
    uint32_t height = 0; ///< Region height in texels. Zero means the whole mip level.
    uint32_t depth = 1;  ///< Region depth in texels (3D textures).
};

//==============================================================================
/** Selects a sub-range of a texture's mip levels and layers.

    @warning A narrowed view is honoured for render pass *attachments* on every
             backend, but not for *sampling* on OpenGL / OpenGL ES, which has no
             glTextureView in GLES3 and silently falls back to the base texture.
             Read a specific mip with an explicit @c textureLod() plus a sampler
             whose minLod / maxLod clamp to that level instead.

    @see GpuTarget::createFromTexture
*/
struct GpuTextureViewDesc
{
    constexpr GpuTextureViewDesc() = default;

    /** Convenience constructor selecting one mip level of one layer. */
    constexpr GpuTextureViewDesc (uint32_t baseMipLevel, uint32_t baseLayer)
        : baseMipLevel (baseMipLevel)
        , baseLayer (baseLayer)
    {
    }

    /** How the view is interpreted. Ignored for attachments, which always target
        a single mip level of a single layer. */
    GpuTextureViewDimension dimension = GpuTextureViewDimension::texture2D;

    GpuTextureAspect aspect = GpuTextureAspect::all; ///< Which depth/stencil planes to expose.

    uint32_t baseMipLevel = 0; ///< First mip level the view covers.
    uint32_t mipCount = 1;     ///< Number of mip levels the view covers.
    uint32_t baseLayer = 0;    ///< First array layer / cube face the view covers.
    uint32_t layerCount = 1;   ///< Number of array layers / cube faces the view covers.

    /** Two descriptors are equal when they select the same range of the same shape. */
    constexpr bool operator== (const GpuTextureViewDesc&) const = default;
};

//==============================================================================
/** Describes a texture sampler.

    @see GpuSampler::create, GpuRenderPass::setSampler
*/
struct GpuSamplerDesc
{
    GpuSamplerDesc() = default;

    /** Convenience constructor for the common filter + wrap case. */
    GpuSamplerDesc (GpuFilter filter, GpuWrapMode wrap)
        : minFilter (filter)
        , magFilter (filter)
        , wrapU (wrap)
        , wrapV (wrap)
        , wrapW (wrap)
    {
    }

    GpuFilter minFilter = GpuFilter::nearest;    ///< Minification filter.
    GpuFilter magFilter = GpuFilter::nearest;    ///< Magnification filter.
    GpuFilter mipmapFilter = GpuFilter::nearest; ///< Filter applied between mip levels.

    GpuWrapMode wrapU = GpuWrapMode::clampToEdge; ///< Addressing mode along U.
    GpuWrapMode wrapV = GpuWrapMode::clampToEdge; ///< Addressing mode along V.
    GpuWrapMode wrapW = GpuWrapMode::clampToEdge; ///< Addressing mode along W.

    /** Set to make this a comparison (shadow) sampler. Leave empty for normal
        filtering. Must be set for a binding declared as a comparison sampler. */
    std::optional<GpuCompareFunction> compare;

    float minLod = 0.0f;  ///< Lowest mip level this sampler will read.
    float maxLod = 32.0f; ///< Highest mip level this sampler will read.

    /** Maximum anisotropy. 1 disables anisotropic filtering; higher values
        require GpuDevice::isAnisotropicFilteringAvailable(). */
    uint32_t maxAnisotropy = 1;

    /** Optional debug label passed through to the native API. */
    String label;
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

/** Describes the layout of one vertex buffer bound to a pipeline.

    The layout owns its attribute list, so a descriptor built from temporaries
    stays valid for as long as the descriptor does.
*/
struct GpuVertexBufferLayout
{
    GpuVertexBufferLayout() = default;

    GpuVertexBufferLayout (uint32_t stride, GpuVertexStepMode stepMode, std::vector<GpuVertexAttribute> attributes)
        : stride (stride)
        , stepMode (stepMode)
        , attributes (std::move (attributes))
    {
    }

    uint32_t stride = 0;                                    ///< Byte stride between vertices.
    GpuVertexStepMode stepMode = GpuVertexStepMode::vertex; ///< Per-vertex or per-instance.
    std::vector<GpuVertexAttribute> attributes;             ///< The attributes packed into each vertex.
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
    GpuColorWriteMask writeMask = GpuColorWriteMask::all;   ///< Channels this target writes.
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

    /** Vertex buffer layouts. Leave empty for fullscreen passes that generate
        vertices from the vertex index. */
    std::vector<GpuVertexBufferLayout> vertexBuffers;

    GpuPrimitiveTopology topology = GpuPrimitiveTopology::triangleList;
    GpuIndexFormat indexFormat = GpuIndexFormat::none;
    GpuCullMode cullMode = GpuCullMode::none;
    GpuFaceWinding winding = GpuFaceWinding::counterClockwise;

    /** Color targets. When empty, a single default alpha-blended rgba8unorm
        target is used. At most four are supported; extra entries are ignored. */
    std::vector<GpuColorTarget> colorTargets;

    GpuDepthStencilState depthStencil;
    GpuStencilFaceState stencilFront;
    GpuStencilFaceState stencilBack;
    uint8_t stencilReadMask = 0xFF;
    uint8_t stencilWriteMask = 0xFF;

    uint32_t sampleCount = 1; ///< MSAA sample count.
};

//==============================================================================
/** A lightweight 4-component color for GPU clear values and render options.

    Lives in yup_rhi to avoid a dependency on yup_graphics. Individual components
    are float in [0, 1]. Implicitly constructable from any type T that exposes
    getRedFloat(), getGreenFloat(), getBlueFloat(), getAlphaFloat() — e.g.
    yup::Color, so GpuRenderOptions { true, Colors::transparentBlack } just works.
*/
struct GpuColor
{
    constexpr GpuColor() = default;

    constexpr GpuColor (float r, float g, float b, float a = 1.0f)
        : red (r)
        , green (g)
        , blue (b)
        , alpha (a)
    {
    }

    /** Implicit conversion from any type with float-component accessors
        (e.g. yup::Color). */
    template <typename T>
        requires requires (const T& c) {
            c.getRedFloat();
            c.getGreenFloat();
            c.getBlueFloat();
            c.getAlphaFloat();
        }
    constexpr GpuColor (const T& color)
        : red (color.getRedFloat())
        , green (color.getGreenFloat())
        , blue (color.getBlueFloat())
        , alpha (color.getAlphaFloat())
    {
    }

    /** Explicit construct from a packed ARGB value (0xAARRGGBB). */
    explicit constexpr GpuColor (uint32_t argb)
        : red (((argb >> 16) & 0xFF) / 255.0f)
        , green (((argb >> 8) & 0xFF) / 255.0f)
        , blue (((argb >> 0) & 0xFF) / 255.0f)
        , alpha (((argb >> 24) & 0xFF) / 255.0f)
    {
    }

    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    float alpha = 0.0f;

    /** Opaque black (0, 0, 0, 1). */
    static constexpr GpuColor black() { return { 0.0f, 0.0f, 0.0f, 1.0f }; }

    /** Transparent black (0, 0, 0, 0). */
    static constexpr GpuColor transparentBlack() { return { 0.0f, 0.0f, 0.0f, 0.0f }; }

    /** Opaque white (1, 1, 1, 1). */
    static constexpr GpuColor white() { return { 1.0f, 1.0f, 1.0f, 1.0f }; }
};

//==============================================================================
/** Describes the GPU frame to open for offscreen 2D rendering.

    Mirrors rive::gpu::RenderContext::FrameDescriptor field-for-field, so it can
    reach GpuDevice::beginOffscreen() and GpuCanvas::beginDraw() without a Python
    or public-API dependency on the Rive renderer's own type.

    @c synthesizedFailureType is deliberately omitted: it is gated behind
    @c WITH_RIVE_TOOLS in Rive's header, and that macro is never defined
    anywhere in this repo's CMake, so there is nothing to mirror.
*/
struct GpuFrameDescriptor
{
    uint32_t renderTargetWidth = 0;  ///< Ignored by GpuCanvas::beginDraw(), which fills it from the canvas.
    uint32_t renderTargetHeight = 0; ///< Ignored by GpuCanvas::beginDraw(), which fills it from the canvas.

    GpuLoadOp loadOp = GpuLoadOp::clear; ///< Attachment load behaviour at the start of the frame.
    GpuColor clearColor = GpuColor::transparentBlack(); ///< Clear color used when loadOp is clear.

    uint32_t msaaSampleCount = 0; ///< If nonzero, the number of MSAA samples to use; forces msaa mode.
    bool disableRasterOrdering = false; ///< Use atomic mode (preferred) or msaa instead of rasterOrdering.
    GpuDitherMode ditherMode = GpuDitherMode::interleavedGradientNoise; ///< Dithering applied to gradients.

    // Vulkan-only virtual tiling; inert on every current YUP backend.
    uint32_t virtualTileWidth = 0;
    uint32_t virtualTileHeight = 0;

    // Testing flags.
    bool wireframe = false;
    bool fillsDisabled = false;
    bool strokesDisabled = false;
    bool clockwiseFillOverride = false; ///< Override all paths' fill rules to emulate clockwiseAtomic mode.
};

//==============================================================================
/** Per-color-attachment options controlling load and store behaviour.

    The @c { bool, GpuColor } constructor is the original two-state form and is
    kept so existing call sites such as @c { true, Colors::transparentBlack }
    keep compiling unchanged.
*/
struct GpuRenderOptions
{
    /** Default constructor: clears to transparent black and stores the result. */
    constexpr GpuRenderOptions() = default;

    /** Constructs options that either clear to @p clearColor or load the existing
        contents, and store the result either way. */
    constexpr GpuRenderOptions (bool clear, GpuColor clearColor)
        : loadOp (clear ? GpuLoadOp::clear : GpuLoadOp::load)
        , clearColor (clearColor)
    {
    }

    /** Constructs options with explicit load and store operations. */
    constexpr GpuRenderOptions (GpuLoadOp loadOp, GpuStoreOp storeOp, GpuColor clearColor = GpuColor::transparentBlack())
        : loadOp (loadOp)
        , storeOp (storeOp)
        , clearColor (clearColor)
    {
    }

    GpuLoadOp loadOp = GpuLoadOp::clear;    ///< What to do with the existing contents.
    GpuStoreOp storeOp = GpuStoreOp::store; ///< What to do with the drawn result.

    /** Clear color used when @c loadOp is GpuLoadOp::clear. */
    GpuColor clearColor = GpuColor::transparentBlack();
};

//==============================================================================
/** Per-depth/stencil-attachment options controlling load and store behaviour.

    @see GpuRenderPass::setDepthStencilAttachment
*/
struct GpuDepthStencilOptions
{
    /** Default constructor: clears depth to 1.0 and discards stencil. */
    constexpr GpuDepthStencilOptions() = default;

    /** Constructs options clearing depth to @p depthClearValue. */
    constexpr GpuDepthStencilOptions (float depthClearValue)
        : depthClearValue (depthClearValue)
    {
    }

    GpuLoadOp depthLoadOp = GpuLoadOp::clear;    ///< What to do with the existing depth.
    GpuStoreOp depthStoreOp = GpuStoreOp::store; ///< What to do with the written depth.
    float depthClearValue = 1.0f;                ///< Depth clear value (far plane by default).

    GpuLoadOp stencilLoadOp = GpuLoadOp::clear;      ///< What to do with the existing stencil.
    GpuStoreOp stencilStoreOp = GpuStoreOp::discard; ///< What to do with the written stencil.
    uint32_t stencilClearValue = 0;                  ///< Stencil clear value.
};

//==============================================================================
/** Workgroup size for compute shader dispatch.

    Captures the local workgroup size declared in the compute shader via
    @c layout(local_size_x = X, local_size_y = Y, local_size_z = Z).

    @see GpuComputePipeline
*/
struct GpuWorkgroupSize
{
    uint32_t x = 1;
    uint32_t y = 1;
    uint32_t z = 1;
};

} // namespace yup
