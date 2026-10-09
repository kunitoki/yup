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

#include <yup_3d/yup_3d.h>

#include <gtest/gtest.h>

using namespace yup;

class ShadowCascadesTests : public ::testing::Test
{
protected:
    using Cascades = std::array<ShadowCascades::Cascade, ShadowCascades::maxCascades>;

    static constexpr float yFov = 0.8f;
    static constexpr float aspect = 16.0f / 9.0f;

    // A camera at (0, 2, 10) looking at the origin, inside receivers so wide that the range is the
    // camera's [near, far] whatever the camera orientation
    static ShadowCascades::Inputs makeInputs()
    {
        ShadowCascades::Inputs inputs;
        inputs.cameraWorld = Matrix4::lookAt ({ 0.0f, 2.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }).inverted();
        inputs.projection = Matrix4::perspective (yFov, aspect, 0.1f, 100.0f);
        inputs.nearPlane = 0.1f;
        inputs.farPlane = 100.0f;
        inputs.lightDirection = Vector3<float> (-0.3f, -1.0f, -0.2f).normalized();
        inputs.receiverBounds = BoundingBox ({ -1.0e4f, -1.0f, -1.0e4f }, { 1.0e4f, 3.0f, 1.0e4f });
        inputs.casterBounds = BoundingBox ({ -50.0f, -1.0f, -50.0f }, { 50.0f, 3.0f, 50.0f });
        inputs.count = 4;
        inputs.resolution = 1024;
        return inputs;
    }

    static std::array<float, 4> clipOf (const Matrix4& m, const Vector3<float>& p)
    {
        return m.transformPoint4 (p.getX(), p.getY(), p.getZ(), 1.0f);
    }

    // Every cascade holds the corners of its slice of the fixture's perspective frustum, and keeps the
    // half texel snapping can move the map inside its radius
    static void expectSlicesContained (const ShadowCascades::Inputs& inputs, const Cascades& cascades)
    {
        const auto range = ShadowCascades::computeShadowRange (inputs);
        const auto splits = ShadowCascades::computeSplits (range.getStart(), range.getEnd(), inputs.count, inputs.lambda);
        const auto tanY = std::tan (yFov * 0.5f);
        const auto tanX = tanY * aspect;

        for (size_t c = 0; c < (size_t) inputs.count; ++c)
        {
            const auto& cascade = cascades[c];
            EXPECT_FLOAT_EQ (cascade.splitFar, splits[c + 1]);

            for (float z : { splits[c], splits[c + 1] })
            {
                for (float sx : { -1.0f, 1.0f })
                {
                    for (float sy : { -1.0f, 1.0f })
                    {
                        const auto corner = inputs.cameraWorld.transformPoint ({ sx * tanX * z, sy * tanY * z, -z });
                        const auto clip = clipOf (cascade.viewProjection, corner);

                        EXPECT_LE (std::abs (clip[0] / clip[3]), 1.0f) << "cascade " << c;
                        EXPECT_LE (std::abs (clip[1] / clip[3]), 1.0f) << "cascade " << c;
                        EXPECT_GE (clip[2] / clip[3], 0.0f) << "cascade " << c;
                        EXPECT_LE (clip[2] / clip[3], 1.0f) << "cascade " << c;
                        EXPECT_GE (cascade.radius, (corner - cascade.center).length() + 0.5f * cascade.texelWorldSize) << "cascade " << c;
                    }
                }
            }
        }
    }
};

TEST_F (ShadowCascadesTests, SplitsStartAtNearEndAtFarAndIncrease)
{
    const auto splits = ShadowCascades::computeSplits (0.1f, 100.0f, 4, 0.75f);

    EXPECT_FLOAT_EQ (splits[0], 0.1f);
    EXPECT_FLOAT_EQ (splits[4], 100.0f);

    for (int i = 0; i < 4; ++i)
        EXPECT_LT (splits[(size_t) i], splits[(size_t) i + 1]);
}

