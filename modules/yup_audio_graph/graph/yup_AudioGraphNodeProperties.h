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
    Persistent metadata used to save and recreate an AudioGraphProcessor node.

    Set identifier to a stable application-defined factory key. If node creation
    needs additional metadata, store it in creationData. Callers that also link a
    plugin-host module can encode plugin descriptions there and recreate hosted
    plugin instances from the node factory.
*/
struct AudioGraphNodeProperties
{
    /** Stable factory key used when loading graph state. */
    String identifier;

    /** Human-readable node name. */
    String name;

    /** Canvas X position in caller-defined units. */
    float positionX = 0.0f;

    /** Canvas Y position in caller-defined units. */
    float positionY = 0.0f;

    /** Opaque caller-defined metadata used to recreate the node. */
    MemoryBlock creationData;
};

} // namespace yup
