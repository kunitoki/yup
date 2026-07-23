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

class GraphicsContext;
class GpuTexture;
class GpuFrame;
class GpuRenderPass;
struct GpuRenderOptions;
class Image;

//==============================================================================
/** A low-level renderable GPU surface for GpuPipeline render passes.

    GpuTarget is the minimal offscreen render surface: it allocates a backing
    texture from the context's main render context (no dedicated 2D render
    context is created) and supports GpuRenderPass-based rendering, texture
    sampling, and CPU readback. Use it for custom GpuPipeline work (e.g. a
    post-process blur) where no 2D drawing primitives are required.

    For 2D drawing with the Graphics API, use GpuCanvas instead, which builds on
    a dedicated render context and adds beginDraw()/commit().

    Typical usage:
    @code
    auto target = yup::GpuTarget::create (ctx, 256, 256);
    if (target != nullptr)
    {
        auto frame = yup::GpuFrame::begin (ctx);
        auto pass = target->beginRenderPass (frame, { true, yup::Colors::transparentBlack });
        pass.setPipeline (*pipeline);
        pass.draw (3);
        pass.finish();
        frame.submit();

        mainGraphics.drawTexture (target->asTexture(), targetBounds);
    }
    @endcode

    The target is reference-counted; keep at least one Ptr alive for as long as
    you need to sample from the rendered texture.

    @see GpuCanvas, GpuTexture, GpuRenderPass, Graphics::drawTexture
*/
class YUP_API GpuTarget : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<GpuTarget>;

    //==============================================================================
    /** Creates a GpuTarget of the given pixel dimensions.

        Returns nullptr if the GraphicsContext cannot allocate offscreen GPU resources
        (e.g. headless context with no GPU).

        @param ctx      The graphics context that owns the GPU device.
        @param width    Width in pixels (must be > 0).
        @param height   Height in pixels (must be > 0).

        @returns A GpuTarget, or nullptr on failure.

        @warning Requires ctx.isGpuAvailable() (GPU context available on this backend).
    */
    static GpuTarget::Ptr create (GraphicsContext& ctx, int width, int height);

    //==============================================================================
    /** Returns the width of this target in pixels. */
    int getWidth() const noexcept;

    /** Returns the height of this target in pixels. */
    int getHeight() const noexcept;

    //==============================================================================
    /** Begins a render pass targeting this target's backing texture.

        The returned GpuRenderPass encodes draws into this target within the
        given frame. The pass must be finished (explicitly or by destruction)
        before the frame is submitted.

        @param frame    The active GpuFrame the pass records into.
        @param options  Attachment load behaviour (clear flag + clear color).

        @returns A GpuRenderPass. Check isValid() before drawing into it.
    */
    GpuRenderPass beginRenderPass (GpuFrame& frame, const GpuRenderOptions& options = {});

    //==============================================================================
    /** Returns a GPU-texture view of the rendered result.

        The GpuTexture holds a reference to the underlying GPU resource.

        @returns A GpuTexture, or nullptr on failure.
    */
    GpuTexture::Ptr asTexture();

    /** Returns an Image with both GPU texture and CPU pixel data populated.

        Calls asTexture() to obtain the GPU resource, creates an Image wrapping it,
        and then calls readPixels() to fill the CPU-side ImagePixelData. Returns an
        empty Image on failure. For GPU-only compositing without CPU readback, prefer
        using asTexture() and Graphics::drawTexture.

        @returns An Image, or an empty Image on failure.
    */
    Image asImage();

    //==============================================================================
    /** Reads rendered pixels back to CPU memory.

        The destination buffer must hold at least getWidth() * getHeight() * 4 bytes
        (RGBA, top-to-bottom row order). Returns false if readback is not available
        for this backend or fails.

        @param dst       Pointer to the destination buffer (must be non-null).
        @param byteSize  Size of the destination buffer in bytes (must be >= width*height*4).

        @returns         True on success, false on failure.
    */
    bool readPixels (void* dst, size_t byteSize);

private:
    //==============================================================================
    friend class GpuCanvas;

    GpuTarget() = default;

    static GpuTarget::Ptr createFromTarget (GraphicsContext& ctx, std::unique_ptr<RenderableTarget> target);

    RenderableTarget* getRenderableTarget() const noexcept { return renderableTarget; }

    GraphicsContext* ctx = nullptr;
    std::unique_ptr<OffscreenTarget> offscreenTarget;
    RenderableTarget* renderableTarget = nullptr;
    GpuTexture::Ptr cachedTexture;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuTarget)
};

} // namespace yup
