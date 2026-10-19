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
/** The scene features of one frame, resolved from the feature nodes of a scene.

    Each field comes from the node of its type nearest to the root in the visible part of the
    tree, the first attached among equals: BackgroundNode, AmbientLightNode, ExposureNode, ToneMappingNode, DefaultLightNode,
    EnvironmentNode and WorldFogNode. A feature without a node keeps the value of a
    default-constructed node, and the scene has no environment and no fog.

    A default-constructed SceneSettings is the settings of a scene without feature nodes.

    @code
    auto scene = yup::Scene::Ptr (new yup::Scene());
    scene->getRoot().attach<yup::ToneMappingNode> (yup::ToneMappingOperator::agx);

    const auto settings = yup::SceneSettings::resolve (*scene);
    jassert (settings.toneMapping == yup::ToneMappingOperator::agx);
    @endcode
*/
struct YUP_API SceneSettings
{
    Color backgroundColor = BackgroundNode().color;                ///< BackgroundNode::color.
    Color ambientColor = AmbientLightNode().color;                 ///< AmbientLightNode::color.
    float exposure = ExposureNode().exposure;                      ///< ExposureNode::exposure.
    ToneMappingOperator toneMapping = ToneMappingNode().type;      ///< ToneMappingNode::type.
    bool useDefaultLight = DefaultLightNode().enabled;             ///< DefaultLightNode::enabled.
    EnvironmentMap::Ptr environment;                               ///< EnvironmentNode::map, nullptr without an environment.
    float environmentIntensity = EnvironmentNode().intensity;      ///< EnvironmentNode::intensity, at least 0.
    bool environmentVisible = EnvironmentNode().visible;           ///< EnvironmentNode::visible.
    float environmentBlur = EnvironmentNode().blur;                ///< EnvironmentNode::blur, from 0 to 1.
    WorldFog fog;                                                  ///< WorldFogNode::fog, disabled without a WorldFogNode.

    /** Resolves the settings of a scene from the feature nodes under its root. */
    static SceneSettings resolve (const Scene& scene);

    /** Resolves the settings from the feature nodes of a tree. */
    static SceneSettings resolve (const Entity& root);
};

} // namespace yup
