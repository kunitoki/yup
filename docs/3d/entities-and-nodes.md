# Entities and nodes

Entities only exist inside a scene. `createEntity()` adds one under the root, or
under the parent you pass:

```cpp
auto scene = yup::Scene::Ptr (new yup::Scene());

yup::Entity body = scene->createEntity ("body");
body.attach<yup::TransformNode> (yup::Vector3<float> { 0.0f, 1.0f, 0.0f },
                                 yup::Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.5f));
body.attach<yup::MeshNode> (bodyMesh);

yup::Entity wheel = scene->createEntity ("wheel", body);
scene->getRoot().addChild (wheel); // moves it; adding an entity under itself is rejected
```

`destroy()` removes an entity, its subtree and their nodes, while
`removeFromParent()` only takes it out of the tree (it stays alive but is no
longer updated or drawn). An `Entity` that
outlives what it refers to becomes invalid (`isValid()` is false), even if the
scene reuses its slot later.

Nodes are addressed by their exact type:

| Call                         | Does                                                                  |
| ---------------------------- | --------------------------------------------------------------------- |
| `attach<T> (args...)`        | Constructs a `T`, replacing the `T` the entity had. Returns `T&`.     |
| `getOrAttach<T> (args...)`   | Returns the existing `T`, or attaches one with `args`.                |
| `get<T>()`                   | Returns `T*`, or `nullptr`.                                           |
| `has<T>()` / `detach<T>()`   | Tests for, or detaches and destroys, the `T`.                         |

A node never moves while it is attached, so the reference returned by
`attach()` stays valid until the node is detached or its entity destroyed. To
keep a node across frames safely, hold a `NodeHandle<T>`: it turns empty
(`explicit operator bool`) when the node goes away and follows a replacement.

```cpp
yup::NodeHandle<yup::LightNode> key = lamp.attach<yup::LightNode> (yup::LightNode::Type::point);

if (key)
    key->intensity = 2.0f;
```

Several meshes or lights on one object are child entities, one node each. A
`MaterialNode` overrides the material of every mesh in the entity's subtree,
its own mesh included; the nearest override up the tree wins.

## Transforms and bounds

Position, `Quaternion` rotation and scale live in a `TransformNode`. Its local
matrix applies scale, then rotation, then translation, and is cached until a
setter changes it. An entity without a `TransformNode` has the identity
transform, so plain groups cost nothing.

```cpp
body.getOrAttach<yup::TransformNode>().setPosition ({ 0.0f, 2.0f, 0.0f });
```

`Entity::getWorldMatrix()` combines the transforms of the entity and its
parents, cached and refreshed when a transform above it changes.
`computeWorldBounds()` returns the world space box around every mesh in the
visible subtree, which is handy to frame a model with the camera.

## Identifiers

Every entity and node has a `Uuid`, to refer to them from saved files, the
network or an editor. An entity's identifier is created the first time
`getUuid()` is called, so entities nobody refers to cost nothing, and
`Scene::findEntity (uuid)` finds it again. A node's identifier is derived from
its entity's identifier and its `NodeTypeId`, so nothing is stored per node.
Both take a few nanoseconds: entity identifiers come from a fast generator
seeded once per scene by `SecureRandom`, so they are unique but not secret.

When loading a saved scene, restore the entity identifiers and the node
identifiers follow:

```cpp
// Saving
writer.write ("id", entity.getUuid().toString());

// Loading
auto entity = scene->createEntity (name, parent, yup::Uuid (reader.read ("id")));
auto& light = entity.attach<yup::LightNode>(); // light.getUuid() matches the saved node
```

`Entity::setUuid()` restores an identifier on an existing entity. An identifier
already used by another entity of the same scene is rejected. Entities created
by instantiating a prefab always get new identifiers.

## Views, collections and signals

A view visits every entity having a node of each listed type. It iterates the
smallest pool, ignores visibility, and supports structured bindings:

```cpp
for (auto [entity, mesh, material] : scene->view<yup::MeshNode, yup::MaterialNode>())
    material.material = mesh.mesh != nullptr ? highlight : nullptr;

scene->view<yup::LightNode>()
     .where ([] (const yup::LightNode& light) { return light.castsShadows; })
     .forEach ([] (yup::Entity entity, yup::LightNode& light) { light.intensity *= 0.5f; });
```

`first()`, `size()` and `isEmpty()` complete views. When the same set is visited
every frame, `scene->collect<Ts...>()` returns a `NodeCollection`, created once
and kept up to date as nodes are attached and detached, so visiting it costs no
lookups. A collection visits its entities in the order they joined it, and the
renderer gathers meshes, lights, probes, volumes and scene features this way.

`onAttach<T>()`, `onDetach<T>()` and `onDestroy()` are `yup::Signal`s: connect a
callback and keep the returned `SignalConnection`, which disconnects it when
destroyed.

```cpp
lightAdded = scene->onAttach<yup::LightNode>().connect ([] (yup::Entity entity, yup::LightNode& light)
{
    DBG ("light added to " << entity.getName());
});
```

