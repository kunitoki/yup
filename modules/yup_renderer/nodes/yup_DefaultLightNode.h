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

#pragma once

namespace yup
{

//==============================================================================
/** A scene feature: whether a default directional light shines when the tree has no LightNode.

    The default light is on unless a DefaultLightNode turns it off. The first DefaultLightNode found
    depth-first in the visible part of the tree wins, see SceneSettings::resolve().

    @code
    scene->getRoot()->attach<yup::DefaultLightNode> (false);
    @endcode

    @see SceneSettings
*/
class YUP_API DefaultLightNode : public Node
{
public:
    /** Creates a node leaving the default light on. */
    DefaultLightNode() = default;

    /** Creates a node turning the default light on or off. */
    explicit DefaultLightNode (bool shouldBeEnabled) noexcept
        : enabled (shouldBeEnabled)
    {
    }

    bool enabled = true; ///< True for the default light to shine when the tree has no LightNode.
};

} // namespace yup
