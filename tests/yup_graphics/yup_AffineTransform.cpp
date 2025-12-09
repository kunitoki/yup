/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

namespace
{
static constexpr float tol = 1e-5f;
} // namespace

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
    constexpr float pi = 3.14159265358979323846f;
    constexpr float angle = pi / 2.0f; // 90 degrees

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

TEST (AffineTransformTests, Inverted_SingularMatrix)
{
    // Create a singular matrix (determinant = 0)
    // Determinant = scaleX * scaleY - shearX * shearY
    // Let's use: 2 * 1 - 2 * 1 = 0
    AffineTransform singular (2.0f, 2.0f, 0.0f, 1.0f, 1.0f, 0.0f);

    float det = singular.getDeterminant();
    EXPECT_FLOAT_EQ (det, 0.0f);

    AffineTransform inv = singular.inverted();

    // The inverted singular matrix should return itself (as per implementation)
    EXPECT_EQ (inv, singular);
}

TEST (AffineTransformTests, Translation_WithPoint)
{
    // Test static translation with Point
    Point<float> p (5.0f, 10.0f);
    AffineTransform t = AffineTransform::translation (p);
    EXPECT_FLOAT_EQ (t.getTranslateX(), 5.0f);
    EXPECT_FLOAT_EQ (t.getTranslateY(), 10.0f);
    EXPECT_TRUE (t.isOnlyTranslation());

    // Test translated method with Point
    AffineTransform t2 = AffineTransform().translated (p);
    EXPECT_FLOAT_EQ (t2.getTranslateX(), 5.0f);
    EXPECT_FLOAT_EQ (t2.getTranslateY(), 10.0f);
}

TEST (AffineTransformTests, WithAbsoluteTranslation)
{
    // Start with a transform that has translation
    AffineTransform t = AffineTransform::translation (10.0f, 20.0f);

    // Test withAbsoluteTranslation with x, y
    AffineTransform t2 = t.withAbsoluteTranslation (5.0f, 15.0f);
    EXPECT_FLOAT_EQ (t2.getTranslateX(), 5.0f);
    EXPECT_FLOAT_EQ (t2.getTranslateY(), 15.0f);
    EXPECT_FLOAT_EQ (t2.getScaleX(), 1.0f);
    EXPECT_FLOAT_EQ (t2.getScaleY(), 1.0f);

    // Test withAbsoluteTranslation with Point
    Point<float> p (7.0f, 8.0f);
    AffineTransform t3 = t.withAbsoluteTranslation (p);
    EXPECT_FLOAT_EQ (t3.getTranslateX(), 7.0f);
    EXPECT_FLOAT_EQ (t3.getTranslateY(), 8.0f);
}

TEST (AffineTransformTests, Rotated_Methods)
{
    const float angle = degreesToRadians (45.0f);

    // Test rotated() around origin
    AffineTransform t = AffineTransform().rotated (angle);
    EXPECT_NEAR (t.getScaleX(), std::cos (angle), tol);
    EXPECT_NEAR (t.getShearX(), -std::sin (angle), tol);
    EXPECT_NEAR (t.getShearY(), std::sin (angle), tol);
    EXPECT_NEAR (t.getScaleY(), std::cos (angle), tol);

    // Test rotated() around a center point with x, y
    AffineTransform t2 = AffineTransform().rotated (angle, 10.0f, 20.0f);
    // The transform should rotate around (10, 20) instead of origin
    EXPECT_NEAR (t2.getScaleX(), std::cos (angle), tol);
    EXPECT_NEAR (t2.getShearX(), -std::sin (angle), tol);

    // Test rotated() around a center point with Point
    Point<float> center (10.0f, 20.0f);
    AffineTransform t3 = AffineTransform().rotated (angle, center);
    EXPECT_NEAR (t3.getScaleX(), std::cos (angle), tol);
    EXPECT_NEAR (t3.getShearX(), -std::sin (angle), tol);
}

