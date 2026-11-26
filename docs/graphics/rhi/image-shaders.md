# Image shaders

A `GpuImageShader` is a fragment shader drawn over a whole
[`GpuTarget`](targets.md): an effect on one or two pictures, or a picture the
shader makes alone. It takes the place of the pipeline, the binding map and the
fullscreen triangle a post-process pass otherwise needs, and
`ShaderUniforms` fills its parameters by name.

```cpp
#include <yup_rhi/yup_rhi.h>
```

Component effects, drawings and filters run their shaders with these two
classes. A [`.yui` document](../../ui/yui/shaders.md) names such a shader in a
`shader` block, with no C++.

## A first shader

A fragment stage that maps the brightness of a picture onto two colors:

```glsl
#version 450

layout(set = 0, binding = 0) uniform Parameters
{
    vec4 shadows;       // the color the dark parts of the picture take
    vec4 highlights;    // the color its bright parts take
    float amount;       // 0 leaves the picture as it is, 1 replaces its colors
} params;

layout(set = 0, binding = 1) uniform texture2D source;
layout(set = 0, binding = 2) uniform sampler sourceSampler;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

void main()
{
    vec4 pixel = texture (sampler2D (source, sourceSampler), uv);

    // The picture is multiplied by its alpha, and so is the color written
    float luma = dot (pixel.rgb, vec3 (0.2126, 0.7152, 0.0722)) / max (pixel.a, 0.001);
    vec4 toned = vec4 (mix (params.shadows.rgb, params.highlights.rgb, luma) * pixel.a, pixel.a);

    color = mix (pixel, toned, params.amount);
}
```

It is created once, from the bytes of its file, and drawn into a target as one
pass of a frame:

```cpp
// Once: needs no device, and tells what is wrong with the shader
const auto created = yup::GpuImageShader::createFromData (source.getData(), source.getSize());
if (created.failed())
    return yup::Result::fail (created.getErrorMessage());

const auto shader = created.getValue();

// At each draw
auto uniforms = shader->createUniforms();
uniforms.set ("shadows", { 0.1f, 0.0f, 0.25f, 1.0f });
uniforms.set ("highlights", { 1.0f, 0.8f, 0.5f, 1.0f });
uniforms.set ("amount", 1.0f);

auto target = yup::GpuTarget::create (device, input->getWidth(), input->getHeight());
auto frame = yup::GpuFrame::begin (device);

const auto drawn = shader->draw (device, frame, *target, uniforms, input);   // input: a GpuTexture::Ptr
frame.submit();

if (drawn.wasOk())
    g.drawTexture (target->asTexture(), bounds);
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/graphics/rhi/image-shaders_draw_light.jpg#gh-light-mode-only" alt="A canvas drawn with the 2D API, a yellow disc and the white letters 2D on dark blue, and the same picture drawn through the shader, its colors replaced by tones from dark purple to peach" width="640">
  <img class="dark-only rounded" src="../../_static/images/graphics/rhi/image-shaders_draw_dark.jpg#gh-dark-mode-only" alt="A canvas drawn with the 2D API, a yellow disc and the white letters 2D on dark blue, and the same picture drawn through the shader, its colors replaced by tones from dark purple to peach" width="640">
</p>

## The shader interface

The fragment stage is written in GLSL 450. What it declares decides what a draw
binds, and nothing has a fixed set or binding number:

| Declaration | What it is |
| --- | --- |
| A uniform block | The parameters. Optional, one at most, no larger than 64 KiB. `createUniforms()` is laid out for it. |
| `uniform texture2D` | An input: none, or up to four (`GpuImageShader::maxInputTextures`). The first input is the texture with the lowest set, then the lowest binding, whatever the order they are declared in, and `getInputTextureNames()` tells the name of each. |
| `uniform sampler` | Declared apart from the textures and combined where they are read. Filters linearly and clamps to the edge. One sampler can read every texture. |
| `layout(location = 0) in vec2 uv` | From (0, 0) at the top left of the target to (1, 1) at its bottom right, on every graphics API. Optional, and the only input. |
| `layout(location = 0) out vec4` | The pixel, and the only output. |

- **Colors are multiplied by their alpha**, in the textures read and in the
  color written. The target is cleared to transparent black and drawn over
  without blending, so the output replaces every pixel.
- Reading an input of the size of the target at `uv` reads the pixel being
  written.
- Sets 0 to 3 can be used, each block, texture and sampler taking a binding of
  its own in its set.
- A combined `uniform sampler2D`, a second uniform block, a fifth texture and a
  texture that is not a `texture2D` can't be bound: the shader is refused when
  it is created, with a message.
- So is what a draw binds nothing to, in either stage: a storage buffer, a
  storage image, a subpass input, a push constant block, an input other than
  `uv` and an output other than the pixel.

A shader that makes a picture alone declares no texture:

```glsl
#version 450

