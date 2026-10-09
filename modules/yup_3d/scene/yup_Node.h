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

class EntityNode;

//==============================================================================
/** Base class for the parts attached to an EntityNode.

    A part gives an entity its behavior: MeshNode draws geometry, CameraNode and LightNode
    describe a camera and a light, MaterialNode overrides the materials of a subtree. Parts
    are not tree children: they are owned by one entity, addressed by their exact type and a
    slot, and follow it wherever it moves in the tree.

    Subclass Node to create custom parts and attach them with EntityNode::attach().

    @code
    class Spinner : public yup::Node
    {
    public:
        void update (double deltaSeconds) override
        {
            angle += static_cast<float> (deltaSeconds);
            getEntity()->setRotation (yup::Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, angle));
        }

        float angle = 0.0f;
    };

    entity->attach<Spinner>();
    @endcode

    Like Component, parts are meant to be used from the message thread only.

    @see EntityNode, Scene
*/
class YUP_API Node
{
public:
    //==============================================================================
    /** Constructs a part that is not attached yet. */
    Node() = default;

    /** Destructor. */
    virtual ~Node() = default;

    //==============================================================================
    /** Returns the entity this part is attached to, or nullptr before it is attached. */
    EntityNode* getEntity() const noexcept { return entity; }

    //==============================================================================
    /** Called once the part has been attached to its entity, getEntity() is valid from here. */
    virtual void attachedToEntity() {}

    /** Called by Scene::update() for every part of every visible entity.

        @param deltaSeconds The time elapsed since the previous update.
    */
    virtual void update ([[maybe_unused]] double deltaSeconds) {}

private:
    friend class EntityNode;

    EntityNode* entity = nullptr;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Node)
};

} // namespace yup
