# Compute Shaders

The `yup_rhi` module supports GPU compute shaders through `GpuComputePipeline`
and `GpuComputePass`. Compute shaders run general-purpose GPU work — audio DSP,
physics simulation, particle systems, image processing — without any window,
framebuffer, or graphics pipeline.

## Availability

Compute shaders are available on backends that expose
`GpuDevice::isComputeAvailable() == true`: **Metal**, **Direct3D 11**,
**WebGPU** (Dawn and Emscripten), and **OpenGL 4.3+** / **OpenGL ES 3.1+**.

Compute is **not** available on the Headless backend.

## Architecture

Compute shaders bypass Rive's ore layer (`rive::ore`) entirely. The ore layer
does not yet expose compute dispatch, so `GpuComputePipeline` and
`GpuComputePass` go directly to the backend-native API:

| Backend      | Pipeline compilation                   | Dispatch                           |
| ------------ | -------------------------------------- | ---------------------------------- |
| Metal        | `MTLComputePipelineState`              | `dispatchThreadgroups:`            |
| Direct3D 11  | `ID3D11ComputeShader`                  | `ID3D11DeviceContext::Dispatch()`  |
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
source.code = mslSource;
source.codeSize = mslLength;

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
allocated directly on the native API (Metal `MTLBuffer`, D3D11 structured
buffer + UAV, WebGPU `Storage` buffer, GL `GL_SHADER_STORAGE_BUFFER`).

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
compute-capable backends (Metal, D3D11, WebGPU/Dawn, OpenGL).

### Buffer binding indices on Metal

`GpuComputePass`'s native dispatch binds Metal buffer arguments directly as
`group*16 + binding` (see `native/yup_GpuComputePass_metal.cpp`) — there is no
reflection layer translating GLSL `(set, binding)` pairs to the compiled
function's actual `[[buffer(N)]]` indices, unlike the render pipeline path.
This requires the transpiled MSL's argument indices to exactly match the
GLSL/SPIR-V declared `binding=N` values. The transpiler enforces this by
setting `CompilerMSL::Options::enable_decoration_binding = true`; without it,
spirv-cross assigns MSL buffer indices via its own auto-incrementing scheme,
which can silently diverge from the declared bindings for any shader with
more than one buffer resource, producing a pipeline that compiles and
dispatches without error but never actually reads/writes the intended data.

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
that does not share state with the render thread. The tiny per-block
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
