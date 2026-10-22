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

/** The number of parents between an entity and @p root, or -1 when it isn't under it or something hides it. */
int getActiveDepthBelow (Entity entity, const Entity& root)
{
    for (int depth = 0; entity.isValid(); entity = entity.getParent(), ++depth)
    {
        if (! entity.isVisible())
            return -1;

        if (entity == root)
            return depth;
    }

    return -1;
}

template <class T>
const T* findSceneFeature (Scene& scene, const Entity& root)
{
    const T* found = nullptr;
    int foundDepth = 0;

    // A strictly shallower node moves ahead, so ties keep the first attached
    scene.collect<T>().forEach ([&] (Entity entity, T& node)
    {
        const auto depth = getActiveDepthBelow (entity, root);
        if (depth >= 0 && (found == nullptr || depth < foundDepth))
        {
            found = &node;
            foundDepth = depth;
        }
    });

    return found;
}

} // namespace

//==============================================================================

SceneSettings SceneSettings::resolve (const Scene& scene)
{
    return resolve (scene.getRoot());
}

SceneSettings SceneSettings::resolve (const Entity& root)
{
    SceneSettings settings;

    auto* scene = root.getScene();
    if (scene == nullptr || ! root.isValid())
        return settings;

    if (const auto* node = findSceneFeature<BackgroundNode> (*scene, root))
        settings.backgroundColor = node->color;

    if (const auto* node = findSceneFeature<AmbientLightNode> (*scene, root))
        settings.ambientColor = node->color;

    if (const auto* node = findSceneFeature<ExposureNode> (*scene, root))
        settings.exposure = node->exposure;

    if (const auto* node = findSceneFeature<ToneMappingNode> (*scene, root))
        settings.toneMapping = node->type;

    if (const auto* node = findSceneFeature<DefaultLightNode> (*scene, root))
        settings.useDefaultLight = node->enabled;

    if (const auto* node = findSceneFeature<EnvironmentNode> (*scene, root))
    {
        settings.environment = node->map;
        settings.environmentIntensity = jmax (0.0f, node->intensity);
        settings.environmentVisible = node->visible;
        settings.environmentBlur = jlimit (0.0f, 1.0f, node->blur);
    }

    if (const auto* node = findSceneFeature<WorldFogNode> (*scene, root))
        settings.fog = node->fog;

    return settings;
}

} // namespace yup
