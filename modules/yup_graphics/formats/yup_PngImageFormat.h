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

#if YUP_IMAGE_FORMAT_PNG

namespace yup
{

//==============================================================================
/**
    Reads image data from PNG formatted streams using libpng.

    Supports 8-bit and 16-bit (normalised to 8-bit) images in the following
    colour modes: RGBA, RGB, Grayscale+Alpha (expanded to RGBA), Grayscale,
    and Paletted (expanded to RGB or RGBA).

    Also supports APNG (animated PNG) via manual chunk parsing for animation
    metadata combined with libpng for per-frame decompression and compositing.

    Width, height, pixelFormat, dpiX, and dpiY are populated during construction.
    tEXt and pHYs metadata chunks are also extracted during construction.
    For animated images, frame count, loop count, and per-frame delays are
    available immediately after construction.

    If the header cannot be decoded, width and height remain zero and readImage()
    returns an invalid Image.

    @see ImageFormatReader, PngImageFormatWriter, PngImageFormat
*/
class YUP_API PngImageFormatReader : public ImageFormatReader
{
public:
    //==============================================================================
    /** Constructs the reader and parses the PNG header from the stream.

        Width, height, pixelFormat, dpiX, dpiY, and metadataValues are populated
        during construction. For APNG files, frame metadata is also extracted.
        If parsing fails, width and height remain zero.

        @param stream  The source stream. This object takes ownership.
        @param options Options controlling metadata extraction.
    */
    explicit PngImageFormatReader (InputStream* stream, const ImageFormat::Options& options = {});

    //==============================================================================
    /** Decodes the full image from the input stream.

        For animated images, returns frame 0 as a fully composited Image.
        The stream is rewound to position 0 before decoding so that the same
        InputStream can be used even after the header has been read.

        @returns The decoded Image, or a default-constructed (invalid) Image on failure.
    */
    Image readImage() override;

    //==============================================================================
    /** Decodes and returns a single animation frame by index.

        Sequential access (0, 1, 2, …) reuses the internal canvas (O(1) per frame).
        Backward seeks re-composite from frame 0.
        @param frameIndex  Zero-based frame index.
        @returns The decoded frame Image, or an invalid Image if frameIndex is out of range.
    */
    Image readFrame (int frameIndex) override;

    /** Decodes a frame directly into an existing Image, avoiding allocation when possible.
        If dest has the correct width, height, and PixelFormat::RGBA, decodes directly
        into dest's raw pixel buffer — zero allocation.
        @param frameIndex  Zero-based frame index.
        @param dest        Image to decode into (may be reallocated).
        @returns true on success.
    */
    bool readFrame (int frameIndex, Image& dest) override;

    /** Returns true when the APNG contains more than one frame. */
    bool isAnimated() const override;

    /** Returns the number of frames in the APNG animation. */
    int getFrameCount() const override;

    /** Returns the loop count from the acTL chunk.
        0 = loop infinitely; 1 = play once (also returned for non-animated images).
    */
    int getLoopCount() const override;

    /** Returns the display duration for a frame in milliseconds. */
    int getFrameDelayMs (int frameIndex) const override;

private:
    //==============================================================================
    struct FrameInfo
    {
        std::vector<uint8_t> imageData; // Concatenated IDAT/fdAT payloads for this frame
        int xOffset = 0;
        int yOffset = 0;
        int frameWidth = 0;
        int frameHeight = 0;
        int delayMs = 0;
        int disposeOp = 0; // 0=none, 1=background, 2=previous
        int blendOp = 0;   // 0=source, 1=over
    };

    void parseHeader(); // Static PNG header parse (libpng-based, existing logic)
    void parseChunks(); // Manual chunk-level parse for APNG metadata + raw chunks
    void compositeFrame (int frameIndex);
    void resetCanvas();
    Image decodeFrameImage (int frameIndex);

    std::vector<uint8_t> fileData;
    std::vector<FrameInfo> frames;
    Image canvas;
    Image previousCanvas;
    int loopCount = 1;
    int lastRenderedFrame = -1;
    bool isApng = false;

    // APNG original encoding parameters (captured before transforms)
    int apngOriginalColorType = 6; // PNG_COLOR_TYPE_RGBA
    int apngOriginalBitDepth = 8;
    std::vector<uint8_t> apngPLTEData;
    std::vector<uint8_t> apngTRNSData;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PngImageFormatReader)
};

