# Pipelines & Shaders

A `GpuPipeline` is an **immutable, compiled** GPU render pipeline: a vertex
shader, a fragment shader, and fixed pipeline state (topology, culling, blending,
depth/stencil, color targets). Once compiled it never changes - all mutable
binding state lives on the [`GpuRenderPass`](frames-and-passes.md). Compile a
pipeline once and reuse it across many frames and passes.

Pipelines support both **fullscreen post-process effects** and **custom geometry
rendering** (indexed or non-indexed) with vertex buffers, culling, and
depth/stencil state.

```{note}
Compiling a pipeline requires a `GpuDevice` with GPU context available.
Check `ctx.isGpuAvailable()` first.
```

## Compiling a pipeline

There are three ways to build a pipeline, in order of preference for most use
cases:

### 1. From a shader bundle (recommended)

`compileFromBundle()` consumes a pre-built [`ShaderBundle`](offline-shaders.md)
(typically compiled offline into a `.ysl` file and embedded). It automatically
picks the native shader variant for the active backend (Metal → MSL, Direct3D →
HLSL, OpenGL(ES) → GLSL/ESSL, WebGPU → WGSL) and derives the mandatory
binding-map sidecar from the bundled reflection data. It works **without** the
shader transpiler compiled in.

For WebGPU, WGSL variants are generated from GLSL source via a direct GLSL→WGSL
transpiler that bypasses SPIR-V for code generation - no SPIRV-Cross WGSL backend
required. The bundled reflection data still derives from SPIR-V so binding
assignment stays consistent across all targets.

```cpp
ResultValue<GpuPipeline::Ptr> GpuPipeline::compileFromBundle (
    GpuDevice::Ptr          ctx,
    const ShaderBundle&       bundle,
    const GpuPipelineOptions& options = {});
```

```cpp
auto result = GpuPipeline::compileFromBundle (ctx, bundle, options);
if (result.wasOk())
    pipeline = result.getValue();
else
    DBG (result.getErrorMessage()); // human-readable failure description
```

### 2. From GLSL 450 source (transpiler required)

`compileFromGlsl()` transpiles GLSL 450 to the backend-native language, derives
the binding-map sidecar via reflection, and compiles. It is only available when
`YUP_ENABLE_SHADER_TRANSPILER == 1`. This is convenient for live-editing shaders
at runtime. All backend targets are supported, including WGSL via the built-in
GLSL→WGSL direct transpiler.

```cpp
#if YUP_ENABLE_SHADER_TRANSPILER
auto result = GpuPipeline::compileFromGlsl (ctx, vertexGlsl, fragmentGlsl, options);
#endif
```

### 3. From explicit shader sources (advanced)