TEST_F (ShadowCascadesTests, LambdaBlendsUniformAndLogarithmic)
{
    const auto uniform = ShadowCascades::computeSplits (1.0f, 101.0f, 4, 0.0f);
    EXPECT_NEAR (uniform[1], 26.0f, 1.0e-3f);
    EXPECT_NEAR (uniform[2], 51.0f, 1.0e-3f);

    const auto logarithmic = ShadowCascades::computeSplits (1.0f, 10000.0f, 4, 1.0f);
    EXPECT_NEAR (logarithmic[1], 10.0f, 1.0e-2f);
    EXPECT_NEAR (logarithmic[2], 100.0f, 1.0e-1f);
    EXPECT_NEAR (logarithmic[3], 1000.0f, 1.0f);

    // Halfway: the mean of 101^(i/4) and 1 + 25 i
    const auto blended = ShadowCascades::computeSplits (1.0f, 101.0f, 4, 0.5f);
    EXPECT_NEAR (blended[1], 14.58508f, 1.0e-3f);
    EXPECT_NEAR (blended[2], 30.52494f, 1.0e-3f);
    EXPECT_NEAR (blended[3], 53.92983f, 1.0e-3f);
}

TEST_F (ShadowCascadesTests, SingleCascadeSpansEverything)
{
    const auto splits = ShadowCascades::computeSplits (0.5f, 20.0f, 1, 0.75f);
    EXPECT_FLOAT_EQ (splits[0], 0.5f);
    EXPECT_FLOAT_EQ (splits[1], 20.0f);
}

TEST_F (ShadowCascadesTests, InfiniteFarClampsToSceneDepth)
{
    auto inputs = makeInputs();
    inputs.farPlane = 0.0f;
    inputs.receiverBounds = BoundingBox ({ -50.0f, -1.0f, -50.0f }, { 50.0f, 3.0f, 50.0f });

    // The deepest receiver corner is about 59.4 units away
    const auto range = ShadowCascades::computeShadowRange (inputs);
    EXPECT_FLOAT_EQ (range.getStart(), inputs.nearPlane);
    EXPECT_TRUE (std::isfinite (range.getEnd()));
    EXPECT_GT (range.getEnd(), 55.0f);
    EXPECT_LT (range.getEnd(), 65.0f);
}

TEST_F (ShadowCascadesTests, FarPlaneBeyondSceneIsClamped)
{
    auto inputs = makeInputs();
    inputs.receiverBounds = BoundingBox ({ -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f });

    EXPECT_LT (ShadowCascades::computeShadowRange (inputs).getEnd(), 13.0f);
}

TEST_F (ShadowCascadesTests, NearClampsToSceneDepth)
{
    auto inputs = makeInputs();
    inputs.receiverBounds = BoundingBox ({ -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f });

    // The nearest receiver corner, (x, 1, 1), is about 9 units away: nothing to shade before it
    const auto range = ShadowCascades::computeShadowRange (inputs);
    EXPECT_GT (range.getStart(), 8.5f);
    EXPECT_LT (range.getStart(), 9.5f);

    Cascades cascades;
    ASSERT_EQ (ShadowCascades::build (inputs, cascades), 4);
    EXPECT_LT (cascades[3].splitFar, 13.0f);
}

TEST_F (ShadowCascadesTests, InfiniteFarCapsAtTheClusterLimit)
{
    Scene scene;
    auto entity = EntityNode::Ptr (new EntityNode ("camera"));
    auto& camera = entity->attach<CameraNode>();
    camera.zNear = 0.25f;
    camera.zFar = 0.0f;
    scene.getRoot()->addChild (entity);
    scene.setActiveCamera (entity.get());

    ViewData view;
    view.update (scene, 64, 64, true, 16, 3);

    auto inputs = makeInputs();
    inputs.setView (view);
    inputs.receiverBounds = BoundingBox ({ -1.0e4f, -1.0e4f, -1.0e4f }, { 1.0e4f, 1.0e4f, 1.0e4f });

    EXPECT_EQ (inputs.projection, view.getProjection());
    EXPECT_FLOAT_EQ (inputs.nearPlane, 0.25f);
    EXPECT_FLOAT_EQ (inputs.farPlane, 0.0f);
    EXPECT_TRUE (inputs.cameraWorld.transformPoint ({}).approximatelyEqualTo (view.getCameraPosition()));

    const auto range = ShadowCascades::computeShadowRange (inputs);
    EXPECT_FLOAT_EQ (range.getStart(), 0.25f);
    EXPECT_FLOAT_EQ (range.getEnd(), LightClusters::getGrid (view).farPlane);
    EXPECT_FLOAT_EQ (range.getEnd(), ShadowCascades::maxShadowDistance);

    Cascades cascades;
    ASSERT_EQ (ShadowCascades::build (inputs, cascades), 4);
    EXPECT_FLOAT_EQ (cascades[3].splitFar, ShadowCascades::maxShadowDistance);

    for (const auto& cascade : cascades)
    {
        EXPECT_TRUE (std::isfinite (cascade.radius));
        EXPECT_GT (cascade.radius, 0.0f);
    }
}

