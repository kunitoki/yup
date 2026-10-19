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

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (GBufferPassMetalTests, MaskedTexelsAreNotWritten)
{
    // Below and above the default 0.5 cutoff, centered at about x = 23 and x = 41
    const auto cutOut = makeMasked (0.2f);
    const auto kept = makeMasked (0.8f);

    Scene scene;
    scene.getRoot()->addChild (makeQuad (-0.6f, 0.4f, cutOut));
    scene.getRoot()->addChild (makeQuad (0.6f, 0.4f, kept));

    const auto targets = render (scene);

    const auto ids = mapTexels (targets.emissionMaterial, "vec4(t.a / 255.0, 0.0, 0.0, 1.0)");
    const auto cleared = mapTexels (targets.depth, "vec4(t.r >= 1.0 ? 1.0 : 0.0, 0.0, 0.0, 1.0)");
    ASSERT_TRUE (ids.isValid());
    ASSERT_TRUE (cleared.isValid());

    const auto keptId = materials.getId (*kept);
    EXPECT_GT (keptId, 0);
    EXPECT_EQ (static_cast<int> (ids.getPixelColor (41, 32).getRed()), keptId);
    EXPECT_EQ (cleared.getPixelColor (41, 32).getRed(), 0);

    EXPECT_EQ (ids.getPixelColor (23, 32).getRed(), 0);
    EXPECT_EQ (cleared.getPixelColor (23, 32).getRed(), 255);
}

TEST_F (GBufferPassMetalTests, BackgroundFillCoversOnlyTheEmptyTexels)
{
    Scene scene;
    scene.getRoot()->addChild (makeQuad (0.0f, 0.5f, Material::Ptr (new Material())));

    auto camera = EntityNode::Ptr (new EntityNode ("camera"));
    camera->attach<CameraNode>();
    camera->setPosition ({ 0.0f, 0.0f, 5.0f });
    scene.getRoot()->addChild (camera);
    scene.setActiveCamera (camera.get());

    // A still camera: the quad, moved right since the previous frame, is the only motion
    const auto moved = readVelocity (render (scene, { 0.2f, 0.0f, 0.0f }).velocity, 1000.0f);
    ASSERT_TRUE (moved.isValid());
    EXPECT_GT (moved.getPixelColor (32, 32).getRed(), 192);
    EXPECT_EQ (velocityDeviation (moved.getPixelColor (2, 32).getRed()), 0);
    EXPECT_EQ (velocityDeviation (moved.getPixelColor (2, 32).getGreen()), 0);

    // A turning camera moves the background
    camera->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.05f));
    const auto turned = readVelocity (render (scene).velocity, 1000.0f);
    ASSERT_TRUE (turned.isValid());
    EXPECT_GT (velocityDeviation (turned.getPixelColor (2, 32).getRed()), 64);

    // And stops moving it once the camera stays
    const auto still = readVelocity (render (scene).velocity, 1000.0f);
    ASSERT_TRUE (still.isValid());
    EXPECT_EQ (maxVelocityDeviation (still), 0);
}
#endif
