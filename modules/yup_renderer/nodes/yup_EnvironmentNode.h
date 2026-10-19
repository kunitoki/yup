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
/** A scene feature: the environment lighting the scene.

    Surfaces reflect the environment, sharply or blurred by their roughness, and receive its diffuse
    light. It adds to the lights and to the ambient color: lower the AmbientLightNode color when an
    environment lights the scene.

    The first EnvironmentNode found depth-first in the visible part of the tree wins, see
    SceneSettings::resolve(). Without one, or with a null map, the scene has no environment.

    @code
    auto& environment = scene->getRoot()->attach<yup::EnvironmentNode> (new yup::EnvironmentMap (sky));
    environment.visible = true;
    environment.blur = 0.5f;
    @endcode

    @see EnvironmentMap, SceneSettings
*/
class YUP_API EnvironmentNode : public Node
{
public:
    /** Creates a node without an environment. */
    EnvironmentNode() = default;

    /** Creates a node with an environment. */
    explicit EnvironmentNode (EnvironmentMap::Ptr environmentMap) noexcept
        : map (std::move (environmentMap))
    {
    }

    EnvironmentMap::Ptr map;  ///< The environment, or nullptr for none.
    float intensity = 1.0f;   ///< The multiplier applied to the light of the environment, negative counts as 0.
    bool visible = false;     ///< True to draw the environment behind the scene instead of the background color.
    float blur = 0.0f;        ///< How blurred the environment is behind the scene, from 0 (sharp) to 1 (as blurred as the reflections of a fully rough surface), clamped.
};

} // namespace yup
