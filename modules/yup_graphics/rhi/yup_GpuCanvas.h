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
class Texture;
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

    @see Texture, Graphics::drawTexture
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

        Valid until commit() is called. Drawing after commit() has undefined behaviour.
    */
    Graphics& getGraphics() noexcept;

    //==============================================================================
    /** Finalises the GPU render pass.

        Must be called before asTexture(), asImage(), or readPixels().
        Returns false if already committed or if no GPU resources were available.
    */
    bool commit();

    //==============================================================================
    /** Returns a GPU-texture view of the rendered result.

        Valid after commit(). The Texture holds a reference to the underlying GPU
        resource; the canvas can be destroyed after this call.
    */
    Texture::Ptr asTexture();

    /** Returns an Image with both GPU texture and CPU pixel data populated.

        Calls asTexture() to obtain the GPU resource, creates an Image wrapping it,
        and then calls readPixels() to fill the CPU-side BitmapData so that
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
    */
    bool readPixels (void* dst, size_t byteSize);

    //==============================================================================
    /** Invokes @c renderFunc with the ore TextureView for this canvas's backing texture.

        Provides a temporary ore TextureView suitable for use as a color attachment in
        an ore RenderPassDesc. The TextureView pointer is only valid within the scope of
        @c renderFunc; it must not be stored beyond the call.

        Typical usage — encode a custom ore render pass targeting this canvas:
        @code
        canvas->withOreAttachment(oreCtx, [&](rive::ore::TextureView* view) {
            rive::ore::RenderPassDesc rpDesc;
            rpDesc.colorAttachments[0].view = view;
            rpDesc.colorCount = 1;
            auto rp = oreCtx->beginRenderPass(rpDesc);
            rp->setPipeline(myPipeline);
            rp->drawIndexed(36);
            rp->finish();
        });
        @endcode

        @returns false if ore is unavailable, @c oreCtx is null, or the canvas is
                 not committed.
        Requires enableOreContext = true on the associated GraphicsContext.
    */
    bool withOreAttachment (rive::ore::Context* oreCtx,
                            std::function<void (rive::ore::TextureView*)> renderFunc) noexcept;

private:
    //==============================================================================
    GpuCanvas() = default;

    GraphicsContext* ctx = nullptr;
    std::unique_ptr<Graphics> graphics;
    Texture::Ptr cachedTexture;
    bool committed = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuCanvas)
};

} // namespace yup
