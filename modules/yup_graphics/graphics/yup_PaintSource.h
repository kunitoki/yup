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

namespace yup
{

//==============================================================================
/** Live-rendered content that shapes, strokes and text can be painted with.

    Subclass it, describe the content box with getContentSize() and draw it in
    paintContent(), then pass it to Graphics::setFillSource() or
    Graphics::setStrokeSource(). The content is drawn into an offscreen GPU canvas
    and sampled like an image, so it repeats, mirrors and filters following the
    ImageSampling given with the paint.

    The canvas is sized automatically from the paint's on-screen scale: it grows at
    once when the content is drawn bigger, and shrinks only after it has been drawn
    smaller for a while, so animated zooms do not reallocate every frame. Its size
    is capped, so content drawn very large is magnified instead.

    The content is drawn again only when markDirty() was called, or when the canvas
    had to grow; otherwise every fill reuses the cached raster. Timing is up to the
    caller: advance the animation between frames, then call markDirty() and repaint.

    @code
    struct Waves : yup::PaintSource
    {
        yup::Size<float> getContentSize() const override { return { 128.0f, 128.0f }; }

        void paintContent (yup::Graphics& g) override
        {
            g.setFillColor (yup::Color::fromHSV (phase, 0.8f, 0.9f, 1.0f));
            g.fillAll();
        }

        float phase = 0.0f;
    };

    void refreshDisplay (double lastFrameTimeSeconds) override
    {
        waves.phase = std::fmod (waves.phase + (float) lastFrameTimeSeconds, 1.0f);
        waves.markDirty();
        repaint();
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillSource (waves, yup::AffineTransform::scaling (0.5f), { yup::ImageWrap::Repeat, yup::ImageWrap::Repeat });
        g.fillRoundedRect (getLocalBounds().to<float>(), 12.0f);
    }
    @endcode

    @note Call markDirty() between frames only. The GPU samples the cached canvas
          when the frame that used it is flushed, so redrawing it twice within one
          frame would show the last drawing in every fill of that frame.

    @note A PaintSource keeps GPU resources of the GraphicsContext it was last used
          with: call releaseResources() before that context goes away.

    @see Graphics::setFillSource, Graphics::setStrokeSource
*/
class YUP_API PaintSource
{
public:
    //==============================================================================
    /** Creates a source whose content will be drawn the first time it is used. */
    PaintSource();

    /** Destructor. */
    virtual ~PaintSource();

    //==============================================================================
    /** Returns the content box, in content units.

        The content spans (0, 0) to this size. The paint transform maps it like an
        image's pixels, so the identity draws one content unit per drawing unit.
        An empty size paints nothing.
    */
    virtual Size<float> getContentSize() const = 0;

    /** Draws the content, in content units.

        Called while a paint using this source is set, only when the cached raster
        is stale. The Graphics is already scaled so that the content box covers the
        whole canvas: use addTransform() rather than setTransform() to keep that.

        Using this source as a paint from within paintContent() draws nothing.

        @param g  The Graphics to draw the content with.
    */
    virtual void paintContent (Graphics& g) = 0;

    //==============================================================================
    /** Flags the content as changed, so the next paint using it draws it again. */
    void markDirty() noexcept;

    /** Frees the cached GPU canvas.

        Call it before the GraphicsContext the source was used with is destroyed.
        The next paint using the source draws the content again.
    */
    void releaseResources();

private:
    friend class Graphics;

    GpuTexture::Ptr rasterize (GraphicsContext& targetContext, const AffineTransform& contentToDevice);

    GraphicsContext* context = nullptr;
    GpuCanvas::Ptr canvas;
    uint8 shrinkStreak = 0;
    bool dirty = true;
    bool rasterizing = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PaintSource)
};

} // namespace yup
