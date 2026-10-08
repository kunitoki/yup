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

#include <yup_3d/yup_3d.h>

using namespace yup;

class ViewDataTests : public ::testing::Test
{
};

TEST_F (ViewDataTests, HaltonMatchesKnownValues)
{
    EXPECT_FLOAT_EQ (ViewData::halton (1, 2), 0.5f);
    EXPECT_FLOAT_EQ (ViewData::halton (2, 2), 0.25f);
    EXPECT_FLOAT_EQ (ViewData::halton (3, 2), 0.75f);
    EXPECT_FLOAT_EQ (ViewData::halton (1, 3), 1.0f / 3.0f);
    EXPECT_FLOAT_EQ (ViewData::halton (2, 3), 2.0f / 3.0f);
    EXPECT_FLOAT_EQ (ViewData::halton (3, 3), 1.0f / 9.0f);
}

TEST_F (ViewDataTests, JitterIsCenteredAndCycles)
{
    Point<float> sum;
    for (uint64 i = 0; i < 16; ++i)
    {
        const auto j = ViewData::getJitter (i, 16);
        EXPECT_GE (j.getX(), -0.5f);
        EXPECT_LT (j.getX(), 0.5f);
        sum += j;
    }

    EXPECT_NEAR (sum.getX() / 16.0f, 0.0f, 0.05f);
    EXPECT_NEAR (sum.getY() / 16.0f, 0.0f, 0.05f);
    EXPECT_EQ (ViewData::getJitter (3, 16), ViewData::getJitter (19, 16));
}

TEST_F (ViewDataTests, UnjitteredWhenNotRequested)
{
    Scene scene;
    ViewData view;
    view.update (scene, 100, 50, false, 16, 7);

    EXPECT_EQ (view.getBlock().jitter[0], 0.0f);
    EXPECT_EQ (view.getBlock().jitter[1], 0.0f);
    EXPECT_TRUE (std::equal (std::begin (view.getBlock().viewProjection), std::end (view.getBlock().viewProjection),
                             std::begin (view.getBlock().unjitteredViewProjection)));
}

TEST_F (ViewDataTests, PreviousIsLastFramesUnjitteredMatrix)
{
    Scene scene;
    ViewData view;
    view.update (scene, 100, 100, true, 16, 1);
    const auto first = view.getUnjitteredViewProjection();

    auto camera = EntityNode::Ptr (new EntityNode ("camera"));
    camera->attach<CameraNode>();
    camera->setPosition ({ 1.0f, 0.0f, 5.0f });
    scene.getRoot()->addChild (camera);
    scene.setActiveCamera (camera.get());

    view.update (scene, 100, 100, true, 16, 2);
    EXPECT_TRUE (std::equal (first.getData(), first.getData() + 16, view.getBlock().previousViewProjection));
}

TEST_F (ViewDataTests, ResetMakesPreviousEqualCurrent)
{
    Scene scene;
    ViewData view;
    view.update (scene, 100, 100, false, 16, 1);
    view.reset();
    view.update (scene, 100, 100, false, 16, 2);
    EXPECT_TRUE (std::equal (std::begin (view.getBlock().previousViewProjection), std::end (view.getBlock().previousViewProjection),
                             std::begin (view.getBlock().unjitteredViewProjection)));
}

TEST_F (ViewDataTests, ToneMappingOperatorMatchesShaderIndices)
{
    EXPECT_EQ (ViewData::getToneMappingOperator (Scene::ToneMapping::reinhard), 0.0f);
    EXPECT_EQ (ViewData::getToneMappingOperator (Scene::ToneMapping::aces), 1.0f);
    EXPECT_EQ (ViewData::getToneMappingOperator (Scene::ToneMapping::agx), 2.0f);

    Scene scene;
    scene.setToneMapping (Scene::ToneMapping::agx);

    ViewData view;
    view.update (scene, 100, 100, false, 16, 1);
    EXPECT_EQ (view.getBlock().clipPlanes[3], 2.0f);
}
