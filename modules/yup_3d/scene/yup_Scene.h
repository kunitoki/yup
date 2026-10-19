/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

class EntityStore;

template <class... Ts>
class NodeView;

template <class... Ts>
class NodeCollection;

//==============================================================================
/** A 3D world: the entities, their nodes, the systems and the camera that views it.

    The scene owns everything in it. Entities are created with createEntity(), under the
    root by default, and live until they are destroyed or the scene is. Build the world, then
    show it with a SceneComponent of the yup_renderer module. Scene-wide features such as the
    background, the environment or the fog are nodes of the yup_renderer module attached to
    the root, see SceneSettings there.

    @code
    auto scene = yup::Scene::Ptr (new yup::Scene());

    auto model = yup::GltfModel::loadFromFile (file);
    if (model.wasOk())
        model.getReference().instantiate (*scene);

    yup::Entity camera = scene->createEntity ("camera");
    camera.attach<yup::TransformNode> (yup::Vector3<float> { 0.0f, 0.0f, 3.0f });
    camera.attach<yup::CameraNode>();
    scene->setActiveCamera (camera);
    @endcode

    Nodes of each type are stored together, so iterating them is fast:

    @code
    for (auto [entity, light] : scene->view<yup::LightNode>())
        light.intensity *= 0.5f;

    scene->view<yup::MeshNode, yup::MaterialNode>()
         .where ([] (const yup::MeshNode& mesh, const yup::MaterialNode&) { return mesh.mesh != nullptr; })
         .forEach ([] (yup::Entity entity, yup::MeshNode& mesh, yup::MaterialNode& material) { ... });
    @endcode

    Detaching nodes and destroying entities while iterating is allowed: they disappear at once,
    and the node objects are destroyed when the outermost iteration ends.

    Like Component, scenes are meant to be used from the message thread only.

    @see Entity, Node, NodeView, NodeCollection, System
*/
class YUP_API Scene : public ReferenceCountedObject
{
public:
    //==============================================================================
    using Ptr = ReferenceCountedObjectPtr<Scene>;

    //==============================================================================
    /** Creates a scene with an empty root entity named "root". */
    Scene();

    /** Destructor. Systems, collections, nodes and entities are destroyed without signals. */
    ~Scene() override;

    //==============================================================================
    /** Returns the root entity, which always exists. */
    Entity getRoot() const noexcept;

    /** Creates a new entity.

        @param name    The name of the entity.
        @param parent  The entity to add it to as the last child, or an invalid entity for the root.
        @param uuid    The identifier to restore, for example when loading a saved scene, or a null
                       Uuid to create one when Entity::getUuid() is first called. An identifier
                       already used by another entity of the scene is rejected and the entity gets
                       a new one instead.
    */
    Entity createEntity (const String& name = {}, Entity parent = {}, const Uuid& uuid = Uuid::null());

    /** Returns the entity with this identifier, or an invalid entity if there is none.

        Only entities whose identifier has been asked for or set can be found, see Entity::getUuid().
    */
    Entity findEntity (const Uuid& uuid) const;

    /** Returns the number of entities, the root and entities outside of the tree included. */
    int getNumEntities() const noexcept;

    //==============================================================================
    /** Returns a view over the entities that have a node of every type in Ts.

        The view iterates the smallest of the pools and ignores visibility. See NodeView.
    */
    template <class... Ts>
    NodeView<Ts...> view()
    {
        static_assert (sizeof... (Ts) > 0, "A view needs at least one node type");
        return NodeView<Ts...> (*this, getPool<Ts>()...);
    }

    /** Returns the collection of the entities that have a node of every type in Ts.

        The collection is created on the first call and kept up to date as nodes are attached
        and detached, so the following calls return it at no cost. See NodeCollection.
    */
    template <class... Ts>
    NodeCollection<Ts...>& collect()
    {
        static_assert (sizeof... (Ts) > 0, "A collection needs at least one node type");

        auto& collection = collections[std::vector<uint64> { NodeTypeId::get<Ts>()... }];
        if (collection == nullptr)
            collection = std::make_shared<NodeCollection<Ts...>> (*this);

        return *std::static_pointer_cast<NodeCollection<Ts...>> (collection);
    }

    //==============================================================================
    /** Emitted once a node of type T has been attached, with its entity and the node. */
    template <class T>
    Signal<Entity, T&>& onAttach()
    {
        return getPool<T>().attachSignal;
    }

