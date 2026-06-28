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
    Abstract base class for writing image data to formatted image streams.

    Each concrete implementation handles encoding for a specific format (BMP, PPM, PNG, WebP)
    while accepting Image data through a unified API. Call writeImage() to encode and write
    the image to the output stream.

    @see ImageFormat, ImageFormatReader, ImageFormatManager
*/
class YUP_API ImageFormatWriter
{
public:
    /** Destructor. */
    virtual ~ImageFormatWriter();

    /** Returns the descriptive name of the format this writer produces. */
    const String& getFormatName() const noexcept { return formatName; }

    /** Returns the pixel format this writer was configured to produce. */
    PixelFormat getPixelFormat() const noexcept { return pixelFormat; }

    /** Encodes and writes the image to the output stream.
        @returns true if the image was encoded and written successfully.
    */
    virtual bool writeImage (const Image& image) = 0;

    /** Flushes any buffered data to the output stream.
        The default implementation returns true without doing anything.
        @returns true if the flush succeeded.
    */
    virtual bool flush();

    //==============================================================================
    /** Returns true if this writer supports animated output.

        The default implementation returns false. Only GIF writers return true.
    */
    virtual bool supportsAnimation() const { return false; }

    /** Begins an animated sequence.

        Must be called before writeFrame(). Writes the format header and, if
        loopCount != 1, the NETSCAPE2.0 loop extension block.

        0 = loop infinitely; 1 = play once; N = play N times.

        Calling this on a writer that does not support animation triggers
        jassertfalse and returns false.

        @param loopCount  Number of times to loop the animation.
        @returns true on success.
    */
    virtual bool beginAnimation (int loopCount = 0);

    /** Encodes and appends one animation frame.

        Must be called between beginAnimation() and endAnimation(). Each call
        quantizes the frame to a 256-color palette, writes a Graphic Control
        Extension, and appends the image descriptor and raster data.

        Calling this on a non-animation writer triggers jassertfalse and returns false.

        @param frame    The frame to encode (RGBA pixel format expected).
        @param delayMs  Display duration for this frame in milliseconds.
        @returns true on success.
    */
    virtual bool writeFrame (const Image& frame, int delayMs);

    /** Finalises an animated sequence.

        Writes the GIF trailer byte. Must be called after the last writeFrame().

        Calling this on a non-animation writer triggers jassertfalse and returns false.

        @returns true on success.
    */
    virtual bool endAnimation();

    /** The output stream, for use by subclasses. */
    std::unique_ptr<OutputStream> output;

protected:
    /** Creates an ImageFormatWriter and takes ownership of the destination stream. */
    ImageFormatWriter (OutputStream* destStream, const String& formatName, PixelFormat pixelFormat);

private:
    String formatName;
    PixelFormat pixelFormat;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageFormatWriter)
};

} // namespace yup
