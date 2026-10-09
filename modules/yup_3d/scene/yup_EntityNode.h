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

//==============================================================================
/** An element of a 3D scene tree.

    An entity has a name, a local transform (translation, rotation and scale), a visibility
    flag, child entities and attached parts. The parts give it its behavior: attach a MeshNode
    to draw geometry, a CameraNode or a LightNode to place a camera or a light, a MaterialNode
    to override the materials of the whole subtree, or any custom Node subclass. An entity
    without parts is a plain group.

    Parts are addressed by their exact type and a slot, so an entity can hold several parts of
    the same type:

    @code
    auto entity = yup::EntityNode::Ptr (new yup::EntityNode ("body"));
    entity->attach<yup::MeshNode> (bodyMesh);       // slot 0
    entity->attach<yup::MeshNode, 1> (detailMesh);  // slot 1

    if (auto* mesh = entity->getNode<yup::MeshNode, 1>())
        mesh->mesh = otherMesh;
    @endcode

    Entities are reference counted: children are held by their parent, and a subtree stays
    alive as long as anything references its root. Adding an entity that already has a parent
    moves it. Entities can be built anywhere, also before they are part of a Scene.

    Like Component, entities are meant to be used from the message thread only.

    @see Node, Scene, GltfModel
*/
class YUP_API EntityNode : public ReferenceCountedObject
{
public:
    //==============================================================================
    using Ptr = ReferenceCountedObjectPtr<EntityNode>;

    //==============================================================================
    /** Creates an entity.

        @param entityName The name of the entity.
    */
    explicit EntityNode (const String& entityName = {});

    /** Destructor. The children lose their parent, the parts are destroyed. */
    ~EntityNode() override;

    //==============================================================================
    /** Returns the name of the entity. */
    const String& getName() const noexcept { return name; }

    /** Changes the name of the entity. */
    void setName (const String& newName) { name = newName; }

    /** Returns false if the entity, and so its whole subtree, is hidden. */
    bool isVisible() const noexcept { return visible; }

    /** Shows or hides the entity and its whole subtree. Hidden entities are neither drawn nor updated. */
    void setVisible (bool shouldBeVisible) noexcept { visible = shouldBeVisible; }

    //==============================================================================
    /** Adds a child entity.

        If the child already has a parent it is moved here. Adding an entity under itself or
        under one of its own descendants would create a cycle: it is rejected.

        @param child  The entity to add.
        @param index  The position among the children, or -1 to add it last.
    */
    void addChild (Ptr child, int index = -1);

    /** Removes a child entity.

        @param child The child to remove.

        @return True if it was a child of this entity and has been removed.
    */
    bool removeChild (EntityNode* child);

    /** Removes this entity from its parent, if it has one. */
    void removeFromParent();

    /** Returns the parent entity, or nullptr for a root. */
    EntityNode* getParent() const noexcept { return parent; }

    /** Returns the number of children. */
    int getNumChildren() const noexcept { return static_cast<int> (children.size()); }

    /** Returns a child, or nullptr if the index is out of range. */
    EntityNode* getChild (int index) const noexcept;

    /** Returns every child, in order. */
    Span<const Ptr> getChildren() const noexcept { return children; }

    /** Returns the first child with a name, or nullptr.

        @param childName  The name to look for.
        @param recursive  True to search the whole subtree depth-first, false for direct children only.
    */
    EntityNode* findChild (StringRef childName, bool recursive = true) const;

    /** Returns true if this entity is a parent, grandparent, etc. of another entity. */
    bool isAncestorOf (const EntityNode* other) const noexcept;

    //==============================================================================
    /** Returns the translation relative to the parent. */
    const Vector3<float>& getPosition() const noexcept { return position; }

    /** Changes the translation relative to the parent. */
    void setPosition (const Vector3<float>& newPosition) noexcept;

    /** Returns the rotation relative to the parent. */
    const Quaternion& getRotation() const noexcept { return rotation; }

    /** Changes the rotation relative to the parent. */
    void setRotation (const Quaternion& newRotation) noexcept;

    /** Returns the scale relative to the parent. */
    const Vector3<float>& getScale() const noexcept { return scale; }

    /** Changes the scale relative to the parent. */
    void setScale (const Vector3<float>& newScale) noexcept;

    /** Returns the local transform: scale, then rotation, then translation.

        The matrix is cached and only computed again after the transform changes.
    */
    const Matrix4& getLocalMatrix() const noexcept;

