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

class SceneTests : public ::testing::Test
{
protected:
    class CountingNode : public Node
    {
    public:
        void update (double deltaSeconds) override
        {
            ++updates;
            totalTime += deltaSeconds;
        }

        int updates = 0;
        double totalTime = 0.0;
    };

    static EntityNode::Ptr makeEntity (const String& name = {})
    {
        return EntityNode::Ptr (new EntityNode (name));
    }

    Scene::Ptr scene { new Scene() };
};

TEST_F (SceneTests, NewSceneHasARootAndNoCamera)
{
    ASSERT_NE (scene->getRoot(), nullptr);
    EXPECT_EQ (scene->getRoot()->getNumChildren(), 0);
    EXPECT_EQ (scene->getActiveCamera(), nullptr);
    EXPECT_TRUE (scene->isUsingDefaultLight());
    EXPECT_FLOAT_EQ (scene->getExposure(), 1.0f);
}

TEST_F (SceneTests, SettingsAreKept)
{
    scene->setBackgroundColor (Colors::red);
    scene->setAmbientColor (Colors::blue);
    scene->setExposure (2.0f);
    scene->setUsingDefaultLight (false);

    EXPECT_EQ (scene->getBackgroundColor(), Colors::red);
    EXPECT_EQ (scene->getAmbientColor(), Colors::blue);
    EXPECT_FLOAT_EQ (scene->getExposure(), 2.0f);
    EXPECT_FALSE (scene->isUsingDefaultLight());
}

TEST_F (SceneTests, UpdateTicksPartsOfVisibleEntities)
{
    auto parent = makeEntity();
    auto child = makeEntity();
    auto hidden = makeEntity();
    scene->getRoot()->addChild (parent);
    parent->addChild (child);
    scene->getRoot()->addChild (hidden);

    auto& rootNode = scene->getRoot()->attach<CountingNode>();
    auto& first = child->attach<CountingNode>();
    auto& second = child->attach<CountingNode, 1>();
    auto& hiddenNode = hidden->attach<CountingNode>();
    hidden->setVisible (false);

    scene->update (0.25);
    scene->update (0.5);

    EXPECT_EQ (rootNode.updates, 2);
    EXPECT_EQ (first.updates, 2);
    EXPECT_EQ (second.updates, 2);
    EXPECT_DOUBLE_EQ (first.totalTime, 0.75);
    EXPECT_EQ (hiddenNode.updates, 0);
}

TEST_F (SceneTests, ActiveCameraUsesTheChosenEntityAndSlot)
{
    auto first = makeEntity();
    auto second = makeEntity();
    scene->getRoot()->addChild (first);
    scene->getRoot()->addChild (second);

    first->attach<CameraNode>();
    second->attach<CameraNode>();
    auto& chosen = second->attach<CameraNode, 1>();

    scene->setActiveCamera (second.get(), 1);

    EXPECT_EQ (scene->getActiveCamera(), &chosen);
}

TEST_F (SceneTests, ActiveCameraFollowsAReplacedCameraInTheSlot)
{
    auto entity = makeEntity();
    scene->getRoot()->addChild (entity);
    entity->attach<CameraNode>();
    scene->setActiveCamera (entity.get());

    auto& replacement = entity->attach<CameraNode>();

    EXPECT_EQ (scene->getActiveCamera(), &replacement);
}

TEST_F (SceneTests, ActiveCameraFallsBackWhenTheEntityIsDeleted)
{
    auto fallback = makeEntity();
    scene->getRoot()->addChild (fallback);
    auto& fallbackCamera = fallback->attach<CameraNode>();

    {
        auto chosen = makeEntity();
        chosen->attach<CameraNode>();
        scene->setActiveCamera (chosen.get());
        EXPECT_NE (scene->getActiveCamera(), &fallbackCamera);
    }

    EXPECT_EQ (scene->getActiveCamera(), &fallbackCamera);
}

TEST_F (SceneTests, ActiveCameraFallsBackWhenTheSlotIsEmpty)
{
    auto withoutCamera = makeEntity();
    auto deep = makeEntity();
    auto withCamera = makeEntity();
    scene->getRoot()->addChild (withoutCamera);
    withoutCamera->addChild (deep);
    scene->getRoot()->addChild (withCamera);

    auto& deepCamera = deep->attach<CameraNode, 2>();
    withCamera->attach<CameraNode>();

    scene->setActiveCamera (withoutCamera.get());

    // Depth-first: the deep camera comes before the next sibling
    EXPECT_EQ (scene->getActiveCamera(), &deepCamera);
}

TEST_F (SceneTests, FallbackSkipsHiddenEntities)
{
    auto hidden = makeEntity();
    auto visible = makeEntity();
    scene->getRoot()->addChild (hidden);
    scene->getRoot()->addChild (visible);
    hidden->attach<CameraNode>();
    auto& visibleCamera = visible->attach<CameraNode>();
    hidden->setVisible (false);

    EXPECT_EQ (scene->getActiveCamera(), &visibleCamera);
}

TEST_F (SceneTests, CameraViewMatrixIsTheInverseOfItsWorldMatrix)
{
    auto entity = makeEntity();
    scene->getRoot()->addChild (entity);
    entity->setPosition ({ 0.0f, 0.0f, 5.0f });
    auto& camera = entity->attach<CameraNode>();

    const auto p = camera.getViewMatrix().transformPoint ({ 0.0f, 0.0f, 0.0f });
    EXPECT_NEAR (p.getZ(), -5.0f, 1.0e-5f);

    EXPECT_TRUE (CameraNode().getViewMatrix().isIdentity());
}

TEST_F (SceneTests, CameraProjectionHandlesInfiniteFarPlane)
{
    CameraNode camera;
    camera.zNear = 0.5f;
    camera.zFar = 0.0f;

    const auto projection = camera.getProjectionMatrix (2.0f);

    // A point on the near plane maps to depth 0, a very far one approaches depth 1
    const auto nearClip = projection.transformPoint4 (0.0f, 0.0f, -0.5f, 1.0f);
    EXPECT_NEAR (nearClip[2] / nearClip[3], 0.0f, 1.0e-5f);

    const auto farClip = projection.transformPoint4 (0.0f, 0.0f, -1.0e6f, 1.0f);
    EXPECT_NEAR (farClip[2] / farClip[3], 1.0f, 1.0e-5f);

    camera.zFar = 100.0f;
    EXPECT_TRUE (camera.getProjectionMatrix (2.0f).approximatelyEqualTo (Matrix4::perspective (camera.yFov, 2.0f, 0.5f, 100.0f)));
}
