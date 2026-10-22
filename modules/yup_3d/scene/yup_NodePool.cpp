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

NodePoolBase::NodePoolBase (Scene& owner, uint64 nodeTypeId, std::string_view nodeTypeName, size_t nodeSize, size_t nodeAlignment, bool nodesUpdate)
    : scene (owner)
    , typeId (nodeTypeId)
    , typeName (nodeTypeName)
    , stride ((nodeSize + nodeAlignment - 1) / nodeAlignment * nodeAlignment)
    , alignment (nodeAlignment)
    , hasUpdate (nodesUpdate)
{
}

NodePoolBase::~NodePoolBase()
{
    for (auto* node : nodes)
    {
        if (node != nullptr)
            node->~Node();
    }

    for (auto* page : pages)
        ::operator delete (page, std::align_val_t (alignment));
}

//==============================================================================

uint32 NodePoolBase::acquireSlot()
{
    // While iterating, freed slots could be behind or ahead of the loop: append instead
    if (! freeSlots.empty() && ! scene.isIterating())
    {
        const auto slot = freeSlots.back();
        freeSlots.pop_back();
        return slot;
    }

    const auto slot = static_cast<uint32> (handles.size());
    handles.emplace_back();
    nodes.push_back (nullptr);

    if (slot / nodesPerPage >= pages.size())
        pages.push_back (static_cast<std::byte*> (::operator new (stride * nodesPerPage, std::align_val_t (alignment))));

    return slot;
}

void* NodePoolBase::getStorage (uint32 slot) const noexcept
{
    return pages[slot / nodesPerPage] + (slot % nodesPerPage) * stride;
}

void NodePoolBase::link (uint32 slot, EntityHandle entity, Node& node)
{
    if (entity.index >= sparse.size())
        sparse.resize (static_cast<size_t> (entity.index) + 1, 0);

    sparse[entity.index] = slot + 1;
    handles[slot] = entity;
    nodes[slot] = &node;
    ++numNodes;

    node.entity = Entity (&scene, entity);
    node.typeId = typeId;
    node.attachedToEntity();
}

uint32 NodePoolBase::unlink (EntityHandle entity) noexcept
{
    if (find (entity) == nullptr)
        return EntityHandle::nullIndex;

    const auto slot = sparse[entity.index] - 1;
    sparse[entity.index] = 0;
    handles[slot] = {};
    --numNodes;
    return slot;
}

void NodePoolBase::release (uint32 slot)
{
    auto* node = nodes[slot];
    node->detachedFromEntity();
    node->entity = {};

    if (scene.isIterating())
    {
        pendingSlots.push_back (slot);
        return;
    }

    nodes[slot] = nullptr;
    node->~Node();
    freeSlots.push_back (slot);
}

void NodePoolBase::destroyPending()
{
    const auto pending = std::exchange (pendingSlots, {});

    for (const auto slot : pending)
    {
        auto* node = std::exchange (nodes[slot], nullptr);
        node->~Node();
        freeSlots.push_back (slot);
    }
}

} // namespace yup
