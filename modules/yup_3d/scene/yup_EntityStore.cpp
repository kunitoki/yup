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

namespace
{

constexpr uint64 uuidStreamIncrement = 0x9e3779b97f4a7c15ull;

// The murmur3 finalizer, a bijection with full avalanche
constexpr uint64 mixUuidBits (uint64 bits) noexcept
{
    bits ^= bits >> 33;
    bits *= 0xff51afd7ed558ccdull;
    bits ^= bits >> 33;
    bits *= 0xc4ceb9fe1a85ec53ull;
    return bits ^ (bits >> 33);
}

Uuid makeUuid (uint64 high, uint64 low, uint8 version) noexcept
{
    uint8 bytes[16];

    for (int i = 0; i < 8; ++i)
    {
        bytes[i] = static_cast<uint8> (high >> (56 - 8 * i));
        bytes[8 + i] = static_cast<uint8> (low >> (56 - 8 * i));
    }

    bytes[6] = static_cast<uint8> ((bytes[6] & 0x0f) | (version << 4));
    bytes[8] = static_cast<uint8> ((bytes[8] & 0x3f) | 0x80);
    return Uuid (bytes);
}

uint64 readUuidWord (const uint8* bytes) noexcept
{
    uint64 word = 0;

    for (int i = 0; i < 8; ++i)
        word = (word << 8) | bytes[i];

    return word;
}

} // namespace

//==============================================================================

EntityStore::EntityStore()
{
    if (! SecureRandom::fillBytes (uuidStreams, sizeof (uuidStreams)))
    {
        jassertfalse;
        uuidStreams[0] = static_cast<uint64> (Random::getSystemRandom().nextInt64());
        uuidStreams[1] = static_cast<uint64> (Time::getHighResolutionTicks());
    }
}

Uuid EntityStore::createUuid() noexcept
{
    uuidStreams[0] += uuidStreamIncrement;
    uuidStreams[1] += uuidStreamIncrement;
    return makeUuid (mixUuidBits (uuidStreams[0]), mixUuidBits (uuidStreams[1]), 4);
}

Uuid EntityStore::deriveUuid (const Uuid& uuid, uint64 typeId) noexcept
{
    const auto* bytes = uuid.getRawData();
    const auto high = mixUuidBits (readUuidWord (bytes) ^ mixUuidBits (typeId + uuidStreamIncrement));
    const auto low = mixUuidBits (readUuidWord (bytes + 8) ^ high);
    return makeUuid (high, low, 8);
}

//==============================================================================

EntityHandle EntityStore::create (const String& name)
{
    uint32 index;

    if (! freeList.empty())
    {
        index = freeList.back();
        freeList.pop_back();
    }
    else
    {
        index = static_cast<uint32> (records.size());
        records.emplace_back();
    }

    auto& record = records[index];
    const auto generation = record.generation;
    record = {};
    record.generation = generation;
    record.alive = true;
    record.name = name;

    ++numEntities;
    return { index, generation };
}

void EntityStore::release (uint32 index)
{
    auto& record = records[index];
    jassert (record.alive && record.parent == EntityHandle::nullIndex && record.children.empty());

    record.alive = false;
    record.name = {};
    record.uuid = Uuid::null();
    ++record.generation;
    freeList.push_back (index);
    --numEntities;
}

EntityStore::Record* EntityStore::find (EntityHandle entity) noexcept
{
    if (entity.index >= records.size())
        return nullptr;

    auto& record = records[entity.index];
    return record.alive && record.generation == entity.generation ? &record : nullptr;
}

const EntityStore::Record* EntityStore::find (EntityHandle entity) const noexcept
{
    return const_cast<EntityStore*> (this)->find (entity);
}

EntityHandle EntityStore::getHandle (uint32 index) const noexcept
{
    if (index >= records.size() || ! records[index].alive)
        return {};

    return { index, records[index].generation };
}

//==============================================================================

bool EntityStore::setParent (uint32 child, uint32 parent, int index)
{
    if (child == parent || isAncestorOf (child, parent))
        return false;

    unlink (child);

    auto& siblings = records[parent].children;
    const auto count = static_cast<int> (siblings.size());
    const auto insertAt = isPositiveAndNotGreaterThan (index, count) ? index : count;
    siblings.insert (siblings.begin() + insertAt, child);

    records[child].parent = parent;
    markWorldDirty (child);
    return true;
}

void EntityStore::unlink (uint32 child)
{
    const auto parent = std::exchange (records[child].parent, EntityHandle::nullIndex);
    if (parent == EntityHandle::nullIndex)
        return;

    auto& siblings = records[parent].children;
    siblings.erase (std::find (siblings.begin(), siblings.end(), child));
    markWorldDirty (child);
}

bool EntityStore::isAncestorOf (uint32 ancestor, uint32 index) const noexcept
{
    for (auto p = records[index].parent; p != EntityHandle::nullIndex; p = records[p].parent)
    {
        if (p == ancestor)
            return true;
    }

    return false;
}

bool EntityStore::isActive (uint32 index, uint32 root) const noexcept
{
    for (; index != EntityHandle::nullIndex; index = records[index].parent)
    {
        if (! records[index].visible)
            return false;

        if (index == root)
            return true;
    }

    return false;
}

//==============================================================================

void EntityStore::markWorldDirty (uint32 index) noexcept
{
    // A clean entity always has clean ancestors, so a dirty one already has a dirty subtree
    auto& record = records[index];
    if (record.worldDirty)
        return;

    record.worldDirty = true;

    for (const auto child : record.children)
        markWorldDirty (child);
}

} // namespace yup
