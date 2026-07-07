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

Texture::Ptr Texture::fromGpuTexture (rive::rcp<rive::gpu::Texture> texture, int width, int height)
{
    if (texture == nullptr || width <= 0 || height <= 0)
        return nullptr;

    auto* t = new Texture();
    t->gpuTexture = std::move (texture);
    t->width = width;
    t->height = height;
    return t;
}

Texture::Ptr Texture::fromRenderCanvas (rive::rcp<rive::gpu::RenderCanvas> canvas, int width, int height)
{
    if (canvas == nullptr || width <= 0 || height <= 0)
        return nullptr;

    auto* t = new Texture();
    t->renderCanvas = std::move (canvas);
    t->renderTarget = true;
    t->width = width;
    t->height = height;
    return t;
}

//==============================================================================

rive::rcp<rive::gpu::Texture> Texture::getOrAdoptGpuTexture() const
{
    if (gpuTexture != nullptr)
        return gpuTexture;

    if (renderCanvas != nullptr)
    {
        if (auto* ri = renderCanvas->renderImage())
            gpuTexture = ri->refTexture();
    }

    return gpuTexture;
}

rive::rcp<rive::gpu::RenderCanvas> Texture::getInternalRenderCanvas() const
{
    return renderCanvas;
}

rive::RenderImage* Texture::getRenderImage() const
{
    if (renderCanvas != nullptr)
        return renderCanvas->renderImage();

    return nullptr;
}

//==============================================================================

int Texture::getWidth() const noexcept
{
    return width;
}

int Texture::getHeight() const noexcept
{
    return height;
}

bool Texture::isValid() const noexcept
{
    return gpuTexture != nullptr || renderCanvas != nullptr;
}

bool Texture::isRenderTarget() const noexcept
{
    return renderTarget;
}

} // namespace yup
