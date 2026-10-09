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
