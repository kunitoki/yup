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
class GpuFrame;
class GpuRenderPass;
struct GpuRenderOptions;
class Image;

//==============================================================================
/** A renderable GPU surface for offscreen 2D drawing.

    GpuCanvas consolidates the creation, rendering, and readback of an offscreen
    GPU target into a single, backend-agnostic object. It replaces the lower-level
    GraphicsContext::createOffscreenTarget / beginOffscreen / endOffscreen API.

    Typical usage:
    @code
    auto canvas = yup::GpuCanvas::create (ctx, 256, 256);
    if (canvas != nullptr)
    {
        auto& g = canvas->getGraphics();
        g.setFillColor (yup::Colors::cornflowerblue);
        g.fillAll();

        canvas->commit();

        // Draw it directly:
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

        Returns nullptr if the GraphicsContext cannot allocate offscreen GPU resources
        (e.g. headless context with no GPU).

        @param ctx      The graphics context that owns the GPU device.
        @param width    Width in pixels (must be > 0).
        @param height   Height in pixels (must be > 0).
    */
    static GpuCanvas::Ptr create (GraphicsContext& ctx, int width, int height);

    //==============================================================================
    /** Returns the width of this canvas in pixels. */
    int getWidth() const noexcept;

    /** Returns the height of this canvas in pixels. */
    int getHeight() const noexcept;

    //==============================================================================
    /** Returns the Graphics object to draw 2D YUP content into this canvas.

        Lazily begins the offscreen 2D GPU frame on first call. Valid until
        commit() is called. Drawing after commit() has undefined behaviour.
    */
    Graphics& getGraphics() noexcept;

    //==============================================================================
    /** Reopens a previously committed 2D canvas for a new frame of drawing.

        Reuses the already-allocated GPU target textures instead of creating a
        new canvas each frame. After this call getGraphics() begins a fresh
        offscreen 2D frame on the same target, and commit() can be called again.

        Only meaningful for canvases drawn via getGraphics()/commit(); it is a
        no-op for canvases used purely as GpuRenderPass render targets. The
        sampled texture returned by asTexture() remains stable across frames and
        reflects the most recently committed content.
    */
    void beginNewFrame();

    //==============================================================================
    /** Finalises any open 2D GPU render pass.

        Only needs to be called when 2D content was drawn via getGraphics(). For
        canvases used purely as a render target for GpuRenderPass, committing is
        not required. Returns false if already committed or if no 2D frame was
        open.
    */
    bool commit();

    //==============================================================================
    /** Returns a GPU-texture view of the rendered result.

        Valid after commit(). The GpuTexture holds a reference to the underlying GPU
        resource; the canvas can be destroyed after this call.
    */
    GpuTexture::Ptr asTexture();

    /** Returns an Image with both GPU texture and CPU pixel data populated.

        Calls asTexture() to obtain the GPU resource, creates an Image wrapping it,
        and then calls readPixels() to fill the CPU-side ImagePixelData so that
        Image::getRawData() returns the rendered pixels.

        Valid after commit(). Returns an empty Image on failure.
        For GPU-only compositing without CPU readback, prefer drawTexture().
    */
    Image asImage();

    //==============================================================================
    /** Reads rendered pixels back to CPU memory.

        Valid after commit(). The destination buffer must hold at least
        getWidth() * getHeight() * 4 bytes (RGBA, top-to-bottom row order).
        Returns false if readback is not available for this backend or fails.

        @param dst       Pointer to the destination buffer (must be non-null).
        @param byteSize  Size of the destination buffer in bytes (must be >= width*height*4).

        @returns         True on success, false on failure.
    */
    bool readPixels (void* dst, size_t byteSize);

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

private:
    //==============================================================================
    GpuCanvas() = default;

    Graphics& ensureGraphics();

    GraphicsContext* ctx = nullptr;
    std::unique_ptr<GraphicsContext::OffscreenTarget> offscreenTarget;
    std::unique_ptr<Graphics> graphics;
    GpuTexture::Ptr cachedTexture;
    bool frameOpen = false;
    bool committed = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuCanvas)
};

} // namespace yup
