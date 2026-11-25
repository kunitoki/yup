# 3D scenes

A `Scene3D` element shows a 3D scene, and the scene is written inside it as
elements: cameras, lights, meshes, glTF models and their materials. Each one
is a component with properties, so what the language does with a widget it
does with a 3D object: bindings, links, style rules, classes, transitions,
`@keyframes`, composites and `id`s.

**Modules covered:** `yup_renderer`, with `yup_gui_markup`. The 3D elements
exist in every application that links both, and need no code.

## A first scene

```yui
Scene3D {
    background: #101116; ambient: #2a3040; toneMapping: aces;

    Camera3D { position: [0.4, 2.4, 4.8]; lookAt: [0.4, 0.45, 0]; fov: 32deg; }
    Light3D  { type: directional; position: [-4, 3, 2]; lookAt: [0, 0, 0]; intensity: 2; castsShadows: true; }

    // The floor: two triangles, counter-clockwise seen from above
    Mesh3D {
        vertices: [-3, 0, -3,  -3, 0, 3,  3, 0, 3,  3, 0, -3];
        indices: [0, 1, 2,  0, 2, 3];

        Material3D { baseColor: #6b7080; metallic: 0; roughness: 0.9; }
    }

    // A pyramid: four triangles sharing their top
    Mesh3D {
        rotation: [0, 30deg, 0];
        vertices: [-0.6, 0, 0.6,  0.6, 0, 0.6,  0.6, 0, -0.6,  -0.6, 0, -0.6,  0, 1.4, 0];
        indices: [0, 1, 4,  1, 2, 4,  2, 3, 4,  3, 0, 4];

        Material3D { baseColor: #ff8800; metallic: 0; roughness: 0.5; }
    }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/scenes_first_light.jpg#gh-light-mode-only" alt="An orange pyramid standing on a square gray floor, lit from the left and casting a shadow to the right, seen from above its front against a dark background" width="640">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/scenes_first_dark.jpg#gh-dark-mode-only" alt="An orange pyramid standing on a square gray floor, lit from the left and casting a shadow to the right, seen from above its front against a dark background" width="640">
</p>

The `Scene3D` is a component like any other: it takes the room its parent
gives it, here the whole document. What is written inside it is the scene. The
camera stands at a `position` and looks at a point, the light shines from
where it stands towards the origin, and each `Mesh3D` is drawn with the
`Material3D` written inside it.

## The elements

| Element | What it is | C++ class |
| --- | --- | --- |
| [`Scene3D`](#scene3d) | The component showing the scene, and its settings. | `SceneComponent` |
| [`Node3D`](#node3d) | A place in the scene: a group, or a part of a model. | `SceneNodeComponent` |
| [`Camera3D`](#camera3d) | A point of view. | `SceneCameraComponent` |
| [`Light3D`](#light3d) | A directional, point or spot light. | `SceneLightComponent` |
| [`Mesh3D`](#mesh3d) | Geometry written as numbers. | `SceneMeshComponent` |
| [`Model3D`](#model3d) | A glTF model, with its clips. | `SceneModelComponent` |
| [`Material3D`](#material3d) | The material of the meshes around it. | `SceneMaterialComponent` |

`Camera3D`, `Light3D`, `Mesh3D` and `Model3D` are made of `Node3D`: they have
its properties, and a rule for `Node3D` matches them. The elements nest as the
objects do: what is written inside a node moves with it.

```yui
Node3D {
    id: arm; position: [0, 1, 0]; rotation: [0, 0, lever.value * 90deg];

    Mesh3D  { vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; }
    Light3D { type: point; position: [1, 0, 0]; intensity: 20; }
}
```

## Transforms and units

- **Axes.** `y` is up. A camera and a light look down their own `-z` axis, with
  `+y` up, as in glTF.
- **Lengths** are in the units of the scene. Lights and the depth of field
  read them as meters.
- **Angles** are in radians, and `deg` converts a number: `45deg`,
  `knob.value * 270deg`.
- **`position`** is `[x, y, z]`, relative to the node the element is in.
- **`rotation`** is three Euler angles `[x, y, z]`. The turn around `z` comes
  first, then the one around `x`, then the one around `y`: for what looks down
  `-z`, `x` is an elevation, `y` a heading and `z` a roll. The angles are kept
  as they are written, so animating to `[0, 360deg, 0]` is a full turn.
- **`scale`** is a number for every axis, or `[x, y, z]`.
- **`lookAt`** is a point, relative to the parent, that the node turns its `-z`
  axis to. It replaces the rotation and is followed when the position or the
  point changes, which aims a camera or a light.

## Properties of the elements

Every element also has the properties of a component: `id`, `class`,
`visible`, `enabled`.

### `Scene3D`

| Property | Type | Default | Meaning |
| --- | --- | --- | --- |
| `background` | color | `#1a1a2e` | The color the scene is cleared to. With an alpha below 1 what lies behind the scene shows through, see [Scenes over 2D](#scenes-over-2d). |
| `ambient` | color | `#595959` | The flat light added to every surface. |
| `exposure` | number | `1` | The multiplier applied to the lit scene before tone mapping. |
| `toneMapping` | `reinhard`, `aces`, `agx` | `reinhard` | How the lit colors are brought into the range of the display. |
| `defaultLight` | boolean | `true` | Whether a default directional light shines while the scene has no light. |
| `continuous` | boolean | `false` | Updates and repaints the scene on every frame, instead of when something in it changes. |

