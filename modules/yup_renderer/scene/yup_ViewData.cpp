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

void copyViewMatrix (const Matrix4& matrix, float* destination) noexcept
{
    std::copy_n (matrix.getData(), 16, destination);
}

} // namespace

float ViewData::getToneMappingOperator (Scene::ToneMapping toneMapping) noexcept
{
    switch (toneMapping)
    {
        case Scene::ToneMapping::reinhard: return 0.0f;
        case Scene::ToneMapping::aces: return 1.0f;
        case Scene::ToneMapping::agx: return 2.0f;
    }

    return 0.0f;
}

//==============================================================================
void ViewData::update (const Scene& scene, int widthPx, int heightPx, bool jittered, int taaSamples, uint64 frameIndex)
{
    const auto width = static_cast<float> (widthPx);
    const auto height = static_cast<float> (heightPx);
    const auto aspect = width / height;

    CameraNode defaultCamera;
    const CameraNode* camera = scene.getActiveCamera();

    if (camera != nullptr)
    {
        const auto cameraWorld = camera->getEntity() != nullptr ? camera->getEntity()->getWorldMatrix() : Matrix4();
        view = cameraWorld.inverted();
        cameraPosition = cameraWorld.transformPoint ({});
    }
    else
    {
        camera = &defaultCamera;
        cameraPosition = { 0.0f, 0.0f, 5.0f };
        view = Matrix4::lookAt (cameraPosition, {}, { 0.0f, 1.0f, 0.0f });
    }

    projection = camera->getProjectionMatrix (aspect);

    Point<float> jitter;
    if (jittered)
        jitter = getJitter (frameIndex, taaSamples);

    const auto jitterNdc = Point<float> { 2.0f * jitter.getX() / width, 2.0f * jitter.getY() / height };
    jitteredProjection = projection.followedBy (Matrix4::translation ({ jitterNdc.getX(), jitterNdc.getY(), 0.0f }));
    const auto jitteredViewProjection = view.followedBy (jitteredProjection);
    const auto currentUnjittered = view.followedBy (projection);

    const auto previousViewProjection = hasPrevious ? unjitteredViewProjection : currentUnjittered;
    const auto previousJitterNdc = hasPrevious ? previousJitter : jitterNdc;
    unjitteredViewProjection = currentUnjittered;
    hasPrevious = true;

    copyViewMatrix (view, block.view);
    copyViewMatrix (jitteredProjection, block.projection);
    copyViewMatrix (jitteredViewProjection, block.viewProjection);
    copyViewMatrix (jitteredViewProjection.inverted(), block.inverseViewProjection);
    copyViewMatrix (currentUnjittered, block.unjitteredViewProjection);
    copyViewMatrix (previousViewProjection, block.previousViewProjection);

    block.cameraPosition[0] = cameraPosition.getX();
    block.cameraPosition[1] = cameraPosition.getY();
    block.cameraPosition[2] = cameraPosition.getZ();
    block.cameraPosition[3] = scene.getExposure();

    block.viewport[0] = width;
    block.viewport[1] = height;
    block.viewport[2] = 1.0f / width;
    block.viewport[3] = 1.0f / height;

    block.jitter[0] = jitterNdc.getX();
    block.jitter[1] = jitterNdc.getY();
    block.jitter[2] = previousJitterNdc.getX();
    block.jitter[3] = previousJitterNdc.getY();
    previousJitter = jitterNdc;

    block.clipPlanes[0] = camera->zNear;
    block.clipPlanes[1] = camera->zFar;
    block.clipPlanes[2] = static_cast<float> (frameIndex % noiseFrameCycle);
    block.clipPlanes[3] = getToneMappingOperator (scene.getToneMapping());
}

void ViewData::setCapture (const Matrix4& viewMatrix, const Matrix4& projectionMatrix, Vector3<float> position, int size)
{
    view = viewMatrix;
    projection = projectionMatrix;
    jitteredProjection = projectionMatrix;
    unjitteredViewProjection = view.followedBy (projection);
    cameraPosition = position;
    previousJitter = {};
    hasPrevious = false;

    copyViewMatrix (view, block.view);
    copyViewMatrix (projection, block.projection);
    copyViewMatrix (unjitteredViewProjection, block.viewProjection);
    copyViewMatrix (unjitteredViewProjection.inverted(), block.inverseViewProjection);
    copyViewMatrix (unjitteredViewProjection, block.unjitteredViewProjection);
    copyViewMatrix (unjitteredViewProjection, block.previousViewProjection);

    block.cameraPosition[0] = position.getX();
    block.cameraPosition[1] = position.getY();
    block.cameraPosition[2] = position.getZ();
    block.cameraPosition[3] = 1.0f;

    const auto extent = static_cast<float> (size);
    block.viewport[0] = extent;
    block.viewport[1] = extent;
    block.viewport[2] = 1.0f / extent;
    block.viewport[3] = 1.0f / extent;

    std::fill (std::begin (block.jitter), std::end (block.jitter), 0.0f);

    // A depth 0 to 1 perspective projection stores near * far / (near - far) at [14] and far / (near - far) at [10]
    const auto* p = projectionMatrix.getData();
    block.clipPlanes[0] = p[14] / p[10];
    block.clipPlanes[1] = p[14] / (p[10] + 1.0f);
    block.clipPlanes[2] = 0.0f;
    block.clipPlanes[3] = 0.0f;
}

void ViewData::reset() noexcept
{
    hasPrevious = false;
}

float ViewData::halton (int index, int base) noexcept
{
    auto result = 0.0f;
    auto fraction = 1.0f;

    while (index > 0)
    {
        fraction /= static_cast<float> (base);
        result += fraction * static_cast<float> (index % base);
        index /= base;
    }

    return result;
}

Point<float> ViewData::getJitter (uint64 frameIndex, int sampleCount) noexcept
{
    const auto index = static_cast<int> (frameIndex % static_cast<uint64> (jmax (1, sampleCount))) + 1;
    return { halton (index, 2) - 0.5f, halton (index, 3) - 0.5f };
}

} // namespace yup
