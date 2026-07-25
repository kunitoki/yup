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

#include <yup_graphics/yup_graphics.h>

#include "yup_rhi.h"

// ==============================================================================
// Test helper: Delegate GraphicsContext that allows injecting a mock ore context.
//
// Wraps a real (headless) GraphicsContext and delegates all methods to it except
// gpuContext(), which returns the supplied rive::ore::Context*.
// ==============================================================================

class OreInjectedGraphicsContext : public yup::GraphicsContext
{
public:
    explicit OreInjectedGraphicsContext (rive::ore::Context* oreContextToUse)
        : real (yup::GraphicsContext::createContext (yup::GpuPlatform::Headless, {}))
        , injectedOreContext (oreContextToUse)
    {
    }

    yup::GpuPlatform getPlatform() const noexcept override { return real->getPlatform(); }

    yup::GpuDevice::Ptr getGpuDevice() const noexcept override { return real->getGpuDevice(); }

    rive::Factory* factory() override { return real->factory(); }

    rive::gpu::RenderContext* renderContext() override { return real->renderContext(); }

    rive::gpu::RenderTarget* renderTarget() override { return real->renderTarget(); }

    std::unique_ptr<rive::Renderer> makeRenderer (int width, int height) override { return real->makeRenderer (width, height); }

    void onSizeChanged (void* nativeHandle, int width, int height, float dpiScale, uint32_t sampleCount) override { real->onSizeChanged (nativeHandle, width, height, dpiScale, sampleCount); }

    void begin (const rive::gpu::RenderContext::FrameDescriptor& desc) override { real->begin (desc); }

    void end (void* nativeHandle) override { real->end (nativeHandle); }

private:
    std::unique_ptr<yup::GraphicsContext> real;
    rive::ore::Context* injectedOreContext = nullptr;
};
