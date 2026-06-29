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

class Animation; // forward declaration — defined in animation/yup_Animation.h

//==============================================================================
/** Renders an Animation to Image frames and/or exports to animated GIF.

    All methods are static — this class has no per-instance state.

    GPU-backed offscreen rendering is used via a `GraphicsContext` passed in by
    the caller. Use `GraphicsContext::createContext(GraphicsContext::Headless, {})`
    to create a suitable headless context for batch/export scenarios:

    @code
    auto ctx = GraphicsContext::createContext (GraphicsContext::Headless, {});
    auto anim = Animation::loadFromFile (file);

    // Export to animated GIF
    auto result = AnimationFrameExporter::exportToGif (*ctx, anim, File ("/tmp/out.gif"));

    // Or render individual frames
    Image frame = AnimationFrameExporter::renderFrame (*ctx, anim, 0.0f);
    @endcode
*/
class YUP_API AnimationFrameExporter
{
public:
    //==============================================================================
    /** Renders a single frame of the animation into a new RGBA Image.

        @param ctx         Graphics context used for GPU-backed offscreen rendering.
        @param anim        The animation to render.
        @param frameNo     The frame number to render.
        @param targetSize  Desired output pixel size. Pass {0,0} to use the
                           composition's native size. The animation is fitted
                           into this size while preserving its aspect ratio.
        @return            An RGBA Image, or an invalid Image on failure.
    */
    [[nodiscard]] static Image renderFrame (GraphicsContext& ctx,
                                            const Animation& anim,
                                            float frameNo,
                                            Size<int> targetSize = {});

    //==============================================================================
    /** Renders every frame of the animation into a vector of RGBA Images.

        @param ctx         Graphics context used for GPU-backed offscreen rendering.
        @param anim        The animation to render.
        @param targetSize  Desired output pixel size. {0,0} = native composition size.
                           The animation is fitted into this size while preserving
                           its aspect ratio.
        @return            One Image per frame in frame order, or an error.
    */
    [[nodiscard]] static ResultValue<std::vector<Image>> renderAllFrames (GraphicsContext& ctx,
                                                                          const Animation& anim,
                                                                          Size<int> targetSize = {});

    //==============================================================================
    /** Exports the animation to an animated GIF file.

        Frame rate and per-frame delay are derived from the animation's frame rate.

        @param ctx          Graphics context used for GPU-backed offscreen rendering.
        @param anim         The animation to export.
        @param destination  Output GIF file path.
        @param targetSize   Desired output pixel size. {0,0} = native composition size.
        @param qualityLevel 0-100, passed to the GIF writer (higher = better quality).
        @return             Result::ok() on success.
    */
    static Result exportToGif (GraphicsContext& ctx,
                               const Animation& anim,
                               const File& destination,
                               Size<int> targetSize = {},
                               int qualityLevel = 80);

    //==============================================================================
    /** Encodes a pre-rendered frame sequence to an animated GIF.

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

private:
    static Size<int> resolveTargetSize (const Animation& anim, Size<int> requested);
};

} // namespace yup