`compile()` takes two `GpuShaderSource` structs directly. Each **must** carry a
pre-compiled RSTB binding-map blob - see [Binding maps](#binding-maps).

```cpp
ResultValue<GpuPipeline::Ptr> GpuPipeline::compile (
    GpuDevice::Ptr          ctx,
    const GpuShaderSource&    vertexShader,
    const GpuShaderSource&    fragmentShader,
    const GpuPipelineOptions& options = {});
```

## `GpuShaderSource`

Describes one pipeline stage's compiled source plus its mandatory metadata:

```cpp
struct GpuShaderSource
{
    GpuShaderLanguage language = GpuShaderLanguage::wgsl; // wgsl | glsl | msl | hlsl
    Span<const uint8> code;                               // source/bytecode
    Span<const uint8> bindingMap;                         // mandatory RSTB sidecar
    Span<const uint8> glFixup;                            // GL-only name→slot table
    String            entryPoint;                         // empty → "vs_main" / "fs_main"
};
```

`GpuShaderLanguage` values: `wgsl` (WebGPU), `glsl` (GLES 3.0+, GL path only),
`msl` (Metal only), `hlsl` (Direct3D only).

The three blob fields are **non-owning views**. Every backend consumes them
synchronously while compiling the shader module, so they only have to stay alive
for the duration of the `compile()` call - but they do have to stay alive for all
of it. `gpuShaderSourceBytes()` builds one from source text:

```cpp
GpuShaderSource vs;
vs.language   = GpuShaderLanguage::glsl;
vs.code       = gpuShaderSourceBytes (vertexSource);   // const char* or String
vs.bindingMap = bindingMapBlob;                        // std::vector<uint8_t>
```

Because they borrow, `GpuShaderSource` is not exposed to Python - a settable
attribute there would store a pointer into a temporary. Scripts compile through
`compileFromGlsl` instead; see [GPU rendering from Python](../../scripting/python-rhi.md).

### Binding maps

`GpuPipeline` (and the underlying GPU layer) require a pre-compiled **RSTB
binding-map sidecar** for every shader stage. `compile()` will assert and fail
if it is missing. The higher-level `compileFromBundle()` and `compileFromGlsl()`
entry points produce this sidecar for you.

To build one yourself from shader reflection data, use the helpers in
`yup_ShaderBindingMap.h`:

```cpp
std::vector<uint8_t> makeShaderBindingMapBlob (const ShaderReflection& reflection,
                                               ShaderStage             stage);
```

The blob maps uniform buffers, separate images (textures), separate samplers,
and read/write storage buffers to their GPU resource kinds, carrying the
reflected native backend slot for the stage. Texture entries also carry the
reflected view dimension, sample type and multisampled flag, which WebGPU
validates against the shader itself - so a `textureCube`, `texture3D`,
`texture2DArray` or depth texture is declared accurately rather than assumed 2D.

> **Shaders must declare textures and samplers separately.** A combined
> `uniform sampler2D` produces *no* binding-map entries at all and will not bind.
> Write Vulkan-style instead - a `texture2D` / `textureCube` plus a `sampler`,
> combined at the use site:
>
> ```glsl
> layout(set = 0, binding = 0) uniform textureCube u_env;
> layout(set = 0, binding = 1) uniform sampler     u_samp;
> ...
> vec3 c = textureLod (samplerCube (u_env, u_samp), direction, lod).rgb;
> ```

### The GL fixup blob

OpenGL / OpenGL ES (GLES 3.00 / WebGL2) cannot express UBO block bindings or
sampler texture units via `layout(binding=)` qualifiers, so the Rive' GPU
specific GL backend assigns them by name after linking. Supply the name→slot
table via `GpuShaderSource::glFixup`, produced by:

```cpp
std::vector<uint8_t> makeGLFixupBlob (const ShaderReflection& reflection);
```

This blob is ignored by every non-GL backend.

## `GpuPipelineOptions`

Full pipeline configuration. The defaults reproduce a classic fullscreen-triangle
post-process pipeline: no vertex buffers, no culling, a single alpha-blended
`rgba8unorm` target - so passing `{}` gives you a working fullscreen pass.

```cpp
struct GpuPipelineOptions
{
    std::vector<GpuVertexBufferLayout> vertexBuffers; // empty for fullscreen

    GpuPrimitiveTopology topology    = GpuPrimitiveTopology::triangleList;
    GpuIndexFormat       indexFormat = GpuIndexFormat::none;
    GpuCullMode          cullMode    = GpuCullMode::none;
    GpuFaceWinding       winding     = GpuFaceWinding::counterClockwise;

    std::vector<GpuColorTarget> colorTargets;  // up to 4; empty → one default target

    GpuDepthStencilState depthStencil;       // enabled = false by default
    GpuStencilFaceState  stencilFront, stencilBack;
    uint8_t              stencilReadMask  = 0xFF;
    uint8_t              stencilWriteMask = 0xFF;

    uint32_t sampleCount = 1;                // MSAA
};
```

`vertexBuffers` and `colorTargets` own their contents, and so does
`GpuVertexBufferLayout::attributes`. Building a descriptor from temporaries is
therefore safe - no `static constexpr` table has to be kept alive alongside it.

### Custom geometry example

For 3D or custom 2D geometry, describe the vertex layout, enable culling, and
(optionally) depth testing:

```cpp
GpuPipelineOptions options;

options.vertexBuffers.emplace_back (
    sizeof (float) * 10,          // stride
    GpuVertexStepMode::vertex,
    std::vector<GpuVertexAttribute> {
        { GpuVertexFormat::float3, 0,                  0 }, // position @location(0)
        { GpuVertexFormat::float4, sizeof (float) * 3, 1 }, // color    @location(1)
        { GpuVertexFormat::float3, sizeof (float) * 7, 2 }, // normal   @location(2)
    });

options.indexFormat = GpuIndexFormat::uint16;
options.cullMode    = GpuCullMode::back;
options.winding     = GpuFaceWinding::counterClockwise;
options.depthStencil.enabled = true;
```

### State enumerations

The pipeline configuration draws on a family of small enums, all mirroring the
`ore` formats:

- **`GpuVertexFormat`** - `float1`…`float4`, `uint8x4`, `sint8x4`, `snorm8x4`,
  `unorm8x4`, `uint16x2`, `sint16x2`, `unorm16x2`, `snorm16x2`, `uint16x4`,
  `sint16x4`, `float16x2`, `float16x4`, `uint32`.
- **`GpuVertexStepMode`** - `vertex`, `instance`. Per-instance buffers advance
  once per instance of a `draw()` / `drawIndexed()` with `instanceCount > 1`.
- **`GpuPrimitiveTopology`** - `pointList`, `lineList`, `lineStrip`,
  `triangleList`, `triangleStrip`.
- **`GpuIndexFormat`** - `none`, `uint16`, `uint32`.
- **`GpuCullMode`** - `none`, `front`, `back`.
- **`GpuFaceWinding`** - `clockwise`, `counterClockwise`.
- **`GpuCompareFunction`** - `never`, `less`, `equal`, `lessEqual`, `greater`,
  `notEqual`, `greaterEqual`, `always`.
- **`GpuStencilOp`** - `keep`, `zero`, `replace`, `incrementClamp`,
  `decrementClamp`, `invert`, `incrementWrap`, `decrementWrap`.
- **`GpuBlendFactor`** / **`GpuBlendOp`** - standard blend factors and equations,
  including `srcAlphaSaturated`, `blendColor` and `oneMinusBlendColor` (the last
  two read the constant set by `GpuRenderPass::setBlendColor()`).
- **`GpuColorWriteMask`** - per-channel write mask on `GpuColorTarget::writeMask`.
- **`GpuTextureFormat`** - the full format set, documented under
  [Buffers & Textures](buffers-and-textures.md#gputexture).

When `colorTargets` holds more than one entry, the pass must bind a matching
attachment for each target with `GpuRenderPass::setColorAttachment()`, and each
`colorTargets[i].format` must equal the format of the texture bound there.
Likewise `depthStencil.enabled` requires the pass to bind a depth attachment of
`depthStencil.format` via `GpuRenderPass::setDepthStencilAttachment()`. A
mismatch is rejected by the backend, which YUP surfaces as a debug assertion
rather than letting the draw silently render nothing.

## `GpuPipelineCache`

Compiling pipelines is expensive. `GpuPipelineCache` is a thread-safe
compile-or-fetch cache keyed by a deterministic hash of the selected native
shader sources, entry points, pipeline options, and graphics API.

```cpp
GpuPipelineCache pipelines (ctx); // ctx must outlive the cache

auto result = pipelines.getOrCompile (bundle, options);
if (result.wasOk())
    auto pipeline = result.getValue();
```

| Method                                    | Description                                              |
| ----------------------------------------- | -------------------------------------------------------- |
| `getOrCompile (bundle, options)`          | Fetches a cached pipeline or compiles one. Key is auto-derived. |
| `getOrCompile (key, bundle, options)`     | Same, with an explicit cache key.                        |
| `store (key, pipeline)`                   | Inserts a pipeline directly.                             |
| `contains (key)` / `remove (key)` / `clear()` | Cache maintenance.                                   |
| `getNumEntries()`                         | Current entry count.                                     |
| `setMaxEntries (n)` / `getMaxEntries()`   | LRU eviction limit (0 = unlimited; default 256).         |
| `generateCacheKey (bundle, options, api)` | Static SHA1-based key generation.                        |

The cache references an keeps the `GpuDevice` alive via `Ptr`.
Eviction is LRU by access order.
