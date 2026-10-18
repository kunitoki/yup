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

class CameraNodeTests : public ::testing::Test
{
protected:
    /** Makes the focal length of a camera exactly 50 mm on the 24 mm sensor. */
    static void set50mm (CameraNode& camera, float fStop, float focusDistance)
    {
        camera.yFov = 2.0f * std::atan (CameraNode::sensorHeight * 0.5f / 0.05f);
        camera.fStop = fStop;
        camera.focusDistance = focusDistance;
    }
};

TEST_F (CameraNodeTests, DepthOfFieldFieldsHaveTheirDefaults)
{
    const CameraNode camera;

    EXPECT_FLOAT_EQ (CameraNode::sensorHeight, 0.024f);
    EXPECT_FLOAT_EQ (camera.fStop, 2.8f);
    EXPECT_FLOAT_EQ (camera.focusDistance, 10.0f);
    EXPECT_EQ (camera.apertureBlades, 0);
}

TEST_F (CameraNodeTests, CircleOfConfusionIsZeroAtTheFocusDistance)
{
    CameraNode camera;
    set50mm (camera, 2.0f, 2.0f);

    EXPECT_NEAR (camera.getCircleOfConfusion (2.0f, 1000.0f), 0.0f, 1.0e-4f);
}

TEST_F (CameraNodeTests, CircleOfConfusionMatchesTheThinLens)
{
    CameraNode camera;
    set50mm (camera, 2.0f, 2.0f);

    // A = 25 mm, f = 50 mm, S = 2 m, D = 4 m: 0.025 * 0.05 * 2 / (4 * 1.95) = 3.2051e-4 m on the sensor
    EXPECT_NEAR (camera.getCircleOfConfusion (4.0f, 1000.0f), 13.3547f, 1.0e-2f);
}

TEST_F (CameraNodeTests, CircleOfConfusionIsSignedAndHalvesWhenStoppingDown)
{
    CameraNode wide;
    set50mm (wide, 2.0f, 2.0f);

    CameraNode narrow;
    set50mm (narrow, 4.0f, 2.0f);

    EXPECT_LT (wide.getCircleOfConfusion (1.0f, 1000.0f), 0.0f);
    EXPECT_GT (wide.getCircleOfConfusion (8.0f, 1000.0f), 0.0f);
    EXPECT_NEAR (narrow.getCircleOfConfusion (8.0f, 1000.0f), 0.5f * wide.getCircleOfConfusion (8.0f, 1000.0f), 1.0e-3f);
}

TEST_F (CameraNodeTests, CircleOfConfusionTendsToAFiniteLimit)
{
    CameraNode camera;
    set50mm (camera, 2.0f, 2.0f);

    const auto limit = (0.025f * 0.05f / 1.95f) / CameraNode::sensorHeight * 1000.0f;

    EXPECT_NEAR (camera.getCircleOfConfusion (1.0e9f, 1000.0f), limit, 1.0e-2f);
}

TEST_F (CameraNodeTests, CircleOfConfusionHandlesDegenerateCameras)
{
    CameraNode ortho;
    set50mm (ortho, 2.0f, 2.0f);
    ortho.projection = CameraNode::Projection::orthographic;
    EXPECT_EQ (ortho.getCircleOfConfusion (8.0f, 1000.0f), 0.0f);

    CameraNode wideOpen;
    set50mm (wideOpen, 0.0f, 2.0f);
    EXPECT_EQ (wideOpen.getCircleOfConfusion (8.0f, 1000.0f), 0.0f);

    CameraNode negativeStop;
    set50mm (negativeStop, -2.0f, 2.0f);
    EXPECT_EQ (negativeStop.getCircleOfConfusion (8.0f, 1000.0f), 0.0f);

    CameraNode noFieldOfView;
    set50mm (noFieldOfView, 2.0f, 2.0f);
    noFieldOfView.yFov = 0.0f;
    EXPECT_EQ (noFieldOfView.getCircleOfConfusion (8.0f, 1000.0f), 0.0f);

    CameraNode camera;
    set50mm (camera, 2.0f, 2.0f);
    EXPECT_EQ (camera.getCircleOfConfusion (0.0f, 1000.0f), 0.0f);
    EXPECT_EQ (camera.getCircleOfConfusion (8.0f, -1000.0f), 0.0f);

    // Focus closer than the focal length is clamped just past it, so a point at 8 m is behind the focus
    CameraNode tooClose;
    set50mm (tooClose, 2.0f, 0.01f);
    const auto tooCloseCoc = tooClose.getCircleOfConfusion (8.0f, 1000.0f);
    EXPECT_TRUE (std::isfinite (tooCloseCoc));
    EXPECT_GT (tooCloseCoc, 0.0f);

    // A point at an infinite distance, as behind an infinite far plane, gets the finite limit
    const auto limit = (0.025f * 0.05f / 1.95f) / CameraNode::sensorHeight * 1000.0f;
    EXPECT_NEAR (camera.getCircleOfConfusion (std::numeric_limits<float>::infinity(), 1000.0f), limit, 1.0e-2f);
}
