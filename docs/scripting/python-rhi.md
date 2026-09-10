# GPU rendering from Python

The `yup_rhi` layer is exposed to Python more or less one-to-one with the C++
API described under [RHI](../graphics/rhi/index.md), so that page remains the
reference for what each call means. This page covers only what is different in
Python.

Working end-to-end scripts live in `python/demos`: `gpu_triangle.py` (fullscreen
pass), `gpu_effects.py` (2D canvas feeding a post-process pass) and
`gpu_cube.py` (vertex + index buffers, an uploaded texture, a sampler and a
depth attachment).

## Getting a device

RHI factories take a `GpuDevice`, not the `GraphicsContext` a `Graphics` hands
you, so go through `getGpuDevice()`:

```python
def paint(self, g: yup.Graphics):
    ctx = g.getGraphicsContext()
    if ctx is None or not ctx.isGpuAvailable():
        return

    device = ctx.getGpuDevice()
```

Off-screen and headless work can create a device directly. It returns `None`
rather than raising when the backend is unavailable:

```python
device = yup.GpuDevice.create(yup.GpuPlatform.Headless, yup.GpuDevice.Options())
if device is None:
    ...
```

## Failures raise, they do not return

The C++ factories that return `ResultValue<T>` raise a `RuntimeError` carrying
the error message instead:

```python
try:
    pipeline = yup.GpuPipeline.compileFromGlsl(device, vertGlsl, fragGlsl, options)
except RuntimeError as error:
    print(f"compile failed: {error}")
```

Factories that return a null pointer in C++ - `GpuTarget.create`,
`GpuTexture.create`, `GpuSampler.create`, `GpuBuffer.create` - return `None`.

`GpuShaderSource` owns its blob fields, so it is fully exposed. `code`,
`bindingMap` and `glFixup` are bytes-in, bytes-out properties accepting any
object supporting the buffer protocol, and read back as `bytes`:

```python
source = yup.GpuShaderSource()
source.language = yup.GpuShaderLanguage.glsl
source.code = vertexGlslBytes
source.bindingMap = bindingMapBlob
pipeline = yup.GpuPipeline.compile(device, source, fragmentSource, options)
```

## Descriptors own their data, and convert by value

`GpuVertexBufferLayout.attributes`, `GpuPipelineOptions.vertexBuffers` and
`GpuPipelineOptions.colorTargets` are `std::vector` members. pybind11 converts
them **by value**, so assign a whole list; mutating the list you read back does
nothing:

```python
options = yup.GpuPipelineOptions()

options.vertexBuffers = [
    yup.GpuVertexBufferLayout(
        44,                                 # stride in bytes
        yup.GpuVertexStepMode.vertex,
        [
            yup.GpuVertexAttribute(yup.GpuVertexFormat.float3, 0, 0),
            yup.GpuVertexAttribute(yup.GpuVertexFormat.float2, 36, 3),
        ],
    ),
]

target = yup.GpuColorTarget()
target.format = yup.GpuTextureFormat.rgba8unorm
options.colorTargets = [target]        # NOT options.colorTargets.append(...)
```

## Passing bytes

Everything that takes raw memory - `GpuRenderPass.setUniformBuffer`,
`GpuComputePass.setUniformBuffer`, `GpuBuffer.create`, `GpuDevice.createBuffer`
/ `updateBuffer` / `readBuffer`, `GpuTexture.upload` - accepts any object
supporting the buffer protocol (`bytes`, `bytearray`, `memoryview`, a numpy
array) and is sized in bytes:

```python
uniforms = struct.pack("<4f", angleY, angleX, aspect, 0.0)
rp.setUniformBuffer(0, 0, uniforms)
```

`readBuffer` writes into a *writable* buffer, so pass a `bytearray` or a numpy
array rather than `bytes`. `GpuTarget.readPixels()` is the exception: it takes no
argument and returns `width * height * 4` RGBA bytes, or `None` when readback is
unavailable. Only a target created by the `width`/`height` overload can be read
back - one backed by a directly allocated texture always returns `None`.

`GpuTexture.upload` takes the pixels separately from the region that places
them, since `GpuTextureDataDesc` has no exposed data pointer:

```python
region = yup.GpuTextureDataDesc()
region.width = region.height = 64
texture.upload(pixels, region)
```

## Frames and passes are context managers

`GpuFrame`, `GpuRenderPass` and `GpuComputePass` are move-only RAII types in
C++, and support `with` in Python. Leaving the block submits the frame or
finishes the pass:

```python
with yup.GpuFrame.begin(device) as frame:
    with target.beginRenderPass(frame, yup.GpuRenderOptions(True, yup.GpuColor.black())) as rp:
        rp.setPipeline(pipeline)
        rp.draw(3)
```

A pass borrows the frame it records into, so the bindings keep the frame (and
the target) alive for as long as the pass object exists.

## Compute

Compute mirrors the render path: compile a `GpuComputePipeline`, open a
`GpuComputePass`, bind storage buffers and uniforms, dispatch. `GpuDevice.isComputeAvailable()`
gates the whole thing.

```python
if not device.isComputeAvailable():
    return

pipeline = yup.GpuComputePipeline.compileFromGlsl(
    device, COMPUTE_GLSL, yup.GpuWorkgroupSize(64, 1, 1))

data = struct.pack(f"<{count}f", *values)
buffer = yup.GpuBuffer.create(device, yup.GpuBufferType.storage, data)

with yup.GpuComputePass.begin(device) as pass_:
    pass_.setPipeline(pipeline)
    pass_.setStorageBuffer(0, 0, buffer)
    pass_.setUniformBuffer(0, 1, struct.pack("<I", count))
    pass_.dispatch((count + 63) // 64)

result = bytearray(len(data))
device.readBuffer(buffer, result)
```

`readBuffer` returns `False` when no new data has arrived yet rather than on
error - on WebGPU the readback trails the GPU by a frame or two - so keep the
destination between calls and redraw its previous contents.

Both `compileFromGlsl` and the raw `compile()` overload (taking a
`GpuShaderSource` per stage) are exposed. `compileFromBundle()` still needs a
`ShaderBundle`, which has no Python binding yet.

## Controlling the offscreen frame with `GpuFrameDescriptor`

`GpuCanvas.beginDraw()` takes an optional `GpuFrameDescriptor`, giving control
over msaa/dither/loadOp/clearColor for the offscreen 2D frame it opens.
`renderTargetWidth`/`renderTargetHeight` are ignored - they are always
auto-filled from the canvas:

```python
frameDesc = yup.GpuFrameDescriptor()
frameDesc.msaaSampleCount = 4
frameDesc.ditherMode = yup.GpuDitherMode.none
frameDesc.loadOp = yup.GpuLoadOp.clear
frameDesc.clearColor = yup.GpuColor.black()

g = canvas.beginDraw(frameDesc)
```

Calling `beginDraw()` with no argument keeps the previous behaviour: clear to
transparent black, no msaa, interleaved-gradient-noise dithering.

## Bit flags

`GpuColorWriteMask` composes with `|` and `&`:

```python
target.writeMask = yup.GpuColorWriteMask.red | yup.GpuColorWriteMask.alpha
```

## Probing capabilities

Block-compressed formats and float render targets are not universally
available. Probe before creating:

```python
if device.isFormatRenderable(yup.GpuTextureFormat.rgba16float):
    ...
print(device.getMaximumSampleCount(), device.isAnisotropicFilteringAvailable())
```
