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
    view.update (scene, SceneSettings::resolve (scene), 100, 50, false, 16, 7);

    EXPECT_EQ (view.getBlock().jitter[0], 0.0f);
    EXPECT_EQ (view.getBlock().jitter[1], 0.0f);
    EXPECT_TRUE (std::equal (std::begin (view.getBlock().viewProjection), std::end (view.getBlock().viewProjection),
                             std::begin (view.getBlock().unjitteredViewProjection)));
}

TEST_F (ViewDataTests, JitteredProjectionIsTheRasterizedOne)
{
    Scene scene;
    ViewData view;

    view.update (scene, SceneSettings::resolve (scene), 100, 50, false, 16, 3);
    EXPECT_TRUE (std::equal (view.getJitteredProjection().getData(), view.getJitteredProjection().getData() + 16, view.getProjection().getData()));

    view.update (scene, SceneSettings::resolve (scene), 100, 50, true, 16, 3);
    EXPECT_TRUE (std::equal (view.getJitteredProjection().getData(), view.getJitteredProjection().getData() + 16, view.getBlock().projection));
    EXPECT_FALSE (std::equal (view.getJitteredProjection().getData(), view.getJitteredProjection().getData() + 16, view.getProjection().getData()));
}

TEST_F (ViewDataTests, PreviousIsLastFramesUnjitteredMatrix)
{
    Scene scene;
    ViewData view;
    view.update (scene, SceneSettings::resolve (scene), 100, 100, true, 16, 1);
    const auto first = view.getUnjitteredViewProjection();

    auto camera = EntityNode::Ptr (new EntityNode ("camera"));
    camera->attach<CameraNode>();
    camera->setPosition ({ 1.0f, 0.0f, 5.0f });
    scene.getRoot()->addChild (camera);
    scene.setActiveCamera (camera.get());

    view.update (scene, SceneSettings::resolve (scene), 100, 100, true, 16, 2);
    EXPECT_TRUE (std::equal (first.getData(), first.getData() + 16, view.getBlock().previousViewProjection));
}

TEST_F (ViewDataTests, ResetMakesPreviousEqualCurrent)
{
    Scene scene;
    ViewData view;
    view.update (scene, SceneSettings::resolve (scene), 100, 100, false, 16, 1);
    view.reset();
    view.update (scene, SceneSettings::resolve (scene), 100, 100, false, 16, 2);
    EXPECT_TRUE (std::equal (std::begin (view.getBlock().previousViewProjection), std::end (view.getBlock().previousViewProjection),
                             std::begin (view.getBlock().unjitteredViewProjection)));
}

TEST_F (ViewDataTests, ToneMappingOperatorMatchesShaderIndices)
{
    EXPECT_EQ (ViewData::getToneMappingOperator (ToneMappingOperator::reinhard), 0.0f);
    EXPECT_EQ (ViewData::getToneMappingOperator (ToneMappingOperator::aces), 1.0f);
    EXPECT_EQ (ViewData::getToneMappingOperator (ToneMappingOperator::agx), 2.0f);

    Scene scene;
    scene.getRoot()->attach<ToneMappingNode> (ToneMappingOperator::agx);

    ViewData view;
    view.update (scene, SceneSettings::resolve (scene), 100, 100, false, 16, 1);
    EXPECT_EQ (view.getBlock().clipPlanes[3], 2.0f);
}

TEST_F (ViewDataTests, ShadowNoiseFrameIndexWrapsOnTheCpu)
{
    // A float holds every index up to the 64 frame noise cycle exactly, however long the view has
    // run: past 2^24 frames an unwrapped index would step by 2, and the rotation would freeze
    Scene scene;
    ViewData view;

    const auto frameIndex = (uint64 (1) << 30) + 5;
    view.update (scene, SceneSettings::resolve (scene), 100, 100, true, 16, frameIndex);
    EXPECT_EQ (view.getBlock().clipPlanes[2], 5.0f);

    // The jitter still follows the whole index
    const auto jitter = ViewData::getJitter (frameIndex, 16);
    EXPECT_FLOAT_EQ (view.getBlock().jitter[0], jitter.getX() * 2.0f / 100.0f);
    EXPECT_FLOAT_EQ (view.getBlock().jitter[1], jitter.getY() * 2.0f / 100.0f);

    view.update (scene, SceneSettings::resolve (scene), 100, 100, true, 16, 63);
    EXPECT_EQ (view.getBlock().clipPlanes[2], 63.0f);

    view.update (scene, SceneSettings::resolve (scene), 100, 100, true, 16, 64);
    EXPECT_EQ (view.getBlock().clipPlanes[2], 0.0f);
}

