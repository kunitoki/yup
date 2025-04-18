/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

#include <yup_graphics/yup_graphics.h>

#include <cmath>

using namespace yup;

static constexpr float tol = 1e-5f;

TEST (AffineTransformTests, Default_IsIdentity)
{
    AffineTransform t;
    EXPECT_TRUE (t.isIdentity());
    EXPECT_FLOAT_EQ (t.getScaleX(), 1.0f);
    EXPECT_FLOAT_EQ (t.getShearX(), 0.0f);
    EXPECT_FLOAT_EQ (t.getTranslateX(), 0.0f);
    EXPECT_FLOAT_EQ (t.getShearY(), 0.0f);
    EXPECT_FLOAT_EQ (t.getScaleY(), 1.0f);
    EXPECT_FLOAT_EQ (t.getTranslateY(), 0.0f);
}

TEST (AffineTransformTests, Parameterized_Constructor)
{
    constexpr float sX = 2.0f, shX = 3.0f, tX = 4.0f;
    constexpr float shY = 5.0f, sY = 6.0f, tY = 7.0f;
    AffineTransform t (sX, shX, tX, shY, sY, tY);

    EXPECT_FLOAT_EQ (t.getScaleX(), sX);
    EXPECT_FLOAT_EQ (t.getShearX(), shX);
    EXPECT_FLOAT_EQ (t.getTranslateX(), tX);
    EXPECT_FLOAT_EQ (t.getShearY(), shY);
    EXPECT_FLOAT_EQ (t.getScaleY(), sY);
    EXPECT_FLOAT_EQ (t.getTranslateY(), tY);
}

TEST (AffineTransformTests, ResetToIdentity)
{
    AffineTransform t (2.0f, 0.0f, 5.0f, 0.0f, 3.0f, 6.0f);
    t.resetToIdentity();
    EXPECT_TRUE (t.isIdentity());
}

TEST (AffineTransformTests, Static_Identity)
{
    AffineTransform t = AffineTransform::identity();
    EXPECT_TRUE (t.isIdentity());
}

TEST (AffineTransformTests, Inversion)
{
    // Use a uniform scaling which is invertible.
    AffineTransform t = AffineTransform::scaling (2.0f);
    AffineTransform inv = t.inverted();
    AffineTransform result = t.followedBy (inv);

    // The result should equal the identity transform.
    EXPECT_NEAR (result.getScaleX(), 1.0f, tol);
    EXPECT_NEAR (result.getShearX(), 0.0f, tol);
    EXPECT_NEAR (result.getTranslateX(), 0.0f, tol);
    EXPECT_NEAR (result.getShearY(), 0.0f, tol);
    EXPECT_NEAR (result.getScaleY(), 1.0f, tol);
    EXPECT_NEAR (result.getTranslateY(), 0.0f, tol);
}

TEST (AffineTransformTests, Translation)
{
    // Test the static translation() function.
    AffineTransform t = AffineTransform::translation (3.0f, 4.0f);
    EXPECT_FLOAT_EQ (t.getScaleX(), 1.0f);
    EXPECT_FLOAT_EQ (t.getShearX(), 0.0f);
    EXPECT_FLOAT_EQ (t.getTranslateX(), 3.0f);
    EXPECT_FLOAT_EQ (t.getShearY(), 0.0f);
    EXPECT_FLOAT_EQ (t.getScaleY(), 1.0f);
    EXPECT_FLOAT_EQ (t.getTranslateY(), 4.0f);

    // Test the translated() method on the identity.
    AffineTransform t2 = AffineTransform().translated (3.0f, 4.0f);
    EXPECT_FLOAT_EQ (t2.getTranslateX(), 3.0f);
    EXPECT_FLOAT_EQ (t2.getTranslateY(), 4.0f);
}

TEST (AffineTransformTests, Rotation)
{
    const float pi = 3.14159265358979323846f;
    const float angle = pi / 2.0f; // 90 degrees

    // Test the static rotation() function.
    AffineTransform t = AffineTransform::rotation (angle);
    // Expected values: cos(pi/2) ~ 0, sin(pi/2) ~ 1.
    EXPECT_NEAR (t.getScaleX(), 0.0f, tol);
    EXPECT_NEAR (t.getShearX(), -1.0f, tol);
    EXPECT_NEAR (t.getTranslateX(), 0.0f, tol);
    EXPECT_NEAR (t.getShearY(), 1.0f, tol);
    EXPECT_NEAR (t.getScaleY(), 0.0f, tol);
    EXPECT_NEAR (t.getTranslateY(), 0.0f, tol);

    // Test the rotated() method on the identity transform.
    AffineTransform t2 = AffineTransform().rotated (angle);
    EXPECT_NEAR (t2.getScaleX(), 0.0f, tol);
    EXPECT_NEAR (t2.getShearX(), -1.0f, tol);
    EXPECT_NEAR (t2.getShearY(), 1.0f, tol);
    EXPECT_NEAR (t2.getScaleY(), 0.0f, tol);
}