TEST_F (ShadowCascadesTests, ReceiversOutOfViewGiveNoCascades)
{
    auto inputs = makeInputs();
    Cascades cascades;

    // Behind the camera
    inputs.receiverBounds = BoundingBox ({ -1.0f, -1.0f, 20.0f }, { 1.0f, 1.0f, 30.0f });
    EXPECT_TRUE (ShadowCascades::computeShadowRange (inputs).isEmpty());
    EXPECT_EQ (ShadowCascades::build (inputs, cascades), 0);

    // Beyond the far plane
    inputs.receiverBounds = BoundingBox ({ -1.0f, -1.0f, -300.0f }, { 1.0f, 1.0f, -200.0f });
    EXPECT_TRUE (ShadowCascades::computeShadowRange (inputs).isEmpty());
    EXPECT_EQ (ShadowCascades::build (inputs, cascades), 0);
}

TEST_F (ShadowCascadesTests, FlatPanelFacingTheCameraGetsCascades)
{
    auto inputs = makeInputs();
    inputs.cameraWorld = Matrix4::translation ({ 0.0f, 0.0f, 5.0f });
    inputs.projection = Matrix4::orthographic (-2.0f, 2.0f, -1.0f, 1.0f, 0.1f, 100.0f);
    inputs.receiverBounds = BoundingBox ({ -2.0f, -1.0f, 0.0f }, { 2.0f, 1.0f, 0.0f });
    inputs.casterBounds = BoundingBox ({ -2.0f, -1.0f, 0.0f }, { 2.0f, 1.0f, 0.5f });

    const auto range = ShadowCascades::computeShadowRange (inputs);
    EXPECT_FALSE (range.isEmpty());
    EXPECT_NEAR (range.getStart(), 5.0f, 1.0e-4f);

    Cascades cascades;
    ASSERT_EQ (ShadowCascades::build (inputs, cascades), 4);

    // Every cascade covers the whole panel
    for (const auto& cascade : cascades)
    {
        for (float x : { -2.0f, 2.0f })
        {
            for (float y : { -1.0f, 1.0f })
            {
                const auto clip = clipOf (cascade.viewProjection, { x, y, 0.0f });
                EXPECT_LE (std::abs (clip[0] / clip[3]), 1.0f);
                EXPECT_LE (std::abs (clip[1] / clip[3]), 1.0f);
            }
        }
    }
}

TEST_F (ShadowCascadesTests, EveryCascadeContainsItsSlice)
{
    const auto inputs = makeInputs();
    Cascades cascades;
    ASSERT_EQ (ShadowCascades::build (inputs, cascades), 4);

    expectSlicesContained (inputs, cascades);
}

TEST_F (ShadowCascadesTests, StraightDownSunGetsFiniteCascades)
{
    auto inputs = makeInputs();
    inputs.lightDirection = { 0.0f, -1.0f, 0.0f };

    Cascades cascades;
    ASSERT_EQ (ShadowCascades::build (inputs, cascades), 4);

    for (const auto& cascade : cascades)
    {
        for (int i = 0; i < 16; ++i)
            EXPECT_TRUE (std::isfinite (cascade.viewProjection.getData()[i]));
    }

    expectSlicesContained (inputs, cascades);
}

