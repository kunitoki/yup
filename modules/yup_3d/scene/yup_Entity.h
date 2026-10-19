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
class Node;
class EntityStore;
class TransformNode;

//==============================================================================
/** An element of a 3D scene: a lightweight handle to an entity owned by a Scene.

    An entity has a name, a local transform (translation, rotation and scale), a visibility
    flag, child entities and attached nodes. The nodes give it its behavior: attach a MeshNode
    to draw geometry, a CameraNode or a LightNode to place a camera or a light, a MaterialNode
    to override the materials of the whole subtree, or any custom Node subclass. An entity
    without nodes is a plain group.

    @code
    yup::Entity body = scene->createEntity ("body");
    body.attach<yup::MeshNode> (bodyMesh);

    yup::Entity detail = body.createChild ("detail");
    detail.attach<yup::MeshNode> (detailMesh);

    if (auto* mesh = body.get<yup::MeshNode>())
        mesh->mesh = otherMesh;
    @endcode

    Entities only exist inside a Scene, which owns them and their nodes. An Entity value is
    cheap to copy and does not keep anything alive: once the entity is destroyed, every copy
    becomes invalid (isValid() returns false), even after the scene reuses its storage for a
    new entity. Using an invalid entity asserts, getters then return defaults and setters do
    nothing. attach() requires a valid entity.

    An entity holds at most one node of each type: use child entities for several meshes or
    lights. Node addresses never change while attached, so references returned by attach()
    and get() stay valid until the node is detached or its entity destroyed; NodeHandle keeps
    a checked reference for longer.

    Like Component, entities are meant to be used from the message thread only.

    @see Node, NodeHandle, Scene, GltfModel
*/
class YUP_API Entity
{
public:
    //==============================================================================
    /** Creates an invalid entity. */
    Entity() noexcept = default;

    /** Creates a handle to an entity of a scene. Use Scene::createEntity() to create entities. */
    Entity (Scene* owner, EntityHandle entityHandle) noexcept;

    //==============================================================================
    /** Returns the scene the entity belongs to, or nullptr for a default constructed entity. */
    Scene* getScene() const noexcept { return scene; }

    /** Returns the versioned identifier of the entity inside its scene. */
    EntityHandle getHandle() const noexcept { return handle; }

    /** Returns true if the entity exists: it was created and has not been destroyed. */
    bool isValid() const noexcept;

    /** Returns true if the entity exists. */
    explicit operator bool() const noexcept { return isValid(); }

    bool operator== (const Entity& other) const noexcept { return scene == other.scene && handle == other.handle; }
    bool operator!= (const Entity& other) const noexcept { return ! operator== (other); }

    //==============================================================================
    /** Returns the name of the entity. */
    String getName() const;

    /** Changes the name of the entity. */
    void setName (const String& newName);

    /** Returns the universally unique identifier of the entity, or a null Uuid when the entity
        is not valid.

        The identifier is created on the first call, so entities that are never asked for one
        cost nothing. It stays the same until the entity is destroyed, and instantiating a
        Prefab gives every new entity its own. Use Scene::findEntity() to look an entity up.

        @see setUuid, Scene::findEntity, Node::getUuid
    */
    Uuid getUuid() const;

    /** Replaces the identifier of the entity, for example to restore a saved scene.

        @return False if the identifier is null or already used by another entity of the scene.
    */
    bool setUuid (const Uuid& newUuid);

    /** Returns false if the entity, and so its whole subtree, is hidden. */
    bool isVisible() const noexcept;

    /** Shows or hides the entity and its whole subtree. Hidden entities are neither drawn nor updated. */
    void setVisible (bool shouldBeVisible) noexcept;

    /** Returns true if the entity and all its parents are visible, up to the root of the scene.

        Entities that were removed from the tree are not active: they are neither drawn nor updated.
    */
    bool isActive() const noexcept;

    //==============================================================================
    /** Creates a new entity as the last child of this one.

        @param childName The name of the new entity.
    */
    Entity createChild (const String& childName = {});

    /** Moves an entity of the same scene under this one.

        If the child already has a parent it is moved here, even when that parent is this
        entity. Adding an entity under itself or under one of its own descendants would create
        a cycle, and adding the root of the scene is not allowed: both are rejected.

        @param child  The entity to move.
        @param index  The position among the children, or -1 to add it last.

        @return True if the child has been moved.
    */
    bool addChild (Entity child, int index = -1);

    /** Removes a child, which stays alive outside of the tree until it is destroyed or added back.

        @return True if it was a child of this entity and has been removed.
    */
    bool removeChild (Entity child);

    /** Removes this entity from its parent, if it has one. */
    void removeFromParent();

    /** Returns the parent entity, or an invalid entity for the root and for entities outside of the tree. */
    Entity getParent() const noexcept;

