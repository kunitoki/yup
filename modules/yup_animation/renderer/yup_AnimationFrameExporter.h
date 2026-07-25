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

class Animation; // forward declaration - defined in animation/yup_Animation.h

//==============================================================================
/** Renders an Animation to Image frames and/or exports to animated GIF.

    An exporter is bound to a `GraphicsContext` for its lifetime and owns the
    persistent GPU resources (e.g. the matte-composite pipeline) reused across
    the frames it renders. Use
    `GraphicsContext::createContext(GraphicsContext::Headless, {})` to create a
    suitable headless context for batch/export scenarios:

    @code
    auto ctx = GraphicsContext::createContext (GraphicsContext::Headless, {});
    auto anim = Animation::loadFromFile (file);

    AnimationFrameExporter exporter (*ctx);

    // Export to animated GIF
    auto result = exporter.exportToGif (anim, File ("/tmp/out.gif"));

    // Or render individual frames
    Image frame = exporter.renderFrame (anim, 0.0f);
    @endcode

    The exporter holds GPU resources tied to the context and MUST be destroyed
    before that GraphicsContext.
*/
class YUP_API AnimationFrameExporter
{
public:
    //==============================================================================
    /** Creates an exporter that renders using @p ctx.

        The context must outlive the exporter.
    */
    explicit AnimationFrameExporter (GraphicsContext& ctx);

    /** Destructor. */
    ~AnimationFrameExporter();

    //==============================================================================
    /** Renders a single frame of the animation into a new RGBA Image.

        @param anim        The animation to render.
        @param frameNo     The frame number to render.
        @param targetSize  Desired output pixel size. Pass {0,0} to use the
                           composition's native size. The animation is fitted
                           into this size while preserving its aspect ratio.
        @return            An RGBA Image, or an invalid Image on failure.
    */
    [[nodiscard]] Image renderFrame (const Animation& anim,
                                     float frameNo,
                                     Size<int> targetSize = {});

    //==============================================================================
    /** Renders every frame of the animation into a vector of RGBA Images.

        @param anim        The animation to render.
        @param targetSize  Desired output pixel size. {0,0} = native composition size.
                           The animation is fitted into this size while preserving
                           its aspect ratio.
        @return            One Image per frame in frame order, or an error.
    */
    [[nodiscard]] ResultValue<std::vector<Image>> renderAllFrames (const Animation& anim,
                                                                   Size<int> targetSize = {});

#if YUP_IMAGE_FORMAT_GIF
    //==============================================================================
    /** Exports the animation to an animated GIF file.

        Frame rate and per-frame delay are derived from the animation's frame rate.

        @param anim         The animation to export.
        @param destination  Output GIF file path.
        @param targetSize   Desired output pixel size. {0,0} = native composition size.
        @param qualityLevel 0-100, passed to the GIF writer (higher = better quality).
        @return             Result::ok() on success.
    */
    Result exportToGif (const Animation& anim,
                        const File& destination,
                        Size<int> targetSize = {},
                        int qualityLevel = 80);

    //==============================================================================
    /** Encodes a pre-rendered frame sequence to an animated GIF.

        This is a stateless helper that needs no GraphicsContext.

        @param frames       Images to encode in display order.
        @param frameRate    Frame rate in frames per second (determines frame delay).
        @param destination  Output GIF file path.
        @param qualityLevel 0-100, passed to the GIF writer.
        @return             Result::ok() on success.
    */
    static Result exportToGif (const std::vector<Image>& frames,
                               float frameRate,
                               const File& destination,
                               int qualityLevel = 80);
#endif

private:
    static Size<int> resolveTargetSize (const Animation& anim, Size<int> requested);

    GraphicsContext& context;
    AnimationRenderResources renderResources;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimationFrameExporter)
};

} // namespace yup
