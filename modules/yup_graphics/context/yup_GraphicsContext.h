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

namespace juce
{

class LowLevelRenderContext;

class JUCE_API GraphicsContext
{
public:
    struct Options
    {
        constexpr Options() noexcept = default;

        bool retinaDisplay                 : 1 = true;
        bool readableFramebuffer           : 1 = true;
        bool synchronousShaderCompilations : 1 = false;
        bool enableReadPixels              : 1 = false;
        bool disableRasterOrdering         : 1 = false;
    };

    enum Api
    {
        OpenGL,
        Direct3D,
        Metal,
        Dawn
    };

    GraphicsContext () noexcept = default;
    virtual ~GraphicsContext() = default;

    GraphicsContext (const GraphicsContext& other) noexcept = delete;
    GraphicsContext (GraphicsContext&& other) noexcept = default;
    GraphicsContext& operator=(const GraphicsContext& other) noexcept = delete;
    GraphicsContext& operator=(GraphicsContext&& other) noexcept = default;

    virtual float dpiScale (void* nativeHandle) const = 0;

    virtual rive::Factory* factory() = 0;

    virtual rive::pls::PLSRenderContext* plsContextOrNull() = 0;
    virtual rive::pls::PLSRenderTarget* plsRenderTargetOrNull() = 0;

    virtual void onSizeChanged (void* nativeHandle, int width, int height, uint32_t sampleCount) = 0;

    virtual std::unique_ptr<rive::Renderer> makeRenderer (int width, int height) = 0;

    virtual void begin (const rive::pls::PLSRenderContext::FrameDescriptor&) = 0;

    virtual void flushPLSContext() = 0; // Called by end()

    virtual void end (void* nativeHandle) = 0;

    virtual void tick() {}

    static std::unique_ptr<GraphicsContext> createContext (Options options);
    static std::unique_ptr<GraphicsContext> createContext (Api graphicsApi, Options options);
};

} // namespace juce