TEST (AffineTransformTests, Rotation_WithCenter)
{
    const float angle = degreesToRadians (90.0f);

    // Test static rotation with center x, y
    AffineTransform t = AffineTransform::rotation (angle, 10.0f, 10.0f);
    Point<float> testPoint (10.0f, 10.0f);
    float x = testPoint.getX();
    float y = testPoint.getY();
    t.transformPoint (x, y);
    // Point at center should not move
    EXPECT_NEAR (x, 10.0f, tol);
    EXPECT_NEAR (y, 10.0f, tol);

    // Test static rotation with center Point
    Point<float> center (5.0f, 5.0f);
    AffineTransform t2 = AffineTransform::rotation (angle, center);
    x = center.getX();
    y = center.getY();
    t2.transformPoint (x, y);
    EXPECT_NEAR (x, 5.0f, tol);
    EXPECT_NEAR (y, 5.0f, tol);
}

TEST (AffineTransformTests, Scaled_WithCenter)
{
    // Test scaled() with center x, y
    AffineTransform t = AffineTransform().scaled (2.0f, 3.0f, 10.0f, 10.0f);
    Point<float> centerPoint (10.0f, 10.0f);
    float x = centerPoint.getX();
    float y = centerPoint.getY();
    t.transformPoint (x, y);
    // Center point should not move
    EXPECT_NEAR (x, 10.0f, tol);
    EXPECT_NEAR (y, 10.0f, tol);

    // Test scaled() with center Point
    Point<float> center (5.0f, 5.0f);
    AffineTransform t2 = AffineTransform().scaled (2.0f, 3.0f, center);
    x = center.getX();
    y = center.getY();
    t2.transformPoint (x, y);
    EXPECT_NEAR (x, 5.0f, tol);
    EXPECT_NEAR (y, 5.0f, tol);
}

TEST (AffineTransformTests, Scaling_WithCenter)
{
    // Test uniform scaling with center
    AffineTransform t = AffineTransform::scaling (2.0f);
    EXPECT_FLOAT_EQ (t.getScaleX(), 2.0f);
    EXPECT_FLOAT_EQ (t.getScaleY(), 2.0f);

    // Test static scaling with center x, y
    AffineTransform t2 = AffineTransform::scaling (2.0f, 3.0f, 10.0f, 10.0f);
    Point<float> centerPoint (10.0f, 10.0f);
    float x = centerPoint.getX();
    float y = centerPoint.getY();
    t2.transformPoint (x, y);
    EXPECT_NEAR (x, 10.0f, tol);
    EXPECT_NEAR (y, 10.0f, tol);

    // Test static scaling with center Point
    Point<float> center (5.0f, 5.0f);
    AffineTransform t3 = AffineTransform::scaling (2.0f, 3.0f, center);
    x = center.getX();
    y = center.getY();
    t3.transformPoint (x, y);
    EXPECT_NEAR (x, 5.0f, tol);
    EXPECT_NEAR (y, 5.0f, tol);
}

TEST (AffineTransformTests, Shearing_WithCenter)
{
    // Test static shearing with center x, y
    AffineTransform t = AffineTransform::shearing (1.0f, 0.5f, 10.0f, 10.0f);
    Point<float> centerPoint (10.0f, 10.0f);
    float x = centerPoint.getX();
    float y = centerPoint.getY();
    t.transformPoint (x, y);
    // Center point should remain at center after shearing
    EXPECT_NEAR (x, 10.0f, tol);
    EXPECT_NEAR (y, 10.0f, tol);

    // Test static shearing with center Point
    Point<float> center (5.0f, 5.0f);
    AffineTransform t2 = AffineTransform::shearing (1.0f, 0.5f, center);
    x = center.getX();
    y = center.getY();
    t2.transformPoint (x, y);
    EXPECT_NEAR (x, 5.0f, tol);
    EXPECT_NEAR (y, 5.0f, tol);
}

