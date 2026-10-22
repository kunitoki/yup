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
    every frame. It ignores visibility, and keeps its entities in the order they joined it.

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
        owner.view<Ts...>().forEach ([this] (Entity entity, Ts&... nodes) { add (entity.getHandle(), &nodes...); });
    }

    //==============================================================================
    /** Returns the number of entities in the collection. */
    int size() const noexcept { return numEntities; }

    /** Returns true if the collection is empty. */
    bool isEmpty() const noexcept { return numEntities == 0; }

    /** Returns true if an entity is in the collection. */
    bool contains (const Entity& entity) const
    {
        const auto index = entity.getHandle().index;
        return entity.getScene() == &scene
            && index < positions.size()
            && positions[index] != 0
            && entries[positions[index] - 1].handle == entity.getHandle();
    }

    /** Returns the first entity of the collection, or an invalid entity. */
    Entity first() const
    {
        for (const auto& entry : entries)
        {
            if (! entry.handle.isNull())
                return { &scene, entry.handle };
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

        for (size_t i = 0, count = entries.size(); i < count; ++i)
        {
            // A copy: fn may make entities join, which grows the entries
            const auto entry = entries[i];
            if (entry.handle.isNull())
                continue;

            std::apply ([&] (Ts*... nodes) { fn (Entity (&scene, entry.handle), *nodes...); }, entry.nodes);
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
                add (entity.getHandle(), entity.template get<Ts>()...);
        }));

        connections.push_back (scene.onDetach<T>().connect ([this] (Entity entity, T&)
        {
            remove (entity.getHandle());
        }));
    }

    // Nodes never move while attached, and detaching one removes its entry first
    struct Entry
    {
        EntityHandle handle;
        std::tuple<Ts*...> nodes;
    };

    void add (EntityHandle handle, Ts*... nodes)
    {
        if (handle.index >= positions.size())
            positions.resize (handle.index + 1, 0);

        if (positions[handle.index] != 0)
            return;

        entries.push_back ({ handle, { nodes... } });
        positions[handle.index] = static_cast<uint32> (entries.size());
        ++numEntities;
    }

    void remove (EntityHandle handle)
    {
        if (handle.index >= positions.size() || positions[handle.index] == 0)
            return;

        auto& entry = entries[positions[handle.index] - 1];
        if (entry.handle != handle)
            return;

        // A hole keeps the order, and lets a running loop neither skip nor repeat an entity
        entry = {};
        positions[handle.index] = 0;
        --numEntities;
        hasHoles = true;

        if (iterating == 0 && static_cast<size_t> (numEntities) * 2 < entries.size())
            compact();
    }

    void compact()
    {
        entries.erase (std::remove_if (entries.begin(), entries.end(), [] (const Entry& entry) { return entry.handle.isNull(); }), entries.end());

        for (size_t i = 0; i < entries.size(); ++i)
            positions[entries[i].handle.index] = static_cast<uint32> (i + 1);

        hasHoles = false;
    }

    Scene& scene;
    std::vector<Entry> entries;
    std::vector<uint32> positions;
    std::vector<SignalConnection> connections;
    int numEntities = 0;
    int iterating = 0;
    bool hasHoles = false;

    YUP_DECLARE_NON_COPYABLE (NodeCollection)
};

} // namespace yup
