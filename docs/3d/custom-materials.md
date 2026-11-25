# Custom materials

A `MaterialShader` is a material written as a GLSL function. The meshes under
a `MaterialShaderNode` are colored by that function instead of the lighting of
their `Material`, while everything around the color stays the renderer's:
depth, culling, alpha modes, the shadows the meshes cast, exposure and tone
mapping, on both [tiers](platform-tiers.md).

```cpp
#include <yup_renderer/yup_renderer.h>
```

In a [`.yui` document](../ui/yui/scenes.md#custom-materials) a material is a
`shader` block named by a `Material3D`, and its inputs are properties.

## A first material

The material is one GLSL file, with no `#version` and no `main()`:

```glsl
YUP_MATERIAL_PARAMS
{
    vec4 tint;          // multiplies the base color of the material, linear
    vec4 light;         // where the light of the material comes from: a direction in the world
    float steps;        // how many flat tones the surface is shaded in
} params;

vec4 yupMaterial (YupSurface surface)
{
    // Nothing lights the surface: the function shades it, here by how much it faces a direction of its own
    float facing = 0.5 + 0.5 * dot (surface.normal, normalize (params.light.xyz));

    // Flat tones: the first where the surface faces the light, each of the others half as bright as the one before
    float level = clamp (ceil (facing * params.steps), 1.0, params.steps);
    float tone = exp2 (level - params.steps);

    return vec4 (surface.baseColor.rgb * params.tint.rgb * tone, surface.baseColor.a);
}
```

A node gives it to an entity, with its own values of the uniform block:

```cpp
auto created = yup::MaterialShader::createFromSource (file.loadFileAsString());
if (created.failed())
{
    YUP_DBG (created.getErrorMessage());
    return;
}

yup::Entity knot = model.instantiate (*scene);

auto& toon = knot.attach<yup::MaterialShaderNode> (created.getValue());
toon.getUniforms().set ("tint", { 1.0f, 0.5f, 0.12f, 1.0f });
toon.getUniforms().set ("light", { -1.0f, 0.6f, 0.2f, 0.0f });
toon.getUniforms().set ("steps", 4.0f);
```

<p align="center">
  <img class="light-only rounded" src="../_static/images/3d/custom-materials_toon_light.jpg#gh-light-mode-only" alt="The same torus knot twice: lit by the renderer with its own gray material, and shaded by a material shader in four flat tones of gold, the lightest where it faces the upper left and the darkest away from it" width="640">
  <img class="dark-only rounded" src="../_static/images/3d/custom-materials_toon_dark.jpg#gh-dark-mode-only" alt="The same torus knot twice: lit by the renderer with its own gray material, and shaded by a material shader in four flat tones of gold, the lightest where it faces the upper left and the darkest away from it" width="640">
</p>

## The material function

`vec4 yupMaterial (YupSurface surface)` is required. It returns the color of
the surface: **linear, with an alpha that is not multiplied into it**. The
renderer then treats that color as the one its own shading computes: it is
exposed, tone mapped and encoded as the frame needs.

The `YupSurface` describes the fragment, in world space:

| Field | What it holds |
| --- | --- |
| `vec3 position` | The position of the fragment. |
| `vec3 normal` | The unit shading normal, after the normal texture of the material. It faces the viewer on the back faces of a double sided material. |
| `vec3 view` | The unit vector from the fragment towards the camera. |
| `vec2 uv` | The texture coordinates of the mesh. |
| `vec4 color` | The vertex color of the mesh. |
| `vec4 baseColor` | The base color of the material: its factor times its texture and the vertex color. Linear, within 0 and 1, its alpha not multiplied in. |
| `vec3 emissive` | The emission of the material: its factor times its texture and its strength. Linear. |
| `float metallic` | The metalness of the material, its factor times its texture, from 0 to 1. |
| `float roughness` | Its perceptual roughness, from 0.04 to 1. |
| `float occlusion` | Its ambient occlusion, 1 where nothing is occluded. |

So the factors and the five textures of the `Material` of the mesh stay
usable, as inputs of the function.

`YUP_MATERIAL_PARAMS` opens the uniform block of the material, which is
optional. Its instance name is free, and its members are set by name from the
node: scalars, vectors and matrices of `float`, `int`, `uint` and `bool`, and
arrays of those, as [`ShaderUniforms`](../graphics/rhi/image-shaders.md#shaderuniforms)
holds them.

A material can also call `yupSrgbToLinear (vec3)`, `yupLinearToSrgb (vec3)`
and `yupLuma (vec3)`, read `yupPi`, and use the GLSL built-ins, `gl_FragCoord`
and the derivatives included.

- A material declares **no texture, sampler or second uniform block** of its
  own, and has no vertex stage. It is refused when it is created otherwise.
- There is no time and no resolution: give the function what it needs in its
  block, and set it from the node.
- The names starting with `yup`, `bsdf`, `material`, `view`, `forward`, `v_`,
  `u_` and `s_` are the renderer's, and so are `main`, `lighting`,
  `linearClamp`, `pointClamp`, `fragColor`, `outVelocity`, `MaterialParams`,
  `MaterialSurface`, `BsdfSurface` and `BsdfLayers`: call your own functions
  and variables something else.
- A material is compiled for every graphics API at once, so it has to stay
  within the GLSL the WebGPU target takes, whatever the application runs on.
  See [GLSL on the WebGPU target](../graphics/rhi/wgsl-shaders.md).

## What the material of the mesh still decides

The `Material` of a mesh is found as without the shader: the one of its
primitive, or the one of a `MaterialNode` above it. It still decides:

| What | How |
| --- | --- |
| The alpha returned | Ignored for `opaque`, compared with the alpha cutoff for `mask`, blended for `blend`. |
| The culling | By its double sided flag. |
| The shadows the mesh casts | Those of the material: its shape, and its cut-out for a masked one. |
| What reflection probes and irradiance volumes capture | The mesh with its material, not with the function. |

A masked material cuts its color out by the alpha the function returns, and its
shadow by the alpha of its base color: give the material a base color texture
with the same cut-out when the shadow matters.

## The node

```cpp
auto& hologram = entity.attach<yup::MaterialShaderNode> (shader);
hologram.getUniforms().set ("rim", { 0.0f, 1.0f, 1.0f, 1.0f });
hologram.getUniforms().set ("time", seconds);

view.repaint();
```

- The shader colors the meshes of the entity and of everything under it. The
  nearest `MaterialShaderNode` up the tree wins, as with `MaterialNode`, and a
  node without a shader changes nothing.
- A shader is created once and shared: each node holds its own values of the
  uniform block, so two nodes sharing a shader can differ.
- Members are named as the block declares them, not `params.rim`. `set()`
  returns false, having written nothing, for a name the block does not have.
- Changing a value restarts what the full tier refines over frames, by itself.
  A value set on every frame, a time for example, keeps the view repainting,
  as any animation does.
- `setShader()` gives the node the zeroed values of the new shader.

## Creating a shader

A shader holds the function compiled for each tier of the renderer, as two
shader bundles: a raster variant and a full variant.

| Function | Takes |
| --- | --- |
| `createFromSource (source)` | The GLSL text of the material, compiled when the application runs. |
| `createFromData (raster, rasterSize, full, fullSize)` | The bytes of the two compiled variants. |
| `createFromBundles (raster, full)` | The two variants, as `ShaderBundle::Ptr`. |
| `createDefault()` | Nothing: the stock material, the base color of the `Material`, unlit. |

Each returns the shader, or what prevents it, without needing a device: a tool
without a GPU can tell what is wrong with a material.

- Either variant can be missing. The shader then colors the meshes on the tier
  it has, and leaves them to their `Material` on the other.
- `createFromSource()` needs the shader transpiler
  (`YUP_ENABLE_SHADER_TRANSPILER`, the `glslang` and `spirv_cross` modules),
  and reads the template the function is compiled in from the `shaders`
  folder of `yup_renderer`, where the binary was built from. It is for
  development, hot reload and tools. Its errors number the lines of the
  material from 1, under the name `material`:

  ```text
  ERROR: material:3: 'missingValue' : undeclared identifier
  ```

- An application that ships creates its shaders from compiled variants. One
  that compiles materials where the sources of YUP are not, in a browser or on
  a phone, bundles `yup_MaterialShader.vert`, `yup_MaterialShader.frag` and the
  `lib` folder of `modules/yup_renderer/shaders` with its resources, and says
  where they are before it creates a material:

  ```cpp
  yup::MaterialShader::setSourceDirectory (bundle.getChildFile ("yup_renderer/shaders"));
  ```

### Compiling ahead of time

The two variants are the files `yup_MaterialShader.vert` and
`yup_MaterialShader.frag` of `modules/yup_renderer/shaders`, compiled around
the material with [`yup_add_shader_bundle`](../graphics/rhi/offline-shaders.md).
The fragment stage includes the material under the name
`yup_MaterialSource.glsl` when it is compiled with `-DYUP_3D_MATERIAL_SOURCE`,
and `-DYUP_3D_TIER_FULL` makes the full variant:

```cmake
# yup_SOURCE_DIR is where FetchContent_MakeAvailable (yup) put the sources of YUP
set (renderer_shaders "${yup_SOURCE_DIR}/modules/yup_renderer/shaders")
set (staged "${CMAKE_CURRENT_BINARY_DIR}/MaterialShaders/toon")

# The template includes the material under a fixed name
configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/fx/toon.glsl" "${staged}/yup_MaterialSource.glsl" COPYONLY)

file (GLOB library_files "${renderer_shaders}/lib/*.glsl")

yup_add_shader_bundle (toon_raster
    VERT "${renderer_shaders}/yup_MaterialShader.vert"
    FRAG "${renderer_shaders}/yup_MaterialShader.frag"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/fx/toon.glsl" ${library_files}
    OPTIONS -DYUP_3D_MATERIAL_SOURCE "-I${renderer_shaders}" "-I${staged}")

yup_add_shader_bundle (toon_full
    VERT "${renderer_shaders}/yup_MaterialShader.vert"
    FRAG "${renderer_shaders}/yup_MaterialShader.frag"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/fx/toon.glsl" ${library_files}
    OPTIONS -DYUP_3D_MATERIAL_SOURCE -DYUP_3D_TIER_FULL "-I${renderer_shaders}" "-I${staged}")
```

```cpp
#include <toon_raster.h>
#include <toon_full.h>

auto created = yup::MaterialShader::createFromData (yup::toon_raster_data, yup::toon_raster_size,
                                                    yup::toon_full_data, yup::toon_full_size);
```

Keep `-I${renderer_shaders}` first, so that a file of the application can't
take the place of one of the renderer. The materials of a `.yui` document need
none of this: [`yup_add_yui_resources`](../build-system/cmake-api.md#yup_add_yui_resources)
compiles the files of its `MATERIAL_SHADERS` argument both ways.

## What a custom material does not get

- **It is unlit.** No light, shadow received, ambient light, environment,
  reflection probe or irradiance volume reaches the function, and nothing is
  added to what it returns. A material shaded by a light takes its direction
  in its block, as the first one of this page does. A material that glows
  lights nothing around it: put a light beside it.
- **Full tier**: its surfaces are not in the G-buffer, so ambient occlusion,
  screen-space reflections, screen-space global illumination and contact
  shadows neither see nor shade them, and the G-buffer debug views do not show
  them.
- **Raster tier**: they occlude the ambient occlusion of their neighbors, and
  receive none.
- **Fog and the post effects still apply.** The world fog, the temporal
  anti-aliasing, the depth of field and motion blur treat an opaque custom
  surface as any opaque surface.
- No clearcoat nor sheen values are given to the function.

A shader whose pipelines can't be compiled for a device leaves its meshes to
their `Material`.

## See also

- [Showing a scene](showing-a-scene.md) - scenes, materials and their
  features.
- [Image shaders](../graphics/rhi/image-shaders.md) - `ShaderUniforms`, and
  shaders drawn over a 2D target.
- [Offline shader compilation](../graphics/rhi/offline-shaders.md) -
  `yup_add_shader_bundle` and the bundler.
- [3D scenes in `.yui`](../ui/yui/scenes.md#custom-materials) -
  `Material3D { shader: Name; }`.
