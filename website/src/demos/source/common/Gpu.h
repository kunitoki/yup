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

#include <cmath>
#include <cstddef>
#include <utility>

//==============================================================================
/** Compiles a pipeline from a shader bundle embedded in the binary. */
inline yup::ResultValue<yup::GpuPipeline::Ptr> compileEmbeddedPipeline (yup::GpuDevice::Ptr device,
                                                                        const void* bundleData,
                                                                        std::size_t bundleSize,
                                                                        const yup::GpuPipelineOptions& options)
{
    auto bundle = yup::ShaderBundle::loadFromData (bundleData, bundleSize);
    if (bundle.failed())
        return yup::makeResultValueFail ("Shader bundle failed to load: " + bundle.getErrorMessage());

    return yup::GpuPipeline::compileFromBundle (std::move (device), bundle.getReference(), options);
}

//==============================================================================
/** The distance from the camera to the plane z = 0 of a camera whose plane z = 0 maps 1:1 onto the viewport.

    World units are pixels, y points up and the origin is the viewport center, so a
    flat mesh at z = 0 placed with pixelToWorld() looks exactly like 2D drawing,
    and anything lifted toward the viewer (positive z) grows in perspective.
*/
inline float pixelCameraDistance (yup::Size<float> viewport, float fovY = 0.6f)
{
    return 0.5f * viewport.getHeight() / std::tan (0.5f * fovY);
}

/** See pixelCameraDistance(): the camera sits on the +z axis at that distance, looking at the origin. */
inline yup::Matrix4 pixelViewProjection (yup::Size<float> viewport, float fovY = 0.6f)
{
    const auto distance = pixelCameraDistance (viewport, fovY);
    const auto view = yup::Matrix4::lookAt ({ 0.0f, 0.0f, distance }, {}, { 0.0f, 1.0f, 0.0f });
    const auto projection = yup::Matrix4::perspective (fovY, viewport.getWidth() / yup::jmax (1.0f, viewport.getHeight()), distance * 0.05f, distance * 4.0f);

    return view.followedBy (projection);
}

/** Converts a pixel position in the viewport, at a depth toward the viewer, to the world of pixelViewProjection(). */
inline yup::Vector3<float> pixelToWorld (yup::Point<float> pixel, float z, yup::Size<float> viewport)
{
    return { pixel.getX() - 0.5f * viewport.getWidth(), 0.5f * viewport.getHeight() - pixel.getY(), z };
}

//==============================================================================
/** A color target with a matching depth buffer, recreated when the size changes. */
class DepthTarget
{
public:
    /** Makes sure both textures exist at the given size; returns false when they can't be created. */
    bool ensure (yup::GpuDevice::Ptr device, int width, int height)
    {
        if (color != nullptr && depth != nullptr && color->getWidth() == width && color->getHeight() == height)
            return true;

        yup::GpuTextureDesc depthDesc;
        depthDesc.width = static_cast<uint32_t> (width);
        depthDesc.height = static_cast<uint32_t> (height);
        depthDesc.format = yup::GpuTextureFormat::depth24plusStencil8;
        depthDesc.renderTarget = true;
        depthDesc.label = "DepthTarget depth";

        color = yup::GpuTarget::create (device, width, height);
        depth = yup::GpuTexture::create (device, depthDesc);

        return color != nullptr && depth != nullptr;
    }

    yup::GpuTarget::Ptr color;
    yup::GpuTexture::Ptr depth;
};
