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

#include <yup_graphics/yup_graphics.h>

using namespace yup;

namespace
{

class TrackingOffscreenTarget : public RenderableTarget
{
public:
    TrackingOffscreenTarget (int targetWidth, int targetHeight)
        : width (targetWidth)
        , height (targetHeight)
    {
    }

    int getWidth() const noexcept override { return width; }

    int getHeight() const noexcept override { return height; }

    rive::gpu::RenderTarget* getRenderTarget() noexcept override { return nullptr; }

    rive::gpu::RenderContext* getRenderContext() noexcept override { return nullptr; }

    rive::rcp<rive::gpu::Texture> adoptAsTexture() override { return nullptr; }

private:
    int width;
    int height;
};

} // namespace