TEST_F (ShadowCascadesTests, PaddingKeepsTheSnappedMapAroundTheSlice)
{
    // One orthographic slice whose bounding sphere sits just below a quantization step (3.0): without
    // padding the radius would round to the sphere itself, and the snap could move the slice out
    constexpr float halfWidth = 2.0f;
    constexpr float halfHeight = 1.998499f;

    auto inputs = makeInputs();
    inputs.cameraWorld = {};
    inputs.projection = Matrix4::orthographic (-halfWidth, halfWidth, -halfHeight, halfHeight, 0.1f, 100.0f);
    inputs.receiverBounds = BoundingBox ({ -1.0f, -1.0f, -7.0f }, { 1.0f, 1.0f, -5.0f });
    inputs.casterBounds = inputs.receiverBounds;
    inputs.count = 1;

    Cascades cascades;
    ASSERT_EQ (ShadowCascades::build (inputs, cascades), 1);

    const auto& cascade = cascades[0];
    auto sphereRadius = 0.0f;

    for (float z : { -5.0f, -7.0f })
    {
        for (float x : { -halfWidth, halfWidth })
        {
            for (float y : { -halfHeight, halfHeight })
            {
                sphereRadius = jmax (sphereRadius, (Vector3<float> (x, y, z) - cascade.center).length());

                const auto clip = clipOf (cascade.viewProjection, { x, y, z });
                EXPECT_LE (std::abs (clip[0]), 1.0f);
                EXPECT_LE (std::abs (clip[1]), 1.0f);
            }
        }
    }

    ASSERT_GT (sphereRadius, 2.998f);
    ASSERT_LT (sphereRadius, 3.0f);
    EXPECT_GE (cascade.radius, sphereRadius + 0.5f * cascade.texelWorldSize);
}

TEST_F (ShadowCascadesTests, CastersBehindTheSliceStayInsideTheDepthRange)
{
    auto inputs = makeInputs();
    inputs.casterBounds = BoundingBox ({ -50.0f, -1.0f, -50.0f }, { 50.0f, 30.0f, 50.0f });

    Cascades cascades;
    ASSERT_EQ (ShadowCascades::build (inputs, cascades), 4);

    // A caster 3 units beyond the first sphere, towards the light, casts into the first slice: it must
    // not be clipped by the light near plane
    const auto& first = cascades[0];
    const auto caster = first.center - inputs.lightDirection * (first.radius + 3.0f);
    ASSERT_GT (caster.getY(), 0.0f);
    ASSERT_LT (caster.getY(), 30.0f);

    const auto clip = clipOf (first.viewProjection, caster);
    EXPECT_GE (clip[2] / clip[3], 0.0f);
    EXPECT_LE (clip[2] / clip[3], 1.0f);
}

TEST_F (ShadowCascadesTests, DepthRangeMapsLightDistanceToClipDepth)
{
    const auto inputs = makeInputs();
    Cascades cascades;
    ASSERT_EQ (ShadowCascades::build (inputs, cascades), 4);

    constexpr float distance = 0.5f;

    for (const auto& cascade : cascades)
    {
        ASSERT_GT (cascade.depthRange, 2.0f * cascade.radius);

        const auto nearer = clipOf (cascade.viewProjection, cascade.center);
        const auto farther = clipOf (cascade.viewProjection, cascade.center + inputs.lightDirection * distance);
        EXPECT_NEAR (farther[2] - nearer[2], distance / cascade.depthRange, 1.0e-5f);
    }
}

