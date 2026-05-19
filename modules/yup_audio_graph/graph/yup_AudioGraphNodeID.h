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
/**
    Opaque identifier for a processor node owned by an AudioGraphProcessor.

    Node identifiers remain stable until the node is removed. The invalid identifier
    is used by graph input and graph output endpoints, and by failed addNode calls.
*/
class AudioGraphNodeID
{
public:
    /** Creates an invalid node identifier. */
    constexpr AudioGraphNodeID() noexcept = default;

    /** Returns an invalid node identifier. */
    static constexpr AudioGraphNodeID invalid() noexcept { return {}; }

    /** Returns true when this identifier names a graph node. */
    constexpr bool isValid() const noexcept { return value != 0; }

    /** Returns the raw integer identifier. */
    constexpr uint64_t getRawID() const noexcept { return value; }

    /** Creates an identifier from a raw value. Prefer IDs returned by addNode(). */
    explicit constexpr AudioGraphNodeID (uint64_t rawValue) noexcept
        : value (rawValue)
    {
    }

    constexpr bool operator== (AudioGraphNodeID other) const noexcept { return value == other.value; }

    constexpr bool operator!= (AudioGraphNodeID other) const noexcept { return value != other.value; }

    constexpr bool operator< (AudioGraphNodeID other) const noexcept { return value < other.value; }

private:
    uint64_t value = 0;
};

} // namespace yup
