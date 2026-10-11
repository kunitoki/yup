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
required. Resources keep their GLSL bindings, and the bundled reflection data,
derived from SPIR-V, describes the WGSL bindings including the extra sampler a
combined image sampler is split into.

The transpiler either produces WGSL that behaves like the GLSL or fails with a
`line:column` diagnostic: see [GLSL on the WebGPU target](wgsl-shaders.md) for
what it translates (implicit conversions, out parameters, switch fallthrough,
std140 layouts, texture functions, ...) and the constructs WGSL can't express.
Fragment shaders turn off WGSL's `derivative_uniformity` check, so `texture()`
can be called inside branches that depend on varyings, as GLSL allows.

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
    Span<const uint8> glTextureSamplerPairs;              // GL-only texture-sampler pairs
    String            entryPoint;                         // empty → "vs_main" / "fs_main"
};
```

`GpuShaderLanguage` values: `wgsl` (WebGPU), `glsl` (GLES 3.0+, GL path only),
`msl` (Metal only), `hlsl` (Direct3D only).

The blob fields **own their data** (`std::vector<uint8>`), so a descriptor
built from temporaries stays valid for as long as the descriptor does.
`gpuShaderSourceBytes()` builds one from source text:

```cpp
GpuShaderSource vs;
vs.language   = GpuShaderLanguage::glsl;
vs.code       = gpuShaderSourceBytes (vertexSource);   // const char* or String
vs.bindingMap = bindingMapBlob;                        // std::vector<uint8_t>
```

`GpuShaderSource` is exposed to Python too, with `code` / `bindingMap` /
`glFixup` as bytes-in, bytes-out properties; see
[GPU rendering from Python](../../scripting/python-rhi.md).

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
On WebGPU, a float texture that the WGSL of every stage only reads with
`texelFetch` (a WGSL `textureLoad`) is declared unfilterable, so a depth texture
can be bound there and read raw next to its comparison binding.

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

### Several bind groups

A graphics pipeline can spread its resources over several bind groups
(`set = 0, 1, 2...`) on every backend, and the groups may reuse binding
numbers: `layout(set = 0, binding = 0)` and `layout(set = 2, binding = 0)` are
two different resources, bound with `pass.setUniformBuffer (0, 0, ...)` and
`pass.setUniformBuffer (2, 0, ...)`.

Metal, Direct3D 11 and OpenGL have no descriptor sets, so `compileFromGlsl()`
and `yup_shader_bundler` assign their native slots automatically when the
vertex and fragment stages are compiled together. Each kind of resource
(buffers, textures, samplers) is numbered in (set, binding) order across both
stages, and a resource both stages use gets the same slot in each. Vulkan and
WebGPU keep the sets as they are. When you drive `ShaderBundleCompiler`
yourself, put the vertex and fragment entries in the same request, each with
its own `ShaderBundleEntry::source`.

Switching pipelines inside a render pass only rebuilds the bind groups of the
sets the two pipelines declare differently. A set declared identically in both
(same resources, stages, native slots and default samplers) keeps its bind
group. Declare the per-view resources the same way in every variant of a
pipeline, for example in `set = 0`, and their bind group is built once and
reused across the switches.

A draw only sends the backend what changed since the previous draw of the
pass. Setting the same pipeline, uniform bytes, texture with the same view
argument, sampler or storage buffer again costs nothing, so a draw can set all
of its resources without checking what is already bound. A different resource
or a different view argument rebuilds the bind group of its set, and so do
different uniform bytes, unless the pipeline declares the block dynamic (see
[Dynamic uniform buffers](#dynamic-uniform-buffers)); setting a
texture without a view where it was set with one counts as a change, even when
that view covered the whole texture. A texture, sampler or uniform block set on a
binding the current pipeline does not declare rebuilds nothing: it is kept, and
used once a pipeline that declares it is set. A different pipeline sends its bind
groups, viewport, scissor, stencil reference and blend color again: sorting
draws by pipeline, and keeping per-view resources in their own set, keeps the
CPU cost per draw low. On OpenGL and OpenGL ES every draw still sends its whole
state, since their bindings are global.

GLSL combines each texture with its sampler on the texture's unit, so on
OpenGL a sampler used with several textures is bound on each of their units.
`compileFromBundle()` passes the pairs to the GL backend in
`GpuShaderSource::glTextureSamplerPairs`, built by
`makeGLTextureSamplerPairsBlob()`.

Compute pipelines are not renumbered: a compute shader's resources bind at
their declared binding, as described in [Compute shaders](compute-shaders.md).

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

    std::vector<GpuColorTarget> colorTargets;  // up to 4; empty → one default target,
                                               // or none when depth is enabled

    GpuDepthStencilState depthStencil;       // enabled = false by default
    GpuStencilFaceState  stencilFront, stencilBack;
    uint8_t              stencilReadMask  = 0xFF;
    uint8_t              stencilWriteMask = 0xFF;

    uint32_t sampleCount = 1;                // MSAA

    std::vector<GpuUniformSlot> dynamicUniformBuffers; // per-draw uniform blocks
};
```

