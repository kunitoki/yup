# Shaders

A `shader` block gives a GLSL file a name and declares its inputs as typed
properties. The fragment stage then runs on the GPU wherever the document
names it:

- as the `effect:` of a component, whose picture goes through the shader;
- painted by a `use` in a [drawing](drawing.md), for a picture the shader
  makes alone;
- as a primitive of the `filter` of a drawing layer.

A fourth kind of block names a *material* instead, a function coloring the
meshes of a [3D scene](scenes.md#custom-materials).

Nothing is registered or written in C++ for any of them: the block is the only
new syntax, and what it declares is set by rules, bindings and transitions like
every other property.

## A first shader

```yui
shader Crt {
    fragment: "fx/crt.frag";
    property intensity: number = 0.5;
    property tint: color = palette.accent;
}

drawing Panel {
    rect { bounds: bounds; cornerRadius: 10; fill: palette.surface; }
}

style {
    .panel   { drawing: Panel; }
    .display { effect: Crt; intensity: 0.8; }
}

component Meter : Column {
    class: panel; padding: 16; gap: 10;

    Label      { text: "LEVEL"; preferredHeight: 20; }
    Slider     { value: 0.7; flexGrow: 1; }
    TextButton { text: "Play"; flexGrow: 1; }
}

Row {
    gap: 24;

    Meter { flexGrow: 1; }
    Meter { class: display; flexGrow: 1; }
}
```

`fx/crt.frag`, next to the `.yui` file:

```glsl
#version 450

layout(set = 0, binding = 0) uniform Parameters
{
    vec4 tint;          // property tint: color, as it is written: sRGB, not multiplied by its alpha
    vec4 resolution;    // filled in: the width and the height of the picture in pixels, then 1 / width and 1 / height
    float intensity;    // property intensity: number
    float scale;        // filled in: the pixels a unit of the component takes, 2 on a dense display
    float time;         // filled in: seconds, which has the component painted again at every frame
} params;

layout(set = 0, binding = 1) uniform texture2D source;         // what the component painted
layout(set = 0, binding = 2) uniform sampler sourceSampler;

layout(location = 0) in vec2 uv;                               // (0, 0) at the top left, (1, 1) at the bottom right
layout(location = 0) out vec4 color;

void main()
{
    vec4 pixel = texture (sampler2D (source, sourceSampler), uv);                     // multiplied by its alpha

    // A line a fifth darker every four units of the component, whatever the density of the display, crawling down
    float row = uv.y * params.resolution.y / params.scale;
    float lines = 0.9 + 0.1 * sin (row * 1.5708 - params.time * 4.0);

    // Only the corners get darker: the middle of an edge is 0.5 away from the middle of the picture, a corner 0.7
    float corners = 1.0 - 0.5 * smoothstep (0.5, 0.7, length (uv - vec2 (0.5)));

    // The tint is a glow added where the picture is, and no brighter than its alpha lets it be
    vec3 glow = min (pixel.rgb + params.tint.rgb * pixel.a * 0.25, vec3 (pixel.a));

    color = mix (pixel, vec4 (glow * lines * corners, pixel.a), params.intensity);    // still multiplied by its alpha
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/shaders_effect_light.jpg#gh-light-mode-only" alt="Two panels holding a label, a slider and a button: the first as it is painted, the second through a shader that adds a glow of the accent color to it, draws faint scan lines over it and darkens its corners" width="640">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/shaders_effect_dark.jpg#gh-dark-mode-only" alt="Two panels holding a label, a slider and a button: the first as it is painted, the second through a shader that adds a glow of the accent color to it, draws faint scan lines over it and darkens its corners" width="640">
</p>

The block names the file and declares two inputs. The rule selects the shader
with `effect:` and gives `intensity` a value, as it would for a property of the
component. The fragment reads its parameters from one uniform block: `tint` and
`intensity` are the declared properties, found by their name, and `resolution`,
`scale` and `time` are members every shader can ask for.

## The `shader` block

```yui
shader Name {
    fragment: "path";                   // or material: "path";
    property name: type = default;      // as many as the shader has inputs
}
```

| Member | What it is |
| --- | --- |
| `fragment` | The path of a GLSL 450 fragment stage, or of its compiled bundle: an *image shader*, for an effect, a drawing or a filter. |
| `material` | The path of a GLSL material function, for the meshes of a 3D scene. See [Material shaders](#material-shaders). |
| `property name: type = default;` | An input: the member of the uniform block that has this name. The default is optional. |

- It is a top-level block, as `drawing` and `palette` are, and its members can
  come in any order.
- A block has exactly one of `fragment` and `material`. There is no vertex
  stage to write.
- The path is a string. It is relative to the folder of the main file of the
  document, also when the block is written in a file that one imports, and is
  read through the document: from that folder, from the resources embedded in
  the binary, or from the unsaved text of an editor. See
  [Loading and shipping](#loading-and-shipping).
- Shaders and drawings share their names: a shader can't be called as a drawing
  of the document, nor as another shader.
- An input is a `number`, an `integer`, a `boolean`, a `color`, a `point`, a
  `rectangle` or a `number[]`. Its default is an expression, evaluated for the
  component using the shader, so `palette.accent` follows the palette of that
  component.
- A property name has one type in a document: rules and elements type a value
  by the first declaration of that name, whether a drawing, a shader, the
  `tokens` or a rule makes it. Give an input shared by two shaders the same
  type in both.

## The fragment stage

An image shader is one fragment stage written in GLSL 450. What it declares
decides what it is given, and nothing has a fixed set or binding number:

| Declaration | What it is |
| --- | --- |
| A uniform block | The parameters. One at most, with any name: its members are matched with the properties by name (`params.tint` is `tint`). |
| `uniform texture2D` | A picture the shader reads: none, one or two. The first one is the texture with the lowest set, then the lowest binding. One named as a `number[]` property is no picture but a [series](#series). Four textures at most. |
| `uniform sampler` | How the pictures are read: filtered linearly and clamped to their edges. One sampler can read both pictures. |
| `layout(location = 0) in vec2 uv` | Where the pixel is: (0, 0) at the top left of the picture and (1, 1) at its bottom right. Optional. |
| `layout(location = 0) out vec4` | The color of the pixel. |

- **Colors are multiplied by their alpha**, in the pictures read and in the
  color written. A shader that fades a pixel multiplies all four of its
  components.
- A texture and its sampler are declared apart and combined where the texture
  is read, as in `texture (sampler2D (source, sourceSampler), uv)`. A combined
  `uniform sampler2D` can't be bound and is refused.
- Sets 0 to 3 can be used, each block, texture and sampler having a binding of
  its own in its set.
- The shader is compiled for every graphics API at once, so it has to stay
  within the GLSL the WebGPU target takes, whatever the application runs on.
  See [GLSL on the WebGPU target](../../graphics/rhi/wgsl-shaders.md).

How many pictures a shader reads decides where it can be used. The textures a
series fills don't count:

| Textures | As an `effect:` | Painted by a `use` | In a `filter` |
| --- | --- | --- | --- |
| None | Fills the component, whose own painting is not shown. | Paints its picture. | Makes a picture alone, as a `flood` does. |
| One | Reads what the component painted. | Not allowed. | Filters `in`. |
| Two | Not allowed. | Not allowed. | Combines `in` and `in2`. |

## Properties and uniforms

Each declared property fills the member of the uniform block that has its name:

| Property type | What is written | GLSL member |
| --- | --- | --- |
| `number` | One number. | `float` (an `int` or a `uint` rounds it) |
| `integer` | One whole number. | `int`, `uint` or `float` |
| `boolean` | 1 or 0. | `bool`, `float` or `int` |
| `color` | Red, green, blue and alpha, from 0 to 1. | `vec4`, or `vec3` without the alpha |
| `point` | x and y. | `vec2` |
| `rectangle` | x, y, width and height. | `vec4` |
| `number[]` | As many numbers as it holds. | `float name[N]`, or a vector |

- A `color` is written as the document has it: sRGB, **not** multiplied by its
  alpha. A shader that mixes it with a picture multiplies it first.
- The member has to be large enough for the type: a `color` needs three
  components, a `point` two. A property without a member of its name, or with
  one too small, is reported and the shader is not used.
- A property with neither a value nor a default is zero.

Three members are filled in when the block has them, without being declared:

| Member | As an `effect:` | Painted by a `use` | In a `filter` |
| --- | --- | --- | --- |
| `resolution` | The size of the picture of the component in pixels, its paint margin included. | The size of the `bounds` of the `use` in pixels. | The size of the `bounds` of the filter in pixels of the screen. |
| `scale` | The pixels a unit of the component takes: 2 on a dense display. | The same, with the transforms around the `use`. | The same, where the layer is drawn. |
| `time` | The seconds the component has been showing for. | The seconds the drawing has been showing its shaders for. | Not filled. |

- `resolution` is a `vec2`, or a `vec4` that also gets `1 / width` and
  `1 / height`, the size of one pixel in `uv`.
- A shader whose block has `time` is animated: its component is painted again
  at every frame of its window while it is showing. The time starts at 0 and
  starts again every hour, so an animation whose period divides 3600 seconds
  loops without a jump. A shader without `time` costs nothing between paints.
- Declaring a property of one of these names takes the member back:
  `property time: number;` lets the document set the time itself, and nothing
  asks for frames anymore.

## Series

A `number[]` property fills an array of the uniform block, which is fine for
a few numbers. A block has a fixed size though, and a shader reading an array
between two numbers has to interpolate them itself. So a `number[]` property
also fills a **texture**, when the fragment stage declares a `texture2D` of
its name instead of a member of the block:

```yui
shader Scope {
    fragment: "fx/scope.frag";
    property levels: number[];
    property tint: color = palette.accent;
}

drawing Spectrum {
    use Scope { levels: levels; }
}

style {
    SpectrumAnalyzer { drawing: Spectrum; }
}
```

```glsl
layout(set = 0, binding = 0) uniform Parameters { vec4 tint; } params;
layout(set = 0, binding = 1) uniform texture2D levels;
layout(set = 0, binding = 2) uniform sampler levelsSampler;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

void main()
{
    float level = texture (sampler2D (levels, levelsSampler), vec2 (uv.x, 0.5)).r;
    float lit = step (1.0 - uv.y, level);

    color = vec4 (params.tint.rgb, 1.0) * lit;
}
```

- The texture is one pixel high and holds one number per pixel, in its red
  channel. The shader reads it along `x`, from 0 at the first number to 1 at
  the last, and the sampler interpolates between two numbers.
- It takes any length, and the numbers are kept as 16 bit floats: exact to
  about three digits, which is what a display shows.
- It is written again only when the series changes. A series a component
  declares with a revision, as the `levels` of a `SpectrumAnalyzer`, tells so
  with it and is not even compared. Any other array is compared with the
  numbers the texture holds.
- A series is read where the component keeps it, as in a
  [drawing](drawing.md#data-series): the numbers go from the component to the
  texture with no conversion in between.
- The same goes for a shader set as an `effect:`, which then reads both the
  picture of the component and the series.
- A `filter` gives its shaders pictures only: one sampling a series is
  reported there.

## As the effect of a component

`effect: Name;` in a rule draws the matched components through the shader:

```yui
style {
    .display       { effect: Crt; intensity: 0.4; transition: intensity 200ms; }
    .display:hover { intensity: 0.9; }
}

Column {
    Component { class: display; preferredHeight: 160; }
    Slider    { id: amount; }
    Component { class: display; intensity: amount.value; }     // a binding drives the uniform
}
```

- The inputs are **properties of the component**. A rule sets them, an element
  sets them or binds them, and a `transition:` blends them, the declared
  default being the value of a component nothing else gives one.
- A default belongs to the component the effect is on: a value given to a
  parent does not replace it. Set an input in a rule matching the component,
  or on the component itself.
- An input called as a property of the component type reads that property:
  `property opacity: number;` gives the shader the opacity of the component,
  and on a `Slider` `property value: number;` its value.
- The picture is the component with its children and its
  [paint margin](styles.md#shadows-glows-filters-effects-and-cursors). A shader
  that draws outside the bounds, as a glow does, needs that margin.
- `effect:` takes one shader, the name of an effect
  [registered in C++](../component-effects.md#using-effects-from-yui-stylesheets), or
  `none`. A shader of the document wins over a registered effect of the same
  name. It composes with `filter:` as any effect does: the effect first, then
  the filter.
- The pointer is not moved with the pixels: a shader that shifts or bends the
  picture leaves the components where they are for the mouse.
- Where the shader can't run, without a GPU for example, the component is drawn
  as it is.

## Painted by a drawing

A `use` naming a shader paints its picture among the shapes of a drawing. The
shader makes the picture alone, so it declares no texture:

```yui
shader Rings {
    fragment: "fx/rings.frag";
    property count: number = 4;
    property tint: color = palette.accent;
}

drawing Sonar {
    property level: number = 0.5;

    rect { bounds: bounds; cornerRadius: 10; fill: palette.surface; }
    use Rings { bounds: bounds.reduced (8); count: 2 + level * 6; }
    circle { center: bounds.center; radius: 5; fill: palette.text; }
}

style {
    .sonar { drawing: Sonar; }
}

Row {
    gap: 16;

    Component { class: sonar; level: 0; flexGrow: 1; }
    Component { class: sonar; level: 0.5; flexGrow: 1; }
    Component { class: sonar; level: 1; flexGrow: 1; }
}
```

`fx/rings.frag`:

```glsl
#version 450

layout(set = 0, binding = 0) uniform Parameters
{
    vec4 tint;          // property tint: color, as it is written: sRGB, not multiplied by its alpha
    vec4 resolution;    // filled in: the width and the height of the picture in pixels, then 1 / width and 1 / height
    float count;        // property count: number
    float time;         // filled in: seconds, which has the component painted again at every frame
} params;

layout(location = 0) in vec2 uv;                               // (0, 0) at the top left, (1, 1) at the bottom right
layout(location = 0) out vec4 color;

void main()
{
    // As wide as high whatever the bounds are: width / height is resolution.x * resolution.w
    vec2 p = (uv - vec2 (0.5)) * vec2 (params.resolution.x * params.resolution.w, 1.0);
    float radius = length (p) * 2.0;                                            // 1 at the top and at the bottom

    // Rings leaving the center, fading out as they go
    float rings = 0.5 + 0.5 * cos (radius * params.count * 6.2832 - params.time * 3.0);
    float alpha = params.tint.a * rings * clamp (1.0 - radius, 0.0, 1.0);

    color = vec4 (params.tint.rgb * alpha, alpha);                              // multiplied by its alpha, as every picture is
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/shaders_drawing_light.jpg#gh-light-mode-only" alt="Three panels painted by one drawing, each with rings drawn by a shader around a dot: two rings, five and eight, following a property of the component" width="640">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/shaders_drawing_dark.jpg#gh-dark-mode-only" alt="Three panels painted by one drawing, each with rings drawn by a shader around a dot: two rings, five and eight, following a property of the component" width="640">
</p>

- The inputs are **expressions of the drawing**, written on the `use` like the
  properties of any element: they read the properties of the drawing, its
  `let`s, `bounds`, the palette and the component. An input the `use` does not
  set has the default of the block.
- The `use` is placed as an `image` is: `bounds` (the bounds around it when
  omitted), `transform`, `visible`, `opacity`, `blend`, `clip` and `clipPath`.
  It has no paint and no children.
- In an expression of the `use`, `bounds` is the bounds around it, not its own:
  to give the shader its own rectangle, name it with a `let`.
- The picture has the pixels of the `bounds` on the screen. It is drawn once,
  and again only when a value, the size or the display changes, unless the
  shader has `time`.
- Where the picture is transparent, what the drawing painted before shows.
- A shader can't be painted inside a `repeat`: write one `use` for each place,
  or draw the repetition in the shader.

A drawing that paints a shader is selected as any drawing is, by a rule
(`drawing: Sonar;`) or on an element (`Component { drawing: Sonar; }`).

## In a filter

Inside the [`filter`](drawing.md#filters) of a layer a `use` is a primitive: the
shader reads the picture of the layer, or the result of another primitive, and
its result goes on to the next one.

```yui
shader Crt {
    fragment: "fx/crt.frag";            // reads one texture
    property intensity: number = 0.5;
}

shader Mix {
    fragment: "fx/mix.frag";            // reads two
    property amount: number = 1;
}

drawing Screen {
    property glow: number = 0.5;

    layer {
        filter {
            bounds: bounds;

            blur { deviation: 4; result: soft; }
            use Mix { in: source; in2: soft; amount: glow; result: lit; }
            use Crt { in: lit; intensity: 0.8; }
        }

        circle { center: bounds.center; radius: 20; fill: palette.accent; }
    }
}
```

`fx/mix.frag`, which adds a picture over another:

```glsl
#version 450

layout(set = 0, binding = 0) uniform Parameters
{
    float amount;       // property amount: number
} params;

layout(set = 0, binding = 1) uniform texture2D base;          // the first texture reads `in`
layout(set = 0, binding = 2) uniform texture2D overlay;       // the second one `in2`
layout(set = 0, binding = 3) uniform sampler linearClamp;     // one sampler can read both

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

void main()
{
    vec4 below = texture (sampler2D (base, linearClamp), uv);
    vec4 above = texture (sampler2D (overlay, linearClamp), uv);

    color = below + above * params.amount;    // multiplied by their alpha in, and out
}
```

- `in`, `in2` and `result` mean what they mean for every primitive: `in` names
  `source`, `sourceAlpha` or an earlier `result`, and is the result of the
  primitive before when omitted. `in2` is only for a shader reading two
  pictures.
- The inputs are expressions, as for a painted `use`. Nothing else is taken:
  `bounds`, `opacity` and the like belong to the `filter` and to its `layer`.
- `resolution` and `scale` are in pixels of the screen, inside the `bounds` of
  the filter: `uv + resolution.zw` is one pixel of the screen away.
- **A filter gives no `time`.** A shader whose block has that member is refused
  there, unless its block declares `property time: number;`, which the `use`
  then sets from a property something animates.
- With `linearColors: true` the pictures read and written are linear. The
  `color` inputs are still written as the document has them: the shader
  converts them itself.
- As for the other primitives, what the shader moves outside the `bounds` of
  the filter is lost, and a filter that can't run leaves the layer as it is.

The same block can be an `effect:` and a filter primitive. A block reading no
picture can also be painted by a `use`.

## Animating a shader

The inputs are properties, so everything that moves a property moves a shader:

```yui
style {
    // The input of an effect: a transition blends it, from and to the default of the block too
    .display       { effect: Crt; transition: intensity 200ms; }
    .display:hover { intensity: 0.9; }

    // The property of a drawing an input is computed from
    .sonar         { drawing: Sonar; level: 0.2; transition: level 300ms easeOut; }
    .sonar:hover   { level: 1; }
}

Component { class: sonar; level: meter.value; }         // or bound to a value
```

- A [transition or an animation](styles.md#transitions-and-animations) blends
  numbers, colors, points, rectangles and lists of the same length.
- A [binding](reactivity.md) follows a widget, a token or a
  [model value](documents.md#models).
- `time` in the uniform block is for what runs by itself: see
  [Properties and uniforms](#properties-and-uniforms).

## Loading and shipping

A block names the source of its shader, and the document reads a compiled
bundle first when there is one:

| Written | Read first | Then |
| --- | --- | --- |
| `fragment: "fx/crt.frag";` | `fx/crt.frag.ysl`, the compiled bundle | `fx/crt.frag`, compiled from its GLSL |
| `fragment: "fx/crt.ysl";` | `fx/crt.ysl`, as it is | |
| `material: "fx/hologram.glsl";` | `fx/hologram.glsl.raster.ysl` and `fx/hologram.glsl.full.ysl` | `fx/hologram.glsl`, compiled from its GLSL |

- **Compiling GLSL when the application runs needs the shader transpiler**,
  which a target has when it lists the `glslang` and `spirv_cross` modules
  (`YUP_ENABLE_SHADER_TRANSPILER`). It takes a moment the first time a shader
  is used, and is meant for development. A material compiled from its source
  also reads the shaders of `yup_renderer` from the YUP sources the binary was
  built from, or from the folder given to
  [`MaterialShader::setSourceDirectory`](../../3d/custom-materials.md#creating-a-shader)
  where those are not, as in a browser.
- **A shipped application embeds the bundles.**
  [`yup_add_yui_resources`](../../build-system/cmake-api.md#yup_add_yui_resources)
  compiles the files listed in `SHADERS` and `MATERIAL_SHADERS` when the
  application is built, and embeds them under the names of the table. The
  document is not changed, it keeps naming the source, and the binary needs no
  transpiler:

  ```cmake
  yup_add_yui_resources (MyAppUi
      BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/ui"
      FILES
          ui/main.yui
      SHADERS
          ui/fx/crt.frag
          ui/fx/rings.frag
      MATERIAL_SHADERS
          ui/fx/hologram.glsl)
  ```

- A shader that does not compile fails the build, for every graphics API at
  once, so it never ships.
- A bundle built another way, with
  [`yup_add_shader_bundle`](../../graphics/rhi/offline-shaders.md) for
  example, is listed in `ASSETS` under the name the document asks for
  (`fx/crt.frag.ysl`), or named directly (`fragment: "fx/crt.ysl";`). Its vertex
  stage is `shaders/yup_GpuImageShader.vert` of the `yup_rhi` module.
- A shader is loaded once for a document, whatever the number of components,
  drawings and filters using it.

## Editing a shader live

A [`YuiLoader`](hot-reload.md) that watches its files also watches the shaders
the document read. Saving one compiles it again and loads the document again,
which keeps the interface as it is: the components using the shader are painted
with the new one, and the diagnostics of the document are those of that load.

- A shader saved with an error keeps running as it was, and the error is
  listed with its line until the file compiles again.
- An editor showing an unsaved shader gives its text to the loader as it does
  for a `.yui` file: `loader.setOverride ("fx/crt.frag", text);` then
  `loader.reload();`.
- When the compiled bundle of a shader is there, its source is never read:
  editing the GLSL does nothing until the bundle goes. Live editing is for a
  document loaded from files, and embedded resources always hold the bundle.

## Diagnostics

What is wrong with a shader is in `YuiDocument::getDiagnostics()`, with the
line and the column of what it is about.

A block that is not valid, reported when the document loads:

```text
The shader 'Crt' needs a 'fragment' or a 'material'
The shader 'Crt' can't have both a 'fragment' and a 'material'
The shader 'Crt' has an unknown property 'vertex', expected 'fragment' or 'material'
The shader 'Crt' has the same name as a drawing
```

A shader that can't be loaded, reported once at its block whatever the number
of its users:

```text
The shader 'Crt' can't read 'fx/crt.frag' nor its compiled bundle 'fx/crt.frag.ysl'
shader Crt: ShaderBundleCompiler: SPIR-V compilation failed for stage fragment: ERROR: 0:3: 'missingValue' : undeclared identifier
The property 'label' of the shader 'Crt' can't be of type 'string': a shader takes a number, an integer, a boolean, a color, a point, a rectangle or a number[]
The uniform block of the shader 'Crt' has no member 'gain'
The member 'intensity' of the uniform block of the shader 'Crt' is too small for a color
```

A shader used where it can't be:

| Where | Message |
| --- | --- |
| `effect:` | `The shader 'Hologram' is a material, an effect takes a shader with a 'fragment'` |
| `effect:` | `The shader 'Mix' reads two textures, an effect gives it one` |
| A `use` | `Screen/use[0]: the shader 'Crt' reads a picture, run it in the 'filter' of a layer or as an 'effect:'` |
| A `use` | `Screen/repeat[0]/use[0]: the shader 'Rings' can't be painted inside a repeat` |
| A `use` | `Screen/use[0]: unknown property 'gain'` |
| A `use` in a `filter` | `Screen/layer[0]/filter[0]/use[0]: unknown shader 'Glow'` |
| A `use` in a `filter` | `Screen/layer[0]/filter[0]/use[0]: the shader 'Clock' has a 'time', which a 'filter' doesn't fill: declare 'property time: number;' in its block to set it here` |
| A `use` in a `filter` | `Screen/layer[0]/filter[0]/use[0]: unknown property 'in2'` |

A component whose effect can't be used is drawn as it is, and a drawing whose
shader could not be loaded paints everything but that shader. Nothing is
reported while painting: a shader the device can't draw shows nothing, or
leaves its component or its layer as they are.

[`yup_yui_tool verify`](tooling.md) loads every shader of the files it checks,
so the same messages stop a build that embeds them with `VERIFY`.

## Material shaders

A block with `material:` names a function that colors the surface of 3D
meshes, instead of the lighting of their material:

```yui
shader Hologram {
    material: "fx/hologram.glsl";
    property rim: color = #35e0ff;
    property strength: number = 1;
}

Model3D {
    source: "models/knot.glb";

    Material3D { shader: Hologram; baseColor: #06202a; }
}
```

The inputs are properties of the `Material3D`, converted as above, with two
differences: a `color` reaches the function **linear**, and of the three
members filled in only `time` exists. An input named like a property of
`Material3D` itself, `roughness` or `opacity`, reads that property and never
the default of the block. A material is not an image shader: it can't be an
`effect:` nor be painted by a `use`.

[Custom materials](scenes.md#custom-materials) shows it in a scene, and
[Custom materials](../../3d/custom-materials.md) in the 3D area describes the
GLSL of a material.

## From C++

The classes behind the block are public, for interfaces built in code:

- [`ShaderEffect`](../component-effects.md#shader-effects) runs a
  `GpuImageShader` as the effect of a component, fed by its properties.
- `DrawingStyle::create` takes a `ShaderResolver`, see
  [Using a drawing from C++](drawing.md#using-a-drawing-from-c).
- `FilterGraph` has a `shader` primitive holding the shader and its uniforms.
- [`GpuImageShader` and `ShaderUniforms`](../../graphics/rhi/image-shaders.md)
  are the two classes all of them are made of.
- `StyleSheet::findShader (name)` returns the block of a shader, and
  `StyleContext::findShader (name)` the shader loaded for it.

## Limits

- One uniform block, and no texture of its own: a shader reads the pictures it
  is given, not an image file.
- No vertex stage, so no geometry of its own.
- One type for a property name in a document.
- A shader given the `time` repaints its whole component at every frame. For a
  small animated picture in a large interface, give it a component of its own.
- The time of a shader painted by a drawing starts again when the drawing is
  made again: when its shader is reloaded, or when the document is built
  again. The one of an effect keeps going across a reload of its shader.
- The effect of a component reads the picture of that component: it can't read
  what lies behind it.

## See also

- [Stylesheets](styles.md) - `effect:`, transitions and keyframes.
- [Data-driven drawing](drawing.md) - `use`, layers and filters.
- [3D scenes](scenes.md) - `Material3D` and material shaders.
- [Hot reloading `.yui` files](hot-reload.md) - watching and embedding the
  files a document reads.
- [Component effects](../component-effects.md) - effects written in C++.
- [Image shaders](../../graphics/rhi/image-shaders.md) - `GpuImageShader`
  from C++.
- The `YUI 3D` page of the graphics example runs shaders as effects and as
  materials, in documents that can be edited while it runs.