layout(set = 0, binding = 0) uniform Parameters { vec4 tint; } params;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

void main() { color = params.tint * uv.x; }
```

## Creating a shader

| Function | Takes |
| --- | --- |
| `createFromBundle (bundle)` | A compiled [`ShaderBundle`](offline-shaders.md) with a vertex and a fragment stage. |
| `createFromData (data, size)` | The bytes of a file: a `.ysl` bundle, or the GLSL source of the fragment stage. |
| `createFromGlsl (source)` | The GLSL source of the fragment stage. |

Each returns a `ResultValue`: the shader, or what prevents it.

- **No device is needed.** The shader is checked when it is created, so a tool
  without a GPU can report what is wrong with it.
- **From GLSL**, the source is paired with a stock vertex stage and compiled
  for every graphics API (GLSL, ESSL, HLSL, MSL, SPIR-V and WGSL): what
  compiles when the application runs also compiles ahead of time, and a shader
  outside what the [WebGPU target](wgsl-shaders.md) takes fails everywhere.
  This needs the shader transpiler (`YUP_ENABLE_SHADER_TRANSPILER`, the
  `glslang` and `spirv_cross` modules): without it `createFromGlsl()` does not
  exist, and `createFromData()` asks for a compiled bundle.
- **Ahead of time**, pair the fragment with `shaders/yup_GpuImageShader.vert`
  of the `yup_rhi` module:

  ```cmake
  yup_add_shader_bundle (duotone_shader
      VERT "${yup_SOURCE_DIR}/modules/yup_rhi/shaders/yup_GpuImageShader.vert"
      FRAG "${CMAKE_CURRENT_LIST_DIR}/shaders/duotone.frag")
  ```

  ```cpp
  #include <duotone_shader.h>

  auto created = yup::GpuImageShader::createFromData (yup::duotone_shader_data, yup::duotone_shader_size);
  ```

- A shader is immutable and reference counted: create it once and draw it from
  as many places as needed.
- A bundle can be built for some graphics APIs only. `canDrawWith (platform)`
  tells whether the shader has a variant of both stages for one, before any
  device draws it: `shader->canDrawWith (device->getPlatform())`.

```text
The fragment stage declares a combined sampler2D, which cannot be bound: declare a texture2D and a sampler, and combine them where the texture is read
The fragment stage declares 2 uniform blocks: an image shader has one at most
The fragment stage declares 5 textures: an image shader has 4 at most
The fragment stage declares the storage buffer Particles, which cannot be bound: an image shader takes its values through one uniform block and its textures
The fragment stage declares the input tint: an image shader has one at most, the vec2 uv at location 0
The fragment stage has to declare one output, the vec4 pixel at location 0
ShaderBundleCompiler: SPIR-V compilation failed for stage fragment: ERROR: 0:3: 'missingValue' : undeclared identifier
```

## `ShaderUniforms`

The bytes of one uniform block, with the layout of its members taken from the
reflection of the shader. Values are set by member name, and converted to the
type the shader declares, so no offset is computed by hand:

```cpp
auto uniforms = shader->createUniforms();                   // zeroed

