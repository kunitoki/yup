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

static CameraNode* findFirstVisibleCamera (const EntityNode& entity)
{
    if (! entity.isVisible())
        return nullptr;

    CameraNode* found = nullptr;
    entity.forEachNode<CameraNode> ([&] (CameraNode& camera, int)
    {
        if (found == nullptr)
            found = &camera;
    });

    if (found != nullptr)
        return found;

    for (const auto& child : entity.getChildren())
    {
        if (auto* camera = findFirstVisibleCamera (*child))
            return camera;
    }

    return nullptr;
}

static void updateEntityParts (EntityNode& entity, double deltaSeconds)
{
    if (! entity.isVisible())
        return;

    entity.forEachAttachedNode ([deltaSeconds] (Node& node)
    {
        node.update (deltaSeconds);
    });

    // Parts may change the tree while updating: hold each child and re-check the count
    for (int i = 0; i < entity.getNumChildren(); ++i)
    {
        const EntityNode::Ptr child = entity.getChild (i);
        updateEntityParts (*child, deltaSeconds);
    }
}

//==============================================================================

Scene::Scene()
    : root (new EntityNode ("root"))
{
}

Scene::~Scene() = default;

//==============================================================================

void Scene::setActiveCamera (EntityNode* entity, int slot)
{
    activeCameraEntity = entity;
    activeCameraSlot = slot;
}

CameraNode* Scene::getActiveCamera() const noexcept
{
    if (auto* entity = activeCameraEntity.get())
    {
        if (auto* camera = entity->getNodeAt<CameraNode> (activeCameraSlot))
            return camera;
    }

    return findFirstVisibleCamera (*root);
}

//==============================================================================

void Scene::update (double deltaSeconds)
{
    updateEntityParts (*root, deltaSeconds);
}

} // namespace yup
