# Compute Shaders

The `yup_rhi` module supports GPU compute shaders through `GpuComputePipeline`
and `GpuComputePass`. Compute shaders run general-purpose GPU work — audio DSP,
physics simulation, particle systems, image processing — without any window,
framebuffer, or graphics pipeline.

## Availability

Compute shaders are available on backends that expose
`GpuDevice::isComputeAvailable() == true`: **Metal**, **Direct3D 11**, **Vulkan**,
**WebGPU** (Emscripten), and **OpenGL 4.3+** / **OpenGL ES 3.1+**.

Compute is **not** available on the Headless backend, nor on WebGL2 (Emscripten
without WebGPU), which is OpenGL ES 3.0 and has no compute shaders at all.

## Architecture

Compute shaders bypass Rive's ore layer (`rive::ore`) entirely. The ore layer
does not yet expose compute dispatch, so `GpuComputePipeline` and
`GpuComputePass` go directly to the backend-native API:

| Backend      | Pipeline compilation                   | Dispatch                           |
| ------------ | -------------------------------------- | ---------------------------------- |
| Metal        | `MTLComputePipelineState`              | `dispatchThreadgroups:`            |
| Direct3D 11  | `ID3D11ComputeShader`                  | `ID3D11DeviceContext::Dispatch()`  |
| Vulkan       | SPIR-V compute `VkPipeline`            | `vkCmdDispatch()`                  |
| WebGPU       | `wgpu::ComputePipeline`                | `DispatchWorkgroups()`             |
| OpenGL       | `GL_COMPUTE_SHADER` + program link     | `glDispatchCompute()`              |

On OpenGL, compute work runs on a dedicated, unshared GL context
(`GpuDevice::Options::computeContextActivator`, routed through
`GpuDevice::runOnComputeContext()`). Some drivers silently stop executing
compute dispatches when they are interleaved with rendering on the same
context — or on any context of the same share group — so pipeline compilation,
dispatches, storage buffer create/update/readback, and the matching deletions
all live exclusively on that context. When no dedicated context is available,
compute falls back to the rendering context.

## Compiling a compute pipeline

### From GLSL (online, requires `YUP_ENABLE_SHADER_TRANSPILER`)

```cpp
auto result = GpuComputePipeline::compileFromGlsl (device, glslSource);
if (result.wasOk())
    auto pipeline = result.getValue();
```

The transpiler compiles GLSL → SPIR-V, reflects the workgroup size from
`layout(local_size_x=...)`, transpiles to the backend-native language (MSL,
HLSL, WGSL), and compiles the final pipeline.

### From a `.ysl` shader bundle (offline)

```cpp
auto bundle = ShaderBundle::loadFromFile (File ("audio_effect.ysl"));
if (bundle.wasOk())
{
    auto result = GpuComputePipeline::compileFromBundle (device, bundle.getReference());
    if (result.wasOk())
        auto pipeline = result.getValue();
}
```

### From raw native source (any backend)

```cpp
GpuShaderSource source;
source.language = GpuShaderLanguage::msl; // or hlsl, wgsl, glsl
source.code = gpuShaderSourceBytes (mslSource); // must outlive the compile call

auto result = GpuComputePipeline::compile (device, source, { 256, 1, 1 });
```

## Dispatching compute work

```cpp
auto pass = GpuComputePass::begin (device);

pass.setPipeline (pipeline);
pass.setStorageBuffer (0, 0, inputBuffer);   // SSBO binding (set=0, binding=0)
pass.setStorageBuffer (0, 1, outputBuffer);  // SSBO binding (set=0, binding=1)
pass.setUniformBuffer (0, 2, &params, sizeof params);

uint32_t groupsX = (numElements + 255) / 256; // workgroupSize.x = 256
pass.dispatch (groupsX, 1, 1);
pass.finish(); // commits work to the GPU
```

To feed render passes from compute results in the same frame, begin the pass on
the frame with `GpuComputePass::begin (frame)`. On Metal and Vulkan the
dispatches are encoded into the frame's command buffer, and on Direct3D 11 into
the immediate context the frame draws with, so they run in order with the
frame's render passes and are submitted with the frame:

```cpp
auto frame = GpuFrame::begin (device);

auto pass = GpuComputePass::begin (frame);
pass.setPipeline (pipeline);
pass.setStorageBuffer (0, 0, particles);
pass.dispatch (groupsX, 1, 1);

auto renderPass = target->beginRenderPass (frame); // its draws see the dispatch
// ...
frame.submit(); // readBuffer sees the results from here on
```

A frame records one pass at a time: a render pass drawing on the frame finishes
the compute pass, and beginning a compute pass closes the open render pass,
which reopens on its next draw. OpenGL and WebGPU begin a standalone pass, as
above.