TEST (AffineTransformTests, Scaling)
{
    // Uniform scaling
    AffineTransform t = AffineTransform::scaling (2.0f);
    EXPECT_FLOAT_EQ (t.getScaleX(), 2.0f);
    EXPECT_FLOAT_EQ (t.getShearX(), 0.0f);
    EXPECT_FLOAT_EQ (t.getTranslateX(), 0.0f);
    EXPECT_FLOAT_EQ (t.getShearY(), 0.0f);
    EXPECT_FLOAT_EQ (t.getScaleY(), 2.0f);
    EXPECT_FLOAT_EQ (t.getTranslateY(), 0.0f);

    // Non-uniform scaling
    AffineTransform t2 = AffineTransform::scaling (3.0f, 4.0f);
    EXPECT_FLOAT_EQ (t2.getScaleX(), 3.0f);
    EXPECT_FLOAT_EQ (t2.getScaleY(), 4.0f);
}

TEST (AffineTransformTests, Shearing)
{
    // Test the static shearing() function.
    AffineTransform t = AffineTransform::shearing (1.0f, 2.0f);
    EXPECT_FLOAT_EQ (t.getScaleX(), 1.0f);
    EXPECT_FLOAT_EQ (t.getShearX(), 1.0f);
    EXPECT_FLOAT_EQ (t.getTranslateX(), 0.0f);
    EXPECT_FLOAT_EQ (t.getShearY(), 2.0f);
    EXPECT_FLOAT_EQ (t.getScaleY(), 1.0f);
    EXPECT_FLOAT_EQ (t.getTranslateY(), 0.0f);

    // Test the sheared() method on the identity transform.
    AffineTransform id;
    AffineTransform t2 = id.sheared (1.0f, 2.0f);
    EXPECT_FLOAT_EQ (t2.getScaleX(), 1.0f);
    EXPECT_FLOAT_EQ (t2.getShearX(), 1.0f);
    EXPECT_FLOAT_EQ (t2.getShearY(), 2.0f);
    EXPECT_FLOAT_EQ (t2.getScaleY(), 1.0f);
}

TEST (AffineTransformTests, FollowedBy)
{
    AffineTransform t1 = AffineTransform::translation (3.0f, 4.0f);
    AffineTransform t2 = AffineTransform::scaling (2.0f);
    AffineTransform combined = t1.followedBy (t2);

    // For t1 (translation): translate = (3,4)
    // For t2 (scaling): scale = 2, so the combined translation is scaled accordingly.
    // Expected combined values:
    //   scaleX: 2 * 1 = 2
    //   shearX: 2 * 0 = 0
    //   translateX: 2 * 3 + 0 = 6
    //   shearY:   0
    //   scaleY: 2 * 1 = 2
    //   translateY: 2 * 4 + 0 = 8
    EXPECT_FLOAT_EQ (combined.getScaleX(), 2.0f);
    EXPECT_FLOAT_EQ (combined.getShearX(), 0.0f);
    EXPECT_FLOAT_EQ (combined.getTranslateX(), 6.0f);
    EXPECT_FLOAT_EQ (combined.getShearY(), 0.0f);
    EXPECT_FLOAT_EQ (combined.getScaleY(), 2.0f);
    EXPECT_FLOAT_EQ (combined.getTranslateY(), 8.0f);
}

TEST (AffineTransformTests, TransformPoint)
{
    AffineTransform t = AffineTransform::translation (5.0f, -3.0f);
    float x = 1.0f, y = 2.0f;
    t.transformPoint (x, y);
    EXPECT_FLOAT_EQ (x, 6.0f);
    EXPECT_FLOAT_EQ (y, -1.0f);
}

TEST (AffineTransformTests, TransformPoints_Multiple)
{
    AffineTransform t = AffineTransform::translation (2.0f, 3.0f);
    float x1 = 0.0f, y1 = 0.0f;
    float x2 = 1.0f, y2 = 1.0f;
    float x3 = -1.0f, y3 = -1.0f;

    t.transformPoints (x1, y1, x2, y2, x3, y3);
    EXPECT_FLOAT_EQ (x1, 2.0f);
    EXPECT_FLOAT_EQ (y1, 3.0f);
    EXPECT_FLOAT_EQ (x2, 3.0f);
    EXPECT_FLOAT_EQ (y2, 4.0f);
    EXPECT_FLOAT_EQ (x3, 1.0f);
    EXPECT_FLOAT_EQ (y3, 2.0f);
}

TEST (AffineTransformTests, EqualityOperators)
{
    AffineTransform t1 = AffineTransform::scaling (2.0f);
    AffineTransform t2 = AffineTransform::scaling (2.0f);
    AffineTransform t3 = AffineTransform::translation (2.0f, 3.0f);

    EXPECT_TRUE (t1 == t2);
    EXPECT_FALSE (t1 != t2);
    EXPECT_FALSE (t1 == t3);
    EXPECT_TRUE (t1 != t3);
}