    /** Returns the transform from this entity's space to the space of its root.

        This walks up through every parent; the renderer instead accumulates world matrices
        top-down while it traverses the tree.
    */
    Matrix4 getWorldMatrix() const noexcept;

    /** Returns the world space bounds of every MeshNode in the visible part of this subtree.

        The result is empty when the entity is hidden or the subtree has no meshes.
    */
    BoundingBox computeWorldBounds() const;

    //==============================================================================
    /** Attaches a part in a slot, replacing whatever part of the same type was there.

        @tparam T     The part type, a Node subclass.
        @tparam Slot  The slot, 0 or above.
        @param args   The arguments forwarded to the constructor of T.

        @return The new part.
    */
    template <class T, int Slot = 0, class... Args>
    T& attach (Args&&... args)
    {
        return attachAt<T> (Slot, std::forward<Args> (args)...);
    }

    /** Returns the part of type T in a slot, or nullptr. Only the exact type T matches. */
    template <class T, int Slot = 0>
    T* getNode() const noexcept
    {
        return getNodeAt<T> (Slot);
    }

    /** Destroys the part of type T in a slot.

        @return True if there was a part to destroy.
    */
    template <class T, int Slot = 0>
    bool detach()
    {
        return detachAt<T> (Slot);
    }

    /** Attaches a part in a slot chosen at runtime, replacing whatever part of the same type was there.

        @param slot  The slot, 0 or above.
        @param args  The arguments forwarded to the constructor of T.

        @return The new part.
    */
    template <class T, class... Args>
    T& attachAt (int slot, Args&&... args)
    {
        static_assert (std::is_base_of_v<Node, T>, "Parts must derive from yup::Node");

        auto node = std::make_unique<T> (std::forward<Args> (args)...);
        auto& result = *node;
        insertNode (getTypeKey<T>(), slot, std::move (node));
        return result;
    }

    /** Returns the part of type T in a slot chosen at runtime, or nullptr. */
    template <class T>
    T* getNodeAt (int slot) const noexcept
    {
        static_assert (std::is_base_of_v<Node, T>, "Parts must derive from yup::Node");

        return static_cast<T*> (findNode (getTypeKey<T>(), slot));
    }

    /** Destroys the part of type T in a slot chosen at runtime.

        @return True if there was a part to destroy.
    */
    template <class T>
    bool detachAt (int slot)
    {
        return eraseNode (getTypeKey<T>(), slot);
    }

    /** Calls a function with every part of type T, by ascending slot.

        @param fn A callable taking (T& part, int slot).
    */
    template <class T, class Fn>
    void forEachNode (Fn&& fn) const
    {
        const auto key = getTypeKey<T>();

        for (const auto& entry : nodes)
        {
            if (entry.typeKey == key)
                fn (static_cast<T&> (*entry.node), entry.slot);
        }
    }

    /** Returns the number of parts of type T. */
    template <class T>
    int getNumNodes() const noexcept
    {
        const auto key = getTypeKey<T>();
        return static_cast<int> (std::count_if (nodes.begin(), nodes.end(), [key] (const auto& entry)
        {
            return entry.typeKey == key;
        }));
    }

    /** Calls a function with every attached part, whatever its type.

        @param fn A callable taking (Node& part).
    */
    template <class Fn>
    void forEachAttachedNode (Fn&& fn) const
    {
        for (const auto& entry : nodes)
            fn (*entry.node);
    }

private:
    //==============================================================================
    struct Entry
    {
        const void* typeKey = nullptr;
        int slot = 0;
        std::unique_ptr<Node> node;
    };

    template <class T>
    static const void* getTypeKey() noexcept
    {
        // Not const: identical read-only data may be folded into one address by the linker
        static char key = 0;
        return &key;
    }

    Node* findNode (const void* typeKey, int slot) const noexcept;
    void insertNode (const void* typeKey, int slot, std::unique_ptr<Node> node);
    bool eraseNode (const void* typeKey, int slot);

    //==============================================================================
    String name;
    bool visible = true;

    EntityNode* parent = nullptr;
    std::vector<Ptr> children;
    std::vector<Entry> nodes;

    Vector3<float> position;
    Quaternion rotation;
    Vector3<float> scale { 1.0f, 1.0f, 1.0f };
    mutable Matrix4 localMatrix;
    mutable bool localMatrixDirty = false;

    YUP_DECLARE_WEAK_REFERENCEABLE (EntityNode)
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EntityNode)
};

} // namespace yup
