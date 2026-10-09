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

#pragma once

#include <yup_3d/yup_3d.h>

#include <array>
#include <cmath>

//==============================================================================
/** A photographic studio to light the device: three large soft boxes, in front on both sides
    and behind, over a dark floor, with dim walls and ceiling.

    The ceiling stays dim on purpose: seen from the top, the flat panel reflects it, and a bright
    overhead light would wash the print out.

    +X is the front of the device and +Y is up.
*/
inline yup::EnvironmentMap::Ptr createStudioEnvironment()
{
    struct SoftBox
    {
        float azimuthDegrees;   // From +X towards +Z
        float elevationDegrees; // Above the horizon
        float halfWidthDegrees;
        float halfHeightDegrees;
        yup::Vector3<float> radiance;
    };

    static constexpr std::array<SoftBox, 3> boxes { {
        { 35.0f, 50.0f, 28.0f, 18.0f, { 5.0f, 4.9f, 4.7f } },   // Key, front left
        { -40.0f, 40.0f, 22.0f, 14.0f, { 1.8f, 1.85f, 1.9f } }, // Fill, front right
        { 180.0f, 32.0f, 45.0f, 6.0f, { 2.6f, 2.6f, 2.6f } },   // Rim strip, behind
    } };

    return new yup::EnvironmentMap ([] (const yup::Vector3<float>& direction)
    {
        const auto y = yup::jlimit (-1.0f, 1.0f, direction.getY());

        // A dark floor fading into dim walls across the horizon
        const auto horizon = yup::jlimit (0.0f, 1.0f, y / 0.1f + 0.5f);
        const auto wall = 0.06f + 0.03f * yup::jmax (y, 0.0f);
        auto result = yup::Vector3<float> (1.0f, 0.98f, 0.95f) * (0.025f * (1.0f - horizon) + wall * horizon);

        // The boxes are rectangles in angle around their center, with soft edges
        const auto edge = yup::degreesToRadians (1.0f);
        const auto coverage = [edge] (float angle, float halfSize)
        {
            return yup::jlimit (0.0f, 1.0f, (halfSize - std::abs (angle)) / edge + 0.5f);
        };

        for (const auto& box : boxes)
        {
            const auto azimuth = yup::degreesToRadians (box.azimuthDegrees);
            const auto elevation = yup::degreesToRadians (box.elevationDegrees);
            const yup::Vector3<float> center (std::cos (elevation) * std::cos (azimuth), std::sin (elevation), std::cos (elevation) * std::sin (azimuth));

            const auto across = yup::Vector3<float> (0.0f, 1.0f, 0.0f).crossProduct (center).normalized();
            const auto along = center.crossProduct (across);

            const auto x = std::atan2 (direction.dotProduct (across), direction.dotProduct (center));
            const auto z = std::asin (yup::jlimit (-1.0f, 1.0f, direction.dotProduct (along)));

            result = result + box.radiance * (coverage (x, yup::degreesToRadians (box.halfWidthDegrees)) * coverage (z, yup::degreesToRadians (box.halfHeightDegrees)));
        }

        return result;
    });
}
