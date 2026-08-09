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
class Graphics;
class GpuTexture;
class GpuTarget;
class GpuFrame;
class GpuRenderPass;
struct GpuRenderOptions;
class Image;

//==============================================================================
/** A renderable GPU surface for offscreen 2D drawing.

    GpuCanvas consolidates the creation, rendering, and readback of an offscreen
    GPU target into a single, backend-agnostic object. It replaces the lower-level
    GraphicsContext::createOffscreenTarget / beginOffscreen / endOffscreen API.

    GpuCanvas builds on a GpuTarget (accessible via getTarget()) but is backed by
    a dedicated render context, which adds 2D drawing through beginDraw()/commit().
    For render-pass-only work that does not need 2D drawing primitives, prefer
    GpuTarget, which avoids allocating a dedicated render context.

    Typical usage:
    @code
    auto canvas = yup::GpuCanvas::create (ctx, 256, 256);
    if (canvas != nullptr)
    {
        auto& g = canvas->beginDraw();
        g.setFillColor (yup::Colors::cornflowerblue);
        g.fillAll();

        // asTexture() auto-commits, so no explicit commit() call is needed.
        mainGraphics.drawTexture (canvas->asTexture(), targetBounds);

        // Or materialise an image:
        Image img = canvas->asImage();
    }
    @endcode

    The canvas is reference-counted; keep at least one Ptr alive for as long as
    you need to sample from the rendered texture.

    @see GpuTexture, Graphics::drawTexture
*/
class YUP_API GpuCanvas : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<GpuCanvas>;

    //==============================================================================
    /** Creates a GpuCanvas of the given pixel dimensions.

        By default the new canvas is filled with transparent black, so it is safe to
        sample from before anything has been drawn into it. Pass std::nullopt to skip
        that and leave the contents undefined, which is only safe when the canvas is
        guaranteed to be fully written before it is next sampled.

        @param ctx         The graphics context that owns the GPU device.
        @param width       Width in pixels (must be > 0).
        @param height      Height in pixels (must be > 0).
        @param clearColor  Color to fill the new canvas with, or std::nullopt to
                           leave its contents undefined.

        @returns A reference-counted pointer to a GpuCanvas, or nullptr on failure.

        @warning Requires ctx.isGpuAvailable() (GPU context available on this backend).
    */
    static GpuCanvas::Ptr create (GraphicsContext& ctx,
                                  int width,
                                  int height,
                                  std::optional<Color> clearColor = Colors::transparentBlack);

    //==============================================================================
    /** Returns the underlying GpuTarget backing this canvas.

        The GpuTarget provides the render-pass, texture, and readback surface that
        GpuCanvas builds its 2D drawing on. May be nullptr if creation failed.
    */
    GpuTarget::Ptr getTarget() const noexcept;

    //==============================================================================
    /** Returns the width of this canvas in pixels. */
    int getWidth() const noexcept;

    /** Returns the height of this canvas in pixels. */
    int getHeight() const noexcept;

    //==============================================================================
    /** Begins a render pass targeting this canvas's backing texture.

        The returned GpuRenderPass encodes draws into this canvas within the
        given frame. The canvas is the render target, so this is where the pass
        originates. The pass must be finished (explicitly or by destruction)
        before the frame is submitted.

        @param frame    The active GpuFrame the pass records into.
        @param options  Attachment load behaviour (clear flag + clear color).

        @returns A GpuRenderPass. Check isValid() before drawing into it.
    */
    GpuRenderPass beginRenderPass (GpuFrame& frame, const GpuRenderOptions& options = {});

    //==============================================================================
    /** Opens (or reopens) a 2D frame and returns the Graphics context to draw into.

        On the first call after create(), this opens a fresh offscreen 2D GPU frame.
        On subsequent calls it discards the previous frame's Graphics and reopens a
        new one on the same already-allocated GPU target, avoiding per-frame GPU
        resource reallocation.

        Not applicable to canvases used only via beginRenderPass().
    */
    Graphics& beginDraw();

    /** Finalises any open 2D GPU render command.

        Normally not needed: asTexture() and asImage() auto-commit when a 2D
        frame is open. Call this explicitly only when you want to flush the 2D
        GPU work without yet obtaining the texture (e.g. to pipeline the flush
        ahead of asImage()). Returns false if already committed or if no 2D
        frame was open.
        
        @note Not needed at all for canvases used only via beginRenderPass().
    */
    bool commit();

    //==============================================================================
    /** Returns a GPU-texture view of the rendered result.

        If a 2D frame was opened via getGraphics() but not yet committed, this
        auto-commits before returning the texture. For canvases used purely as
        GpuRenderPass targets, no commit is needed and this returns immediately.

        The GpuTexture holds a reference to the underlying GPU resource.
    */
    GpuTexture::Ptr asTexture();

    /** Returns an Image with both GPU texture and CPU pixel data populated.

        Calls asTexture() to obtain the GPU resource, creates an Image wrapping it,
        and then calls readPixels() to fill the CPU-side ImagePixelData so that
        Image::getRawData() returns the rendered pixels.

        Auto-commits any open 2D frame (via asTexture()). Returns an empty Image on failure.
        For GPU-only compositing without CPU readback, prefer using asTexture() and
        Graphics::drawTexture.
    */
    Image asImage();

    //==============================================================================
    /** Reads rendered pixels back to CPU memory.

        Auto-commits any open 2D frame. The destination buffer must hold at least
        getWidth() * getHeight() * 4 bytes (RGBA, top-to-bottom row order).
        Returns false if readback is not available for this backend or fails.

        @param dst       Pointer to the destination buffer (must be non-null).
        @param byteSize  Size of the destination buffer in bytes (must be >= width*height*4).

        @returns         True on success, false on failure.
    */
    bool readPixels (void* dst, size_t byteSize);

private:
    //==============================================================================
    GpuCanvas() = default;

    GraphicsContext* context = nullptr;
    GpuTarget::Ptr target;
    std::unique_ptr<Graphics> graphics;
    bool frameOpen = false;
    bool committed = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuCanvas)
};

} // namespace yup