    /** Emitted once a node of type T has been detached, before it is destroyed. Destroying an
        entity detaches its nodes, so this is also emitted then.
    */
    template <class T>
    Signal<Entity, T&>& onDetach()
    {
        return getPool<T>().detachSignal;
    }

    /** Emitted when an entity is about to be destroyed, before its children and its nodes. */
    Signal<Entity>& onDestroy() noexcept { return destroySignal; }

    //==============================================================================
    /** Adds a system, run by update() where the order says.

        The order gives the phase and any SystemOrder::before() and SystemOrder::after()
        constraints against System or Node types. A constraint is the same as the matching
        addSystemRule() call, so it holds for every system of the type being added.

        @code
        scene->addSystem<Gravity> (yup::SystemOrder()
                                       .beforeNodes()
                                       .before<Physics>()
                                       .after<Wind>(),
                                   9.81f); // Gravity (float)
        @endcode

        Within a phase, systems run in the order required by the rules, and otherwise in the
        order they were added. Systems added while updating run from the next update.

        @param order  The phase and constraints of the system.
        @param args   The arguments forwarded to the constructor of T.

        @return The new system, owned by the scene.
    */
    template <class T, class... Args>
    T& addSystem (const SystemOrder& order, Args&&... args)
    {
        static_assert (std::is_base_of_v<System, T>, "Systems must derive from yup::System");

        constexpr auto typeId = NodeTypeId::get<T>();

        for (const auto& constraint : order.getConstraints())
            addSystemConstraint (typeId, constraint);

        auto system = std::make_unique<T> (std::forward<Args> (args)...);
        auto& result = *system;
        systems.push_back (std::make_unique<SystemEntry> (SystemEntry { order.getPhase(), typeId, std::move (system) }));
        scheduleDirty = true;
        return result;
    }

    /** Removes and destroys a system.

        While updating, a removed system doesn't run anymore and is destroyed once the update
        ends, so a system may remove itself or others.

        @return True if the system belonged to the scene.
    */
    bool removeSystem (const System& system);

    /** Requires the updates of type First to run before the updates of type Then.

        Both types are System or Node types: a System type stands for its systems, a Node type
        for the Node::update() calls of its nodes, so rules also order the node types.

        Rules are about types: they hold for every system and node of those types, also for ones
        added later, and are transitive, so "A before B" and "B before C" put A before C even
        without a B. They order within a phase (beforeNodes systems, node updates, afterNodes
        systems): a rule can't move a system to another phase, and a rule contradicting the
        phases asserts when update() schedules them.

        @code
        scene->addSystem<Physics> (yup::SystemOrder().beforeNodes());
        scene->addSystem<Animation> (yup::SystemOrder().beforeNodes());

        scene->addSystemRule<Animation, Physics>();   // Animation, then Physics
        scene->addSystemRule<SpinnerNode, MeshNode>(); // spinners update before meshes
        @endcode

        @return False, asserting, if the rule would make a cycle with the existing ones.
    */
    template <class First, class Then>
    bool addSystemRule()
    {
        static_assert (isOrderable<First> && isOrderable<Then>, "Rules order yup::System or yup::Node types");
        return addSystemRule (NodeTypeId::get<First>(), NodeTypeId::get<Then>());
    }

    //==============================================================================
    /** Chooses the camera that renders the scene.

        The camera is looked up again every frame, so replacing the CameraNode of the entity
        is picked up automatically.

        @param entity The entity holding the camera, or an invalid entity to use the fallback.
    */
    void setActiveCamera (Entity entity) noexcept;

    /** Returns the camera that renders the scene.

        This is the camera of the entity chosen with setActiveCamera(). When that entity was
        destroyed or has no CameraNode, it is the first CameraNode found depth-first in the
        visible part of the tree. When there is no camera at all it is nullptr, and the
        renderer uses a default perspective camera at (0, 0, 5) looking at the origin.
    */
    CameraNode* getActiveCamera() const noexcept;

    //==============================================================================
    /** Runs the systems and updates the nodes.

        First the systems added with SystemOrder::beforeNodes(), ordered as described in
        addSystem(), then Node::update() for every node of every active entity, one node type
        after the other (ordered by addSystemRule(), otherwise by first use) and only for the
        types that override it, then the systems added with SystemOrder::afterNodes().

        Nodes attached while updating are updated from the next call.

        @param deltaSeconds The time elapsed since the previous update.
    */
    void update (double deltaSeconds);

