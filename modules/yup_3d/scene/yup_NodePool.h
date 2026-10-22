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

class Scene;

//==============================================================================
/** The storage of the nodes of one type in a Scene, without knowing the type.

    Nodes live in fixed size pages and never move. A sparse array maps the index of an entity
    to a slot, so finding, attaching and detaching a node are constant time. Freed slots are
    reused, except while the scene is iterating: then new nodes are appended, and detached
    nodes are only destroyed when the iteration ends.

    The Scene owns one pool per node type, see NodePool.

    @see NodePool, Scene
*/
class YUP_API NodePoolBase
{
public:
    //==============================================================================
    /** Destroys every node still in the pool, without calling Node::detachedFromEntity(). */
    virtual ~NodePoolBase();

    //==============================================================================
    /** Returns the NodeTypeId of the nodes. */
    uint64 getTypeId() const noexcept { return typeId; }

    /** Returns the NodeTypeId name of the nodes. */
    std::string_view getTypeName() const noexcept { return typeName; }

    //==============================================================================
    /** Returns the node of an entity, or nullptr. */
    Node* find (EntityHandle entity) const noexcept
    {
        if (entity.index >= sparse.size() || sparse[entity.index] == 0)
            return nullptr;

        const auto slot = sparse[entity.index] - 1;
        return handles[slot] == entity ? nodes[slot] : nullptr;
    }

    /** Returns the number of attached nodes. */
    int size() const noexcept { return numNodes; }

    /** Returns the number of slots, used or free. Slots are never removed, so iterating up
        to the number of slots counted when starting skips nodes attached while iterating.
    */
    uint32 getNumSlots() const noexcept { return static_cast<uint32> (handles.size()); }

    /** Returns the entity of the node in a slot, or the null handle for a free slot. */
    EntityHandle getEntityAt (uint32 slot) const noexcept { return handles[slot]; }

    /** Returns the node in a slot, or nullptr for a free slot. */
    Node* getNodeAt (uint32 slot) const noexcept { return handles[slot].isNull() ? nullptr : nodes[slot]; }

    /** Returns true if the node type overrides Node::update(). */
    bool updatesNodes() const noexcept { return hasUpdate; }

    //==============================================================================
    /** Detaches the node of an entity and destroys it, now or when the scene stops iterating.

        @return True if the entity had a node in this pool.
    */
    virtual bool remove (EntityHandle entity) = 0;

    /** Destroys the nodes detached while the scene was iterating, called when it stops. */
    void destroyPending();

protected:
    //==============================================================================
    NodePoolBase (Scene& owner, uint64 nodeTypeId, std::string_view nodeTypeName, size_t nodeSize, size_t nodeAlignment, bool nodesUpdate);

    uint32 acquireSlot();
    void* getStorage (uint32 slot) const noexcept;
    void link (uint32 slot, EntityHandle entity, Node& node);
    uint32 unlink (EntityHandle entity) noexcept;
    Node& getStoredNode (uint32 slot) const noexcept { return *nodes[slot]; }
    void release (uint32 slot);

    Scene& scene;

private:
    //==============================================================================
    static constexpr uint32 nodesPerPage = 64;

    const uint64 typeId;
    const std::string_view typeName;
    const size_t stride;
    const size_t alignment;
    const bool hasUpdate;

    std::vector<uint32> sparse;
    std::vector<EntityHandle> handles;
    std::vector<Node*> nodes;
    std::vector<uint32> freeSlots;
    std::vector<uint32> pendingSlots;
    std::vector<std::byte*> pages;
    int numNodes = 0;

    YUP_DECLARE_NON_COPYABLE (NodePoolBase)
};

//==============================================================================
/** The storage of the nodes of type T in a Scene, with its attach and detach signals.

    Use Scene::view() and Scene::collect() to iterate nodes, and Entity to attach them: pools
    are the building block underneath.

    @see NodePoolBase, Scene
*/
template <class T>
class NodePool final : public NodePoolBase
{
public:
    static_assert (std::is_base_of_v<Node, T>, "Nodes must derive from yup::Node");

    //==============================================================================
    /** Creates the pool of a scene, use Scene::getPool() instead. */
    explicit NodePool (Scene& owner)
        : NodePoolBase (owner, NodeTypeId::get<T>(), NodeTypeId::getName<T>(), sizeof (T), alignof (T), ! std::is_same_v<decltype (&T::update), void (Node::*) (double)>)
    {
    }

    //==============================================================================
    /** Attaches a new node to an entity, replacing its node of this type if it has one. */
    template <class... Args>
    T& attach (EntityHandle entity, Args&&... args)
    {
        remove (entity);

        const auto slot = acquireSlot();
        auto* node = new (getStorage (slot)) T (std::forward<Args> (args)...);
        link (slot, entity, *node);

        attachSignal.emit (Entity (&scene, entity), *node);
        return *node;
    }

    /** Returns the node of an entity, or nullptr. */
    T* get (EntityHandle entity) const noexcept
    {
        return static_cast<T*> (find (entity));
    }

    /** Returns the node in a slot, or nullptr for a free slot. */
    T* getAt (uint32 slot) const noexcept
    {
        return static_cast<T*> (getNodeAt (slot));
    }

    bool remove (EntityHandle entity) override
    {
        const auto slot = unlink (entity);
        if (slot == EntityHandle::nullIndex)
            return false;

        detachSignal.emit (Entity (&scene, entity), static_cast<T&> (getStoredNode (slot)));
        release (slot);
        return true;
    }

    //==============================================================================
    /** Emitted once a node has been attached. */
    Signal<Entity, T&> attachSignal;

    /** Emitted once a node has been detached, before it is destroyed. */
    Signal<Entity, T&> detachSignal;
};

} // namespace yup