TEST (AffineTransformTests, PrependedBy)
{
    // Create two transforms
    AffineTransform t1 = AffineTransform::scaling (2.0f);
    AffineTransform t2 = AffineTransform::translation (5.0f, 10.0f);

    // prependedBy means: other * this = t2 * t1
    // This applies t1 first, then t2
    AffineTransform result = t1.prependedBy (t2);
    // This is: translate then scale
    // Point (1, 1) -> translate (5, 10) -> (6, 11) -> scale (2) -> (12, 22)
    Point<float> p (1.0f, 1.0f);
    float x = p.getX();
    float y = p.getY();
    result.transformPoint (x, y);
    EXPECT_FLOAT_EQ (x, 12.0f);
    EXPECT_FLOAT_EQ (y, 22.0f);

    // Compare with followedBy which is: this * other = t1 * t2
    // This applies t2 first, then t1
    AffineTransform result2 = t1.followedBy (t2);
    x = 1.0f;
    y = 1.0f;
    result2.transformPoint (x, y);
    // This is: scale then translate
    // Point (1, 1) -> scale (2) -> (2, 2) -> translate (5, 10) -> (7, 12)
    EXPECT_FLOAT_EQ (x, 7.0f);
    EXPECT_FLOAT_EQ (y, 12.0f);
}

TEST (AffineTransformTests, MultiplicationOperator)
{
    // Create two transforms
    AffineTransform t1 = AffineTransform::scaling (2.0f);
    AffineTransform t2 = AffineTransform::translation (5.0f, 10.0f);

    // operator* is equivalent to followedBy
    // t1 * t2 means: apply t2 first, then t1
    AffineTransform result = t1 * t2;

    // This is equivalent to: scale then translate
    // Point (1, 1) -> translate (5, 10) -> (6, 11) -> scale (2) -> (12, 22)
    Point<float> p (1.0f, 1.0f);
    float x = p.getX();
    float y = p.getY();
    result.transformPoint (x, y);
    EXPECT_FLOAT_EQ (x, 12.0f);
    EXPECT_FLOAT_EQ (y, 22.0f);

    // Verify it's the same as followedBy
    AffineTransform result2 = t1.followedBy (t2);
    EXPECT_EQ (result, result2);

    // Test chaining multiple operators
    AffineTransform t3 = AffineTransform::rotation (degreesToRadians (90.0f));
    AffineTransform chained = t1 * t2 * t3;

    // Should be equivalent to t1.followedBy(t2).followedBy(t3)
    AffineTransform expected = t1.followedBy (t2).followedBy (t3);
    EXPECT_TRUE (chained.approximatelyEqualTo (expected));
}

TEST (AffineTransformTests, ToString)
{
    AffineTransform t (1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f);
    String str = t.toString();

    // The string should contain all 6 matrix values
    EXPECT_TRUE (str.contains ("1"));
    EXPECT_TRUE (str.contains ("2"));
    EXPECT_TRUE (str.contains ("3"));
    EXPECT_TRUE (str.contains ("4"));
    EXPECT_TRUE (str.contains ("5"));
    EXPECT_TRUE (str.contains ("6"));
}

TEST (AffineTransformTests, EdgeCases_ZeroRotation)
{
    // Test rotation with zero angle (should be identity)
    AffineTransform t = AffineTransform::rotation (0.0f);
    EXPECT_TRUE (t.isIdentity());

    AffineTransform t2 = AffineTransform().rotated (0.0f);
    EXPECT_TRUE (t2.isIdentity());
}

TEST (AffineTransformTests, EdgeCases_MultipleTransforms)
{
    // Combine multiple transforms
    AffineTransform t = AffineTransform::translation (10.0f, 20.0f)
                            .followedBy (AffineTransform::rotation (degreesToRadians (90.0f)))
                            .followedBy (AffineTransform::scaling (2.0f));

    // Transform a test point
    Point<float> p (1.0f, 0.0f);
    float x = p.getX();
    float y = p.getY();
    t.transformPoint (x, y);

    // Expected: (1, 0) -> translate -> (11, 20) -> rotate 90° -> (-20, 11) -> scale 2 -> (-40, 22)
    EXPECT_NEAR (x, -40.0f, tol);
    EXPECT_NEAR (y, 22.0f, tol);
}

