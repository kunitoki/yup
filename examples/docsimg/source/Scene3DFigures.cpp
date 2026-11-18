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

#include "Figures.h"

#include <yup_renderer/yup_renderer.h>

namespace
{

//==============================================================================
/** A studio to light the coin: a bright soft box above on the left and a dimmer one on the
    right, in a room that is darker towards the floor. */
yup::EnvironmentMap::Ptr createStudio()
{
    return new yup::EnvironmentMap ([] (const yup::Vector3<float>& direction)
    {
        const auto softBox = [&direction] (yup::Vector3<float> center, float size)
        {
            return yup::jlimit (0.0f, 1.0f, (direction.dotProduct (center.normalized()) - (1.0f - size)) / 0.02f);
        };

        const auto room = 0.08f + 0.25f * yup::jmax (0.0f, direction.getY());

        return yup::Vector3<float> (room, room, room * 1.05f)
             + yup::Vector3<float> (9.0f, 8.6f, 8.0f) * softBox ({ -0.6f, 0.7f, 0.5f }, 0.12f)
             + yup::Vector3<float> (2.5f, 2.6f, 2.8f) * softBox ({ 0.8f, 0.4f, 0.6f }, 0.08f);
    });
}

/** The scene of the figures: three coins standing on a floor, further and further from the camera. */
yup::Scene::Ptr createCoinScene()
{
    using namespace yup;

    auto scene = Scene::Ptr (new Scene());

    auto ground = Material::Ptr (new Material());
    ground->baseColorFactor = { 0.45f, 0.46f, 0.5f, 1.0f };
    ground->metallicFactor = 0.0f;
    ground->roughnessFactor = 0.7f;

    auto floor = Mesh::Ptr (new Mesh ("floor"));
    floor->addPrimitive ({ { { -60.0f, 0.0f, 12.0f } }, { { 60.0f, 0.0f, 12.0f } }, { { 60.0f, 0.0f, -90.0f } }, { { -60.0f, 0.0f, -90.0f } } }, { 0, 1, 2, 0, 2, 3 }, ground);
    scene->createEntity ("floor").attach<MeshNode> (floor);

    if (auto model = GltfModel::loadFromFile (docsimgFile ("data/coin.glb")); model.wasOk())
    {
        const std::pair<Vector3<float>, float> coins[] = { { { 0.0f, 1.0f, 0.0f }, 0.45f }, { { -3.6f, 1.0f, -4.0f }, -0.3f }, { { 4.4f, 1.0f, -9.0f }, 0.2f } };

        for (const auto& [position, turn] : coins)
        {
            auto coin = scene->createEntity ("coin");
            coin.attach<TransformNode> (position, Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, turn));
            model.getReference().instantiate (*scene, coin);
        }
    }

    auto camera = scene->createEntity ("camera");
    camera.attach<CameraNode>();
    camera.attach<TransformNode> (Vector3<float> { 0.0f, 1.9f, 8.0f }, Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, -0.1f));
    scene->setActiveCamera (camera);

    return scene;
}

/** Lights a scene with the studio alone, in place of the flat ambient light. */
void addStudio (yup::Scene& scene)
{
    scene.getRoot().attach<yup::EnvironmentNode> (createStudio());
    scene.getRoot().attach<yup::AmbientLightNode> (yup::Colors::black);
}

/** Adds a sun shining from above, in front and on the left, and returns its light. */
yup::LightNode& addSun (yup::Scene& scene)
{
    using namespace yup;

    auto sun = scene.createEntity ("sun");
    sun.attach<TransformNode>().setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, -0.7f) * Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, -0.9f));

    auto& light = sun.attach<LightNode>();
    light.intensity = 3.0f;
    return light;
}

std::unique_ptr<yup::SceneComponent> show (yup::Scene::Ptr scene, const yup::RenderSettings& settings = {})
{
    auto view = std::make_unique<yup::SceneComponent>();
    view->getRenderer().setRenderSettings (settings);
    view->setScene (std::move (scene));
    return view;
}

/** The frames a view is drawn for, so what the renderer refines over frames has settled. */
constexpr int settledFrames = 48;

} // namespace

//==============================================================================

void addScene3DFigures (Figures& f)
{
    using namespace yup;

    auto& coin = f.component ("3d/showing-a-scene_coin", { 560.0f, 262.0f }, []
    {
        return show (createCoinScene());
    });
    coin.padding = 0.0f;
    coin.frames = settledFrames;
    coin.themed = false;

    f.component ("3d/indirect-lighting_environment", { 560.0f, 192.0f }, []() -> std::unique_ptr<Component>
    {
        auto gallery = std::make_unique<Gallery> (2);
        gallery->add ("flat ambient light", show (createCoinScene()));

        auto scene = createCoinScene();
        addStudio (*scene);
        gallery->add ("lit by an environment", show (scene));

        return gallery;
    }).frames = settledFrames;

    f.component ("3d/lights-and-shadows_shadows", { 560.0f, 166.0f }, []() -> std::unique_ptr<Component>
    {
        const char* const names[] = { "castsShadows = false", "castsShadows = true", "radius = 0.06" };
        auto gallery = std::make_unique<Gallery> (3);

        for (int index = 0; index < 3; ++index)
        {
            auto scene = createCoinScene();

            auto& sun = addSun (*scene);
            sun.castsShadows = index > 0;
            sun.radius = index == 2 ? 0.06f : 0.0f;

            gallery->add (names[index], show (scene));
        }

        return gallery;
    }).frames = settledFrames;

    f.component ("3d/post-effects_effects", { 560.0f, 166.0f }, []() -> std::unique_ptr<Component>
    {
        const char* const names[] = { "no effect", "bloom", "depthOfField" };
        auto gallery = std::make_unique<Gallery> (3);

        for (int index = 0; index < 3; ++index)
        {
            auto scene = createCoinScene();
            addStudio (*scene);

            RenderSettings settings;
            settings.bloom = index == 1;
            settings.bloomIntensity = 0.25f;
            settings.depthOfField = index == 2;

            if (auto* camera = scene->getActiveCamera())
            {
                camera->fStop = 0.05f;
                camera->focusDistance = 8.0f;
            }

            gallery->add (names[index], show (scene, settings));
        }

        return gallery;
    }).frames = settledFrames;

    f.component ("3d/render-settings_tone-mapping", { 560.0f, 166.0f }, []() -> std::unique_ptr<Component>
    {
        const char* const names[] = { "reinhard", "aces", "agx" };
        auto gallery = std::make_unique<Gallery> (3);

        for (int index = 0; index < 3; ++index)
        {
            auto scene = createCoinScene();
            addStudio (*scene);
            scene->getRoot().attach<ToneMappingNode> (static_cast<ToneMappingOperator> (index));

            gallery->add (names[index], show (scene));
        }

        return gallery;
    }).frames = settledFrames;
}
