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

Prefab::Part::Part (const String& name)
    : name (name)
{
}

Prefab::Part::~Part()
{
    auto pending = std::move (children);

    while (! pending.empty())
    {
        auto part = std::move (pending.back());
        pending.pop_back();

        for (auto& child : part->children)
            pending.push_back (std::move (child));

        part->children.clear();
    }
}

Prefab::Part& Prefab::Part::addChild (const String& childName)
{
    auto& child = *children.emplace_back (std::make_unique<Part> (childName));
    child.rootPart = rootPart;
    child.index = rootPart->numParts++;
    return child;
}

Prefab::Part& Prefab::Part::getChild (int index) noexcept
{
    jassert (isPositiveAndBelow (index, getNumChildren()));
    return *children[static_cast<size_t> (index)];
}

const Prefab::Part& Prefab::Part::getChild (int index) const noexcept
{
    jassert (isPositiveAndBelow (index, getNumChildren()));
    return *children[static_cast<size_t> (index)];
}

//==============================================================================

Prefab::Prefab (const String& rootName)
    : root (rootName)
{
}

void Prefab::addLink (std::function<void (Span<const Entity>)> link)
{
    if (link == nullptr)
    {
        jassertfalse; // A link needs something to call
        return;
    }

    links.push_back (std::move (link));
}

Entity Prefab::instantiate (Scene& scene, Entity parent) const
{
    std::vector<Entity> entities (static_cast<size_t> (root.numParts));
    std::vector<std::pair<const Part*, Entity>> pending { { &root, parent } };

    while (! pending.empty())
    {
        const auto [part, partParent] = pending.back();
        pending.pop_back();

        // An attach callback can destroy an entity of the tree: nothing is made of the parts it held
        if (part != &root && ! partParent.isValid())
            continue;

        auto entity = scene.createEntity (part->name, partParent);
        entities[static_cast<size_t> (part->index)] = entity;

        for (const auto& factory : part->factories)
        {
            if (! entity.isValid())
                break;

            factory (entity);
        }

        // Reversed, so siblings are made in order
        for (auto it = part->children.rbegin(); it != part->children.rend(); ++it)
            pending.emplace_back (it->get(), entity);
    }

    for (const auto& link : links)
        link (entities);

    return entities.front();
}

} // namespace yup