TEST (AffineTransformTests, Identity_Operations)
{
    AffineTransform identity = AffineTransform::identity();
    AffineTransform translation = AffineTransform::translation (5.0f, 10.0f);

    // Identity followed by translation should equal translation
    AffineTransform result1 = identity.followedBy (translation);
    EXPECT_EQ (result1, translation);

    // Translation followed by identity should equal translation
    AffineTransform result2 = translation.followedBy (identity);
    EXPECT_EQ (result2, translation);

    // Identity prepended by translation should equal translation
    AffineTransform result3 = identity.prependedBy (translation);
    EXPECT_EQ (result3, translation);
}

TEST (AffineTransformTests, Inversion_RoundTrip)
{
    // Create a complex transform
    AffineTransform t = AffineTransform::translation (5.0f, 10.0f)
                            .followedBy (AffineTransform::rotation (degreesToRadians (30.0f)))
                            .followedBy (AffineTransform::scaling (2.0f));

    // Invert it
    AffineTransform inv = t.inverted();

    // Apply transform then inverse should give identity
    AffineTransform roundTrip = t.followedBy (inv);

    EXPECT_NEAR (roundTrip.getScaleX(), 1.0f, tol);
    EXPECT_NEAR (roundTrip.getShearX(), 0.0f, tol);
    EXPECT_NEAR (roundTrip.getTranslateX(), 0.0f, tol);
    EXPECT_NEAR (roundTrip.getShearY(), 0.0f, tol);
    EXPECT_NEAR (roundTrip.getScaleY(), 1.0f, tol);
    EXPECT_NEAR (roundTrip.getTranslateY(), 0.0f, tol);
}

TEST (AffineTransformTests, IsOnlyTranslation)
{
    // Test identity (no translation)
    AffineTransform identity = AffineTransform::identity();
    EXPECT_TRUE (identity.isOnlyTranslation());

    // Test pure translation
    AffineTransform translation = AffineTransform::translation (5.0f, 10.0f);
    EXPECT_TRUE (translation.isOnlyTranslation());

    // Test translation with Point
    AffineTransform translationPoint = AffineTransform::translation (Point<float> (3.0f, 7.0f));
    EXPECT_TRUE (translationPoint.isOnlyTranslation());

    // Test rotation (should not be only translation)
    AffineTransform rotation = AffineTransform::rotation (degreesToRadians (45.0f));
    EXPECT_FALSE (rotation.isOnlyTranslation());

    // Test scaling (should not be only translation)
    AffineTransform scaling = AffineTransform::scaling (2.0f);
    EXPECT_FALSE (scaling.isOnlyTranslation());

    // Test shearing (should not be only translation)
    AffineTransform shearing = AffineTransform::shearing (1.0f, 0.5f);
    EXPECT_FALSE (shearing.isOnlyTranslation());

    // Test combined transforms (translation + rotation)
    AffineTransform combined = AffineTransform::translation (5.0f, 10.0f)
                                   .followedBy (AffineTransform::rotation (degreesToRadians (30.0f)));
    EXPECT_FALSE (combined.isOnlyTranslation());

    // Test combined transforms (translation + scaling)
    AffineTransform combinedScale = AffineTransform::translation (5.0f, 10.0f)
                                        .followedBy (AffineTransform::scaling (2.0f));
    EXPECT_FALSE (combinedScale.isOnlyTranslation());

    // Test custom transform with only translation components
    AffineTransform customTranslation (1.0f, 0.0f, 15.0f, 0.0f, 1.0f, 20.0f);
    EXPECT_TRUE (customTranslation.isOnlyTranslation());

    // Test custom transform with scale
    AffineTransform customScale (2.0f, 0.0f, 15.0f, 0.0f, 2.0f, 20.0f);
    EXPECT_FALSE (customScale.isOnlyTranslation());

    // Test custom transform with shear
    AffineTransform customShear (1.0f, 0.5f, 15.0f, 0.0f, 1.0f, 20.0f);
    EXPECT_FALSE (customShear.isOnlyTranslation());
}