    /** Returns the number of children. */
    int getNumChildren() const noexcept;

    /** Returns a child, or an invalid entity if the index is out of range. */
    Entity getChild (int index) const noexcept;

    //==============================================================================
    class Children;

    /** Returns the children, in order.

        The range counts the children when it is created: children removed while iterating
        show up as invalid entities, children added while iterating are not visited.
    */
    Children getChildren() const noexcept;

    /** Returns the first child with a name, or an invalid entity.

        @param childName  The name to look for.
        @param recursive  True to search the whole subtree depth-first, false for direct children only.
    */
    Entity findChild (StringRef childName, bool recursive = true) const;

    /** Returns true if this entity is a parent, grandparent, etc. of another entity. */
    bool isAncestorOf (const Entity& other) const noexcept;

    //==============================================================================
    /** Returns the transform relative to the parent: the one of the TransformNode, or the
        identity when the entity has none and so follows its parent.
    */
    Matrix4 getLocalMatrix() const noexcept;

    /** Returns the transform from this entity's space to the space of the scene.

        World matrices are cached: changing a TransformNode, or moving an entity in the tree,
        only marks its subtree, which is computed again when asked for.
    */
    Matrix4 getWorldMatrix() const noexcept;

    /** Returns the world space bounds of every MeshNode in the visible part of this subtree.

        The result is empty when the entity is hidden or the subtree has no meshes.
    */
    BoundingBox computeWorldBounds() const;

    //==============================================================================
    /** Destroys the entity, its whole subtree and all their nodes.

        Scene::onDestroy() is emitted for each entity before its children are destroyed. While
        the scene is iterating (a view, a collection, Scene::update()) the entities are gone at
        once, but the node objects are destroyed when the iteration ends. The root of the scene
        can't be destroyed.
    */
    void destroy();

    //==============================================================================
    /** Attaches a node, replacing the node of the same type if there is one.

        The entity must be valid. Node::attachedToEntity() is called and Scene::onAttach() is
        emitted once the node is attached.

        @tparam T     The node type, a Node subclass.
        @param args   The arguments forwarded to the constructor of T.

        @return The new node, its address doesn't change until it is detached.
    */
    template <class T, class... Args>
    T& attach (Args&&... args);

    /** Returns the node of type T, attaching one first if the entity has none.

        The arguments are forwarded to the constructor of T only when a node is attached, an
        existing node is returned unchanged.

        @code
        entity.getOrAttach<yup::TransformNode>().setPosition ({ 0.0f, 1.0f, 0.0f });
        @endcode

        @tparam T     The node type, a Node subclass.
        @param args   The arguments forwarded to the constructor of T, if a node is attached.

        @return The node, its address doesn't change until it is detached.

        @see attach, get
    */
    template <class T, class... Args>
    T& getOrAttach (Args&&... args);

    /** Returns the node of type T, or nullptr. Only the exact type T matches. */
    template <class T>
    T* get() const noexcept;

    /** Returns true if the entity has a node of type T. */
    template <class T>
    bool has() const noexcept
    {
        return get<T>() != nullptr;
    }

    /** Detaches and destroys the node of type T.

        @return True if there was a node to detach.

        @see destroy
    */
    template <class T>
    bool detach();

    /** Calls a function with every attached node, whatever its type.

        @param fn A callable taking (Node& node).
    */
    template <class Fn>
    void forEachAttachedNode (Fn&& fn) const;

private:
    friend class TransformNode;

    EntityStore* findStore() const noexcept;
    void setTransform (const TransformNode* transform) const noexcept;
    void invalidateWorldMatrix() const noexcept;

    Scene* scene = nullptr;
    EntityHandle handle;
};

//==============================================================================
/** The children of an entity, in order, as returned by Entity::getChildren(). */
class Entity::Children
{
public:
    class Iterator
    {
    public:
        Iterator (Entity parentEntity, int childIndex) noexcept : parent (parentEntity), index (childIndex) {}

        Entity operator*() const noexcept { return parent.getChild (index); }
        Iterator& operator++() noexcept { ++index; return *this; }
        bool operator!= (const Iterator& other) const noexcept { return index != other.index; }

    private:
        Entity parent;
        int index = 0;
    };

    explicit Children (Entity parentEntity) noexcept : parent (parentEntity), count (parentEntity.getNumChildren()) {}

    Iterator begin() const noexcept { return { parent, 0 }; }
    Iterator end() const noexcept { return { parent, count }; }

    /** Returns the number of children when the range was created. */
    int size() const noexcept { return count; }

    /** Returns a child, or an invalid entity if the index is out of range. */
    Entity operator[] (int index) const noexcept { return parent.getChild (index); }

private:
    Entity parent;
    int count = 0;
};

inline Entity::Children Entity::getChildren() const noexcept
{
    return Children (*this);
}

} // namespace yup
