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

class TransformNodeTests : public ::testing::Test
{
protected:
    static void expectNear (const Vector3<float>& actual, const Vector3<float>& expected)
    {
        EXPECT_NEAR (actual.getX(), expected.getX(), 1.0e-4f);
        EXPECT_NEAR (actual.getY(), expected.getY(), 1.0e-4f);
        EXPECT_NEAR (actual.getZ(), expected.getZ(), 1.0e-4f);
    }

    Scene scene;
};

TEST_F (TransformNodeTests, DefaultsToIdentity)
{
    const TransformNode transform;

    expectNear (transform.getPosition(), { 0.0f, 0.0f, 0.0f });
    expectNear (transform.getScale(), { 1.0f, 1.0f, 1.0f });
    EXPECT_TRUE (transform.getLocalMatrix().approximatelyEqualTo (Matrix4()));
}

TEST_F (TransformNodeTests, ConstructorSetsEveryComponent)
{
    const auto rotation = Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.5f);
    const TransformNode transform ({ 1.0f, 2.0f, 3.0f }, rotation, { 2.0f, 2.0f, 2.0f });

    expectNear (transform.getPosition(), { 1.0f, 2.0f, 3.0f });
    expectNear (transform.getScale(), { 2.0f, 2.0f, 2.0f });
    EXPECT_TRUE (transform.getRotation().toMatrix4().approximatelyEqualTo (rotation.toMatrix4()));
}

TEST_F (TransformNodeTests, LocalMatrixScalesThenRotatesThenTranslates)
{
    const TransformNode transform ({ 10.0f, 0.0f, 0.0f },
                                   Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, MathConstants<float>::halfPi),
                                   { 2.0f, 2.0f, 2.0f });

    expectNear (transform.getLocalMatrix().transformPoint ({ 1.0f, 0.0f, 0.0f }), { 10.0f, 2.0f, 0.0f });
}

TEST_F (TransformNodeTests, SettersUpdateTheLocalMatrix)
{
    TransformNode transform;
    const auto before = transform.getLocalMatrix();

    transform.setPosition ({ 0.0f, 5.0f, 0.0f });
    EXPECT_FALSE (transform.getLocalMatrix().approximatelyEqualTo (before));
    expectNear (transform.getLocalMatrix().transformPoint ({}), { 0.0f, 5.0f, 0.0f });

    transform.setScale ({ 3.0f, 3.0f, 3.0f });
    expectNear (transform.getLocalMatrix().transformPoint ({ 1.0f, 0.0f, 0.0f }), { 3.0f, 5.0f, 0.0f });
}

TEST_F (TransformNodeTests, SettersMoveTheWorldMatrixOfTheSubtree)
{
    auto parent = scene.createEntity();
    auto child = scene.createEntity ("child", parent);
    auto& transform = parent.attach<TransformNode>();

    expectNear (child.getWorldMatrix().transformPoint ({}), { 0.0f, 0.0f, 0.0f });

    transform.setPosition ({ 4.0f, 0.0f, 0.0f });

    expectNear (child.getWorldMatrix().transformPoint ({}), { 4.0f, 0.0f, 0.0f });
}

TEST_F (TransformNodeTests, AttachAndDetachMoveTheWorldMatrix)
{
    auto entity = scene.createEntity();
    expectNear (entity.getWorldMatrix().transformPoint ({}), { 0.0f, 0.0f, 0.0f });

    entity.attach<TransformNode> (Vector3<float> { 0.0f, 0.0f, 7.0f });
    expectNear (entity.getWorldMatrix().transformPoint ({}), { 0.0f, 0.0f, 7.0f });

    entity.detach<TransformNode>();
    expectNear (entity.getWorldMatrix().transformPoint ({}), { 0.0f, 0.0f, 0.0f });
}