TEST (AffineTransformTests, IsOnlyRotation)
{
    // Test identity (special case - 0 rotation)
    AffineTransform identity = AffineTransform::identity();
    EXPECT_TRUE (identity.isOnlyRotation());

    // Test pure rotation (90 degrees)
    AffineTransform rotation90 = AffineTransform::rotation (degreesToRadians (90.0f));
    EXPECT_TRUE (rotation90.isOnlyRotation());

    // Test pure rotation (45 degrees)
    AffineTransform rotation45 = AffineTransform::rotation (degreesToRadians (45.0f));
    EXPECT_TRUE (rotation45.isOnlyRotation());

    // Test pure rotation (180 degrees)
    AffineTransform rotation180 = AffineTransform::rotation (degreesToRadians (180.0f));
    EXPECT_TRUE (rotation180.isOnlyRotation());

    // Test pure rotation (negative angle)
    AffineTransform rotationNeg = AffineTransform::rotation (degreesToRadians (-30.0f));
    EXPECT_TRUE (rotationNeg.isOnlyRotation());

    // Test rotation with translation
    AffineTransform rotationWithTranslation = AffineTransform::rotation (degreesToRadians (45.0f))
                                                  .translated (5.0f, 10.0f);
    EXPECT_FALSE (rotationWithTranslation.isOnlyRotation());

    // Test rotation with scaling
    AffineTransform rotationWithScaling = AffineTransform::rotation (degreesToRadians (45.0f))
                                              .scaled (2.0f);
    EXPECT_FALSE (rotationWithScaling.isOnlyRotation());

    // Test pure translation (not rotation)
    AffineTransform translation = AffineTransform::translation (5.0f, 10.0f);
    EXPECT_FALSE (translation.isOnlyRotation());

    // Test pure scaling (not rotation)
    AffineTransform scaling = AffineTransform::scaling (2.0f);
    EXPECT_FALSE (scaling.isOnlyRotation());

    // Test shearing (not rotation)
    AffineTransform shearing = AffineTransform::shearing (1.0f, 0.5f);
    EXPECT_FALSE (shearing.isOnlyRotation());
}

TEST (AffineTransformTests, IsOnlyUniformScaling)
{
    // Test identity (scale = 1, should return false)
    AffineTransform identity = AffineTransform::identity();
    EXPECT_FALSE (identity.isOnlyUniformScaling());

    // Test pure uniform scaling (scale = 2)
    AffineTransform scaling2 = AffineTransform::scaling (2.0f);
    EXPECT_TRUE (scaling2.isOnlyUniformScaling());

    // Test pure uniform scaling (scale = 0.5)
    AffineTransform scalingHalf = AffineTransform::scaling (0.5f);
    EXPECT_TRUE (scalingHalf.isOnlyUniformScaling());

    // Test pure uniform scaling (scale = 3)
    AffineTransform scaling3 = AffineTransform::scaling (3.0f);
    EXPECT_TRUE (scaling3.isOnlyUniformScaling());

    // Test non-uniform scaling
    AffineTransform nonUniform = AffineTransform::scaling (2.0f, 3.0f);
    EXPECT_FALSE (nonUniform.isOnlyUniformScaling());

    // Test uniform scaling with translation
    AffineTransform scalingWithTranslation = AffineTransform::scaling (2.0f)
                                                 .translated (5.0f, 10.0f);
    EXPECT_FALSE (scalingWithTranslation.isOnlyUniformScaling());

    // Test uniform scaling with rotation
    AffineTransform scalingWithRotation = AffineTransform::scaling (2.0f)
                                              .rotated (degreesToRadians (45.0f));
    EXPECT_FALSE (scalingWithRotation.isOnlyUniformScaling());

    // Test pure translation (not scaling)
    AffineTransform translation = AffineTransform::translation (5.0f, 10.0f);
    EXPECT_FALSE (translation.isOnlyUniformScaling());

    // Test pure rotation (not scaling)
    AffineTransform rotation = AffineTransform::rotation (degreesToRadians (45.0f));
    EXPECT_FALSE (rotation.isOnlyUniformScaling());
}