TEST_F (ShadowCascadesTests, RadiusIsQuantizedAndStableUnderSmallRangeChanges)
{
    // Receivers ending about 59.4 units away: moving their far side by 0.6 moves the range end by 1%
    auto inputs = makeInputs();
    inputs.receiverBounds = BoundingBox ({ -1.0e4f, -1.0f, -50.0f }, { 1.0e4f, 3.0f, 1.0e4f });

    Cascades a, b;
    ASSERT_EQ (ShadowCascades::build (inputs, a), 4);
    const auto endA = ShadowCascades::computeShadowRange (inputs).getEnd();

    inputs.receiverBounds = BoundingBox ({ -1.0e4f, -1.0f, -50.6f }, { 1.0e4f, 3.0f, 1.0e4f });
    ASSERT_EQ (ShadowCascades::build (inputs, b), 4);
    const auto endB = ShadowCascades::computeShadowRange (inputs).getEnd();

    ASSERT_NEAR (endB / endA, 1.01f, 2.0e-3f);

    for (size_t c = 0; c < 4; ++c)
    {
        EXPECT_FLOAT_EQ (a[c].radius, b[c].radius) << "cascade " << c;

        const auto step = std::exp2 (std::ceil (std::log2 (a[c].radius))) / 16.0f;
        EXPECT_FLOAT_EQ (a[c].radius / step, std::round (a[c].radius / step)) << "cascade " << c;
    }
}

TEST_F (ShadowCascadesTests, SphereRadiusIgnoresCameraRotation)
{
    auto inputs = makeInputs();
    Cascades a, b;
    ASSERT_EQ (ShadowCascades::build (inputs, a), 4);

    inputs.cameraWorld = Matrix4::rotationY (0.37f).followedBy (inputs.cameraWorld);
    ASSERT_EQ (ShadowCascades::build (inputs, b), 4);

    for (size_t c = 0; c < 4; ++c)
        EXPECT_NEAR (a[c].radius, b[c].radius, 1.0e-3f * a[c].radius);
}

TEST_F (ShadowCascadesTests, SnappedMatrixMovesByWholeTexels)
{
    auto inputs = makeInputs();
    Cascades a, b;
    ASSERT_EQ (ShadowCascades::build (inputs, a), 4);

    // Move the camera by a fraction of a texel of the first cascade
    inputs.cameraWorld = Matrix4::translation ({ a[0].texelWorldSize * 0.3f, 0.0f, 0.0f }).followedBy (inputs.cameraWorld);
    ASSERT_EQ (ShadowCascades::build (inputs, b), 4);

    for (size_t c = 0; c < 4; ++c)
    {
        ASSERT_FLOAT_EQ (a[c].radius, b[c].radius) << "cascade " << c;

        const auto pa = clipOf (a[c].viewProjection, {});
        const auto pb = clipOf (b[c].viewProjection, {});
        const auto texelsX = (pb[0] - pa[0]) * 0.5f * (float) inputs.resolution;
        const auto texelsY = (pb[1] - pa[1]) * 0.5f * (float) inputs.resolution;

        EXPECT_NEAR (texelsX, std::round (texelsX), 1.0e-2f) << "cascade " << c;
        EXPECT_NEAR (texelsY, std::round (texelsY), 1.0e-2f) << "cascade " << c;
    }
}

TEST_F (ShadowCascadesTests, SnapPutsTheOriginOnAWholeTexel)
{
    const auto lightViewProjection = Matrix4::lookAt ({ 0.3f, 7.1f, 2.9f }, { 0.17f, 0.0f, -0.4f }, { 0.0f, 1.0f, 0.0f })
                                         .followedBy (Matrix4::orthographic (-3.3f, 3.3f, -3.3f, 3.3f, 0.0f, 20.0f));
    const auto snapped = ShadowCascades::snapToTexels (lightViewProjection, 512);
    const auto origin = clipOf (snapped, {});
    const auto texelsX = origin[0] * 0.5f * 512.0f;
    const auto texelsY = origin[1] * 0.5f * 512.0f;

    EXPECT_NEAR (texelsX, std::round (texelsX), 1.0e-3f);
    EXPECT_NEAR (texelsY, std::round (texelsY), 1.0e-3f);
    EXPECT_FLOAT_EQ (origin[2], clipOf (lightViewProjection, {})[2]);
}

TEST_F (ShadowCascadesTests, OrthographicCameraGetsCascades)
{
    auto inputs = makeInputs();
    inputs.projection = Matrix4::orthographic (-5.0f, 5.0f, -3.0f, 3.0f, 0.1f, 100.0f);

    Cascades cascades;
    ASSERT_EQ (ShadowCascades::build (inputs, cascades), 4);

    // Every slice spans the whole orthographic view
    for (const auto& cascade : cascades)
        EXPECT_GE (cascade.radius, std::hypot (5.0f, 3.0f));
}

