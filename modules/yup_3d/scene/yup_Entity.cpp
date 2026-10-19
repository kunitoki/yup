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

Entity::Entity (Scene* owner, EntityHandle entityHandle) noexcept
    : scene (owner)
    , handle (entityHandle)
{
}

EntityStore* Entity::findStore() const noexcept
{
    if (isValid())
        return scene->store.get();

    jassertfalse; // Using an entity that was destroyed or never created
    return nullptr;
}

bool Entity::isValid() const noexcept
{
    return scene != nullptr && scene->store->find (handle) != nullptr;
}

//==============================================================================

String Entity::getName() const
{
    if (auto* store = findStore())
        return store->get (handle.index).name;

    return {};
}

void Entity::setName (const String& newName)
{
    if (auto* store = findStore())
        store->get (handle.index).name = newName;
}

Uuid Entity::getUuid() const
{
    auto* store = findStore();
    if (store == nullptr)
        return Uuid::null();

    auto& record = store->get (handle.index);
    if (record.uuid.isNull())
    {
        record.uuid = store->createUuid();
        scene->entitiesByUuid.emplace (record.uuid, handle);
    }

    return record.uuid;
}

bool Entity::setUuid (const Uuid& newUuid)
{
    auto* store = findStore();
    if (store == nullptr)
        return false;

    auto& record = store->get (handle.index);
    if (record.uuid == newUuid)
        return true;

    if (newUuid.isNull() || scene->entitiesByUuid.count (newUuid) != 0)
    {
        jassertfalse; // A null identifier, or one used by another entity of the scene
        return false;
    }

    if (! record.uuid.isNull())
        scene->entitiesByUuid.erase (record.uuid);

    record.uuid = newUuid;
    scene->entitiesByUuid.emplace (newUuid, handle);
    return true;
}

bool Entity::isVisible() const noexcept
{
    if (auto* store = findStore())
        return store->isVisible (handle.index);

    return false;
}

void Entity::setVisible (bool shouldBeVisible) noexcept
{
    if (auto* store = findStore())
        store->setVisible (handle.index, shouldBeVisible);
}

bool Entity::isActive() const noexcept
{
    return isValid() && scene->store->isActive (handle.index);
}

//==============================================================================

Entity Entity::createChild (const String& childName)
{
    if (findStore() == nullptr)
        return {};

    return scene->createEntity (childName, *this);
}

bool Entity::addChild (Entity child, int index)
{
    auto* store = findStore();
    if (store == nullptr)
        return false;

    if (child.scene != scene || ! child.isValid() || child.handle == scene->root)
    {
        jassertfalse; // Adding an invalid entity, the root, or an entity of another scene
        return false;
    }

    if (! store->setParent (child.handle.index, handle.index, index))
    {
        jassertfalse; // Adding an entity under itself or under one of its own descendants
        return false;
    }

    return true;
}

bool Entity::removeChild (Entity child)
{
    auto* store = findStore();
    if (store == nullptr || child.scene != scene || ! child.isValid() || store->get (child.handle.index).parent != handle.index)
        return false;

    store->unlink (child.handle.index);
    return true;
}

void Entity::removeFromParent()
{
    if (auto* store = findStore())
        store->unlink (handle.index);
}

Entity Entity::getParent() const noexcept
{
    auto* store = findStore();
    if (store == nullptr)
        return {};

    const auto parent = store->get (handle.index).parent;
    return parent != EntityHandle::nullIndex ? Entity (scene, store->getHandle (parent)) : Entity();
}

int Entity::getNumChildren() const noexcept
{
    if (auto* store = findStore())
        return static_cast<int> (store->get (handle.index).children.size());

    return 0;
}

Entity Entity::getChild (int index) const noexcept
{
    auto* store = findStore();
    if (store == nullptr)
        return {};

    const auto& children = store->get (handle.index).children;
    if (! isPositiveAndBelow (index, static_cast<int> (children.size())))
        return {};

    return { scene, store->getHandle (children[static_cast<size_t> (index)]) };
}

Entity Entity::findChild (StringRef childName, bool recursive) const
{
    for (const auto child : getChildren())
    {
        if (child.getName() == childName)
            return child;

        if (recursive)
        {
            if (auto found = child.findChild (childName, true))
                return found;
        }
    }

    return {};
}

bool Entity::isAncestorOf (const Entity& other) const noexcept
{
    if (other.scene != scene || ! isValid() || ! other.isValid())
        return false;

    return scene->store->isAncestorOf (handle.index, other.handle.index);
}

//==============================================================================

Matrix4 Entity::getLocalMatrix() const noexcept
{
    if (auto* transform = get<TransformNode>())
        return transform->getLocalMatrix();

    return {};
}

Matrix4 Entity::getWorldMatrix() const noexcept
{
    auto* store = findStore();
    if (store == nullptr)
        return {};

    return store->getWorldMatrix (handle.index);
}

void Entity::setTransform (const TransformNode* transform) const noexcept
{
    if (isValid())
        scene->store->setTransform (handle.index, transform);
}

void Entity::invalidateWorldMatrix() const noexcept
{
    if (isValid())
        scene->store->markWorldDirty (handle.index);
}

static void expandEntityWorldBounds (Entity entity, const Matrix4& world, BoundingBox& bounds)
{
    if (! entity.isVisible())
        return;

    if (auto* node = entity.get<MeshNode>(); node != nullptr && node->mesh != nullptr)
        bounds.expand (node->mesh->getBounds().transformedBy (world));

    for (const auto child : entity.getChildren())
        expandEntityWorldBounds (child, child.getLocalMatrix().followedBy (world), bounds);
}

BoundingBox Entity::computeWorldBounds() const
{
    BoundingBox bounds;

    if (isValid())
        expandEntityWorldBounds (*this, getWorldMatrix(), bounds);

    return bounds;
}

//==============================================================================

void Entity::destroy()
{
    if (isValid())
        scene->destroyEntity (handle);
}

} // namespace yup