TEST (AffineTransformTests, IsOnlyNonUniformScaling)
{
    // Test identity (not non-uniform scaling)
    AffineTransform identity = AffineTransform::identity();
    EXPECT_FALSE (identity.isOnlyNonUniformScaling());

    // Test pure non-uniform scaling
    AffineTransform nonUniform = AffineTransform::scaling (2.0f, 3.0f);
    EXPECT_TRUE (nonUniform.isOnlyNonUniformScaling());

    // Test pure non-uniform scaling (different factors)
    AffineTransform nonUniform2 = AffineTransform::scaling (0.5f, 2.0f);
    EXPECT_TRUE (nonUniform2.isOnlyNonUniformScaling());

    // Test uniform scaling (not non-uniform)
    AffineTransform uniform = AffineTransform::scaling (2.0f);
    EXPECT_FALSE (uniform.isOnlyNonUniformScaling());

    // Test non-uniform scaling with translation
    AffineTransform nonUniformWithTranslation = AffineTransform::scaling (2.0f, 3.0f)
                                                    .translated (5.0f, 10.0f);
    EXPECT_FALSE (nonUniformWithTranslation.isOnlyNonUniformScaling());

    // Test pure translation (not scaling)
    AffineTransform translation = AffineTransform::translation (5.0f, 10.0f);
    EXPECT_FALSE (translation.isOnlyNonUniformScaling());

    // Test pure rotation (not scaling)
    AffineTransform rotation = AffineTransform::rotation (degreesToRadians (45.0f));
    EXPECT_FALSE (rotation.isOnlyNonUniformScaling());
}

TEST (AffineTransformTests, IsOnlyScaling)
{
    // Test identity (not scaling)
    AffineTransform identity = AffineTransform::identity();
    EXPECT_FALSE (identity.isOnlyScaling());

    // Test pure uniform scaling
    AffineTransform uniform = AffineTransform::scaling (2.0f);
    EXPECT_TRUE (uniform.isOnlyScaling());

    // Test pure non-uniform scaling
    AffineTransform nonUniform = AffineTransform::scaling (2.0f, 3.0f);
    EXPECT_TRUE (nonUniform.isOnlyScaling());

    // Test scaling with translation
    AffineTransform scalingWithTranslation = AffineTransform::scaling (2.0f)
                                                 .translated (5.0f, 10.0f);
    EXPECT_FALSE (scalingWithTranslation.isOnlyScaling());

    // Test scaling with rotation
    AffineTransform scalingWithRotation = AffineTransform::scaling (2.0f)
                                              .rotated (degreesToRadians (45.0f));
    EXPECT_FALSE (scalingWithRotation.isOnlyScaling());

    // Test pure translation (not scaling)
    AffineTransform translation = AffineTransform::translation (5.0f, 10.0f);
    EXPECT_FALSE (translation.isOnlyScaling());

    // Test pure rotation (not scaling)
    AffineTransform rotation = AffineTransform::rotation (degreesToRadians (45.0f));
    EXPECT_FALSE (rotation.isOnlyScaling());

    // Test shearing (not scaling)
    AffineTransform shearing = AffineTransform::shearing (1.0f, 0.5f);
    EXPECT_FALSE (shearing.isOnlyScaling());
}

