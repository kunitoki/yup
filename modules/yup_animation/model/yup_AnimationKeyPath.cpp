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
// KeyPath

KeyPath::KeyPath (const String& keyPath)
{
    if (keyPath.isEmpty())
        return;

    StringArray parts;
    parts.addTokens (keyPath, ".", StringRef());
    keys.reserve (static_cast<size_t> (parts.size()));
    for (int i = 0; i < parts.size(); ++i)
        keys.push_back (parts[i]);
}

bool KeyPath::matchesComponent (const String& key, size_t depth) const noexcept
{
    if (depth >= keys.size())
        return false;
    return keys[depth] == key || keys[depth] == "*" || keys[depth] == "**";
}

size_t KeyPath::nextDepth (const String& key, size_t depth) const noexcept
{
    if (depth >= keys.size())
        return depth;

    if (keys[depth] != "**")
        return depth + 1;

    if (depth + 1 == keys.size())
        return depth;

    if (depth + 1 < keys.size() && keys[depth + 1] == key)
        return depth + 2;

    return depth;
}

bool KeyPath::fullyResolvesTo (const String& key, size_t depth) const noexcept
{
    if (depth >= keys.size())
        return false;

    const bool isLast = (depth + 1 == keys.size());

    if (! isGlobstar (depth))
    {
        const bool matches = (keys[depth] == key) || isGlob (depth);
        return (isLast || (depth + 2 == keys.size() && endsWithGlobstar())) && matches;
    }

    const bool globstarButNextKeyMatches = ! isLast && depth + 1 < keys.size() && keys[depth + 1] == key;
    if (globstarButNextKeyMatches)
    {
        return depth + 2 == keys.size() || (depth + 3 == keys.size() && endsWithGlobstar());
    }

    if (isLast)
        return true;

    if (depth + 2 < keys.size())
        return false;

    return depth + 1 < keys.size() && keys[depth + 1] == key;
}

bool KeyPath::propagate (const String& key, size_t depth) const noexcept
{
    if (key == "__")
        return true;

    if (depth < keys.size())
        return keys[depth] == "**";

    return false;
}

//==============================================================================
// PropertyOverrideSet

void PropertyOverrideSet::setFloatOverride (AnimationPropertyID id, AnimationPropertyOverride<float> callback)
{
    auto* entry = findOrCreate (id);
    entry->floatFunc = std::move (callback);
}

void PropertyOverrideSet::setColorOverride (AnimationPropertyID id, AnimationPropertyOverride<Color> callback)
{
    auto* entry = findOrCreate (id);
    entry->colorFunc = std::move (callback);
}

void PropertyOverrideSet::setPointOverride (AnimationPropertyID id, AnimationPropertyOverride<Point<float>> callback)
{
    auto* entry = findOrCreate (id);
    entry->pointFunc = std::move (callback);
}

void PropertyOverrideSet::setSizeOverride (AnimationPropertyID id, AnimationPropertyOverride<Size<float>> callback)
{
    auto* entry = findOrCreate (id);
    entry->sizeFunc = std::move (callback);
}

bool PropertyOverrideSet::hasOverride (AnimationPropertyID id) const noexcept
{
    return find (id) != nullptr;
}

float PropertyOverrideSet::evaluateFloat (AnimationPropertyID id, float frameNo, float fallback) const
{
    const auto* entry = find (id);
    if (entry != nullptr && entry->floatFunc)
    {
        if (auto result = entry->floatFunc (frameNo))
            return *result;
    }
    return fallback;
}

Color PropertyOverrideSet::evaluateColor (AnimationPropertyID id, float frameNo, const Color& fallback) const
{
    const auto* entry = find (id);
    if (entry != nullptr && entry->colorFunc)
    {
        if (auto result = entry->colorFunc (frameNo))
            return *result;
    }
    return fallback;
}

Point<float> PropertyOverrideSet::evaluatePoint (AnimationPropertyID id, float frameNo, const Point<float>& fallback) const
{
    const auto* entry = find (id);
    if (entry != nullptr && entry->pointFunc)
    {
        if (auto result = entry->pointFunc (frameNo))
            return *result;
    }
    return fallback;
}

Size<float> PropertyOverrideSet::evaluateSize (AnimationPropertyID id, float frameNo, const Size<float>& fallback) const
{
    const auto* entry = find (id);
    if (entry != nullptr && entry->sizeFunc)
    {
        if (auto result = entry->sizeFunc (frameNo))
            return *result;
    }
    return fallback;
}

PropertyOverrideSet::TypedOverride* PropertyOverrideSet::findOrCreate (AnimationPropertyID id)
{
    for (auto& o : overrides)
    {
        if (o.id == id)
            return &o;
    }
    TypedOverride entry;
    entry.id = id;
    overrides.push_back (std::move (entry));
    return &overrides.back();
}

const PropertyOverrideSet::TypedOverride* PropertyOverrideSet::find (AnimationPropertyID id) const noexcept
{
    for (const auto& o : overrides)
    {
        if (o.id == id)
            return &o;
    }
    return nullptr;
}

} // namespace yup
