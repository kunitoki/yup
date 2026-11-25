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

class SkinTests : public ::testing::Test
{
};

TEST_F (SkinTests, KeepsItsNameJointsAndInverseBindMatrices)
{
    const auto lowered = Matrix4::translation ({ 0.0f, -1.0f, 0.0f });
    const Skin skin ("arm", { 4, 7 }, { Matrix4(), lowered });

    EXPECT_EQ (skin.getName(), "arm");
    EXPECT_EQ (skin.getNumJoints(), 2);

    ASSERT_EQ (skin.getJoints().size(), 2u);
    EXPECT_EQ (skin.getJoints()[0], 4);
    EXPECT_EQ (skin.getJoints()[1], 7);

    ASSERT_EQ (skin.getInverseBindMatrices().size(), 2u);
    EXPECT_TRUE (skin.getInverseBindMatrices()[0].isIdentity());
    EXPECT_TRUE (skin.getInverseBindMatrices()[1] == lowered);
}

TEST_F (SkinTests, InverseBindMatricesAreTheIdentityWhenNoneIsGiven)
{
    const Skin skin ("arm", { 0, 1, 2 });

    ASSERT_EQ (skin.getInverseBindMatrices().size(), 3u);

    for (const auto& matrix : skin.getInverseBindMatrices())
        EXPECT_TRUE (matrix.isIdentity());
}

TEST_F (SkinTests, SkinWithoutJointsIsEmpty)
{
    const Skin skin ("none", {});

    EXPECT_EQ (skin.getNumJoints(), 0);
    EXPECT_TRUE (skin.getJoints().empty());
    EXPECT_TRUE (skin.getInverseBindMatrices().empty());
}

TEST_F (SkinTests, HasOneInverseBindMatrixPerJointWhateverIsGiven)
{
#if YUP_ASSERTIONS_ENABLED_OR_LOGGED
    GTEST_SKIP() << "Another number of inverse bind matrices than joints triggers an assertion when assertions are enabled or logged";
#endif

    const auto lowered = Matrix4::translation ({ 0.0f, -1.0f, 0.0f });

    // The matrices missing are the identity, the ones in excess are dropped
    const Skin tooFew ("arm", { 0, 1 }, { lowered });
    ASSERT_EQ (tooFew.getInverseBindMatrices().size(), 2u);
    EXPECT_TRUE (tooFew.getInverseBindMatrices()[0] == lowered);
    EXPECT_TRUE (tooFew.getInverseBindMatrices()[1].isIdentity());

    const Skin tooMany ("arm", { 0 }, { lowered, lowered });
    EXPECT_EQ (tooMany.getInverseBindMatrices().size(), 1u);
}
