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

#include <gtest/gtest.h>

#include <yup_renderer/yup_renderer.h>

using namespace yup;

class SceneSettingsTests : public ::testing::Test
{
protected:
    static EnvironmentMap::Ptr makeEnvironment()
    {
        return new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (1.0f, 1.0f, 1.0f); }, 16, 2);
    }

    Entity addChild()
    {
        return scene->createEntity();
    }

    Scene::Ptr scene { new Scene() };
};

TEST_F (SceneSettingsTests, SceneWithoutFeatureNodesUsesTheDefaults)
{
    const auto settings = SceneSettings::resolve (*scene);

    EXPECT_EQ (settings.backgroundColor, Color (0xff1a1a2e));
    EXPECT_EQ (settings.ambientColor, Color (0xff595959));
    EXPECT_FLOAT_EQ (settings.exposure, 1.0f);
    EXPECT_EQ (settings.toneMapping, ToneMappingOperator::reinhard);
    EXPECT_TRUE (settings.useDefaultLight);
    EXPECT_EQ (settings.environment, nullptr);
    EXPECT_FLOAT_EQ (settings.environmentIntensity, 1.0f);
    EXPECT_FALSE (settings.environmentVisible);
    EXPECT_FLOAT_EQ (settings.environmentBlur, 0.0f);
    EXPECT_FALSE (settings.fog.enabled);
    EXPECT_TRUE (settings.fog == WorldFog {});
}

TEST_F (SceneSettingsTests, DefaultConstructedSettingsMatchAnEmptyScene)
{
    const SceneSettings defaults;
    const auto resolved = SceneSettings::resolve (*scene);

    EXPECT_EQ (defaults.backgroundColor, resolved.backgroundColor);
    EXPECT_EQ (defaults.ambientColor, resolved.ambientColor);
    EXPECT_FLOAT_EQ (defaults.exposure, resolved.exposure);
    EXPECT_EQ (defaults.toneMapping, resolved.toneMapping);
    EXPECT_EQ (defaults.useDefaultLight, resolved.useDefaultLight);
    EXPECT_TRUE (defaults.fog == resolved.fog);
}

TEST_F (SceneSettingsTests, FeatureNodesOnTheRootAreResolved)
{
    auto root = scene->getRoot();
    root.attach<BackgroundNode> (Colors::red);
    root.attach<AmbientLightNode> (Colors::blue);
    root.attach<ExposureNode> (2.0f);
    root.attach<ToneMappingNode> (ToneMappingOperator::agx);
    root.attach<DefaultLightNode> (false);

    const auto settings = SceneSettings::resolve (*scene);

    EXPECT_EQ (settings.backgroundColor, Colors::red);
    EXPECT_EQ (settings.ambientColor, Colors::blue);
    EXPECT_FLOAT_EQ (settings.exposure, 2.0f);
    EXPECT_EQ (settings.toneMapping, ToneMappingOperator::agx);
    EXPECT_FALSE (settings.useDefaultLight);
}

TEST_F (SceneSettingsTests, NodeNearestToTheRootWins)
{
    auto first = addChild();
    auto deep = first.createChild();
    auto second = addChild();

    deep.attach<ToneMappingNode> (ToneMappingOperator::aces);
    second.attach<ToneMappingNode> (ToneMappingOperator::agx);

    EXPECT_EQ (SceneSettings::resolve (*scene).toneMapping, ToneMappingOperator::agx);

    scene->getRoot().attach<ToneMappingNode> (ToneMappingOperator::aces);
    EXPECT_EQ (SceneSettings::resolve (*scene).toneMapping, ToneMappingOperator::aces);
}

TEST_F (SceneSettingsTests, FirstAttachedWinsAtTheSameDepth)
{
    auto first = addChild();
    auto second = addChild();

    second.attach<ExposureNode> (2.0f);
    first.attach<ExposureNode> (3.0f);
    EXPECT_FLOAT_EQ (SceneSettings::resolve (*scene).exposure, 2.0f);

    second.detach<ExposureNode>();
    second.attach<ExposureNode> (2.0f);
    EXPECT_FLOAT_EQ (SceneSettings::resolve (*scene).exposure, 3.0f);
}

TEST_F (SceneSettingsTests, HiddenEntitiesAreIgnored)
{
    auto hidden = addChild();
    hidden.attach<ExposureNode> (4.0f);
    hidden.setVisible (false);

    auto visible = addChild();
    visible.attach<ExposureNode> (0.5f);

    EXPECT_FLOAT_EQ (SceneSettings::resolve (*scene).exposure, 0.5f);
}

TEST_F (SceneSettingsTests, EnvironmentIsResolvedAndLimited)
{
    const auto environment = makeEnvironment();
    auto& node = scene->getRoot().attach<EnvironmentNode> (environment);
    node.visible = true;
    node.intensity = -1.0f;
    node.blur = 2.0f;

    auto settings = SceneSettings::resolve (*scene);
    EXPECT_EQ (settings.environment, environment);
    EXPECT_TRUE (settings.environmentVisible);
    EXPECT_FLOAT_EQ (settings.environmentIntensity, 0.0f);
    EXPECT_FLOAT_EQ (settings.environmentBlur, 1.0f);

    node.blur = -1.0f;
    EXPECT_FLOAT_EQ (SceneSettings::resolve (*scene).environmentBlur, 0.0f);
}

TEST_F (SceneSettingsTests, WorldFogNodeEnablesTheFog)
{
    auto& node = scene->getRoot().attach<WorldFogNode>();
    node.fog.density = 0.1f;
    node.fog.anisotropy = 0.3f;

    const auto settings = SceneSettings::resolve (*scene);
    EXPECT_TRUE (settings.fog.enabled);
    EXPECT_TRUE (settings.fog == node.fog);
}
