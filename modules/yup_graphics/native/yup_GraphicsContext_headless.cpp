/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

class NoOpRenderBuffer : public rive::RenderBuffer
{
public:
    NoOpRenderBuffer()
        : rive::RenderBuffer (rive::RenderBufferType::index, rive::RenderBufferFlags::none, 0)
    {
    }

    void* onMap() override { return nullptr; }

    void onUnmap() override {}
};

//==============================================================================

class NoOpRenderShader : public rive::RenderShader
{
public:
};

//==============================================================================

class NoOpRenderImage : public rive::RenderImage
{
public:
};

//==============================================================================

class NoOpRenderPaint : public rive::RenderPaint
{
public:
    void color (unsigned int) override {}

    void style (rive::RenderPaintStyle) override {}

    void thickness (float) override {}

    void join (rive::StrokeJoin) override {}

    void cap (rive::StrokeCap) override {}

    void blendMode (rive::BlendMode) override {}

    void shader (rive::rcp<rive::RenderShader>) override {}

    void invalidateStroke() override {}

    void feather (float) override {}
};

//==============================================================================

class NoOpRenderPath : public rive::RenderPath
{
public:
    void rewind() override {}

    void fillRule (rive::FillRule) override {}

    void addPath (rive::CommandPath*, const rive::Mat2D&) override {}

    void addRenderPath (rive::RenderPath*, const rive::Mat2D&) override {}

    void moveTo (float, float) override {}

    void lineTo (float, float) override {}

    void cubicTo (float, float, float, float, float, float) override {}

    void close() override {}

    void addRawPath (const rive::RawPath&) override {}
};

//==============================================================================

class NoOpFactory : public rive::Factory
{
public:
    NoOpFactory() = default;

    rive::rcp<rive::RenderBuffer> makeRenderBuffer (
        rive::RenderBufferType,
        rive::RenderBufferFlags,
        size_t) override
    {
        return rive::make_rcp<NoOpRenderBuffer>();
    }

    rive::rcp<rive::RenderShader> makeLinearGradient (
        float sx,
        float sy,
        float ex,
        float ey,
        const rive::ColorInt colors[],
        const float stops[],
        size_t count) override
    {
        return rive::make_rcp<NoOpRenderShader>();
    }

    rive::rcp<rive::RenderShader> makeRadialGradient (
        float cx,
        float cy,
        float radius,
        const rive::ColorInt colors[],
        const float stops[],
        size_t count) override
    {
        return rive::make_rcp<NoOpRenderShader>();
    }

    rive::rcp<rive::RenderPath> makeRenderPath (rive::RawPath&, rive::FillRule) override
    {
        return rive::make_rcp<NoOpRenderPath>();
    }

    rive::rcp<rive::RenderPath> makeEmptyRenderPath() override
    {
        return rive::make_rcp<NoOpRenderPath>();
    }

    rive::rcp<rive::RenderPaint> makeRenderPaint() override
    {
        return rive::make_rcp<NoOpRenderPaint>();
    }

    rive::rcp<rive::RenderImage> decodeImage (rive::Span<const uint8_t>) override
    {
        return rive::make_rcp<NoOpRenderImage>();
    }
};

//==============================================================================

class NoOpRenderer : public rive::Renderer
{
public:
    NoOpRenderer() = default;

    void save() override {}

    void restore() override {}

    void transform (const rive::Mat2D&) override {}

    void drawPath (rive::RenderPath*, rive::RenderPaint*) override {}

    void clipPath (rive::RenderPath*) override {}

    void drawImage (const rive::RenderImage*, rive::ImageSampler, rive::BlendMode, float) override {}

    void drawImageMesh (const rive::RenderImage*,
                        rive::ImageSampler,
                        rive::rcp<rive::RenderBuffer>,
                        rive::rcp<rive::RenderBuffer>,
                        rive::rcp<rive::RenderBuffer>,
                        uint32_t vertexCount,
                        uint32_t indexCount,
                        rive::BlendMode,
                        float) override {}
};

//==============================================================================

class NoOpGraphicsContext : public GraphicsContext
{
public:
    NoOpGraphicsContext() = default;

    float dpiScale (void*) const override
    {
        return 1.0f;
    }

    rive::Factory* factory() override
    {
        return std::addressof (noOpFactory);
    }

    rive::gpu::RenderContext* renderContext() override
    {
        return nullptr;
    }

    rive::gpu::RenderTarget* renderTarget() override
    {
        return nullptr;
    }

    std::unique_ptr<rive::Renderer> makeRenderer (int, int) override
    {
        return std::make_unique<NoOpRenderer>();
    }

    void onSizeChanged (void*, int, int, uint32_t) override
    {
    }

    void begin (const rive::gpu::RenderContext::FrameDescriptor&) override
    {
    }

    void end (void*) override
    {
    }

private:
    NoOpFactory noOpFactory;
};

//==============================================================================

std::unique_ptr<GraphicsContext> yup_constructHeadlessGraphicsContext (GraphicsContext::Options fiddleOptions)
{
    return std::make_unique<NoOpGraphicsContext>();
}

} // namespace yup