**Changes while iterating.** Inside a view, a collection's `forEach()`,
`Scene::update()` or a signal callback, attaching, detaching and destroying are
allowed. A detached node or destroyed entity disappears from the iteration at
once, but its memory is released only when the outermost iteration ends; a node
attached meanwhile is visited from the next iteration.

## Custom nodes

Subclass `Node` to add data or behavior. `Scene::update()` calls `update()` on
every node of every visible entity, only for the node types that override it:

```cpp
class Spinner : public yup::Node
{
public:
    void update (double deltaSeconds) override
    {
        angle += static_cast<float> (deltaSeconds);
        getEntity().getOrAttach<yup::TransformNode>()
            .setRotation (yup::Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, angle));
    }

private:
    float angle = 0.0f;
};

entity.attach<Spinner>();
```

`attachedToEntity()` and `detachedFromEntity()` are called when the node joins
and leaves its entity. Each node type gets a stable id, `NodeTypeId::get<T>()`, a
64-bit hash of its qualified name computed at compile time. Declare
`static constexpr std::string_view nodeTypeName = "my::Spinner";` to keep the id
when renaming the class or across compilers, or if two names ever collide (the
scene asserts on that in debug builds).

## Systems

A `System` runs once per `Scene::update()`, before or after the node updates,
which suits work across many nodes:

```cpp
class Gravity : public yup::System
{
public:
    void update (yup::Scene& scene, double deltaSeconds) override
    {
        for (auto [entity, body] : scene.view<RigidBody>())
            body.velocity += gravity * static_cast<float> (deltaSeconds);
    }

    yup::Vector3<float> gravity { 0.0f, -9.81f, 0.0f };
};

scene->addSystem<Wind> (yup::SystemOrder().beforeNodes());
scene->addSystem<Gravity> (yup::SystemOrder()
                               .beforeNodes() // the default, or afterNodes()
                               .before<Physics>()
                               .after<Wind>());
```

`SystemOrder` is built with chained calls: a phase, then any number of
`before<T>()` and `after<T>()` constraints. The arguments after it go to the
system's constructor. `T` is a `System` type, or a `Node` type standing for the
updates of its nodes.
`scene->addSystemRule<First, Then>()` adds the same rules on their own, and
also orders node types against each other:

```cpp
scene->addSystemRule<Spinner, Follower>(); // spinners update before followers
```

Rules are about types: they hold for every system or node of those types,
including ones added later, and chain (A before B and B before C put A before C).
Otherwise systems run in the order they were added, and node types in the order
they were first attached. Rules order within a phase only; a cycle or a rule
contradicting the phases asserts. `removeSystem()` removes a system, also while
updating.

## Prefabs

A `Prefab` is a reference-counted template of an entity tree, which
`instantiate()` builds again each time, in one or several scenes. It is made of
parts: each part becomes an entity named like it, with the nodes it attaches
and its child parts.

```cpp
auto lamp = yup::Prefab::Ptr (new yup::Prefab ("lamp"));
lamp->getRoot()
    .attach<yup::TransformNode> (yup::Vector3<float> (0.0f, 2.0f, 0.0f))
    .attach<yup::MeshNode> (lampMesh);

lamp->getRoot().addChild ("bulb")
    .attachWith<yup::LightNode> ([] (yup::LightNode& light) { light.intensity = 4.0f; });

yup::Entity first = lamp->instantiate (*scene);
yup::Entity second = lamp->instantiate (*scene, first); // as a child of first
```

`attach<T> (args...)` copies its arguments into the prefab and constructs a new
node from them for every instance, so pass shared resources by their `Ptr`:
every lamp draws the same `lampMesh`. `attachWith<T> (setup)` default-constructs
the node and lets `setup` configure it. Build a prefab completely before using
it: instances don't follow later changes. glTF assets are loaded as prefabs, see
[Loading glTF](meshes-and-gltf.md#loading-gltf).

## Upgrading from `EntityNode`

| Before                                                 | Now                                                         |
| ------------------------------------------------------ | ----------------------------------------------------------- |
| `Entity::Ptr (new Entity ("a"))` + `addChild`          | `scene->createEntity ("a", parent)`                         |
| `removeChild (child)` dropping the last reference      | `child.destroy()`                                           |
| `entity->setPosition / setRotation / setScale`         | `entity.getOrAttach<TransformNode>().setPosition (...)`     |
| `getNode<T>()`                                         | `get<T>()`                                                  |
| `attach<T, N>()`, `attachAt<T>()`, slots               | One node per type, child entities for more                  |
| `forEachNode (...)`                                    | `scene->view<T>()` or `entity.forEachAttachedNode (...)`    |
| `scene->getRoot()->...`                                | `scene->getRoot()....`                                      |
| `model.createEntity()`                                 | `model.instantiate (*scene, parent)`                        |
| `setActiveCamera (entity, slot)`                       | `setActiveCamera (entity)`                                  |