    /** Returns true while a view, a collection, update() or an IterationGuard iterates the scene. */
    bool isIterating() const noexcept { return iterationDepth > 0; }

    //==============================================================================
    /** Returns the pool storing the nodes of type T, creating it if needed. */
    template <class T>
    NodePool<T>& getPool()
    {
        constexpr auto id = NodeTypeId::get<T>();

        if (auto* pool = findPool (id))
        {
            jassert (pool->getTypeName() == NodeTypeId::getName<T>()); // Two node types have the same id: give one a nodeTypeName
            return static_cast<NodePool<T>&> (*pool);
        }

        auto& pool = pools.emplace_back (std::make_unique<NodePool<T>> (*this));
        poolsByType.emplace (id, pool.get());
        scheduleDirty = scheduleDirty || pool->updatesNodes();
        return static_cast<NodePool<T>&> (*pool);
    }

    /** Returns the pool of a NodeTypeId, or nullptr if no node of that type was ever attached. */
    NodePoolBase* findPool (uint64 typeId) const noexcept
    {
        const auto it = poolsByType.find (typeId);
        return it != poolsByType.end() ? it->second : nullptr;
    }

private:
    //==============================================================================
    friend class Entity;
    friend class IterationGuard;

    struct SystemEntry
    {
        SystemOrder::Phase phase;
        uint64 typeId;
        std::unique_ptr<System> system;
        bool removed = false;
    };

    struct UpdateStep
    {
        uint64 typeId = 0;
        SystemEntry* system = nullptr;
        NodePoolBase* pool = nullptr;
    };

    template <class T>
    static constexpr bool isOrderable = std::is_base_of_v<System, T> || std::is_base_of_v<Node, T>;

    void destroyEntity (EntityHandle entity);
    void addSystemConstraint (uint64 systemType, const SystemOrder::Constraint& constraint);
    bool addSystemRule (uint64 first, uint64 then);
    bool isOrderedBefore (uint64 first, uint64 then) const;
    void scheduleUpdates();
    void endIteration();

    std::unique_ptr<EntityStore> store;
    EntityHandle root;
    std::vector<std::unique_ptr<NodePoolBase>> pools;
    std::unordered_map<uint64, NodePoolBase*> poolsByType;
    std::unordered_map<Uuid, EntityHandle> entitiesByUuid;
    std::map<std::vector<uint64>, std::shared_ptr<void>> collections;
    std::vector<std::unique_ptr<SystemEntry>> systems;
    std::vector<std::pair<uint64, uint64>> systemRules;
    std::vector<UpdateStep> schedule;
    bool scheduleDirty = false;
    bool updatingSystems = false;
    Signal<Entity> destroySignal;
    Entity activeCamera;
    int iterationDepth = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Scene)
};

//==============================================================================

template <class T, class... Args>
T& Entity::attach (Args&&... args)
{
    static_assert (std::is_base_of_v<Node, T>, "Nodes must derive from yup::Node");
    jassert (isValid()); // Attaching to an entity that was destroyed or never created

    return scene->getPool<T>().attach (handle, std::forward<Args> (args)...);
}

template <class T, class... Args>
T& Entity::getOrAttach (Args&&... args)
{
    if (auto* node = get<T>())
        return *node;

    return attach<T> (std::forward<Args> (args)...);
}

template <class T>
T* Entity::get() const noexcept
{
    static_assert (std::is_base_of_v<Node, T>, "Nodes must derive from yup::Node");

    if (scene == nullptr)
        return nullptr;

    auto* pool = scene->findPool (NodeTypeId::get<T>());
    return pool != nullptr ? static_cast<T*> (pool->find (handle)) : nullptr;
}

template <class T>
bool Entity::detach()
{
    if (scene == nullptr)
        return false;

    auto* pool = scene->findPool (NodeTypeId::get<T>());
    return pool != nullptr && pool->remove (handle);
}

template <class Fn>
void Entity::forEachAttachedNode (Fn&& fn) const
{
    if (scene == nullptr)
        return;

    // By index: fn may attach nodes of a new type, which adds a pool
    for (size_t i = 0; i < scene->pools.size(); ++i)
    {
        if (auto* node = scene->pools[i] != nullptr ? scene->pools[i]->find (handle) : nullptr)
            fn (*node);
    }
}

} // namespace yup
