# Animation

glTF models move in three ways, and `yup_3d` reads all of them: **clips**
animate the transforms of their nodes, **skins** bend a mesh after its joints,
and **morph targets** blend a mesh between shapes. An `AnimationNode` plays the
clips of an instance, and a `DeformNode` keeps each deformed mesh up to date.

In a [`.yui` document](../ui/yui/scenes.md#clips-and-morph-weights) the same
classes are driven by the properties of `Model3D` and `Node3D`, with no code.

## Playing a clip

`GltfModel` attaches an `AnimationNode` to the root entity of every instance
of a model that has animations. It starts paused on the first clip, so an
instance shows the pose of its file until it is asked:

```cpp
class Viewer : public yup::Component
{
public:
    explicit Viewer (const yup::GltfModel& model)
    {
        auto scene = yup::Scene::Ptr (new yup::Scene());
        yup::Entity instance = model.instantiate (*scene);

        if (auto* animation = instance.get<yup::AnimationNode>())
        {
            animation->setCurrentClip (animation->findClip ("Open"));
            animation->setPlaying (true);

            // The update that ends the clip poses it at its end: paint that frame, then stop updating
            clipEnded = animation->onClipEnded().connect ([this]
            {
                view.setContinuousUpdates (false);
                view.repaint();
            });
        }

        view.setScene (scene);
        view.setContinuousUpdates (true);   // Scene::update() and a repaint on every frame
        addAndMakeVisible (view);
    }

    void resized() override { view.setBounds (getLocalBounds()); }

private:
    yup::SceneComponent view;
    yup::SignalConnection clipEnded;
};
```

`Scene::update()` is what advances a clip: a view that updates continuously
calls it on every frame, and code calls it otherwise.

A clip can also be moved by hand. Setting its time poses the entities at once,
playing or not, and the meshes that deform follow when the scene is updated:

```cpp
yup::Entity lamp = model.instantiate (*scene);

if (auto* animation = lamp.get<yup::AnimationNode>())
{
    animation->setCurrentClip (animation->findClip ("Bow"));
    animation->setTime (0.7);       // poses the joints of the lamp at once
}

scene->update (0.0);                // its skinned arm follows them
view.repaint();
```

<p align="center">
  <img class="light-only rounded" src="../_static/images/3d/animation_clip_light.jpg#gh-light-mode-only" alt="A desk lamp seen from its side at the start, the middle and the end of the clip that bows it: its arm bends smoothly and its head comes down until it looks straight down" width="640">
  <img class="dark-only rounded" src="../_static/images/3d/animation_clip_dark.jpg#gh-dark-mode-only" alt="A desk lamp seen from its side at the start, the middle and the end of the clip that bows it: its arm bends smoothly and its head comes down until it looks straight down" width="640">
</p>

| `AnimationNode` | What it does |
| --- | --- |
| `getNumClips()`, `getClip (index)`, `findClip (name)` | The clips of the model, and the index of the first one with a name, or -1. |
| `setCurrentClip (index)` | Selects a clip at its start and poses the entities there, once what the clip before it animated is back in its rest pose. Out of range, or an index without a clip, selects none and stops playing: the pose stays. |
| `getDuration()` | The length of the selected clip, in seconds. |
| `setTime (seconds)`, `getTime()` | Moves to a time, limited to the clip, and poses the entities there. |
| `setSpeed (speed)` | 1 by default, 2 twice as fast, negative to play backwards. |
| `setLooping (bool)` | Carries on from the other end instead of stopping. Off by default. |
| `setPlaying (bool)`, `isPlaying()` | Plays or pauses. Off by default, and while no clip is selected. |
| `onClipEnded()` | A `Signal<>` emitted once when a clip that does not loop stops. |
| `resetPose()` | Puts every entity back in its [rest pose](#the-rest-pose). |
| `getTargets()` | The entities the clips animate: the target of a track is an index among them. |
| `getWeights (target)` | The morph weights a clip left for a target entity, none once they are back at rest. |

- A clip that does not loop stops at the end it plays towards, its end or its
  start for a negative speed: the entities are posed there, `isPlaying()` turns
  false and `onClipEnded()` is emitted.
- `setPlaying (true)` on a clip stopped at that end starts it over from the
  other one, as the play button of a media player does. A clip paused anywhere
  else carries on where it is.
- The speed that counts for that is the one set when `setPlaying (true)` is
  called, and selecting a clip goes to its start: select the clip and set the
  speed first.
- A clip writes what its tracks animate into the `TransformNode` of each
  entity. What it does not animate keeps the value it has: the one of the
  file, or one set by code. What the clip selected before it animated does not
  stay: it goes back to [the rest pose](#the-rest-pose) first.
- A clip that starts playing poses its entities at once, then each time an
  update moves it on. An update in which no time goes by, with a delta or a
  speed of 0, poses nothing: what code, or a `Node3D` of a document bound to
  a joint, wrote into an entity since then stays until the clip moves on.
- A node plays one clip at a time, and clips are not blended. The instances of
  a model play apart, and a second node plays
  [two clips at once](#two-clips-at-once) on one of them.
- The signal calls back from `Scene::update()`, on the message thread, as its
  last step: the callback may select another clip, play again or destroy the
  entity. Keep the `SignalConnection` that `connect()` returns.
- A hidden instance is not updated, like every node: its clip stands still.

## The rest pose

An `AnimationNode` remembers how its entities were when it was created: their
translation, rotation, scale and morph weights, which for a model are the ones
of its file. That is their rest pose, and what a clip moved goes back to it:

```cpp
if (auto* animation = instance.get<yup::AnimationNode>())
{
    animation->setCurrentClip (-1);     // nothing plays, and the pose stays
    animation->resetPose();             // every entity as the file has it
}

scene->update (0.0);                    // the meshes that deform follow
view.repaint();
```

- **Selecting a clip** first puts back what the clip before it animated, so
  nothing of a clip is left once another one is selected: a clip that only
  opens a door starts from a fan standing as the file has it, however the clip
  before turned it.
- What no clip posed is left alone: a part placed by code, or by a `Node3D` of
  a document, stays where it is when a clip is selected.
- **Selecting no clip**, `setCurrentClip (-1)`, stops playing and leaves the
  pose as it is.
- **`resetPose()`** puts every entity back, also where code moved it. The clip
  stays selected where it is, and one that is playing poses what it animates
  again at its next update: pause it, or select no clip, to keep the model at
  rest.
- The rest pose is read once, when the node is created. Before making a node
  from code, place its entities and attach their `DeformNode`: an entity
  without a `TransformNode` rests where its parent has it, and one without a
  `DeformNode` has no weights to go back to.
- Entities that were destroyed are skipped.

## Two clips at once

An entity holds one `AnimationNode`, and a node plays one clip. A second node,
on another entity, plays another clip of the model beside it once it is given
the same entities, which `getTargets()` returns:

```cpp
if (auto* animation = instance.get<yup::AnimationNode>())
{
    const auto targets = animation->getTargets();

    // Made before a clip poses the model: the rest pose of a node is the one its entities have then
    auto& fan = instance.createChild ("fan").attach<yup::AnimationNode> (
        std::vector<yup::AnimationClip::Ptr> { animation->getClip (animation->findClip ("Fan")) },
        std::vector<yup::Entity> (targets.begin(), targets.end()));

    fan.setLooping (true);
    fan.setPlaying (true);                                          // the fan turns

    animation->setCurrentClip (animation->findClip ("Open"));
    animation->setPlaying (true);                                   // while the door opens
}
```

- Each node puts back what its own clips animated: selecting a clip on one
  leaves what the other plays.
- The two clips are not blended: where both animate the same value of the
  same entity, one of them shows.

## Clips

An `AnimationClip` is a name and tracks, shared by every instance of its
model. Each track animates one path of one target:

```cpp
for (const auto& clip : model.getAnimations())
{
    if (clip != nullptr)                                    // an animation that can't be played has no clip
        YUP_DBG (clip->getName() << ": " << clip->getDuration() << " s");
}

auto clip = yup::AnimationClip::Ptr (new yup::AnimationClip ("rise"));

yup::AnimationClip::Track track;
track.target = 0;                                           // the first target of the node
track.path = yup::AnimationClip::Path::translation;
track.times = { 0.0f, 2.0f };
track.values = { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };      // x, y, z of each key

if (auto result = clip->addTrack (std::move (track)); result.failed())
    YUP_DBG (result.getErrorMessage());

entity.attach<yup::AnimationNode> (std::vector<yup::AnimationClip::Ptr> { clip },
                                   std::vector<yup::Entity> { platform });
```

- A target is an index: the `AnimationNode` holds the entity each index stands
  for, which is how one clip serves every instance.
- The paths are `translation`, `rotation`, `scale` and `weights`. A key holds
  3 values for a translation or a scale, 4 for a rotation (`x, y, z, w`), and
  one per morph target for weights.
- The interpolation of a track is `step`, `linear` or `cubicSpline`, which
  stores an in-tangent, a value and an out-tangent for each key. Rotations
  always come out as unit quaternions, and turn the shorter way from a key to
  the next.
- Before its first key a track gives its first value, and from its last key on
  its last one. The duration of a clip is the time of the last key of its
  tracks.
- `addTrack()` refuses a track with a negative target, with no key, with times
  that are negative, not finite or go back, with a number of values that does
  not fit its keys, or with a value that is not finite.

## Skins

A skinned mesh follows joints, which are plain entities of the instance. Each
vertex names up to four of them with a weight (`Mesh::Influences`), and a
`Skin` holds where each joint stood when the mesh was bound to it. Nothing has
to be done for a glTF model: every entity drawing a skinned mesh has its
`DeformNode`, which follows the joints wherever a clip, code or a binding
moves them.

```cpp
yup::Entity head = instance.findChild ("Head");
head.getOrAttach<yup::TransformNode>().setRotation (yup::Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.4f));

scene->update (0.0);   // the meshes follow their joints when the scene is updated
view.repaint();
```

A mesh built from code is skinned the same way:

```cpp
std::vector<yup::Mesh::Influences> influences (vertices.size());
influences[0] = { { 0, 1, 0, 0 }, { 0.75f, 0.25f, 0.0f, 0.0f } };   // joints 0 and 1 of the skin
// ...

auto mesh = yup::Mesh::Ptr (new yup::Mesh ("arm"));
mesh->addPrimitive (vertices, indices, material, influences);

// The second joint was bound 1 up: its inverse bind matrix takes that point to its origin
auto skin = yup::Skin::Ptr (new yup::Skin ("arm", { 0, 1 }, { yup::Matrix4(), yup::Matrix4::translation ({ 0.0f, -1.0f, 0.0f }) }));

entity.attach<yup::MeshNode> (mesh);
entity.attach<yup::DeformNode> (mesh, skin, std::vector<yup::Entity> { shoulder, elbow });
```

- A `DeformNode` draws a copy of the mesh it is given, and deforms that copy:
  the instances of a model deform apart, and the meshes of
  `GltfModel::getMeshes()` stay in the pose they were modeled in. The mesh an
  entity really draws is `entity.get<yup::MeshNode>()->mesh`: a `MeshNode`
  given the shared mesh again takes the copy back at the next update.
- **A skinned mesh is placed by its joints alone**, as in glTF. Moving the
  entity that draws it changes nothing: move the joints, or a parent of both.
- `addPrimitive()` makes the weights of a vertex add up to 1. A vertex whose
  weights are all zero is not skinned, and stays where the mesh has it.
- A joint that is destroyed keeps the place it last had, so its vertices do
  not jump.
- A `Skin` names its joints as indices into the nodes of its model, and holds
  no entity: it is shared, and each `DeformNode` holds the entities of its own
  instance.

## Morph targets

A morph target moves the vertices of a primitive by a delta, scaled by a
weight. The weights live on the `DeformNode` of the entity, one per target:

```cpp
yup::Entity face = instance.findChild ("Face");

if (auto* deform = face.get<yup::DeformNode>())
{
    std::vector<float> weights (deform->getWeights().begin(), deform->getWeights().end());
    weights[0] = 1.0f;                  // the first target, fully
    deform->setWeights (weights);
}

scene->update (0.0);
view.repaint();
```

- A deformed vertex is its bind pose plus the sum of `weight * delta` of every
  target, then the skin if there is one. Positions, normals and tangents are
  morphed.
- The weight at an index is the one of that target for every primitive of the
  mesh. Weights are not limited to 0 and 1: 2 goes twice as far.
- They start as the file has them, on the mesh or on the node.
- A `weights` track of a clip sets them each time the clip poses its targets,
  so while such a clip plays its weights replace the ones set by code. They go
  back to their [rest pose](#the-rest-pose) once another clip is selected.
- A mesh from code takes its targets in `addPrimitive()`, as
  `Mesh::MorphTarget` values holding one delta per vertex, and gets a
  `DeformNode` without a skin: `entity.attach<yup::DeformNode> (mesh)`.
- Targets are told apart by their index. glTF has no place for their names:
  the ones exporters write to `extras.targetNames` of a mesh are kept, and
  `Mesh::findMorphTarget (name)` gives the index of a target, or -1.

```cpp
// The index of a target is the index of its weight, in the weights of the snippet above
if (const auto smile = deform->getSourceMesh()->findMorphTarget ("Smile"); smile >= 0)
    weights[static_cast<size_t> (smile)] = 1.0f;
```

## The order of an update

In one `Scene::update()` the clips pose the joints and the weights before the
meshes are deformed, whatever the order the models were loaded in: a pose is
drawn by the frame that made it. A custom node that moves joints from its own
`update()` asks for the same order:

```cpp
scene->addSystemRule<MyNode, yup::DeformNode>();
```

A view that does not update continuously shows a change made by hand, to a
joint, a weight or the time of a clip, after `scene->update (0.0)`, or
`deform->update (0.0)` for one mesh.

## What the loader reads

`GltfModel` reads, for each file:

- **animations**, each as an `AnimationClip` named after it (`Animation N`
  without a name), in `getAnimations()`. Channels animating the translation,
  the rotation, the scale and the weights of a node, with `STEP`, `LINEAR` and
  `CUBICSPLINE` samplers, and outputs of any component type;
- **skins**, each as a `Skin`, in `getSkins()`, with the `JOINTS_0` and
  `WEIGHTS_0` of the primitives. The weights of a vertex are made to add up
  to 1;
- **morph targets**, as the `POSITION`, `NORMAL` and `TANGENT` deltas of the
  primitives, dense or sparse, with the weights of the mesh and the ones a node
  replaces them with, and the names of `extras.targetNames`.

Clips and skins have the index of their animation and of their skin in the
file: one that is left out keeps its place, as a `nullptr`.

What it leaves out, each with a warning in `getWarnings()`:

- `JOINTS_1` and the sets after it: a vertex follows four joints at most;
- morph targets of texture coordinates and colors;
- a channel that can't be played (no target node, an unknown path, keys that
  do not fit their values, values that are not finite, weights for a node
  that draws no mesh), and an animation left without any channel;
- a skin that can't move its mesh, by a joint it does not have for example:
  the mesh is then drawn as it is.

```text
Animation 0 channel 2: unknown path "visibility", ignored
Mesh 3 primitive 0: only the four joints of JOINTS_0 move a vertex, JOINTS_1 and its weights are ignored
Mesh 1 primitive 0 target 2: morphing TEXCOORD_0 is not supported, ignored
Node 7: mesh 2 uses joint 12, which skin 0 doesn't have, it is not skinned
```

### The budget of a model

Files are not trusted. An accessor can state any size, and one accessor can be
used by any number of primitives, targets and channels, so what a model keeps
is bounded by its size:

- a model keeps 2^26 values of what its accessors hold, about 256 MB, or 8 for
  each byte of its buffers when that is more, so a large model is not cut;
- the copy each deformed node takes in an instance counts too, and so does what
  the scenes of a file keep for the nodes they share;
- what would take a model over is left out, with a warning, and the load goes
  on: primitives, skins, morph targets, scenes, deformed nodes and animation
  channels, the last ones first. A node left out draws its mesh in its bind
  pose, and a scene left out has nothing to instantiate;
- a model keeps 200 warnings at most, the last one telling that more were left
  out.

```text
Node 4 deforming mesh 1: over the 67108864 values a model can keep, ignored
```

A file where very many nodes draw one skinned or morphed mesh, a crowd for
example, reaches the budget first: each node counts a copy of the mesh.

## Deforming a mesh yourself

`DeformNode` is built on `Mesh::setVertices()`, which any code can call to
give a primitive new vertices: a flag, a wave, a simulation. The renderer
follows by itself. See [Changing a mesh](meshes-and-gltf.md#changing-a-mesh).

A node that needs the other entities of its own instance, as an
`AnimationNode` needs its targets and a `DeformNode` its joints, gets them
from a [prefab link](entities-and-nodes.md#prefabs).

## Costs

- **Everything is computed on the CPU**, on the thread that updates the scene.
- **A model standing still costs a comparison per joint** and per update: a
  `DeformNode` does nothing while no joint moved for its mesh and no weight
  changed. An instance moved, turned or scaled as a whole, on a turntable for
  example, is not deformed again either.
- **A change computes every vertex** of the primitives it deforms again: the
  morph targets that have a weight, up to four joints for each vertex, then
  the vertex buffer of the copy is written when the mesh is next drawn. The
  cost grows with the vertices, so keep meshes that deform at every frame
  light.
- **Each instance keeps a copy** of the vertices and of the indices of every
  mesh it deforms, and as many vertices again to work on. The deltas and the
  influences stay on the shared mesh, once.
- **A deformation restarts what the renderer refines over frames**: the shadow
  maps the mesh casts into are drawn again, and the temporal accumulation of
  the full tier starts over, so a view keeps repainting while a clip plays and
  settles when it stops.

## Limits

- Four joints for each vertex.
- One clip at a time for each `AnimationNode`, no blending between clips.
- A `SceneComponent` that is not showing takes no display frame, so its scene
  is not updated: the clips of a hidden view stand still, `onClipEnded()` waits
  with them, and they carry on from where they were once the view shows again.
- A `Scene` is meant to be shown by one `SceneComponent` at a time: two of
  them updating it at every frame play its clips twice as fast.
- A mesh that deforms has no motion of its own: the temporal anti-aliasing
  does not reproject it and motion blur does not smear it, so a fast
  deformation can ghost slightly at its silhouette for a few frames on the full
  tier.
- Reflection probes and irradiance volumes do not bake again when a mesh
  deforms: call `requestBake()` on them.
- A model whose joints are mirrored relative to the entity drawing its mesh is
  drawn inside out. A model mirrored as a whole is fine.

## See also

- [Meshes, textures and glTF](meshes-and-gltf.md) - loading a model, and
  changing a mesh from code.
- [Entities and nodes](entities-and-nodes.md) - nodes, systems and prefabs.
- [Platform tiers](platform-tiers.md) - what restarts the convergence of the
  full tier.
- [3D scenes in `.yui`](../ui/yui/scenes.md#clips-and-morph-weights) - clips
  and morph weights without code.