TEST_F (ShadowCascadesTests, EmptyBoundsGiveNoCascades)
{
    Cascades cascades;

    auto inputs = makeInputs();
    inputs.casterBounds = BoundingBox();
    EXPECT_EQ (ShadowCascades::build (inputs, cascades), 0);

    inputs = makeInputs();
    inputs.receiverBounds = BoundingBox();
    EXPECT_TRUE (ShadowCascades::computeShadowRange (inputs).isEmpty());
    EXPECT_EQ (ShadowCascades::build (inputs, cascades), 0);
}

TEST_F (ShadowCascadesTests, TexelSizeGrowsWithCascade)
{
    const auto inputs = makeInputs();
    Cascades cascades;
    ASSERT_EQ (ShadowCascades::build (inputs, cascades), 4);

    for (size_t c = 0; c < 4; ++c)
        EXPECT_FLOAT_EQ (cascades[c].texelWorldSize, 2.0f * cascades[c].radius / (float) inputs.resolution);

    for (size_t c = 1; c < 4; ++c)
        EXPECT_GT (cascades[c].texelWorldSize, cascades[c - 1].texelWorldSize);
}

TEST_F (ShadowCascadesTests, ReceiversInsideTheFarPlaneAreNeverCutOff)
{
    // The panel case: the deepest receiver ends the range, exactly where the last split lies. Over
    // many camera poses the decision must not hang on rounding between the two
    int poses = 0;
    for (int i = 0; i < 256; ++i)
    {
        const auto yaw = static_cast<float> (i) * 0.37f;
        const auto pitch = -0.7f + static_cast<float> (i % 9) * 0.17f;
        const auto distance = 2.0f + static_cast<float> (i % 5) * 2.3f;
        const Vector3<float> target (0.05f * static_cast<float> (i % 3), 0.0f, -0.07f * static_cast<float> (i % 4));
        const auto eye = target + Vector3<float> (std::sin (yaw) * std::cos (pitch), std::sin (pitch), std::cos (yaw) * std::cos (pitch)) * distance;

        auto inputs = makeInputs();
        inputs.cameraWorld = Matrix4::lookAt (eye, target, { 0.0f, 1.0f, 0.0f }).inverted();
        inputs.receiverBounds = BoundingBox ({ -1.0f, -1.0f, -0.5f }, { 1.0f, 1.0f, 0.5f });
        inputs.casterBounds = inputs.receiverBounds;

        Cascades cascades;
        if (ShadowCascades::build (inputs, cascades) == 0)
            continue;

        ++poses;
        EXPECT_FALSE (ShadowCascades::cutsOffReceivers (inputs)) << "pose " << i;
    }

    EXPECT_GT (poses, 200);
}

TEST_F (ShadowCascadesTests, FarPlaneOrShadowDistanceCutsReceiversOff)
{
    // The fixture's receivers reach 10 km: the 100 m far plane cuts them, and so does the
    // shadow distance of an infinite far plane
    auto inputs = makeInputs();
    EXPECT_TRUE (ShadowCascades::cutsOffReceivers (inputs));

    inputs.farPlane = 0.0f;
    EXPECT_TRUE (ShadowCascades::cutsOffReceivers (inputs));

    // Receivers ending at 50 m stay inside both
    inputs.receiverBounds = BoundingBox ({ -20.0f, -1.0f, -40.0f }, { 20.0f, 3.0f, 5.0f });
    EXPECT_FALSE (ShadowCascades::cutsOffReceivers (inputs));

    inputs.farPlane = 100.0f;
    EXPECT_FALSE (ShadowCascades::cutsOffReceivers (inputs));

    // ... unless the far plane comes nearer than them
    inputs.farPlane = 30.0f;
    EXPECT_TRUE (ShadowCascades::cutsOffReceivers (inputs));

    inputs.receiverBounds = BoundingBox();
    EXPECT_FALSE (ShadowCascades::cutsOffReceivers (inputs));
}
