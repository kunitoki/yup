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

//==============================================================================
// AnimationFrameExporter

Size<int> AnimationFrameExporter::resolveTargetSize (const Animation& anim, Size<int> requested)
{
    if (requested.getWidth() > 0 && requested.getHeight() > 0)
        return requested;

    const Size<float> native = anim.size();
    return { (int) native.getWidth(), (int) native.getHeight() };
}

Image AnimationFrameExporter::renderFrame (GraphicsContext& ctx,
                                           const Animation& anim,
                                           float frameNo,
                                           Size<int> targetSize)
{
    if (! anim.isValid())
        return {};

    const Size<int> sz = resolveTargetSize (anim, targetSize);
    if (sz.getWidth() <= 0 || sz.getHeight() <= 0)
        return {};

    Image img (sz.getWidth(), sz.getHeight(), PixelFormat::RGBA);

    {
        Graphics g (ctx, img, 0x00000000u);
        anim.renderFrame (g, frameNo, { 0.0f, 0.0f, (float) sz.getWidth(), (float) sz.getHeight() }, true);
        g.readPixelsToImage();
    }

    return img;
}

ResultValue<std::vector<Image>> AnimationFrameExporter::renderAllFrames (GraphicsContext& ctx,
                                                                         const Animation& anim,
                                                                         Size<int> targetSize)
{
    if (! anim.isValid())
        return makeResultValueFail ("Animation is not valid");

    const float totalFrames = anim.totalFrames();
    if (totalFrames <= 0.0f)
        return makeResultValueFail ("Animation has no frames");

    const Size<int> sz = resolveTargetSize (anim, targetSize);
    if (sz.getWidth() <= 0 || sz.getHeight() <= 0)
        return makeResultValueFail ("Invalid target size");

    std::vector<Image> frames;
    frames.reserve ((size_t) totalFrames);

    for (float f = 0.0f; f < totalFrames; f += 1.0f)
        frames.push_back (renderFrame (ctx, anim, f, sz));

    return makeResultValueOk (std::move (frames));
}

Result AnimationFrameExporter::exportToGif (GraphicsContext& ctx,
                                            const Animation& anim,
                                            const File& destination,
                                            Size<int> targetSize,
                                            int qualityLevel)
{
    if (! anim.isValid())
        return Result::fail ("Animation is not valid");

    const float totalFrames = anim.totalFrames();
    if (totalFrames <= 0.0f)
        return Result::fail ("Animation has no frames");

    const float fps = anim.frameRate();
    if (fps <= 0.0f)
        return Result::fail ("Animation has invalid frame rate");

    const Size<int> sz = resolveTargetSize (anim, targetSize);
    if (sz.getWidth() <= 0 || sz.getHeight() <= 0)
        return Result::fail ("Invalid target size");

    std::vector<Image> frames;
    frames.reserve ((size_t) totalFrames);
    for (float f = 0.0f; f < totalFrames; f += 1.0f)
        frames.push_back (renderFrame (ctx, anim, f, sz));

    return exportToGif (frames, fps, destination, qualityLevel);
}

Result AnimationFrameExporter::exportToGif (const std::vector<Image>& frames,
                                            float frameRate,
                                            const File& destination,
                                            int qualityLevel)
{
    if (frames.empty())
        return Result::fail ("No frames to export");

    if (frameRate <= 0.0f)
        return Result::fail ("Invalid frame rate");

    if (! destination.getParentDirectory().exists())
        destination.getParentDirectory().createDirectory();

    auto* outStream = new FileOutputStream (destination);
    if (outStream->failedToOpen())
    {
        delete outStream;
        return Result::fail ("Cannot open output file: " + destination.getFullPathName());
    }

    GifImageFormat gifFormat;
    auto writer = gifFormat.createWriterFor (outStream,
                                             PixelFormat::RGBA,
                                             StringPairArray {},
                                             qualityLevel);
    if (writer == nullptr)
        return Result::fail ("Failed to create GIF writer");

    auto* gifWriter = dynamic_cast<GifImageFormatWriter*> (writer.get());
    if (gifWriter == nullptr)
        return Result::fail ("GIF writer does not support animation");

    const int frameDelayMs = jmax (1, (int) (1000.0f / frameRate));

    if (! gifWriter->beginAnimation (0)) // 0 = loop infinitely
        return Result::fail ("Failed to begin GIF animation");

    for (const auto& frame : frames)
    {
        if (! frame.isValid())
            continue;

        if (! gifWriter->writeFrame (frame, frameDelayMs))
            return Result::fail ("Failed to write GIF frame");
    }

    if (! gifWriter->endAnimation())
        return Result::fail ("Failed to finalise GIF animation");

    return Result::ok();
}

} // namespace yup