TEST_F (ViewDataTests, CaptureHasExposureOneNoJitterAndItsClipPlanes)
{
    const Vector3<float> position (1.0f, 2.0f, 3.0f);
    const auto view = Matrix4::lookAt (position, position + Vector3<float> (1.0f, 0.0f, 0.0f), { 0.0f, 1.0f, 0.0f });
    const auto projection = Matrix4::perspective (MathConstants<float>::halfPi, 1.0f, 0.05f, 100.0f);

    ViewData data;
    data.setCapture (view, projection, position, 128);

    const auto& block = data.getBlock();
    EXPECT_FLOAT_EQ (block.cameraPosition[3], 1.0f);
    EXPECT_FLOAT_EQ (block.cameraPosition[0], 1.0f);
    EXPECT_FLOAT_EQ (block.viewport[0], 128.0f);
    EXPECT_FLOAT_EQ (block.viewport[3], 1.0f / 128.0f);
    EXPECT_NEAR (block.clipPlanes[0], 0.05f, 1.0e-5f);
    EXPECT_NEAR (block.clipPlanes[1], 100.0f, 0.1f); // Read back from the float projection
    EXPECT_FLOAT_EQ (block.clipPlanes[2], 0.0f);

    for (const auto jitter : block.jitter)
        EXPECT_FLOAT_EQ (jitter, 0.0f);

    // No motion: the previous matrix is the current one, unjittered
    EXPECT_TRUE (std::equal (std::begin (block.viewProjection), std::end (block.viewProjection), block.previousViewProjection));
    EXPECT_TRUE (std::equal (std::begin (block.viewProjection), std::end (block.viewProjection), block.unjitteredViewProjection));
    EXPECT_EQ (data.getCameraPosition(), position);
}

/** CPU only: mirrors viewLinearDepth of lib/view.glsl on the ViewBlock the shaders read. */
class ViewDataLinearDepthTests : public ::testing::Test
{
protected:
    /** Mirrors viewLinearDepth: the view depth of a stored depth, from the (jittered) projection. */
    static float viewLinearDepth (const ViewData::Block& block, float depth)
    {
        const auto* p = block.projection;
        return (depth * p[15] - p[14]) / (depth * p[11] - p[10]);
    }

    /** The depth a surface at view depth @p viewDepth stores, projected through the block's projection. */
    static float storedDepth (const ViewData::Block& block, float viewDepth)
    {
        const auto* p = block.projection;
        const std::array<float, 4> position { 0.3f, -0.2f, -viewDepth, 1.0f };

        auto z = 0.0f;
        auto w = 0.0f;
        for (int j = 0; j < 4; ++j)
        {
            z += p[j * 4 + 2] * position[static_cast<size_t> (j)];
            w += p[j * 4 + 3] * position[static_cast<size_t> (j)];
        }

        return z / w;
    }

    /** Renders nothing: updates a jittered view of a camera configured by @p configure, and checks
        that viewLinearDepth recovers view depths between its planes. */
    template <class Configure>
    static void checkRecoversViewDepth (Configure&& configure, std::initializer_list<float> viewDepths)
    {
        Scene scene;
        auto camera = EntityNode::Ptr (new EntityNode ("camera"));
        configure (camera->attach<CameraNode>());
        scene.getRoot()->addChild (camera);
        scene.setActiveCamera (camera.get());

        ViewData view;
        view.update (scene, SceneSettings::resolve (scene), 100, 50, true, 16, 3);
        ASSERT_NE (view.getBlock().jitter[0], 0.0f);

        for (const auto viewDepth : viewDepths)
        {
            const auto depth = storedDepth (view.getBlock(), viewDepth);
            ASSERT_GT (depth, 0.0f);
            ASSERT_LT (depth, 1.0f);
            EXPECT_NEAR (viewLinearDepth (view.getBlock(), depth), viewDepth, viewDepth * 1.0e-3f) << viewDepth;
        }
    }
};

TEST_F (ViewDataLinearDepthTests, RecoversPerspectiveViewDepth)
{
    checkRecoversViewDepth ([] (CameraNode& camera)
    {
        camera.zNear = 0.1f;
        camera.zFar = 100.0f;
    },
                            { 0.15f, 1.0f, 7.5f, 90.0f });
}

TEST_F (ViewDataLinearDepthTests, RecoversViewDepthWithAnInfiniteFarPlane)
{
    checkRecoversViewDepth ([] (CameraNode& camera)
    {
        camera.zNear = 0.1f;
        camera.zFar = 0.0f;
    },
                            { 0.15f, 1.0f, 7.5f, 500.0f });
}

TEST_F (ViewDataLinearDepthTests, RecoversOrthographicViewDepth)
{
    checkRecoversViewDepth ([] (CameraNode& camera)
    {
        camera.projection = CameraNode::Projection::orthographic;
        camera.zNear = 0.5f;
        camera.zFar = 50.0f;
    },
                            { 0.75f, 3.0f, 25.0f, 49.0f });
}
