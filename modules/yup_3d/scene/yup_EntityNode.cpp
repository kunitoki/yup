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

EntityNode::EntityNode (const String& entityName)
    : name (entityName)
{
}

EntityNode::~EntityNode()
{
    for (auto& child : children)
        child->parent = nullptr;
}

//==============================================================================

void EntityNode::addChild (Ptr child, int index)
{
    if (child == nullptr || child->isAncestorOf (this) || child.get() == this)
    {
        jassertfalse; // Adding a null entity, or an entity under itself or its own descendant
        return;
    }

    if (child->parent != nullptr)
        child->parent->removeChild (child.get());

    const auto insertAt = isPositiveAndNotGreaterThan (index, getNumChildren()) ? index : getNumChildren();

    child->parent = this;
    children.insert (children.begin() + insertAt, std::move (child));
}

bool EntityNode::removeChild (EntityNode* child)
{
    const auto it = std::find_if (children.begin(), children.end(), [child] (const Ptr& p)
    {
        return p.get() == child;
    });

    if (it == children.end())
        return false;

    // Keep the child alive until it is fully detached
    const Ptr removed = *it;
    children.erase (it);
    removed->parent = nullptr;
    return true;
}

void EntityNode::removeFromParent()
{
    if (parent != nullptr)
        parent->removeChild (this);
}

EntityNode* EntityNode::getChild (int index) const noexcept
{
    return isPositiveAndBelow (index, getNumChildren()) ? children[static_cast<size_t> (index)].get() : nullptr;
}

EntityNode* EntityNode::findChild (StringRef childName, bool recursive) const
{
    for (const auto& child : children)
    {
        if (child->getName() == childName)
            return child.get();

        if (recursive)
        {
            if (auto* found = child->findChild (childName, true))
                return found;
        }
    }

    return nullptr;
}

bool EntityNode::isAncestorOf (const EntityNode* other) const noexcept
{
    for (auto* p = other != nullptr ? other->parent : nullptr; p != nullptr; p = p->parent)
    {
        if (p == this)
            return true;
    }

    return false;
}

//==============================================================================

void EntityNode::setPosition (const Vector3<float>& newPosition) noexcept
{
    position = newPosition;
    localMatrixDirty = true;
}

void EntityNode::setRotation (const Quaternion& newRotation) noexcept
{
    rotation = newRotation;
    localMatrixDirty = true;
}

void EntityNode::setScale (const Vector3<float>& newScale) noexcept
{
    scale = newScale;
    localMatrixDirty = true;
}

const Matrix4& EntityNode::getLocalMatrix() const noexcept
{
    if (localMatrixDirty)
    {
        localMatrix = Matrix4::scaling (scale)
                          .followedBy (rotation.toMatrix4())
                          .followedBy (Matrix4::translation (position));

        localMatrixDirty = false;
    }

    return localMatrix;
}

Matrix4 EntityNode::getWorldMatrix() const noexcept
{
    auto world = getLocalMatrix();

    for (auto* p = parent; p != nullptr; p = p->parent)
        world = world.followedBy (p->getLocalMatrix());

    return world;
}

static void expandEntityWorldBounds (const EntityNode& entity, const Matrix4& world, BoundingBox& bounds)
{
    if (! entity.isVisible())
        return;

    entity.forEachNode<MeshNode> ([&] (MeshNode& node, int)
    {
        if (node.mesh != nullptr)
            bounds.expand (node.mesh->getBounds().transformedBy (world));
    });

    for (const auto& child : entity.getChildren())
        expandEntityWorldBounds (*child, child->getLocalMatrix().followedBy (world), bounds);
}

BoundingBox EntityNode::computeWorldBounds() const
{
    BoundingBox bounds;
    expandEntityWorldBounds (*this, getWorldMatrix(), bounds);
    return bounds;
}

//==============================================================================

Node* EntityNode::findNode (const void* typeKey, int slot) const noexcept
{
    for (const auto& entry : nodes)
    {
        if (entry.typeKey == typeKey && entry.slot == slot)
            return entry.node.get();
    }

    return nullptr;
}

void EntityNode::insertNode (const void* typeKey, int slot, std::unique_ptr<Node> node)
{
    jassert (slot >= 0);

    node->entity = this;
    auto* attached = node.get();

    const auto less = std::less<const void*>();
    const auto it = std::find_if (nodes.begin(), nodes.end(), [&] (const Entry& entry)
    {
        return less (typeKey, entry.typeKey) || (entry.typeKey == typeKey && entry.slot >= slot);
    });

    if (it != nodes.end() && it->typeKey == typeKey && it->slot == slot)
        it->node = std::move (node);
    else
        nodes.insert (it, Entry { typeKey, slot, std::move (node) });

    attached->attachedToEntity();
}

bool EntityNode::eraseNode (const void* typeKey, int slot)
{
    const auto it = std::find_if (nodes.begin(), nodes.end(), [&] (const Entry& entry)
    {
        return entry.typeKey == typeKey && entry.slot == slot;
    });

    if (it == nodes.end())
        return false;

    auto removed = std::move (it->node);
    nodes.erase (it);
    return true;
}

} // namespace yup