On WebGPU the standalone pass is submitted to the queue when it finishes, and
the frame's render passes when the frame is submitted, so render passes still
see what the dispatch wrote as long as the compute pass finishes first (end its
scope or call `finish()`). The reverse does not hold: a dispatch that reads what
an earlier render pass of the same frame drew runs before that render pass.

## Storage buffers

Storage buffers (`GpuBufferType::storage`) are read-write GPU buffers for compute
shaders. Create them with `GpuBuffer::create()`:

```cpp
std::vector<float> data (numSamples, 0.0f);
auto buf = GpuBuffer::create (device,
                               GpuBufferType::storage,
                               data.data(),
                               data.size() * sizeof (float));
```

Unlike vertex/index/uniform buffers that go through ore, storage buffers are
allocated directly on the native API (Metal `MTLBuffer`, D3D11 raw buffer
with a UAV and an SRV, WebGPU `Storage` buffer, GL `GL_SHADER_STORAGE_BUFFER`).

### Updating a storage buffer in place

`GpuBuffer` is immutable once created — `GpuDevice::createBuffer()` always
allocates a new native resource. For code that feeds a storage buffer new
data every frame or audio callback (a compute effect processing a live
stream, for instance), reallocating on every iteration is expensive and, on
a real-time thread such as an audio callback, unsafe: buffer allocation has
unbounded, driver-dependent latency and can cause audible dropouts.

`GpuDevice::updateBuffer()` writes new data into an *existing* storage buffer
without reallocating it:

```cpp
// Once, outside the hot loop:
auto buf = GpuBuffer::create (device, GpuBufferType::storage, initialData, byteSize);

// Every frame / audio callback — no allocation:
device->updateBuffer (buf, newData, byteSize);
```

`byteSize` must not exceed the buffer's original size. Supported on all
compute-capable backends (Metal, D3D11, WebGPU, OpenGL).

### Binding indices on Metal and Direct3D 11

On Metal and Direct3D 11, compute resources must be declared in group (set) 0.
The transpiled MSL and HLSL (shader model 5.0, which has no register spaces)
index every resource by its GLSL `binding` alone, ignoring the set:
`binding = N` becomes `[[buffer(N)]]`, `[[texture(N)]]` or `[[sampler(N)]]` in
MSL, and `register(uN)` (storage buffers and storage textures), `register(tN)`
(textures), `register(sN)` (samplers) or `register(bN)` (uniform buffers) in
HLSL. `GpuComputePass` binds each resource kind at that index, with no
reflection layer translating `(set, binding)` pairs as the render pipeline
path has. A resource in any other set lands on the wrong slot (debug builds
assert).

On Metal, buffers, textures and samplers each have their own index space, so
a storage buffer, a texture and a sampler may share a binding number, but a
combined `sampler2D` takes both the texture and the sampler slot of its
binding.

