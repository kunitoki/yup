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
/** A scene feature: the flat ambient light color added to every surface.

    Lower it, often to black, when an EnvironmentNode lights the scene. The first AmbientLightNode
    found depth-first in the visible part of the tree wins, see SceneSettings::resolve(). Without
    one the scene behaves as if it had a default-constructed one.

    @see SceneSettings
*/
class YUP_API AmbientLightNode : public Node
{
public:
    /** Creates an ambient light of the default color. */
    AmbientLightNode() = default;

    /** Creates an ambient light of a color. */
    explicit AmbientLightNode (Color ambientColor) noexcept
        : color (ambientColor)
    {
    }

    Color color { 0xff595959 }; ///< The ambient light color, sRGB encoded.
};

} // namespace yup
