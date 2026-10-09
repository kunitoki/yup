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

namespace yup
{

//==============================================================================
namespace
{

// Rounds up to a sixteenth of the next power of two, so the tiny changes a camera rotation makes
// to the radius of a slice leave the cascade size untouched
float quantizeRadius (float radius) noexcept
{
    const auto step = std::exp2 (std::ceil (std::log2 (radius))) / 16.0f;
    return std::ceil (radius / step) * step;
}

// The farthest view depth shadows reach: the far plane, or maxShadowDistance when it is infinite
float getFarLimit (const ShadowCascades::Inputs& inputs) noexcept
{
    return inputs.farPlane > 0.0f ? inputs.farPlane : ShadowCascades::maxShadowDistance;
}

} // namespace

//==============================================================================
void ShadowCascades::Inputs::setView (const ViewData& view) noexcept
{
    cameraWorld = view.getView().inverted();
    projection = view.getProjection();
    nearPlane = view.getBlock().clipPlanes[0];
    farPlane = view.getBlock().clipPlanes[1];
}

//==============================================================================
std::array<float, ShadowCascades::maxCascades + 1> ShadowCascades::computeSplits (float nearPlane, float farPlane, int count, float lambda) noexcept
{
    const auto numCascades = jlimit (1, maxCascades, count);

    std::array<float, maxCascades + 1> splits;
    splits.fill (farPlane);
    splits[0] = nearPlane;

    for (int i = 1; i < numCascades; ++i)
    {
        const auto t = static_cast<float> (i) / static_cast<float> (numCascades);
        const auto logarithmic = nearPlane * std::pow (farPlane / nearPlane, t);
        const auto uniform = nearPlane + (farPlane - nearPlane) * t;
        splits[static_cast<size_t> (i)] = lambda * logarithmic + (1.0f - lambda) * uniform;
    }

    return splits;
}

Range<float> ShadowCascades::computeShadowRange (const Inputs& inputs) noexcept
{
    if (inputs.receiverBounds.isEmpty())
        return {};

    const auto viewBounds = inputs.receiverBounds.transformedBy (inputs.cameraWorld.inverted());
    const auto nearest = -viewBounds.getMax().getZ();
    const auto deepest = -viewBounds.getMin().getZ();

    const auto zNear = jmax (inputs.nearPlane, 1.0e-4f);
    const auto zFar = getFarLimit (inputs);

    if (deepest < zNear || nearest > zFar)
        return {};

    const auto start = jmax (zNear, nearest);
    return { start, jmin (zFar, jmax (deepest, start * 1.01f)) };
}

bool ShadowCascades::cutsOffReceivers (const Inputs& inputs) noexcept
{
    if (inputs.receiverBounds.isEmpty())
        return false;

    const auto deepest = -inputs.receiverBounds.transformedBy (inputs.cameraWorld.inverted()).getMin().getZ();
    return deepest > getFarLimit (inputs);
}

Matrix4 ShadowCascades::snapToTexels (const Matrix4& lightViewProjection, int resolution) noexcept
{
    const auto origin = lightViewProjection.transformPoint4 (0.0f, 0.0f, 0.0f, 1.0f);
    const auto texelsPerUnit = static_cast<float> (resolution) * 0.5f;

    const auto snap = [texelsPerUnit] (float clip)
    {
        return std::round (clip * texelsPerUnit) / texelsPerUnit - clip;
    };

    return lightViewProjection.followedBy (Matrix4::translation ({ snap (origin[0]), snap (origin[1]), 0.0f }));
}

int ShadowCascades::build (const Inputs& inputs, std::array<Cascade, maxCascades>& cascades) noexcept
{
    const auto lightLength = inputs.lightDirection.length();
    jassert (lightLength > 0.0f && inputs.resolution > 0);

    if (! (lightLength > 0.0f) || inputs.resolution <= 0 || inputs.casterBounds.isEmpty())
        return 0;

    const auto range = computeShadowRange (inputs);
    if (range.isEmpty())
        return 0;

    const auto lightDirection = inputs.lightDirection / lightLength;
    const auto up = std::abs (lightDirection.getY()) < 0.99f ? Vector3<float> (0.0f, 1.0f, 0.0f) : Vector3<float> (1.0f, 0.0f, 0.0f);
    const auto resolution = static_cast<float> (inputs.resolution);

    auto casterNear = std::numeric_limits<float>::max();
    for (int corner = 0; corner < 8; ++corner)
        casterNear = jmin (casterNear, inputs.casterBounds.getCorner (corner).dotProduct (lightDirection));

    // Two finite points of each corner ray of the frustum, at NDC depths 0 and 0.5, as in
    // LightClusters: NDC depth 1 lies at infinity with an infinite far plane
    const auto inverseProjection = inputs.projection.inverted();
    std::array<Vector3<float>, 4> rayOrigins, rayStepsPerDepth;

    for (size_t corner = 0; corner < 4; ++corner)
    {
        const auto ndcX = (corner & 1) != 0 ? 1.0f : -1.0f;
        const auto ndcY = (corner & 2) != 0 ? 1.0f : -1.0f;
        const auto a = inverseProjection.transformPoint ({ ndcX, ndcY, 0.0f });
        const auto b = inverseProjection.transformPoint ({ ndcX, ndcY, 0.5f });

        rayOrigins[corner] = a;
        rayStepsPerDepth[corner] = (b - a) / (a.getZ() - b.getZ());
    }

    const auto numCascades = jlimit (1, maxCascades, inputs.count);
    const auto splits = computeSplits (range.getStart(), range.getEnd(), numCascades, inputs.lambda);

    for (size_t c = 0; c < static_cast<size_t> (numCascades); ++c)
    {
        const auto nearDepth = splits[c];
        const auto farDepth = splits[c + 1];

        // The squared distance of the farthest corner from the view axis, at both ends of the slice
        auto nearLateral = 0.0f, farLateral = 0.0f;

        for (size_t corner = 0; corner < 4; ++corner)
        {
            const auto& origin = rayOrigins[corner];
            const auto lateral = [&] (float depth)
            {
                const auto point = origin + rayStepsPerDepth[corner] * (depth + origin.getZ());
                return point.getX() * point.getX() + point.getY() * point.getY();
            };

            nearLateral = jmax (nearLateral, lateral (nearDepth));
            farLateral = jmax (farLateral, lateral (farDepth));
        }

        // The smallest sphere centered on the view axis (Valient, "Stable Rendering of Cascaded Shadow
        // Maps", ShaderX6): built in the camera frame, so a camera rotation leaves it unchanged
        const auto axisDepth = jlimit (nearDepth, farDepth, (farDepth * farDepth - nearDepth * nearDepth + farLateral - nearLateral) / (2.0f * (farDepth - nearDepth)));
        const auto sphereRadius = std::sqrt (jmax (square (axisDepth - nearDepth) + nearLateral, square (farDepth - axisDepth) + farLateral));
        const auto center = inputs.cameraWorld.transformPoint ({ 0.0f, 0.0f, -axisDepth });

        // Snapping moves the projection by up to half a texel: keep a texel of margin around the sphere
        const auto radius = quantizeRadius (jmax (sphereRadius, 1.0e-4f) * (1.0f + 2.0f / resolution));
        const auto texelWorldSize = 2.0f * radius / resolution;

        // The light view starts at the nearest caster towards the light, so casters outside the slice
        // still land in its depth range, and ends behind the sphere
        const auto centerDepth = center.dotProduct (lightDirection);
        const auto backoff = centerDepth - jmin (casterNear, centerDepth - radius) + texelWorldSize;
        const auto lightView = Matrix4::lookAt (center - lightDirection * backoff, center, up);
        const auto depthRange = backoff + radius + texelWorldSize;
        const auto lightProjection = Matrix4::orthographic (-radius, radius, -radius, radius, 0.0f, depthRange);

        auto& cascade = cascades[c];
        cascade.viewProjection = snapToTexels (lightView.followedBy (lightProjection), inputs.resolution);
        cascade.splitFar = splits[c + 1];
        cascade.texelWorldSize = texelWorldSize;
        cascade.center = center;
        cascade.radius = radius;
        cascade.depthRange = depthRange;
    }

    return numCascades;
}

} // namespace yup
