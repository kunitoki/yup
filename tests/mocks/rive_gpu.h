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

#include <gmock/gmock.h>

#include <rive/renderer/gpu.hpp>
#include <rive/renderer/texture.hpp>
#include <rive/renderer/render_canvas.hpp>
#include <rive/renderer/rive_renderer.hpp>
#include <rive/renderer/rive_render_image.hpp>
#include <rive/renderer/render_target.hpp>

// ==============================================================================
// Test-friendly subclasses for rive::gpu types with restricted constructors.
// ==============================================================================

struct TestGpuTexture : public rive::gpu::Texture
{
    TestGpuTexture()
        : rive::gpu::Texture (1, 1)
    {
    }

    explicit TestGpuTexture (uint32_t w, uint32_t h)
        : rive::gpu::Texture (w, h)
    {
    }
};

// ==============================================================================
// Minimal mock rive::Factory — only override the methods that get called.
// ==============================================================================

class MockRiveFactory : public rive::Factory
{
public:
    MOCK_METHOD (rive::rcp<rive::RenderBuffer>, makeRenderBuffer, (rive::RenderBufferType, rive::RenderBufferFlags, size_t), (override));
    MOCK_METHOD (rive::rcp<rive::RenderShader>, makeLinearGradient, (float, float, float, float, const rive::ColorInt*, const float*, size_t), (override));
    MOCK_METHOD (rive::rcp<rive::RenderShader>, makeRadialGradient, (float, float, float, const rive::ColorInt*, const float*, size_t), (override));
    MOCK_METHOD (rive::rcp<rive::RenderPath>, makeRenderPath, (rive::RawPath&, rive::FillRule), (override));
    MOCK_METHOD (rive::rcp<rive::RenderPath>, makeEmptyRenderPath, (), (override));
    MOCK_METHOD (rive::rcp<rive::RenderPaint>, makeRenderPaint, (), (override));
    MOCK_METHOD (rive::rcp<rive::RenderImage>, decodeImage, (rive::Span<const uint8_t>), (override));
    MOCK_METHOD (rive::ore::Context*, ore, (), (override));
};