TEST (AffineTransformTests, IsOnlyShearing)
{
    // Test identity (not shearing)
    AffineTransform identity = AffineTransform::identity();
    EXPECT_FALSE (identity.isOnlyShearing());

    // Test pure shearing
    AffineTransform shearing = AffineTransform::shearing (1.0f, 0.5f);
    EXPECT_TRUE (shearing.isOnlyShearing());

    // Test shearing with only x factor
    AffineTransform shearingX = AffineTransform::shearing (1.0f, 0.0f);
    EXPECT_TRUE (shearingX.isOnlyShearing());

    // Test shearing with only y factor
    AffineTransform shearingY = AffineTransform::shearing (0.0f, 0.5f);
    EXPECT_TRUE (shearingY.isOnlyShearing());

    // Test shearing with translation
    AffineTransform shearingWithTranslation = AffineTransform::shearing (1.0f, 0.5f)
                                                  .translated (5.0f, 10.0f);
    EXPECT_FALSE (shearingWithTranslation.isOnlyShearing());

    // Test shearing with scaling
    AffineTransform shearingWithScaling = AffineTransform::shearing (1.0f, 0.5f)
                                              .scaled (2.0f);
    EXPECT_FALSE (shearingWithScaling.isOnlyShearing());

    // Test pure translation (not shearing)
    AffineTransform translation = AffineTransform::translation (5.0f, 10.0f);
    EXPECT_FALSE (translation.isOnlyShearing());

    // Test pure rotation (not shearing)
    AffineTransform rotation = AffineTransform::rotation (degreesToRadians (45.0f));
    EXPECT_FALSE (rotation.isOnlyShearing());

    // Test pure scaling (not shearing)
    AffineTransform scaling = AffineTransform::scaling (2.0f);
    EXPECT_FALSE (scaling.isOnlyShearing());
}

TEST (AffineTransformTests, TransformationType_Combinations)
{
    // Test that identity satisfies both translation and rotation
    AffineTransform identity = AffineTransform::identity();
    EXPECT_TRUE (identity.isIdentity());
    EXPECT_TRUE (identity.isOnlyTranslation());
    EXPECT_TRUE (identity.isOnlyRotation());
    EXPECT_FALSE (identity.isOnlyScaling());
    EXPECT_FALSE (identity.isOnlyShearing());

    // Test that each pure transformation is mutually exclusive
    AffineTransform translation = AffineTransform::translation (5.0f, 10.0f);
    EXPECT_TRUE (translation.isOnlyTranslation());
    EXPECT_FALSE (translation.isOnlyRotation());
    EXPECT_FALSE (translation.isOnlyScaling());
    EXPECT_FALSE (translation.isOnlyShearing());

    AffineTransform rotation = AffineTransform::rotation (degreesToRadians (45.0f));
    EXPECT_FALSE (rotation.isOnlyTranslation());
    EXPECT_TRUE (rotation.isOnlyRotation());
    EXPECT_FALSE (rotation.isOnlyScaling());
    EXPECT_FALSE (rotation.isOnlyShearing());

    AffineTransform scaling = AffineTransform::scaling (2.0f);
    EXPECT_FALSE (scaling.isOnlyTranslation());
    EXPECT_FALSE (scaling.isOnlyRotation());
    EXPECT_TRUE (scaling.isOnlyScaling());
    EXPECT_FALSE (scaling.isOnlyShearing());

    AffineTransform shearing = AffineTransform::shearing (1.0f, 0.5f);
    EXPECT_FALSE (shearing.isOnlyTranslation());
    EXPECT_FALSE (shearing.isOnlyRotation());
    EXPECT_FALSE (shearing.isOnlyScaling());
    EXPECT_TRUE (shearing.isOnlyShearing());

    // Test combined transforms are not "only" any single type
    AffineTransform combined = AffineTransform::translation (5.0f, 10.0f)
                                   .followedBy (AffineTransform::rotation (degreesToRadians (45.0f)))
                                   .followedBy (AffineTransform::scaling (2.0f));
    EXPECT_FALSE (combined.isOnlyTranslation());
    EXPECT_FALSE (combined.isOnlyRotation());
    EXPECT_FALSE (combined.isOnlyScaling());
    EXPECT_FALSE (combined.isOnlyShearing());
}
