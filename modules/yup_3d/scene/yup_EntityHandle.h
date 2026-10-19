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
/** The versioned identifier of an entity inside its Scene.

    The index addresses the entity storage and is reused once an entity is destroyed; the
    generation is bumped every time that happens, so a handle to a destroyed entity never
    matches the entity that later reuses its index.

    Most code uses Entity, which pairs a handle with its Scene.

    @see Entity, Scene
*/
struct EntityHandle
{
    /** The index of the null handle. */
    static constexpr uint32 nullIndex = std::numeric_limits<uint32>::max();

    uint32 index = nullIndex; ///< The slot in the entity storage.
    uint32 generation = 0;    ///< The number of times the slot has been reused.

    /** Returns true for the null handle, which never refers to an entity. */
    constexpr bool isNull() const noexcept { return index == nullIndex; }

    constexpr bool operator== (const EntityHandle& other) const noexcept { return index == other.index && generation == other.generation; }
    constexpr bool operator!= (const EntityHandle& other) const noexcept { return ! operator== (other); }
};

} // namespace yup
