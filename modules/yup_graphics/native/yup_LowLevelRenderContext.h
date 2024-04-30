/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

#include <memory>
#include <vector>

#include "rive/pls/pls_render_context.hpp"

namespace juce
{

class JUCE_API LowLevelRenderContext
{
public:
    struct Options
    {
        constexpr Options() noexcept {}

        bool retinaDisplay = true;
        bool readableFramebuffer = true;
        bool synchronousShaderCompilations = false;
        bool enableReadPixels = false;
        bool disableRasterOrdering = false;
    };

    virtual ~LowLevelRenderContext() = default;

    virtual float dpiScale (void* nativeHandle) const = 0;

    virtual rive::Factory* factory() = 0;

    virtual rive::pls::PLSRenderContext* plsContextOrNull() = 0;
    virtual rive::pls::PLSRenderTarget* plsRenderTargetOrNull() = 0;

    virtual void onSizeChanged (void* nativeHandle, int width, int height, uint32_t sampleCount) {}

    virtual std::unique_ptr<rive::Renderer> makeRenderer (int width, int height) = 0;

    virtual void begin (const rive::pls::PLSRenderContext::FrameDescriptor&) = 0;

    virtual void flushPLSContext() = 0; // Called by end()

    virtual void end (void* nativeHandle) = 0;

    virtual void tick() {}

    /**
     * @brief OpenGL renderer.
     */
#if JUCE_LINUX || JUCE_WASM || JUCE_ANDROID
    static std::unique_ptr<LowLevelRenderContext> makeGLPLS();
#else
    static std::unique_ptr<LowLevelRenderContext> makeGLPLS() { return nullptr; }
#endif

    /**
     * @brief Metal renderer.
     */
#if JUCE_MAC || JUCE_IOS
    static std::unique_ptr<LowLevelRenderContext> makeMetalPLS (Options = {});
#else
    static std::unique_ptr<LowLevelRenderContext> makeMetalPLS (Options = {}) { return nullptr; }
#endif

    /**
     * @brief Direct3D renderer.
     */
#if JUCE_WINDOWS
    static std::unique_ptr<LowLevelRenderContext> makeD3DPLS (Options = {});
#else
    static std::unique_ptr<LowLevelRenderContext> makeD3DPLS (Options = {}) { return nullptr; }
#endif

    /**
     * @brief WebGPU (Dawn) renderer.
     */
    static std::unique_ptr<LowLevelRenderContext> makeDawnPLS (Options = {});
};

} // namespace juce