- A `Scene3D` is sized as every component is: by the layout it is in
  (`flexGrow: 1`), or by its `width` and `height`.
- A component that is not a 3D element, written directly inside the `Scene3D`,
  is painted over the scene: a heads-up display. The `Scene3D` is not a layout
  container, so such a component is placed with `x`, `y`, `width` and `height`,
  which can read `parent.width` and `parent.height`.
- Clips, transitions, keyframes and shaders given the time repaint the scene by
  themselves. `continuous` is for scenes that code animates.
- The environment, the fog and the settings of the renderer have no property:
  see [What is left to C++](#what-is-left-to-c).

### `Node3D`

| Property | Type | Default | Meaning |
| --- | --- | --- | --- |
| `position` | number[3] | `[0, 0, 0]` | Where the node is, relative to its parent. |
| `rotation` | number[3] | `[0, 0, 0]` | Its Euler angles, in radians. |
| `scale` | number or number[3] | `1` | Its scale. |
| `lookAt` | number[3] | not set | A point the node turns to. Replaces the rotation. |
| `visible` | boolean | `true` | `false` hides the node and everything inside it. |
| `node` | string | `""` | Binds the node to a part of the model it is in, see [Parts of a model](#parts-of-a-model). |
| `travel` | number[3] | not set | How far the node moves over its range, see [Draggable parts](#draggable-parts). |
| `turn` | number[3] | not set | The angles the node turns by over its range. |
| `value` | number | `0` | Where the node is along its travel and its turn, from 0 to 1. |
| `morphWeights` | number[] | not set | The weights of the morph targets of its mesh, see [Clips and morph weights](#clips-and-morph-weights). |
| `surface` | `emissive`, `baseColor` | `emissive` | How the components written inside the node show on its mesh, see [Components on 3D objects](#components-on-3d-objects). |
| `onClick` | signal | | The node was clicked, see [Interaction](#interaction). |

A node paints nothing and takes no room in a layout: the 2D properties every
component has (`x`, `width`, `flexGrow`, `opacity`, the 2D `transform`) do
nothing on it. A node that sets none of its own is a plain group.

### `Camera3D`

A `Node3D`, with:

| Property | Type | Default | Meaning |
| --- | --- | --- | --- |
| `fov` | number | `0.8` | The vertical field of view of a perspective camera, in radians. |
| `near` | number | `0.1` | The distance of the near clipping plane. |
| `far` | number | `1000` | The distance of the far clipping plane, `0` for a perspective camera to see infinitely far. |
| `projection` | `perspective`, `orthographic` | `perspective` | The kind of projection. |
| `orthographicSize` | number | `1` | Half the height of what an orthographic camera sees. The width follows the shape of the `Scene3D`. |
| `active` | boolean | `true` | Whether the camera may be the one of the scene. |

The camera of the scene is the first `Camera3D` that is `active`, in the order
the elements are written. Switching cameras is switching `active`:

```yui
Camera3D { position: [0, 2, 6]; lookAt: [0, 0.5, 0]; active: !overview.checked; }
Camera3D { position: [0, 8, 2]; lookAt: [0, 0, 0]; projection: orthographic; orthographicSize: 3; active: overview.checked; }
```

Without an active `Camera3D` the scene is seen through a camera of one of its
models, or else from `[0, 0, 5]` towards the origin.

### `Light3D`

A `Node3D`, with:

| Property | Type | Default | Meaning |
| --- | --- | --- | --- |
| `type` | `directional`, `point`, `spot` | `directional` | Parallel rays down `-z`, rays from the position in every direction, or a cone around `-z`. |
| `color` | color | `#ffffff` | The color of the light. |
| `intensity` | number | `1` | Its brightness: in lux for a directional light, in candela for a point or a spot light. |
| `range` | number | `0` | The distance where a point or a spot light reaches zero, `0` for it to fade with the distance alone. |
| `innerCone` | number | `0` | The angle from its axis where a spot light starts to fade, in radians. |
| `outerCone` | number | `45deg` | The angle from its axis where a spot light ends. |
| `radius` | number | `0` | The size of the emitter, which softens its highlights and its shadows: a radius for a point or a spot light, an angle for a directional one. |
| `castsShadows` | boolean | `false` | Whether the light casts the shadows of the opaque objects. |

A directional light shines along the direction it looks in, wherever it
stands: `position` with `lookAt` only aims it. See
[Lights and shadows](../../3d/lights-and-shadows.md) for what each kind costs.

### `Mesh3D`

A `Node3D` drawing the geometry its arrays describe:

| Property | Type | Meaning |
| --- | --- | --- |
| `vertices` | number[] | The x, y, z of each vertex, one after the other. |
| `indices` | integer[] | Three vertices for each triangle, counter-clockwise seen from its front. Without them the vertices are taken three by three. |
| `normals` | number[] | The x, y, z of the normal of each vertex. Without them each triangle is flat. |
| `uvs` | number[] | The u, v of each vertex, `v` pointing down. |
| `colors` | number[] | The linear red, green, blue and alpha of each vertex, multiplied with the base color. |

- The mesh is drawn with the `Material3D` written inside it or around it, or
  else with a default material.
- The arrays are properties: a transition blends two arrays of the same length
  number by number, so a shape can turn into another.
- Arrays that do not fit together draw nothing, and the diagnostic names the
  array and what was expected of it.
- A `Mesh3D` is for a few triangles: a floor, a panel, a marker. Larger
  geometry belongs in a glTF file.

### `Model3D`

A `Node3D` showing a glTF model:

| Property | Type | Default | Meaning |
| --- | --- | --- | --- |
| `source` | string | `""` | The path of a `.glb` or `.gltf` file, see [Assets and hot reload](#assets-and-hot-reload). |
| `scene` | integer | `-1` | The glTF scene of the file to show, `-1` for its default one. |
| `clip` | string | `""` | The clip to play: its name, or its place among the clips of the file. |
| `playing` | boolean | `false` | Plays the clip. |
| `speed` | number | `1` | 2 is twice as fast, a negative speed plays backwards. |
| `loop` | boolean | `false` | Carries on from the other end instead of stopping. |
| `time` | number | `0` | Where the clip is, in seconds. Setting it moves the clip. |
| `duration` | number | read only | The length of the clip, in seconds. |
| `clips` | string[] | read only | The names of the clips of the model. |
| `onClipEnded` | signal | | A clip that does not loop stopped at its end. |

The models of one file share their meshes, their materials and their textures,
however many `Model3D` name it. [Clips and morph weights](#clips-and-morph-weights)
describes the clip properties.

### `Material3D`

Not a node: it gives its material to the meshes of the element it is written
in, and of everything inside that element, the nearest one winning. Inside a
`Mesh3D` it is the material of that mesh, inside a `Model3D` the one of the
whole model, and directly inside the `Scene3D` the one of the whole scene.

| Property | Type | Default | Meaning |
| --- | --- | --- | --- |
| `baseColor` | color | `#ffffff` | The color of the surface, and its alpha. |
| `baseColorTexture` | string | `""` | The path of the image of the base color, multiplied by `baseColor`. |
| `metallic` | number | `1` | From 0 for a non-metal to 1. |
| `roughness` | number | `1` | From 0 for a mirror to 1. |
| `metallicRoughnessTexture` | string | `""` | The image holding the roughness in green and the metalness in blue. |
| `normalTexture`, `normalScale` | string, number | `""`, `1` | The image of the tangent space normals, and how strongly it bends the surface. |
| `occlusionTexture`, `occlusionStrength` | string, number | `""`, `1` | The image holding the ambient occlusion in red, and how much of it is applied. |
| `emissive` | color | `#000000` | The color of the light the surface emits. |
| `emissiveTexture`, `emissiveStrength` | string, number | `""`, `1` | The image of the emitted light, and its multiplier. |
| `alphaMode` | `opaque`, `mask`, `blend` | `opaque` | Whether the alpha is ignored, cuts the surface out below `alphaCutoff`, or blends it over what is behind. |
| `alphaCutoff` | number | `0.5` | The alpha below which a masked surface is cut out. |
| `doubleSided` | boolean | `false` | Whether the back faces are drawn too. |
| `clearcoat`, `clearcoatRoughness` | number | `0` | A clear varnish over the surface, and how rough it is. |
| `sheenColor`, `sheenRoughness` | color, number | `#000000`, `0` | A velvet-like sheen at grazing angles, and how wide it spreads. |
| `shader` | name | none | A `shader` block coloring the meshes, see [Custom materials](#custom-materials). |

- A new material is **fully metallic and fully rough**, as in glTF: give a
  plain colored surface `metallic: 0`.
- A `Material3D` on a model replaces the materials of the file, textures
  included: it is not blended with them.
- Every property is styleable, so a class, a state and a transition restyle the
  meshes. The first `Material3D` of an element is the one used.

## Parts of a model

A `Node3D` with `node`, written inside a `Model3D`, is bound to a part of the
model instead of being a place of its own. It then drives that part:

```yui
Column {
    gap: 12;

    Scene3D {
        flexGrow: 1; background: #101116; ambient: #2a3040; toneMapping: aces;

        Camera3D { position: [3.3, 2.9, 5.2]; lookAt: [-0.25, 0.8, 0]; fov: 32deg; }
        Light3D  { type: directional; position: [-3.5, 6, 3]; lookAt: [0, 0, 0]; intensity: 1.8; radius: 0.015; castsShadows: true; }
        Light3D  { type: point; position: [3.2, 2.2, -2.8]; color: #5c7cff; intensity: 40; range: 14; }

        Model3D { source: "models/stage.glb"; }

        Model3D {
            source: "models/computer.glb";

            // Each node drives the part of the model its `node` names, and only what it sets
            Node3D { node: "Lid"; rotation: [-108deg * lid.value, 0, 0]; }
            Node3D { node: "KnobA"; rotation: [0, -45deg - 270deg * knob.value, 0]; }
            Node3D { node: "Pad3"; visible: pad.checked; }
        }
    }

    Row {
        gap: 12; preferredHeight: 28;

        Slider       { id: lid; value: 0.75; flexGrow: 1; }
        Slider       { id: knob; value: 0.5; flexGrow: 1; }
        ToggleButton { id: pad; text: "Pad 3"; preferredWidth: 80; }
    }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/scenes_model_light.jpg#gh-light-mode-only" alt="A portable computer on a round stage, its lid held three quarters open by a slider below the scene and one of its eight pads hidden by a toggle button" width="640">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/scenes_model_dark.jpg#gh-dark-mode-only" alt="A portable computer on a round stage, its lid held three quarters open by a slider below the scene and one of its eight pads hidden by a toggle button" width="640">
</p>

- `node` is the name the part has in the file: the first one of that name,
  looked for depth first. Names that are not unique are told apart by the
  names leading to them, separated by `/`, as in `"Body/Lid"`.
- A bound node drives **only what it sets**, and the whole of it: a `rotation`
  replaces the rotation the model gave the part, and its position stays as the
  model, or a clip, has it. `visible` is always driven.
- The part gets back what the model gave it when the node stops setting a
  value, or goes.
- What is written inside a bound node is placed on that part: a `Light3D` for
  a lamp of the model, a `Material3D` for the material of that part alone, a
  component for [its screen](#components-on-3d-objects). A light with no
  `position` stands at the origin of the part and shines down its `-z`. Where
  that origin lies inside the mesh, in the middle of a bulb for instance, give
  the light a `position` that brings it out: a light that casts shadows is
  shut in by what surrounds it.
- Only a `Node3D` can be bound, directly inside its `Model3D` or inside another
  bound node of it.

## Styling and animating

A 3D element is matched by its type, its classes, its `id` and its states, and
its properties are set by rules as those of a widget are:

```yui
style {
    Model3D             { scale: 1; transition: scale 150ms easeOut; }
    Model3D:hover       { scale: 1.1; }

    .ghost > Material3D { baseColor: #8090a040; alphaMode: blend; }
    .night              { background: #000010; exposure: 0.5; }

    .spinning           { animation: turn 8s linear infinite; }

    @keyframes turn { from { rotation: [0, 0, 0]; } to { rotation: [0, 360deg, 0]; } }
}
```

- Everything that is a number, a color or a list of numbers can be blended:
  positions, rotations, scales, the colors of lights and materials, the
  factors of materials, morph weights, the arrays of a `Mesh3D`.
- Write both ends of a transition the same way. A `scale` going from a number
  to three numbers is not blended, and neither are lists of different lengths.
- A [transition needs a value on both sides](styles.md#transitions-and-animations):
  give the resting rule one.
- `toneMapping`, `continuous`, `projection`, `type`, `surface`, `node`,
  `source` and `scene` are not styleable: set them on the element.
- `@keyframes` hold the values themselves. To move several objects the same
  way around where each stands, put each in a `Node3D` that places it, and
  animate the object inside.

## Clips and morph weights

A `Model3D` plays the animations of its file, its clips, and a bound node
sets the weights of the morph targets of a part:

```yui
Column {
    gap: 12;

    Scene3D {
        flexGrow: 1; background: #101116; ambient: #2a3040; toneMapping: aces;

        Camera3D { position: [3.3, 2.9, 5.2]; lookAt: [-0.25, 0.8, 0]; fov: 32deg; }
        Light3D  { type: directional; position: [-3.5, 6, 3]; lookAt: [0, 0, 0]; intensity: 1.8; radius: 0.015; castsShadows: true; }
        Light3D  { type: point; position: [3.2, 2.2, -2.8]; color: #5c7cff; intensity: 40; range: 14; }

        Model3D { source: "models/stage.glb"; }
        Model3D { source: "models/computer.glb"; clip: "Open"; time: 1.2; }

        Model3D {
            id: lamp; source: "models/lamp.glb"; position: [-2.03, 0, -0.54]; rotation: [0, 58deg, 0];
            clip: "Bow"; time: 0.7;

            // The weight of the morph target opening the shade, and a light shining out of it
            Node3D {
                node: "LampHead"; morphWeights: [1];

                Light3D { type: spot; color: #ffd9a8; intensity: 30; range: 9; innerCone: 22deg; outerCone: 46deg; castsShadows: true; }
            }
        }
    }

    // A scrub bar: it follows the clip while it plays, and moves it when dragged
    Slider { preferredHeight: 28; maximum: lamp.duration; value <=> lamp.time; }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/scenes_clip_light.jpg#gh-light-mode-only" alt="A desk lamp beside an open portable computer, halfway through the clip that bows it: its shade is opened wide and its warm light falls on the pads and the keys below it, with a scrub bar at the middle of its range under the scene" width="640">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/scenes_clip_dark.jpg#gh-dark-mode-only" alt="A desk lamp beside an open portable computer, halfway through the clip that bows it: its shade is opened wide and its warm light falls on the pads and the keys below it, with a scrub bar at the middle of its range under the scene" width="640">
</p>

- **`clip`** names a clip of the file. Left empty it is the first one, and the
  model then shows the pose of its file until the clip plays or `time` is set.
  A string of digits that is not the name of a clip is its place among the
  clips, from 0: `clip: "1"` is the second one. Naming a clip puts it at its
  start, and shows the model there at once.
- **`time`** poses the model at once, playing or not, so a paused clip is a
  pose: `clip: "Open"; time: 1.2;` shows the lid open. While the clip plays,
  `time` tells where it is at every frame. Link a slider to it with `<=>`: a
  one-way binding on `time` would be dropped as soon as the clip plays.
- **`playing`** is what the model is asked. It stays true when a clip that
  does not loop reaches its end, where the clip stays until `playing` turns
  true again, or another clip is named. It then plays again from the other
  end.
- **`speed`** and **`loop`** are applied before `playing`, whatever the order
  they are written in: a model asked to play backwards from its start plays
  from the end of its clip.
- **`duration`** and **`clips`** are read: `maximum: lamp.duration`,
  `ComboBox { items: lamp.clips; }`.
- **`onClipEnded`** is raised once when a clip that does not loop stops:
  `onClipEnded: action ("opened");`.
- One clip plays at a time on a model. A clip and a bound node driving the
  same value of the same part take turns: the last one to write shows. Use
  the clip for one motion, and bound nodes for the parts it does not move.
- **`morphWeights`** is the weight of each morph target of the mesh of a part,
  in the order of the file. A target given no weight keeps the weight of the
  model. A transition blends two lists of the same length, so give the resting
  state a list too (`[0, 0]`).

A scene with a playing clip is updated and painted at every frame, and at no
other time. [Animation](../../3d/animation.md) describes what a clip and a
deformed mesh cost.

## Interaction

A node takes the pointer where what it shows is the nearest thing under it, to
the triangle. It is then a component like any other for the mouse: it has the
`:hover` and `:pressed` states, a `cursor:`, a tooltip and `onClick`.

```yui
style {
    Model3D:hover  { scale: 1.1; transition: scale 150ms easeOut; }
    Mesh3D:pressed { scale: 0.95; }
    .knob          { cursor: hand; }
}

Scene3D {
    Camera3D { position: [0, 0.5, 4]; lookAt: [0, 0, 0]; }

    Model3D {
        source: "models/computer.glb";

        Node3D { node: "KnobA"; class: knob; onClick: action ("knob"); }
    }

    Mesh3D {
        onClick: action ("picked");
        vertices: [-1, -1, 0,  1, -1, 0,  1, 1, 0,  -1, 1, 0];
        indices: [0, 1, 2,  0, 2, 3];
    }

    Label { text: "Over the scene"; x: 12; y: 12; width: 160; height: 24; }
}
```

Which element answers for what is under the pointer:

| Element | Answers for |
| --- | --- |
| `Mesh3D` | Its mesh. |
| `Node3D` with `node` | That part of the model, and the parts under it that no other bound node names. |
| `Model3D` | The rest of its model. |
| `Node3D` used as a group | Nothing by itself: the pointer goes to what is inside it. |
| A component written over the scene | Itself, before any mesh under it. |

- The states reach the elements around the one under the pointer too:
  `Model3D:hover` holds while the pointer is over a knob bound inside that
  model, and `Scene3D:hover` while it is over anything in the scene.
- `onClick` is raised when a press is released over the node that was pressed.
  Clicks do not go up: a click on a bound knob is not a click of its model.
- A node with `visible: false` is not there for the pointer, and what is behind
  it is picked. One with `enabled: false` is drawn, so it is there: it hides
  what is behind it and gets no press.
- Triangles seen from behind are picked only where they are drawn: with a
  `doubleSided` material.
- Where neither a component nor a mesh is under the pointer, the `Scene3D`
  gets the event.

### Draggable parts

A node that sets `travel` or `turn` is a control. Its `value`, from 0 to 1,
places it along them, and pressing the node and moving the pointer changes the
value, as the handle of a slider or a knob is dragged:

```yui
style {
    .control { cursor: hand; }
}

Column {
    gap: 12;

    Scene3D {
        flexGrow: 1; background: #101116; ambient: #2a3040; toneMapping: aces;

        Camera3D { position: [0.9, 2.3, 2.9]; lookAt: [0.15, 0.2, 0.1]; fov: 34deg; }
        Light3D  { type: directional; position: [-3.5, 6, 3]; lookAt: [0, 0, 0]; intensity: 1.8; radius: 0.015; castsShadows: true; }
        Light3D  { type: point; position: [3.2, 2.2, -2.8]; color: #5c7cff; intensity: 40; range: 14; }

        Model3D { source: "models/stage.glb"; }

        Model3D {
            source: "models/computer.glb"; clip: "Open"; time: 1.2;

            // A fader: the part slides 0.78 towards the back over its range. Grab it in the scene, or drag the slider
            Node3D { node: "FaderA"; class: control; travel: [0, 0, -0.78]; value <=> cutoff.value; }

            // A knob: the part turns 270 degrees around its own axis over its range, from where `rotation` puts it
            Node3D { node: "KnobA"; class: control; rotation: [0, -45deg, 0]; turn: [0, -270deg, 0]; value <=> resonance.value; }
        }
    }

    Row {
        gap: 12; preferredHeight: 28;

        Slider { id: cutoff; value: 0.7; flexGrow: 1; }
        Slider { id: resonance; value: 0.3; flexGrow: 1; }
    }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/scenes_controls_light.jpg#gh-light-mode-only" alt="The deck of a portable computer seen from close: the first of its three faders is most of the way up its slot and the first of its two knobs a third of the way round, each linked to a slider below the scene" width="640">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/scenes_controls_dark.jpg#gh-dark-mode-only" alt="The deck of a portable computer seen from close: the first of its three faders is most of the way up its slot and the first of its two knobs a third of the way round, each linked to a slider below the scene" width="640">
</p>

| Property | Type | Meaning |
| --- | --- | --- |
| `travel` | number[3] | How far the node moves over its range, in the space of its parent. Set, the node is a slider. |
| `turn` | number[3] | The Euler angles the node turns by over its range, in its own space. Set without `travel`, the node is a knob. |
| `value` | number | Where the node is along both, from 0 to 1. It tells its changes, so it links with `<=>`. |

Where the node is:

- Its position is its resting position plus `travel * value`, and its rotation
  its resting rotation turned by `turn * value`. The resting ones are the
  `position` and the `rotation` the node sets, or else, for a bound node, the
  ones the model gave the part.
- The turn is in the node's own space: a knob turns around its own axis
  whatever way its rotation orients it.
- `lookAt` replaces the rotation, the turn included.
- The travel of a part of a model is the difference between the positions the
  part has at the two ends of its range, read on its parent in the modeling
  tool.

How it is dragged:

- **With a `travel`**, the node follows the pointer along the line it travels
  on, as the scene shows it, whatever the camera. Moving the pointer across
  that line does nothing, and a travel pointing at the camera can't be dragged.
- **With a `turn` alone**, the pointer moves the value up and down: up for
  more, 200 logical pixels for the whole range.
- The node pressed is the node dragged, and it keeps the pointer until the
  release, also when the pointer leaves what the node shows. `:pressed` holds
  meanwhile.
- A press that changed the value is not a click: `onClick` is for a press
  released without one.
- Link the value with `<=>`. A one-way binding on `value` is dropped by the
  first drag, as the one of a `Slider` is.

There is no range, step or wheel: scale the value where it is read, as in
`20 + cutoff.value * 980`. A node used as a group shows nothing to press, so
`travel` moves it with its value but nothing drags it. From C++ the value is
`SceneNodeComponent::setValue()` and `getValue()`.

## Components on 3D objects

A component that is not a 3D element, written inside a node, is shown on the
mesh of that node and used there: a slider drags, a button clicks, and hover
styles work.

```yui
palette Display { colors: "#071318 #35e0c2 #d9fff7"; }

drawing Glass {
    rect { bounds: bounds; fill: palette.background; }
}

style {
    .display { palette: Display; drawing: Glass; }
}

Scene3D {
    background: #101116; ambient: #2a3040; toneMapping: aces;

    Camera3D { position: [3.3, 2.9, 5.2]; lookAt: [-0.25, 0.8, 0]; fov: 32deg; }
    Light3D  { type: directional; position: [-3.5, 6, 3]; lookAt: [0, 0, 0]; intensity: 1.8; radius: 0.015; castsShadows: true; }
    Light3D  { type: point; position: [3.2, 2.2, -2.8]; color: #5c7cff; intensity: 40; range: 14; }

    Model3D { source: "models/stage.glb"; }

    Model3D {
        source: "models/computer.glb"; clip: "Open"; time: 1.2;

        // What is written inside a node is shown on its mesh, and used there
        Node3D {
            node: "Screen";

            Column {
                class: display; width: 416; height: 260; padding: 22; gap: 10;

                Label  { text: format ("%.0f Hz", 20 + cutoff.value * 980); flexGrow: 1; textHeightProportion: 0.6; }
                Slider { id: cutoff; value: 0.4; preferredHeight: 28; }
            }
        }

        // The fader of the model and the slider on its screen are one value
        Node3D { node: "FaderA"; travel: [0, 0, -0.78]; value <=> cutoff.value; }
    }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/scenes_surface_light.jpg#gh-light-mode-only" alt="An open portable computer on a stage whose screen shows a 2D interface, a frequency in large digits over a slider, following the perspective of the lid" width="640">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/scenes_surface_dark.jpg#gh-dark-mode-only" alt="An open portable computer on a stage whose screen shows a 2D interface, a frequency in large digits over a slider, following the perspective of the lid" width="640">
</p>

- **Where it shows.** On the mesh the node itself draws: the mesh of a
  `Mesh3D`, or of the part of a model a `Node3D` is bound to. A `Model3D`, a
  group, a camera and a light draw no mesh of their own, and say so in the
  diagnostics.
- **How it is mapped.** The components of a node make its surface: the area
  from the origin to their far corner, in their own units. It lies over the
  texture coordinates 0 to 1 of the mesh, `v` pointing down. So give the
  components a `width` and a `height` of the shape the mesh has, and the
  resolution wanted: they are rendered at that size, whatever size the mesh
  has on the screen. A mesh without texture coordinates, a `Mesh3D` given no
  `uvs` or a part of a model made without them, has nowhere to show them: the
  node shows none of its components, keeps the pointer, and reports it.
- **`surface`** chooses how they are lit. `emissive`, the default, shows them
  with their own colors whatever the light, as a screen does. `baseColor`
  lights them as the rest of the object.
- **Input.** The pointer reaches the components where the mesh shows them,
  before the node itself. A drag that leaves the mesh carries on, as one
  leaving a component does.
- **The material.** The surface is shown through the `Material3D` of the node
  when it has one, or else the mesh keeps the look it has. With `emissive` the
  lit color of the mesh is added to the interface: a screen is a dark
  material, `Material3D { baseColor: #000000; }` on the node if the model does
  not have one.
- Wrap the interface in one container, as the `Column` above: popups, such as
  the menu of a `ComboBox`, then open inside it, on the mesh.
- Put a surface on a part that has nothing under it in the model: its material
  is also the one of the parts inside it.

Directly inside the `Scene3D` a component stays flat, over the scene. The same
`Slider` is a heads-up display there and a control on a screen inside a node.

## Custom materials

A `Material3D` naming a `shader` block has its meshes colored by the GLSL
function of that block, instead of the lighting of the material. The inputs of
the block are properties of the `Material3D`:

```yui
shader Hologram {
    material: "fx/hologram.glsl";
    property rim: color = #35e0ff;
    property strength: number = 1;
}

style {
    Material3D                 { strength: 1; transition: strength 200ms; }
    Model3D:hover > Material3D { strength: 2.5; }
}

Scene3D {
    background: #101116; toneMapping: aces;

    Camera3D { position: [0, 0.4, 5]; lookAt: [0, 0, 0]; fov: 32deg; }

    Model3D {
        source: "models/knot.glb"; rotation: [60deg, 30deg, 0]; scale: 0.6;

        // The material gives the base color the function starts from, the shader the rest
        Material3D { shader: Hologram; baseColor: #06202a; }
    }
}
```

`fx/hologram.glsl`:

```glsl
YUP_MATERIAL_PARAMS
{
    vec4 rim;           // property rim: color, given linear, its alpha as it is
    float strength;     // property strength: number
    float time;         // no property has that name: the seconds the material has been shown for
} params;

vec4 yupMaterial (YupSurface surface)
{
    // Light where the surface turns away from the eye
    float fresnel = pow (1.0 - max (dot (surface.normal, surface.view), 0.0), 3.0);

    // Level lines climbing through the object: they follow the height in the world
    float lines = 0.5 + 0.5 * sin (surface.position.y * 40.0 - params.time * 4.0);

    vec3 glow = params.rim.rgb * (fresnel + 0.35 * lines * lines) * params.strength;

    // A linear color, and the alpha the alpha mode of the Material3D uses
    return vec4 (surface.baseColor.rgb + glow, surface.baseColor.a);
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/scenes_material_light.jpg#gh-light-mode-only" alt="A torus knot colored by a material shader: dark, with horizontal lines of light and edges glowing cyan" width="480">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/scenes_material_dark.jpg#gh-dark-mode-only" alt="A torus knot colored by a material shader: dark, with horizontal lines of light and edges glowing cyan" width="480">
</p>

- `shader:` takes the name of a `shader` block that has a `material`, written
  bare, or `none`.
- The inputs are converted as [those of an image shader](shaders.md#properties-and-uniforms)
  are, except that a `color` reaches the function **linear**, with its alpha
  as it is.
- An input the `Material3D` gives no value takes the one a component around
  it has under that name, the nearest first, then the default of the block:
  `strength: 2` on the `Scene3D` reaches every material in it. What those
  components have as a property of their own is not taken, so an input named
  `scale` or `time` never gets the scale of a node or the time of a model,
  while one named like a property of `Material3D` itself, `roughness` or
  `opacity`, reads that property and never the default of the block.
- `time` is filled in when the uniform block has it and no property takes its
  name: the scene is then painted at every frame while the meshes show. There
  is no `resolution` nor `scale` for a material.
- The defaults of the block are not styled values: a transition has nothing to
  blend from. Give the resting rule a value, as `strength: 1` above.
- The material still decides the rest: `alphaMode`, `alphaCutoff`,
  `doubleSided`, the shadows the mesh casts, and the base color and textures
  the function reads from its `YupSurface`.
- The function is **unlit**: no light, shadow or environment reaches it. What
  it returns is exposed and tone mapped as the rest of the scene.
- A material that can't be loaded leaves the meshes to the `Material3D`, and is
  reported once at its block.

The GLSL of a material, what it is given and what it can't do are in
[Custom materials](../../3d/custom-materials.md), and how a material is found,
compiled and shipped in [Loading and shipping](shaders.md#loading-and-shipping).

## Scenes over 2D

A `background` with an alpha below 1 lets what lies behind the `Scene3D` show
wherever no surface covers it, so a scene is one layer among the others of an
interface:

```yui
drawing Backdrop {
    rect { bounds: bounds; cornerRadius: 12;
           fillGradient: linearGradient (palette.accent, point (0, 0), palette.surface, point (bounds.width, bounds.height)); }
    text { bounds: bounds.reduced (24); text: "2D"; justification: centerLeft;
           font: defaultFont.withHeight (bounds.height * 0.6); fill: palette.text.withAlpha (0.35); }
}

Stack {
    Component { drawing: Backdrop; }

    Scene3D {
        background: colors.transparentBlack; toneMapping: aces;

        Camera3D { position: [-1.1, 0.4, 5]; lookAt: [-1.1, 0, 0]; fov: 32deg; }
        Light3D  { type: directional; position: [-3.5, 6, 3]; lookAt: [0, 0, 0]; intensity: 2.5; }

        Model3D {
            source: "models/knot.glb"; rotation: [60deg, 30deg, 0]; scale: 0.6;

            Material3D { baseColor: #ff8800; metallic: 0; roughness: 0.35; }
        }
    }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/scenes_transparent_light.jpg#gh-light-mode-only" alt="An orange torus knot drawn over a 2D panel painted by a drawing, a gradient with the large letters 2D, which shows all around the knot and through its holes" width="560">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/scenes_transparent_dark.jpg#gh-dark-mode-only" alt="An orange torus knot drawn over a 2D panel painted by a drawing, a gradient with the large letters 2D, which shows all around the knot and through its holes" width="560">
</p>

- The alpha is how much of the background is there: 0 for none of it, 0.2 for
  a tint over what is behind.
- What is behind also shows through materials with `alphaMode: blend`, by what
  their alpha lets through, and along the antialiased edges of the objects.
- What is behind the scene is painted again each time the scene is, which an
  opaque scene spares.
- Fog covers what it fills, and a visible environment covers everything. See
  [Showing a scene](../../3d/showing-a-scene.md#scenes-over-2d).

## Assets and hot reload

The models of `Model3D`, the textures of `Material3D` and the shaders of
`shader` blocks are read through the document, by paths relative to the folder
of its main file:

- from that folder, for a document loaded from a file;
- from the resources embedded in the binary, listed in the `ASSETS`, `SHADERS`
  and `MATERIAL_SHADERS` of
  [`yup_add_yui_resources`](hot-reload.md#embedded-resources);
- from the unsaved text of an editor, for text files.

A [`YuiLoader`](hot-reload.md) that watches its files watches these too: a
model, a texture or a shader that is saved is read again, every element using
it follows, and the document is loaded again, which keeps the interface and
brings its diagnostics up to date.

- A `.gltf` keeping its buffers and its images in other files reads them from
  its folder, which embedded resources do not have: embed a `.glb`.
- A file is read once for a `Scene3D`, whatever the number of elements naming
  it. `a.glb` and `./a.glb` are two files for that.
- Textures are decoded with the image formats the application links.

## What is left to C++

A document describes the scene. The renderer and what has no property are
reached through the component:

```cpp
auto* view = document.findById<yup::SceneComponent> ("view");     // Scene3D { id: view; }

// The environment and the fog are nodes of the root of the scene
view->getOrCreateScene().getRoot().attach<yup::EnvironmentNode> (environment);

// The tier, bloom, depth of field and the other settings of the renderer
auto settings = view->getRenderer().getRenderSettings();
settings.bloom = true;
view->getRenderer().setRenderSettings (settings);

// What a camera or a light has beyond its properties
if (auto* camera = document.findById<yup::SceneCameraComponent> ("camera"))
    if (auto* lens = camera->getEntity().get<yup::CameraNode>())
        lens->fStop = 1.4f;
```

- `SceneNodeComponent::getEntity()` is the entity of an element, and
  `SceneModelComponent::getInstance()` the root of the model it shows, for
  code that attaches nodes of its own. Both are made again when the element
  moves to another parent or its model is loaded again.
- A component that moves the camera with the mouse listens to the elements of
  the scene too, with `addMouseListener (this, true)`: a drag that starts on a
  node goes to that node, not to the `Scene3D`.
- `SceneComponent::findNodeAt (position)` tells what is under a point without
  a mouse event.
- The pages of the [3D area](../../3d/index.md) describe the scene, its nodes
  and the renderer.

## Diagnostics

What is wrong with a scene is in `YuiDocument::getDiagnostics()`, at the line
of the property it is about, and stays there for as long as it stands:

```text
Node3D.position: expected 3 numbers
Node3D.node: the model has no entity 'Handle'
Light3D.node: only a Node3D can be bound to an entity of a model
Mesh3D.indices: the index 7 is not the one of a vertex, there are 3
Model3D.source: the file 'models/chair.glb' can't be read
Model3D.clip: the model has no clip 'Walk'
Node3D.morphWeights: the entity of the node draws no mesh with morph targets
Material3D.baseColorTexture: the file 'textures/wood.png' can't be read
Material3D.shader: the stylesheet has no shader 'Hologram'
Material3D: the parent already has a Material3D, this one is not used
Mesh3D: the components inside a node are shown over the texture coordinates of its mesh, and the ones of this mesh span no area: give the Mesh3D its uvs, or the part of the model its texture coordinates
```

What a glTF file holds that could not be converted is listed as warnings on
the `source` of its `Model3D`. [`yup_yui_tool verify`](tooling.md) loads the
models, the textures and the shaders of every scene, with no GPU, and reports
all of these before the application runs.

## Limits and costs

- **A still scene costs nothing.** It is drawn when something in it changes.
  On desktop GPUs the renderer then refines the picture over a few frames, and
  stops. See [Platform tiers](../../3d/platform-tiers.md).
- **What runs at every frame**: a playing clip, an `animation:`, a running
  transition, a material or an effect given the `time`, and `continuous`.
- **Skinned and morphed meshes are deformed on the CPU**, each time their
  joints or their weights change.
- **Picking tests triangles**: fine for instrument panels and product models,
  slow for scanned meshes of hundreds of thousands of triangles under the
  pointer.
- **A surface is a texture** of the size of its components, drawn again when
  they repaint. Seen from far it has no smaller levels, so it can shimmer.
- **One clip at a time** on a model, and no blending between clips.
- **A clip keeps playing while its `Scene3D` is hidden**, or on a tab that is
  not shown: its `time` moves on and `onClipEnded` is raised with nothing
  drawn. Bind `playing` to what shows the scene to pause it. A hidden
  `Model3D` does stand still.
- **A scene is shown by one `Scene3D`.** Given to a second view from C++, its
  clips play twice as fast.
- **A bound node replaces what it sets.** It can't turn a part relative to the
  rotation the model gave it, but for `turn` and `travel`.
- **Rotations are Euler angles.** A part standing on a slope is turned around
  its own axis with `turn`, or by a `Node3D` inside it.
- **Events set no value.** A click raises an action or a signal: a state that
  a click toggles lives in a widget, or in the application.
- **A `Mesh3D` has one material**, no tangents of its own, and is built again
  when one of its arrays changes.

## See also

- [Shaders](shaders.md) - the `shader` block, effects and materials.
- [Stylesheets](styles.md) - selectors, transitions and `@keyframes`.
- [Hot reloading `.yui` files](hot-reload.md) - watching and embedding the
  files a scene reads.
- [3D](../../3d/index.md) - scenes, entities, glTF and the renderer from C++.
- [Animation](../../3d/animation.md) - clips, skins and morph targets.
- [Custom materials](../../3d/custom-materials.md) - the GLSL of a material.
- The `YUI 3D` page of the graphics example shows scenes that can be edited
  while they run: Computer, Cameras, Shapes, Materials, Post effects and
  Timeline.
