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
/** Base class for the nodes attached to an Entity.

    A node gives an entity its behavior: TransformNode places it, MeshNode draws geometry,
    CameraNode and LightNode describe a camera and a light, MaterialNode overrides the
    materials of a subtree. Nodes are not tree children: an entity holds at most one node of
    each type, and they follow it wherever it moves in the tree.

    Subclass Node to create custom nodes and attach them with Entity::attach().

    @code
    class Spinner : public yup::Node
    {
    public:
        void update (double deltaSeconds) override
        {
            angle += static_cast<float> (deltaSeconds);
            if (auto* transform = getEntity().get<yup::TransformNode>())
                transform->setRotation (yup::Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, angle));
        }

        float angle = 0.0f;
    };

    entity.attach<Spinner>();
    @endcode

    The Scene stores the nodes of each type together, and a node never moves while it is
    attached. Detaching a node, or destroying its entity, destroys it: at once, or when the
    current iteration of the scene ends (see Entity::destroy()). Nodes still attached when the
    scene is destroyed are destroyed without detachedFromEntity() being called.

    Scene::update() only visits the node types that override update(), so plain data nodes
    cost nothing per frame.

    Like Component, nodes are meant to be used from the message thread only.

    @see Entity, NodeHandle, Scene
*/
class YUP_API Node
{
public:
    //==============================================================================
    /** Constructs a node that is not attached yet. */
    Node() = default;

    /** Destructor. */
    virtual ~Node() = default;

    //==============================================================================
    /** Returns the entity this node is attached to, or an invalid entity when it is not attached. */
    Entity getEntity() const noexcept { return entity; }

    /** Returns the universally unique identifier of the node, or a null Uuid when it is not
        attached.

        An entity has at most one node of each type, so the identifier is derived from the
        identifier of the entity and the NodeTypeId of the node (a RFC 9562 version 8 Uuid):
        nothing is stored per node, and it is the same after the scene is saved and restored.
        Detaching a node and attaching a new one of the same type gives the same identifier.

        @see Entity::getUuid
    */
    Uuid getUuid() const;

    //==============================================================================
    /** Called once the node has been attached to its entity, getEntity() is valid from here. */
    virtual void attachedToEntity() {}

    /** Called once the node has been detached from its entity, or its entity destroyed, just
        before getEntity() becomes invalid. The node is destroyed right after, or when the
        current iteration of the scene ends.
    */
    virtual void detachedFromEntity() {}

    /** Called by Scene::update() for every node of every active entity.

        @param deltaSeconds The time elapsed since the previous update.
    */
    virtual void update ([[maybe_unused]] double deltaSeconds) {}

private:
    friend class NodePoolBase;

    Entity entity;
    uint64 typeId = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Node)
};

} // namespace yup