uniforms.set ("intensity", 0.5f);                           // a number
uniforms.set ("enabled", true);                             // a boolean
uniforms.set ("tint", { 1.0f, 0.5f, 0.25f, 1.0f });         // a list in braces
uniforms.set ("points", points);                            // a std::vector<float>, a std::array, a Span
```

| Method | What it does |
| --- | --- |
| `set (name, values)` | Sets the scalars of a member. Returns false, having written nothing, for a name the block does not have. |
| `hasMember (name)` | Whether the block has a member `set()` can write. |
| `getMemberType (name)` | The type of its scalars: `float32`, `int32`, `uint32` or `boolean`. |
| `getNumComponents (name)` | How many scalars it holds: 3 for a `vec3`, 16 for a `mat4`, 8 for a `vec2[4]`. |
| `getData()`, `getSizeInBytes()` | The bytes, for `GpuRenderPass::setUniformBuffer()`. |

- A member is a scalar, a vector, a matrix of up to 4 columns and rows, or a
  one-dimensional array of those, made of `float`, `int`, `uint` or `bool`.
  Structures, arrays of arrays and `double` keep their room in the block,
  zeroed, and can't be set.
- Values come in the order GLSL constructors take them: the components, then
  the columns of a matrix, then the elements of an array. Fewer values leave
  the rest of the member as it was, and the values past its end are ignored.
- Integers are rounded and kept inside their range, and a `bool` is true when
  not zero. Values go through `float`, so integers above 16777216 are not
  exact.
- A caller can set a member a shader may or may not declare: `set ("time",
  seconds)` is harmless on a shader without a `time`.
- Names are the ones of the members, not of the block nor of its instance:
  `params.tint` is `"tint"`.
- It is a plain value, with no GPU object behind it: it can be copied, kept
  from one draw to the next and compared.
- It knows nothing of what the values mean. A color is 3 or 4 numbers, which
  the caller multiplies by its alpha when the shader expects that.

`ShaderUniforms` is not tied to image shaders: it is made from any reflected
uniform block (`ShaderUniforms uniforms (reflection.uniformBuffers.front());`),
and a [material shader](../../3d/custom-materials.md) uses it for its own.

## Drawing

```cpp
Result draw (const GpuDevice::Ptr& device,
             GpuFrame& frame,
             GpuTarget& target,
             const ShaderUniforms& uniforms,
             const GpuTexture::Ptr& firstInput = nullptr,
             const GpuTexture::Ptr& secondInput = nullptr) const;

Result draw (const GpuDevice::Ptr& device,
             GpuFrame& frame,
             GpuTarget& target,
             const ShaderUniforms& uniforms,
             Span<const GpuTexture::Ptr> inputs) const;
```

`draw()` records one render pass into the frame: it clears the target, binds
the uniforms and the inputs the shader declares, and draws one triangle
covering the target. The frame is left open, so several shaders can be chained
in one frame before it is submitted:

```cpp
auto frame = yup::GpuFrame::begin (device);

if (blur->draw (device, frame, *blurred, blurUniforms, input).wasOk())
    mix->draw (device, frame, *result, mixUniforms, input, blurred->asTexture());

frame.submit();
```

- The device is the one the frame was begun on, and the one the target and the
  inputs were created with.
- The inputs a shader does not declare are ignored, so a caller can always pass
  what it has. An input can't be the texture of the target: the draw fails
  rather than read what it writes.
- A shader reading more than two inputs takes them as a span, in the order of
  `getInputTextureNames()`.
- The uniforms have to come from `createUniforms()` of the same shader: the
  draw fails with those of another block, of the same size or not
  (`ShaderUniforms::hasSameLayoutAs()` tells them apart).
- The target has a color format and is not multisampled.
- It returns a failed `Result`, having recorded nothing, when the device has no
  GPU or was lost, an input the shader declares is missing or is the texture of
  the target, the uniforms are not the ones of the shader, or the pipeline
  can't be compiled for the device.
- Any thread drawing with the device can call it, also while other threads
  draw the same shader.

## Pipelines belong to the device

A shader holds no device and no pipeline. The first time it is drawn with a
device into a target of a format, that device compiles the pipeline and keeps
it for the next draws:

- the windows of an application, each with a device of its own, draw the same
  shader object, and compile once each;
- a pipeline that can't be compiled is not tried again by every draw: the
  failure is kept and returned;
- a device releases the pipelines it compiled when it is destroyed, or once
  the shader is gone, so a shader can outlive any window and never keeps one
  alive.

A `GpuTarget` and a `GpuTexture` do hold their device: what keeps a target for
its shader keeps that device alive as long as it lives.

## Where it is used

| Class | What it runs |
| --- | --- |
| [`ShaderEffect`](../../ui/component-effects.md#shader-effects) | A shader on the picture of a component. |
| `DrawingStyle` | The shaders a [drawing](../../ui/yui/drawing.md) paints with `use`. |
| `FilterGraph` | Its `shader` primitive, among the other primitives of a filter. |

## See also

- [Offscreen targets & canvases](targets.md) - `GpuTarget` and `GpuCanvas`.
- [Pipelines & shaders](pipelines.md) - a pipeline of your own, for what an
  image shader does not cover: geometry, blending, depth.
- [Offline shader compilation](offline-shaders.md) - building `.ysl` bundles.
- [GLSL on the WebGPU target](wgsl-shaders.md) - the GLSL every target takes.
- [Shaders in `.yui`](../../ui/yui/shaders.md) - image shaders declared in a
  document.
