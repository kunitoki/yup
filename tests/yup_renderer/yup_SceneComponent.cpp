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

class SceneComponentTests : public ::testing::Test
{
};

TEST_F (SceneComponentTests, IsOpaqueAndTakesFocusWhenClicked)
{
    SceneComponent component;

    EXPECT_TRUE (component.isOpaque());
    EXPECT_TRUE (component.getWantsKeyboardFocus());
}

TEST_F (SceneComponentTests, KeepsSceneAndUpdateMode)
{
    SceneComponent component;
    auto scene = Scene::Ptr (new Scene());

    EXPECT_EQ (component.getScene(), nullptr);
    EXPECT_FALSE (component.isUpdatingContinuously());

    component.setScene (scene);
    component.setContinuousUpdates (true);

    EXPECT_EQ (component.getScene(), scene);
    EXPECT_TRUE (component.isUpdatingContinuously());
}

TEST_F (SceneComponentTests, SetSceneMakesTheComponentOpaqueOrNotAfterItsBackground)
{
    SceneComponent component;

    // The default background is opaque
    component.setScene (Scene::Ptr (new Scene()));
    EXPECT_TRUE (component.isOpaque());

    auto seeThrough = Scene::Ptr (new Scene());
    seeThrough->getRoot().attach<BackgroundNode> (Colors::transparentBlack);
    component.setScene (seeThrough);
    EXPECT_FALSE (component.isOpaque());

    // Without a scene the component fills itself
    component.setScene (nullptr);
    EXPECT_TRUE (component.isOpaque());
}

TEST_F (SceneComponentTests, OpaqueStateFollowsTheBackgroundOnEveryDisplayFrame)
{
    SceneComponent component;
    auto scene = Scene::Ptr (new Scene());
    component.setScene (scene);
    ASSERT_TRUE (component.isOpaque());

    // Nothing tells the component that the scene changed: its next display frame finds out
    auto& background = scene->getRoot().attach<BackgroundNode> (Colors::black.withAlpha (0.5f));
    EXPECT_TRUE (component.isOpaque());

    component.refreshDisplay (0.0);
    EXPECT_FALSE (component.isOpaque());

    // So does a color edited in place
    background.color = Colors::black;
    component.refreshDisplay (0.0);
    EXPECT_TRUE (component.isOpaque());

    background.color = Colors::transparentBlack;
    component.refreshDisplay (0.0);
    EXPECT_FALSE (component.isOpaque());

    // Without its node the background is the default one again
    EXPECT_TRUE (scene->getRoot().detach<BackgroundNode>());
    component.refreshDisplay (0.0);
    EXPECT_TRUE (component.isOpaque());
}
