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
/**
    Abstract base class for reading image data from formatted image streams.

    Each concrete implementation handles decoding for a specific format (BMP, PPM, PNG, WebP)
    while presenting a unified API. Call readImage() to decode the full image.

    Properties width, height, pixelFormat, dpiX, dpiY, and metadataValues are populated
    by the concrete reader during construction or the first readImage() call.

    @see ImageFormat, ImageFormatWriter, ImageFormatManager
*/
class YUP_API ImageFormatReader
{
public:
    /** Destructor. */
    virtual ~ImageFormatReader() = default;

    /** Returns the descriptive name of the format this reader handles. */
    const String& getFormatName() const noexcept { return formatName; }

    /** Returns the options this reader was constructed with. */
    const ImageFormat::Options& getOptions() const noexcept { return options; }

    /** Decodes and returns the complete image from the input stream.
        @returns The decoded Image, or an invalid Image on failure.
    */
    virtual Image readImage() = 0;

    //==============================================================================
    /** Decodes and returns a single animation frame by index.

        The default implementation calls readImage() for frame 0 and returns an
        invalid Image for any other index. Override in animated-format readers.

        @param frameIndex  Zero-based frame index.
        @returns The decoded frame Image, or an invalid Image on failure.
    */
    virtual Image readFrame (int frameIndex);

    /** Decodes a single animation frame into an existing Image, reusing its buffer
        when dimensions and pixel format match (zero allocation on reuse).

        If dest already has the correct width, height, and PixelFormat::RGBA,
        the implementation writes directly into dest's raw data - no allocation.
        If dest has wrong dimensions or format, it is reallocated first.

        The default implementation calls readFrame(frameIndex) and moves the result
        into dest.

        @param frameIndex  Zero-based frame index.
        @param dest        Image to decode into (may be reallocated).
        @returns true if the frame was decoded successfully.
    */
    virtual bool readFrame (int frameIndex, Image& dest);

    //==============================================================================
    /** Returns true if this image contains more than one animation frame.

        The default implementation returns false. Override in animated-format readers.
    */
    virtual bool isAnimated() const { return false; }

    /** Returns the total number of frames.

        The default implementation returns 1. Override in animated-format readers.
    */
    virtual int getFrameCount() const { return 1; }

    /** Returns the loop count for the animation.

        0 means loop infinitely; 1 means play once; N means play N times.
        The default implementation returns 1. Override in animated-format readers.
    */
    virtual int getLoopCount() const { return 1; }

    /** Returns the display duration of a frame in milliseconds.

        The default implementation returns 0. Override in animated-format readers.

        @param frameIndex  Zero-based frame index.
    */
    virtual int getFrameDelayMs (int frameIndex) const { return 0; }

    //==============================================================================
    /** The image width in pixels (populated after construction). */
    int width = 0;

    /** The image height in pixels (populated after construction). */
    int height = 0;

    /** The pixel format of the decoded image. */
    PixelFormat pixelFormat = PixelFormat::RGBA;

    /** The input stream, for use by subclasses. */
    std::unique_ptr<InputStream> input;

    /** Metadata extracted from the image file (nullptr if no metadata was requested or found). */
    ImageMetadata::Ptr metadata;

protected:
    /** Creates an ImageFormatReader and takes ownership of the source stream. */
    ImageFormatReader (InputStream* sourceStream, const String& formatName);

    /** Creates an ImageFormatReader with options. */
    ImageFormatReader (InputStream* sourceStream, const String& formatName, const ImageFormat::Options& opts);

private:
    String formatName;

    ImageFormat::Options options;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageFormatReader)
};

} // namespace yup
