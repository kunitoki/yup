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

static CameraNode* findFirstVisibleCamera (Entity entity)
{
    if (! entity.isVisible())
        return nullptr;

    if (auto* camera = entity.get<CameraNode>())
        return camera;

    for (const auto child : entity.getChildren())
    {
        if (auto* camera = findFirstVisibleCamera (child))
            return camera;
    }

    return nullptr;
}

//==============================================================================

Scene::Scene()
    : store (std::make_unique<EntityStore>())
{
    root = store->create ("root");
}

Scene::~Scene()
{
    systems.clear();
    collections.clear();
    poolsByType.clear();
    pools.clear();
    store.reset();
}

//==============================================================================

Entity Scene::getRoot() const noexcept
{
    return { const_cast<Scene*> (this), root };
}

Entity Scene::createEntity (const String& name, Entity parent, const Uuid& uuid)
{
    if (! parent.isValid())
    {
        jassert (parent.getScene() == nullptr); // The parent was destroyed
        parent = getRoot();
    }

    if (parent.getScene() != this)
    {
        jassertfalse; // The parent belongs to another scene
        parent = getRoot();
    }

    const auto handle = store->create (name);
    store->setParent (handle.index, parent.getHandle().index, -1);

    Entity entity (this, handle);
    if (! uuid.isNull())
        entity.setUuid (uuid);

    return entity;
}

Entity Scene::findEntity (const Uuid& uuid) const
{
    const auto it = entitiesByUuid.find (uuid);
    if (it == entitiesByUuid.end())
        return {};

    return { const_cast<Scene*> (this), it->second };
}

int Scene::getNumEntities() const noexcept
{
    return store->getNumEntities();
}

void Scene::destroyEntity (EntityHandle entity)
{
    if (entity == root)
    {
        jassertfalse; // The root can't be destroyed
        return;
    }

    if (store->find (entity) == nullptr)
        return;

    destroySignal.emit (Entity (this, entity));

    // The signal handlers may have destroyed it already
    if (store->find (entity) == nullptr)
        return;

    while (! store->get (entity.index).children.empty())
        destroyEntity (store->getHandle (store->get (entity.index).children.back()));

    for (size_t i = 0; i < pools.size(); ++i)
    {
        if (pools[i] != nullptr)
            pools[i]->remove (entity);
    }

    if (const auto& uuid = store->get (entity.index).uuid; ! uuid.isNull())
        entitiesByUuid.erase (uuid);

    store->unlink (entity.index);
    store->release (entity.index);
}

//==============================================================================

bool Scene::removeSystem (const System& system)
{
    const auto it = std::find_if (systems.begin(), systems.end(), [&] (const auto& entry)
    {
        return entry->system.get() == &system && ! entry->removed;
    });

    if (it == systems.end())
        return false;

    (*it)->removed = true;
    scheduleDirty = true;

    // While updating the schedule still points at it, so it's erased when the update ends
    if (! updatingSystems)
        systems.erase (it);

    return true;
}

void Scene::addSystemConstraint (uint64 systemType, const SystemOrder::Constraint& constraint)
{
    if (constraint.runsBefore)
        addSystemRule (systemType, constraint.typeId);
    else
        addSystemRule (constraint.typeId, systemType);
}

bool Scene::addSystemRule (uint64 first, uint64 then)
{
    if (first == then || isOrderedBefore (then, first))
    {
        jassertfalse; // The rule would make a cycle
        return false;
    }

    if (! isOrderedBefore (first, then))
    {
        systemRules.emplace_back (first, then);
        scheduleDirty = true;
    }

    return true;
}

bool Scene::isOrderedBefore (uint64 first, uint64 then) const
{
    std::vector<uint64> pending { first };
    std::vector<uint64> visited;

    while (! pending.empty())
    {
        const auto type = pending.back();
        pending.pop_back();

        for (const auto& [before, after] : systemRules)
        {
            if (before != type || std::find (visited.begin(), visited.end(), after) != visited.end())
                continue;

            if (after == then)
                return true;

            visited.push_back (after);
            pending.push_back (after);
        }
    }

    return false;
}

void Scene::scheduleUpdates()
{
    schedule.clear();

    const auto systemSteps = [this] (SystemOrder::Phase phase)
    {
        std::vector<UpdateStep> steps;

        for (const auto& entry : systems)
        {
            if (! entry->removed && entry->phase == phase)
                steps.push_back ({ entry->typeId, entry.get(), nullptr });
        }

        return steps;
    };

    std::vector<UpdateStep> nodeSteps;

    for (const auto& pool : pools)
    {
        if (pool->updatesNodes())
            nodeSteps.push_back ({ pool->getTypeId(), nullptr, pool.get() });
    }

    for (auto pending : { systemSteps (SystemOrder::Phase::beforeNodes), nodeSteps, systemSteps (SystemOrder::Phase::afterNodes) })
    {
        // Stable topological sort: the earliest step that nothing pending must precede
        while (! pending.empty())
        {
            const auto next = std::find_if (pending.begin(), pending.end(), [&] (const UpdateStep& candidate)
            {
                return std::none_of (pending.begin(), pending.end(), [&] (const UpdateStep& other)
                {
                    return isOrderedBefore (other.typeId, candidate.typeId);
                });
            });

            jassert (next != pending.end()); // Rules are acyclic, so something is always ready

            schedule.push_back (*next);
            pending.erase (next);
        }
    }

    for (size_t later = 0; later < schedule.size(); ++later)
    {
        for (size_t earlier = 0; earlier < later; ++earlier)
        {
            // A rule asks a step to run before the phase it belongs to
            jassert (! isOrderedBefore (schedule[later].typeId, schedule[earlier].typeId));
        }
    }

    scheduleDirty = false;
}

//==============================================================================

void Scene::setActiveCamera (Entity entity) noexcept
{
    jassert (entity.getScene() == nullptr || entity.getScene() == this);
    activeCamera = entity;
}

CameraNode* Scene::getActiveCamera() const noexcept
{
    if (auto* camera = activeCamera.get<CameraNode>())
        return camera;

    return findFirstVisibleCamera (getRoot());
}

//==============================================================================

void Scene::update (double deltaSeconds)
{
    const IterationGuard guard (*this);

    if (scheduleDirty)
        scheduleUpdates();

    // A copy: systems may add or remove systems, the new schedule is used from the next update
    const auto current = schedule;
    const auto wasUpdatingSystems = std::exchange (updatingSystems, true);

    for (const auto& step : current)
    {
        if (step.system != nullptr)
        {
            if (! step.system->removed)
                step.system->system->update (*this, deltaSeconds);

            continue;
        }

        auto* pool = step.pool;

        for (uint32 slot = 0, numSlots = pool->getNumSlots(); slot < numSlots; ++slot)
        {
            auto* node = pool->getNodeAt (slot);
            if (node != nullptr && store->isActive (pool->getEntityAt (slot).index, root.index))
                node->update (deltaSeconds);
        }
    }

    updatingSystems = wasUpdatingSystems;

    if (! updatingSystems)
        systems.erase (std::remove_if (systems.begin(), systems.end(), [] (const auto& entry) { return entry->removed; }), systems.end());
}

void Scene::endIteration()
{
    jassert (iterationDepth > 0);

    if (--iterationDepth > 0)
        return;

    for (size_t i = 0; i < pools.size(); ++i)
    {
        if (pools[i] != nullptr)
            pools[i]->destroyPending();
    }
}

} // namespace yup
