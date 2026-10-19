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
/** The entities of a Scene that have a node of every type in Ts, returned by Scene::view().

    A view doesn't store anything: it walks the smallest of the pools and looks the other
    node types up for each entity, in the order of that pool. It ignores visibility.

    @code
    for (auto [entity, mesh, material] : scene->view<yup::MeshNode, yup::MaterialNode>())
        mesh.mesh = nullptr;

    scene->view<yup::LightNode>()
         .where ([] (const yup::LightNode& light) { return light.castsShadows; })
         .forEach ([] (yup::Entity entity, yup::LightNode& light) { light.intensity = 1.0f; });
    @endcode

    Range-for yields a std::tuple<Entity, Ts&...> by value: bind it with auto or auto&&.

    Predicates are part of the type of the view and called inline, so where() is only available
    on a temporary view: chain it, or std::move() a stored view into it.

    The scene is iterating for as long as the view lives: nodes detached and entities destroyed
    meanwhile are skipped at once, but only destroyed when the view goes away. Nodes attached
    while iterating are not visited. Keep views short-lived.

    @see Scene::view, NodeCollection
*/
template <class Filter, class... Ts>
class BasicNodeView
{
public:
    //==============================================================================
    /** Creates a view over pools of a scene, use Scene::view() instead. */
    BasicNodeView (Scene& owner, NodePool<Ts>&... nodePools)
        : guard (owner)
        , scene (&owner)
        , pools (&nodePools...)
    {
        ((lead = (lead == nullptr || nodePools.size() < lead->size()) ? &nodePools : lead), ...);
    }

    BasicNodeView (BasicNodeView&& other) = default;

    //==============================================================================
    /** Returns a view that only keeps the entities whose nodes pass a predicate, on top of the previous ones.

        @param predicate A callable taking (const Ts&... nodes) and returning bool.
    */
    template <class Predicate>
        requires std::predicate<const std::decay_t<Predicate>&, const Ts&...>
    auto where (Predicate&& predicate) &&
    {
        if constexpr (hasFilter)
        {
            auto combined = [previous = std::move (filter), next = std::forward<Predicate> (predicate)] (const Ts&... n)
            {
                return previous (n...) && next (n...);
            };

            return BasicNodeView<decltype (combined), Ts...> (std::move (*this), std::move (combined));
        }
        else
        {
            return BasicNodeView<std::decay_t<Predicate>, Ts...> (std::move (*this), std::forward<Predicate> (predicate));
        }
    }

    //==============================================================================
    /** Calls a function for every matching entity.

        @param fn A callable taking (Entity entity, Ts&... nodes).
    */
    template <class Fn>
    void forEach (Fn&& fn) const
    {
        std::tuple<Ts*...> nodes;

        for (uint32 slot = 0, numSlots = lead->getNumSlots(); slot < numSlots; ++slot)
        {
            if (fetch (slot, nodes))
                std::apply ([&] (Ts*... n) { fn (Entity (scene, lead->getEntityAt (slot)), *n...); }, nodes);
        }
    }

    /** Returns the first matching entity, or an invalid entity. */
    Entity first() const
    {
        std::tuple<Ts*...> nodes;

        for (uint32 slot = 0, numSlots = lead->getNumSlots(); slot < numSlots; ++slot)
        {
            if (fetch (slot, nodes))
                return { scene, lead->getEntityAt (slot) };
        }

        return {};
    }

    /** Returns the number of matching entities, walking the whole view unless it has a single node type and no predicate. */
    int size() const
    {
        if constexpr (sizeof... (Ts) == 1 && ! hasFilter)
        {
            return lead->size();
        }
        else
        {
            int count = 0;
            forEach ([&count] (Entity, Ts&...) { ++count; });
            return count;
        }
    }

    /** Returns true if no entity matches. */
    bool isEmpty() const { return ! first().isValid(); }

    //==============================================================================
    class Iterator
    {
    public:
        using value_type = std::tuple<Entity, Ts&...>;

        Iterator (const BasicNodeView& owner, uint32 startSlot, uint32 endSlot)
            : view (&owner)
            , slot (startSlot)
            , numSlots (endSlot)
        {
            skipToMatch();
        }

        value_type operator*() const
        {
            return std::apply ([this] (Ts*... n) { return value_type (Entity (view->scene, view->lead->getEntityAt (slot)), *n...); }, nodes);
        }

        Iterator& operator++()
        {
            ++slot;
            skipToMatch();
            return *this;
        }

        bool operator!= (const Iterator& other) const noexcept { return slot != other.slot; }

    private:
        void skipToMatch()
        {
            while (slot < numSlots && ! view->fetch (slot, nodes))
                ++slot;
        }

        const BasicNodeView* view = nullptr;
        uint32 slot = 0;
        uint32 numSlots = 0;
        std::tuple<Ts*...> nodes;
    };

    Iterator begin() const { return { *this, 0, lead->getNumSlots() }; }
    Iterator end() const { return { *this, lead->getNumSlots(), lead->getNumSlots() }; }

private:
    //==============================================================================
    template <class, class...>
    friend class BasicNodeView;

    static constexpr bool hasFilter = ! std::is_void_v<Filter>;
    using FilterStorage = std::conditional_t<hasFilter, Filter, std::monostate>;

    template <class OtherFilter, class NewFilter>
    BasicNodeView (BasicNodeView<OtherFilter, Ts...>&& other, NewFilter&& newFilter)
        : guard (std::move (other.guard))
        , scene (other.scene)
        , pools (other.pools)
        , lead (other.lead)
        , filter (std::forward<NewFilter> (newFilter))
    {
    }

    bool fetch (uint32 slot, std::tuple<Ts*...>& nodes) const
    {
        const auto entity = lead->getEntityAt (slot);
        if (entity.isNull())
            return false;

        nodes = std::tuple<Ts*...> { std::get<NodePool<Ts>*> (pools)->get (entity)... };

        return std::apply ([this] (Ts*... n)
        {
            if (! ((n != nullptr) && ...))
                return false;

            if constexpr (hasFilter)
                return static_cast<bool> (filter (std::as_const (*n)...));
            else
                return true;
        }, nodes);
    }

    IterationGuard guard;
    Scene* scene = nullptr;
    std::tuple<NodePool<Ts>*...> pools;
    NodePoolBase* lead = nullptr;
    [[no_unique_address]] FilterStorage filter;
};

} // namespace yup