`vertexBuffers`, `colorTargets` and `dynamicUniformBuffers` own their contents,
and so does `GpuVertexBufferLayout::attributes`. Building a descriptor from
temporaries is therefore safe - no `static constexpr` table has to be kept alive
alongside it.

### Dynamic uniform buffers

A uniform block whose bytes change on every draw, such as an object's transform,
rebuilds the bind group of its set on every draw. List it in
`dynamicUniformBuffers` and the draws write it into a buffer the frame shares,
moving its offset instead:

```cpp
GpuPipelineOptions options;
options.dynamicUniformBuffers = { { 2, 0 }, { 2, 1 } }; // GpuUniformSlot { group, binding }

// ... then, for every object of a pass:
pass.setUniformBuffer (2, 0, &drawBlock, sizeof (drawBlock));
pass.setUniformBuffer (2, 1, &materialBlock, sizeof (materialBlock));
pass.drawIndexed (indexCount);
```

`setUniformBuffer` is called the same way. The set is then built once and reused
until one of its textures, samplers or storage buffers changes, or the frame's
buffer fills up and moves to a new one (about every 250 blocks).

- Slots the shaders do not declare are ignored, so pipelines that declare only
  some of the blocks can share one list.
- Every listed slot the shaders declare must be a uniform buffer, and at most 8
  are honored. Debug builds assert on a slot of another kind, which stays
  static, and on a list longer than 8, of which only the first 8 count.
- Each block takes 256 bytes of the shared buffer, or its size rounded up to 256.
- `GpuDevice::isDynamicUniformOffsetAvailable()` tells whether the device
  supports it: Metal, Vulkan, WebGPU and OpenGL / OpenGL ES / WebGL2 do, and
  Direct3D 11 does with the 11.1 runtime. Elsewhere the list is ignored and the
  blocks take the per-draw path, so a pipeline can list them unconditionally.
- The list is part of the `GpuPipelineCache` key.

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

### Depth-only pipelines and depth bias

Leave `colorTargets` empty and enable `depthStencil` to build a **depth-only**
pipeline, with no color output, for a [depth-only target](targets.md#depth-only-targets).
The fragment shader may be empty. Without depth, an empty `colorTargets` still
means one default `rgba8unorm` target.

`GpuDepthStencilState` also carries a rasterizer depth bias, the usual cure for
shadow acne:

```cpp
GpuPipelineOptions options;                       // no color targets
options.cullMode = GpuCullMode::back;
options.depthStencil.enabled = true;
options.depthStencil.format = GpuTextureFormat::depth32float;
options.depthStencil.depthCompare = GpuCompareFunction::lessEqual;
options.depthStencil.depthBias = 2;               // constant, in depth-format units
options.depthStencil.depthBiasSlopeScale = 1.5f;  // scaled by the polygon's depth slope
options.depthStencil.depthBiasClamp = 0.0f;       // 0 = unclamped
```

- The bias is added to the written and tested depth, pushing it away from the
  viewer (depth grows).
- `depthBiasClamp` limits the total bias. It is ignored where the backend lacks
  bias clamping: OpenGL / OpenGL ES (`glPolygonOffset` has no clamp) and Vulkan
  devices without the `depthBiasClamp` feature.

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