On Direct3D 11 the index spaces are the HLSL register letters instead. In
compute shaders every storage buffer, `readonly` or not, and every storage
texture shares the `u` registers, so they need distinct bindings. Textures use
`t`, samplers `s` and uniform buffers `b`; a combined `sampler2D` takes both
`tN` and `sN`. Vertex and fragment shaders are not bound this way: their
slots are assigned automatically, see
[Several bind groups](pipelines.md#several-bind-groups).

The transpiler keeps the declared bindings by setting
`CompilerMSL::Options::enable_decoration_binding = true`; without it,
spirv-cross assigns indices with its own auto-incrementing scheme, producing a
pipeline that compiles and dispatches without error but never reads or writes
the intended resources.

## Textures and storage textures

A compute shader can read textures and write storage textures, and render
passes later in the same `GpuFrame` sample what it wrote:

```cpp
GpuTextureDesc desc (256, 256, GpuTextureFormat::rgba16float);
desc.storage = true; // compute shaders may write it
auto image = GpuTexture::create (device, desc);

auto frame = GpuFrame::begin (device);

auto pass = GpuComputePass::begin (frame);
pass.setPipeline (generate);              // layout(set = 0, binding = 0, rgba16f) uniform writeonly image2D target;
pass.setStorageTexture (0, 0, image);     // mip 0 by default, or pass a GpuTextureViewDesc
pass.dispatch (256 / 8, 256 / 8, 1);

auto renderPass = target->beginRenderPass (frame);
renderPass.setTexture (0, 0, image);      // the draws see the dispatch
// ...
frame.submit();
```

- `GpuTextureDesc::storage` allocates a texture compute shaders can write. It
  requires `GpuDevice::isStorageTextureAvailable()`, and cannot be combined
  with multisampling or a depth / stencil format: `GpuTexture::create` returns
  `nullptr` in those cases.
- `GpuComputePass::setStorageTexture` binds one mip level for writing, chosen
  by the view's `baseMipLevel`. For a 2D texture it binds that level, declared
  as `image2D`; for a 2D-array or 3D texture it binds every layer or slice of
  the level, declared as `image2DArray` / `image3D` in the shader. Cube
  textures cannot be bound as storage textures on any backend: the call
  asserts and is ignored, so write cube faces with render passes.
- `GpuComputePass::setTexture` binds a whole texture for reading, with
  `texelFetch` or through a sampler. Compute passes bind no default samplers,
  so set every sampler the shader declares with `GpuComputePass::setSampler`,
  even one only used to build a `sampler2D` for `texelFetch`.

Storage textures are implemented on Metal, Direct3D 11, Vulkan and
WebGPU. Desktop OpenGL,
OpenGL ES and WebGL2 are raster-only and have no storage textures:
`isStorageTextureAvailable()` returns false there.

On Direct3D 11 a storage texture needs a format with typed UAV support:
`GpuTexture::create` returns `nullptr` for block-compressed formats, and for
`bgra8unorm` on hardware that lacks it. Writing with `imageStore` works for the
other color formats, but reading a storage image with `imageLoad` is only
guaranteed for single-channel 32-bit formats such as `r32float`.

On Vulkan:

- A pass writing a storage texture must be begun on a `GpuFrame`. A pass begun
  on a device submits its own command buffer, and its dispatch fails with an
  assertion.
- A pass begun on a device can only sample a texture that a submitted frame
  has rendered into, or one uploaded before a frame began. Textures backed by
  a Rive image or canvas can only be sampled by a pass begun on a `GpuFrame`.
  Otherwise the dispatch fails with an assertion.
- The storage format must support storage images on the device. `rgba8unorm`,
  `rgba16float` and `rgba32float` always do.
- Declare the image format in the shader (`rgba16f`, `rgba8`, ...). Writing a
  storage image without a format qualifier needs an optional device feature.
- A texture uploaded in the middle of a frame receives its data at the next
  render pass of that frame. A compute pass recorded before that render pass
  does not see the upload.

### Bindings on WebGPU

WebGPU compute pipelines lay out every resource the shader declares, whether
it uses it or not, and a dispatch must bind all of them: when one is missing,
`dispatch()` asserts and returns false. A combined `sampler2D` also gets a
companion sampler binding, which the transpiler places after the set's declared
bindings, so compute shaders meant for WebGPU should declare a separate
`texture2D` and `sampler` instead.

Each GLSL `set` becomes its own bind group, so on WebGPU resources may live in
any group. Storage textures must use a format WebGPU can write: `rgba8unorm`,
`rgba8snorm`, `rgba16float`, `rgba32float`, `r32float` or `rg32float` (not in
compatibility mode), with the matching image format qualifier in the shader
(`rgba8`, `rgba8_snorm`, `rgba16f`, `rgba32f`, `r32f`, `rg32f`). Other formats
produce an invalid texture and a validation error in the browser console.

## Example: GPU audio effect

The `GpuAudioProcessingDemo` example demonstrates real-time audio processing on
the GPU:

1. `AudioIODeviceCallback` captures live audio input
2. Audio samples are written into a preallocated `GpuBuffer` storage buffer via
   `updateBuffer()` — no GPU allocation happens on the audio thread
3. A compute shader applies gain + soft clipping
4. Processed samples are read back from the GPU
5. Results are routed to the audio output

The compute shader runs on the audio I/O thread, using a dedicated `GpuDevice`
that does not share state with the render thread. It needs no window: a Vulkan
device created without `GpuDevice::Options::vulkan` loads the system Vulkan loader
itself. An OpenGL device needs a context current on the creating thread and a
`loaderFunction`; without them it reports no compute, and the demo plays the
audio unprocessed. The tiny per-block
parameters (gain, mix) stay a uniform buffer bound via `setUniformBuffer()` —
`dispatch()` allocates a small temporary buffer for it on every call, but at
16 bytes that's negligible next to the audio-block-sized input buffer that
`updateBuffer()` now avoids reallocating.

```glsl
#version 450
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout(std430, set = 0, binding = 0) buffer InputBuf  { float inData[]; };
layout(std430, set = 0, binding = 1) buffer OutputBuf { float outData[]; };
layout(std140, set = 0, binding = 2) uniform Params   { float gain; float mix; };

void main() {
    uint i = gl_GlobalInvocationID.x;
    float s = inData[i] * gain;
    outData[i] = tanh(s) * mix + inData[i] * (1.0 - mix);
}
```
