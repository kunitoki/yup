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

Prefab::Part& Prefab::Part::addChild (const String& childName)
{
    return *children.emplace_back (std::make_unique<Part> (childName));
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

Entity Prefab::instantiate (Scene& scene, Entity parent) const
{
    Entity result;
    std::vector<std::pair<const Part*, Entity>> pending { { &root, parent } };

    while (! pending.empty())
    {
        const auto [part, partParent] = pending.back();
        pending.pop_back();

        auto entity = scene.createEntity (part->name, partParent);
        if (! result.isValid())
            result = entity;

        for (const auto& factory : part->factories)
            factory (entity);

        // Reversed, so siblings are made in order
        for (auto it = part->children.rbegin(); it != part->children.rend(); ++it)
            pending.emplace_back (it->get(), entity);
    }

    return result;
}

} // namespace yup
