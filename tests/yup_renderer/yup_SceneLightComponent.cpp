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

#include <yup_renderer/yup_renderer.h>

#include <gtest/gtest.h>

using namespace yup;

/** The `Light3D` of a `.yui` document: a node giving its entity a LightNode. */
class SceneLightComponentTests : public SceneDocumentTests
{
};

TEST_F (SceneLightComponentTests, PropertiesReachTheLightNode)
{
    YuiDocument document;
    load (document, R"yui(
        Scene3D {
            Light3D {
                id: lamp;
                type: spot;
                color: #ff8000;
                intensity: 8;
                range: 12;
                innerCone: 10deg;
                outerCone: 30deg;
                radius: 0.25;
                castsShadows: true;
            }

            Light3D { id: plain; }
        })yui");

    auto* lamp = document.findById<SceneLightComponent> ("lamp");
    auto* plain = document.findById<SceneLightComponent> ("plain");

    ASSERT_NE (lamp, nullptr);
    ASSERT_NE (plain, nullptr);

    const auto* light = lamp->getEntity().get<LightNode>();
    ASSERT_NE (light, nullptr);

    EXPECT_EQ (light->type, LightNode::Type::spot);
    EXPECT_FLOAT_EQ (light->intensity, 8.0f);
    EXPECT_FLOAT_EQ (light->range, 12.0f);
    EXPECT_NEAR (light->innerConeAngle, MathConstants<float>::pi / 18.0f, 1.0e-5f);
    EXPECT_NEAR (light->outerConeAngle, MathConstants<float>::pi / 6.0f, 1.0e-5f);
    EXPECT_FLOAT_EQ (light->radius, 0.25f);
    EXPECT_TRUE (light->castsShadows);

    // The light shades with the linear values of the color, which reads back as it was set
    EXPECT_NEAR (light->color[0], 1.0f, 1.0e-5f);
    EXPECT_NEAR (light->color[1], toLinear (128.0f / 255.0f), 1.0e-5f);
    EXPECT_NEAR (light->color[2], 0.0f, 1.0e-5f);
    EXPECT_LT (light->color[1], 0.25f);
    EXPECT_EQ (lamp->getLightColor(), Color (0xffff8000));
    EXPECT_EQ (VariantConverter<Color>::fromVar (lamp->getProperty (Identifier ("color"))), Color (0xffff8000));

    // A light that sets nothing is the one a LightNode is by default
    const auto* plainLight = plain->getEntity().get<LightNode>();
    const LightNode defaults {};

    ASSERT_NE (plainLight, nullptr);
    EXPECT_EQ (plainLight->type, LightNode::Type::directional);
    EXPECT_FLOAT_EQ (plainLight->intensity, defaults.intensity);
    EXPECT_FLOAT_EQ (plainLight->outerConeAngle, defaults.outerConeAngle);
    EXPECT_FALSE (plainLight->castsShadows);
    EXPECT_EQ (plain->getLightColor(), Colors::white);
}

TEST_F (SceneLightComponentTests, AChangeReachesTheLightNode)
{
    YuiDocument document;
    load (document, R"yui(
        style {
            .dim { intensity: 0.5; color: #000000; }
        }

        Scene3D { Light3D { id: lamp; type: point; intensity: 4; } })yui");

    auto* lamp = document.findById<SceneLightComponent> ("lamp");
    ASSERT_NE (lamp, nullptr);

    const auto* light = lamp->getEntity().get<LightNode>();
    ASSERT_NE (light, nullptr);

    lamp->setIntensity (6.0f);
    lamp->setType (LightNode::Type::directional);

    EXPECT_FLOAT_EQ (light->intensity, 6.0f);
    EXPECT_EQ (light->type, LightNode::Type::directional);
    EXPECT_EQ (lamp->getProperty (Identifier ("type")).toString(), "directional");

    // A style rule drives the light as it drives a widget
    lamp->addStyleClass (Identifier ("dim"));
    document.getRoot()->getStyleContext()->restyleNow();

    EXPECT_FLOAT_EQ (light->intensity, 0.5f);
    EXPECT_FLOAT_EQ (light->color[0], 0.0f);

    lamp->removeStyleClass (Identifier ("dim"));
    document.getRoot()->getStyleContext()->restyleNow();

    EXPECT_FLOAT_EQ (light->intensity, 6.0f);
    EXPECT_FLOAT_EQ (light->color[0], 1.0f);
}