//==============================================================================
/**
    Writes image data to PNG formatted streams using libpng.

    Supports writing Grayscale (PNG_COLOR_TYPE_GRAY), RGB (PNG_COLOR_TYPE_RGB),
    and RGBA (PNG_COLOR_TYPE_RGBA) images at 8-bit depth with default compression.

    Also supports APNG (animated PNG) output via the beginAnimation() /
    writeFrame() / endAnimation() sequence.

    @see ImageFormatWriter, PngImageFormatReader, PngImageFormat
*/
class YUP_API PngImageFormatWriter : public ImageFormatWriter
{
public:
    //==============================================================================
    /** Constructs the writer for the given stream and desired pixel format.

        @param stream  The destination stream. This object takes ownership.
        @param fmt     The pixel format to write. Determines the PNG colour type.
    */
    PngImageFormatWriter (OutputStream* stream, PixelFormat fmt);

    //==============================================================================
    /** Encodes and writes the image to the output stream.

        @returns true if the image was written successfully, false otherwise.
    */
    bool writeImage (const Image& image) override;

    //==============================================================================
    /** Returns true — PNG supports animated output via APNG. */
    bool supportsAnimation() const override { return true; }

    /** Begins an APNG animation sequence.

        Writes the PNG signature, IHDR, and acTL chunks. Frame data is buffered
        and written during endAnimation().

        @param loopCount  0 = loop infinitely; 1 = play once; N = play N times.
        @returns true on success.
    */
    bool beginAnimation (int loopCount = 0) override;

    /** Encodes and buffers one animation frame.

        Must be called between beginAnimation() and endAnimation(). Each frame
        is encoded as a complete PNG via libpng to a memory buffer from which
        the IDAT data is extracted and stored with fcTL metadata.

        @param frame    Frame image to encode.
        @param delayMs  Display duration for this frame in milliseconds.
        @returns true on success.
    */
    bool writeFrame (const Image& frame, int delayMs) override;

    /** Finalises the APNG animation sequence.

        Writes all buffered frames with fcTL/fdAT chunks and the IEND chunk
        to the output stream.

        @returns true on success.
    */
    bool endAnimation() override;

private:
    //==============================================================================
    struct BufferedFrame
    {
        std::vector<uint8_t> idatData;
        int width = 0;
        int height = 0;
        int xOffset = 0;
        int yOffset = 0;
        int delayMs = 0;
    };

    std::vector<uint8_t> encodeFrameToPng (const Image& frame);

    std::vector<BufferedFrame> bufferedFrames;
    int canvasWidth = 0;
    int canvasHeight = 0;
    int animLoopCount = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PngImageFormatWriter)
};

//==============================================================================
/**
    ImageFormat implementation for the PNG (Portable Network Graphics) image format.

    Uses libpng for decoding and encoding. Supports lossless, compressed PNG files
    with RGB, RGBA, and Grayscale pixel formats.

    Extensions handled: .png

    @see ImageFormat, PngImageFormatReader, PngImageFormatWriter
*/
class YUP_API PngImageFormat : public ImageFormat
{
public:
    //==============================================================================
    /** Constructs a PngImageFormat instance. */
    PngImageFormat();

    //==============================================================================
    /** Returns "PNG Image". */
    const String& getFormatName() const override;

    /** Returns {".png"} for both reading and writing. */
    StringArray getFileExtensions (Mode mode) const override;

    /** Returns true if the stream starts with the 8-byte PNG signature. */
    bool canHandleStream (InputStream& stream, Mode mode) const override;

    //==============================================================================
    /** Creates a PngImageFormatReader for the given stream. */
    std::unique_ptr<ImageFormatReader> createReaderFor (InputStream* sourceStream,
                                                        const Options& options = {}) override;

    /** Creates a PngImageFormatWriter for the given stream and pixel format. */
    std::unique_ptr<ImageFormatWriter> createWriterFor (OutputStream* destStream,
                                                        PixelFormat pixelFormat,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex) override;

    //==============================================================================
    /** Returns {PixelFormat::Grayscale, PixelFormat::RGB, PixelFormat::RGBA}. */
    Array<PixelFormat> getPossiblePixelFormats() const override;

    /** Returns true because PNG uses zlib compression. */
    bool isCompressed() const override { return true; }

private:
    //==============================================================================
    String formatName;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PngImageFormat)
};

} // namespace yup

#endif // YUP_IMAGE_FORMAT_PNG
