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
/** The entities of a Scene that have a node of every type in Ts, kept up to date.

    Returned by Scene::collect(), which creates it on the first call. Unlike a NodeView, a
    collection stores its entities and follows Scene::onAttach() and Scene::onDetach(), so
    walking it costs nothing beyond the matching entities. It suits sets that are walked
    every frame. It ignores visibility, and its order changes as entities leave it.

    @code
    auto& lights = scene->collect<yup::LightNode>();

    lights.forEach ([] (yup::Entity entity, yup::LightNode& light) { light.intensity = 1.0f; });
    @endcode

    @see Scene::collect, NodeView
*/
template <class... Ts>
class NodeCollection
{
public:
    //==============================================================================
    /** Creates the collection of a scene, use Scene::collect() instead. */
    explicit NodeCollection (Scene& owner)
        : scene (owner)
    {
        (connectTo<Ts>(), ...);
        owner.view<Ts...>().forEach ([this] (Entity entity, Ts&...) { add (entity.getHandle()); });
    }

    //==============================================================================
    /** Returns the number of entities in the collection. */
    int size() const noexcept { return numEntities; }

    /** Returns true if the collection is empty. */
    bool isEmpty() const noexcept { return numEntities == 0; }

    /** Returns true if an entity is in the collection. */
    bool contains (const Entity& entity) const
    {
        const auto it = positions.find (entity.getHandle().index);
        return entity.getScene() == &scene && it != positions.end() && entities[it->second] == entity.getHandle();
    }

    /** Returns the first entity of the collection, or an invalid entity. */
    Entity first() const
    {
        for (const auto& handle : entities)
        {
            if (! handle.isNull())
                return { &scene, handle };
        }

        return {};
    }

    /** Calls a function for every entity of the collection.

        Entities that leave the collection meanwhile are skipped, entities that join it are
        visited from the next call.

        @param fn A callable taking (Entity entity, Ts&... nodes).
    */
    template <class Fn>
    void forEach (Fn&& fn)
    {
        const IterationGuard guard (scene);
        ++iterating;

        for (size_t i = 0, count = entities.size(); i < count; ++i)
        {
            const auto handle = entities[i];
            if (handle.isNull())
                continue;

            const Entity entity (&scene, handle);
            fn (entity, *entity.template get<Ts>()...);
        }

        if (--iterating == 0 && hasHoles)
            compact();
    }

private:
    //==============================================================================
    template <class T>
    void connectTo()
    {
        connections.push_back (scene.onAttach<T>().connect ([this] (Entity entity, T&)
        {
            if ((entity.template has<Ts>() && ...))
                add (entity.getHandle());
        }));

        connections.push_back (scene.onDetach<T>().connect ([this] (Entity entity, T&)
        {
            remove (entity.getHandle());
        }));
    }

    void add (EntityHandle handle)
    {
        if (! positions.emplace (handle.index, entities.size()).second)
            return;

        entities.push_back (handle);
        ++numEntities;
    }

    void remove (EntityHandle handle)
    {
        const auto it = positions.find (handle.index);
        if (it == positions.end() || entities[it->second] != handle)
            return;

        const auto position = it->second;
        positions.erase (it);
        --numEntities;

        // While iterating leave a hole, so the loop neither skips nor repeats an entity
        if (iterating > 0)
        {
            entities[position] = {};
            hasHoles = true;
            return;
        }

        if (position + 1 < entities.size())
        {
            entities[position] = entities.back();
            positions[entities[position].index] = position;
        }

        entities.pop_back();
    }

    void compact()
    {
        entities.erase (std::remove_if (entities.begin(), entities.end(), [] (const auto& handle) { return handle.isNull(); }), entities.end());

        for (size_t i = 0; i < entities.size(); ++i)
            positions[entities[i].index] = i;

        hasHoles = false;
    }

    Scene& scene;
    std::vector<EntityHandle> entities;
    std::unordered_map<uint32, size_t> positions;
    std::vector<SignalConnection> connections;
    int numEntities = 0;
    int iterating = 0;
    bool hasHoles = false;

    YUP_DECLARE_NON_COPYABLE (NodeCollection)
};

} // namespace yup