TEST (AffineTransformTests, Determinant)
{
    // Identity determinant should be 1.
    AffineTransform t;
    EXPECT_FLOAT_EQ (t.getDeterminant(), 1.0f);

    // For a uniform scaling transform, determinant is (scale factor)^2.
    AffineTransform t2 = AffineTransform::scaling (3.0f);
    EXPECT_FLOAT_EQ (t2.getDeterminant(), 9.0f);
}

TEST (AffineTransformTests, ScaleFactor)
{
    // For a uniform scaling of 4, the scale factor should be 4.
    AffineTransform t = AffineTransform::scaling (4.0f);
    EXPECT_FLOAT_EQ (t.getScaleFactor(), 4.0f);
}

TEST (AffineTransformTests, MatrixPoints)
{
    AffineTransform t (2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f);
    auto span = t.getMatrixPoints();

    ASSERT_EQ (span.size(), static_cast<std::size_t> (6));
    EXPECT_FLOAT_EQ (span[0], 2.0f);
    EXPECT_FLOAT_EQ (span[1], 3.0f);
    EXPECT_FLOAT_EQ (span[2], 4.0f);
    EXPECT_FLOAT_EQ (span[3], 5.0f);
    EXPECT_FLOAT_EQ (span[4], 6.0f);
    EXPECT_FLOAT_EQ (span[5], 7.0f);
}

TEST (AffineTransformTests, ToMat2D_Identity)
{
    AffineTransform affineIdentity = AffineTransform::identity();
    auto matrixFromAffine = affineIdentity.toMat2D();

    rive::Mat2D identityMatrix;

    EXPECT_FLOAT_EQ (matrixFromAffine.xx(), identityMatrix.xx());
    EXPECT_FLOAT_EQ (matrixFromAffine.xy(), identityMatrix.xy());
    EXPECT_FLOAT_EQ (matrixFromAffine.yx(), identityMatrix.yx());
    EXPECT_FLOAT_EQ (matrixFromAffine.yy(), identityMatrix.yy());
    EXPECT_FLOAT_EQ (matrixFromAffine.tx(), identityMatrix.tx());
    EXPECT_FLOAT_EQ (matrixFromAffine.ty(), identityMatrix.ty());
}

TEST (AffineTransformTests, ToMat2D_CustomTransform)
{
    AffineTransform affine (2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f);
    auto matrixFromAffine = affine.toMat2D();

    rive::Mat2D expectedMatrix (2.0f, -3.0f, -5.0f, 6.0f, 4.0f, 7.0f);

    EXPECT_FLOAT_EQ (matrixFromAffine.xx(), expectedMatrix.xx());
    EXPECT_FLOAT_EQ (matrixFromAffine.xy(), expectedMatrix.xy());
    EXPECT_FLOAT_EQ (matrixFromAffine.yx(), expectedMatrix.yx());
    EXPECT_FLOAT_EQ (matrixFromAffine.yy(), expectedMatrix.yy());
    EXPECT_FLOAT_EQ (matrixFromAffine.tx(), expectedMatrix.tx());
    EXPECT_FLOAT_EQ (matrixFromAffine.ty(), expectedMatrix.ty());
}

TEST (AffineTransformTests, ToMat2D_TranslationTransform)
{
    AffineTransform affine = AffineTransform::translation (10.0f, -20.0f);
    auto matrixFromAffine = affine.toMat2D();

    rive::Mat2D expectedMatrix = rive::Mat2D::fromTranslate (10.0f, -20.0f);

    EXPECT_FLOAT_EQ (matrixFromAffine.xx(), expectedMatrix.xx());
    EXPECT_FLOAT_EQ (matrixFromAffine.xy(), expectedMatrix.xy());
    EXPECT_FLOAT_EQ (matrixFromAffine.yx(), expectedMatrix.yx());
    EXPECT_FLOAT_EQ (matrixFromAffine.yy(), expectedMatrix.yy());
    EXPECT_FLOAT_EQ (matrixFromAffine.tx(), expectedMatrix.tx());
    EXPECT_FLOAT_EQ (matrixFromAffine.ty(), expectedMatrix.ty());
}

TEST (AffineTransformTests, ToMat2D_RotationTransform)
{
    const float angle = degreesToRadians (90.0f);

    AffineTransform affine = AffineTransform::rotation (angle);
    auto matrixFromAffine = affine.toMat2D();

    rive::Mat2D expectedMatrix = rive::Mat2D::fromRotation (angle);

    EXPECT_NEAR (matrixFromAffine.xx(), expectedMatrix.xx(), tol);
    EXPECT_NEAR (matrixFromAffine.xy(), expectedMatrix.xy(), tol);
    EXPECT_NEAR (matrixFromAffine.yx(), expectedMatrix.yx(), tol);
    EXPECT_NEAR (matrixFromAffine.yy(), expectedMatrix.yy(), tol);
    EXPECT_NEAR (matrixFromAffine.tx(), expectedMatrix.tx(), tol);
    EXPECT_NEAR (matrixFromAffine.ty(), expectedMatrix.ty(), tol);
}
