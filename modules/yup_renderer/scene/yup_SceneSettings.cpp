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
namespace
{

struct SceneFeatureNodes
{
    const BackgroundNode* background = nullptr;
    const AmbientLightNode* ambient = nullptr;
    const ExposureNode* exposure = nullptr;
    const ToneMappingNode* toneMapping = nullptr;
    const DefaultLightNode* defaultLight = nullptr;
    const EnvironmentNode* environment = nullptr;
    const WorldFogNode* fog = nullptr;
};

template <class T>
void takeFirstSceneFeature (const Entity& entity, const T*& found)
{
    if (found != nullptr)
        return;

    entity.forEachNode<T> ([&] (T& node, int)
    {
        if (found == nullptr)
            found = &node;
    });
}

void collectSceneFeatures (const Entity& entity, SceneFeatureNodes& nodes)
{
    if (! entity.isVisible())
        return;

    takeFirstSceneFeature (entity, nodes.background);
    takeFirstSceneFeature (entity, nodes.ambient);
    takeFirstSceneFeature (entity, nodes.exposure);
    takeFirstSceneFeature (entity, nodes.toneMapping);
    takeFirstSceneFeature (entity, nodes.defaultLight);
    takeFirstSceneFeature (entity, nodes.environment);
    takeFirstSceneFeature (entity, nodes.fog);

    for (const auto& child : entity.getChildren())
        collectSceneFeatures (*child, nodes);
}

} // namespace

//==============================================================================

SceneSettings SceneSettings::resolve (const Scene& scene)
{
    return resolve (*scene.getRoot());
}

SceneSettings SceneSettings::resolve (const Entity& root)
{
    SceneFeatureNodes nodes;
    collectSceneFeatures (root, nodes);

    SceneSettings settings;

    if (nodes.background != nullptr)
        settings.backgroundColor = nodes.background->color;

    if (nodes.ambient != nullptr)
        settings.ambientColor = nodes.ambient->color;

    if (nodes.exposure != nullptr)
        settings.exposure = nodes.exposure->exposure;

    if (nodes.toneMapping != nullptr)
        settings.toneMapping = nodes.toneMapping->type;

    if (nodes.defaultLight != nullptr)
        settings.useDefaultLight = nodes.defaultLight->enabled;

    if (nodes.environment != nullptr)
    {
        settings.environment = nodes.environment->map;
        settings.environmentIntensity = jmax (0.0f, nodes.environment->intensity);
        settings.environmentVisible = nodes.environment->visible;
        settings.environmentBlur = jlimit (0.0f, 1.0f, nodes.environment->blur);
    }

    if (nodes.fog != nullptr)
        settings.fog = nodes.fog->fog;

    return settings;
}

} // namespace yup
