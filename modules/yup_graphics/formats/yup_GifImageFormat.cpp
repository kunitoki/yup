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

#if YUP_IMAGE_FORMAT_GIF

namespace yup
{

//==============================================================================
// Internal helpers (file-scope, anonymous namespace)
//==============================================================================

namespace
{

struct GifMemorySource
{
    const uint8_t* data = nullptr;
    size_t size = 0;
    size_t pos = 0;
};

int gifReadCallback (GifFileType* gif, GifByteType* buf, int count)
{
    auto* src = static_cast<GifMemorySource*> (gif->UserData);
    const int available = static_cast<int> (src->size - src->pos);
    const int toRead = std::min (count, available);
    if (toRead > 0)
    {
        std::memcpy (buf, src->data + src->pos, static_cast<size_t> (toRead));
        src->pos += static_cast<size_t> (toRead);
    }
    return toRead;
}

int gifWriteCallback (GifFileType* gif, const GifByteType* buf, int count)
{
    auto* stream = static_cast<OutputStream*> (gif->UserData);
    stream->write (buf, static_cast<size_t> (count));
    return count;
}

// Parse per-frame GCE: returns delay in ms and disposal method.
// Returns false if no GCE found (no delay, disposal = 0).
bool parseGce (const SavedImage& frame, int& delayMs, int& disposal, bool& hasTransparency, int& transparentIndex)
{
    for (int i = 0; i < frame.ExtensionBlockCount; ++i)
    {
        const auto& ext = frame.ExtensionBlocks[i];
        if (ext.Function == GRAPHICS_EXT_FUNC_CODE && ext.ByteCount >= 4)
        {
            disposal = (ext.Bytes[0] >> 2) & 0x07;
            hasTransparency = (ext.Bytes[0] & 0x01) != 0;
            const int centiseconds = (static_cast<int> (static_cast<uint8_t> (ext.Bytes[2])) << 8)
                                   | static_cast<int> (static_cast<uint8_t> (ext.Bytes[1]));
            delayMs = centiseconds * 10;
            transparentIndex = hasTransparency ? static_cast<int> (static_cast<uint8_t> (ext.Bytes[3])) : -1;
            return true;
        }
    }
    delayMs = 0;
    disposal = 0;
    hasTransparency = false;
    transparentIndex = -1;
    return false;
}

// Parse loop count from NETSCAPE2.0 application extension.
// Returns 1 (play once) if not found.
int parseLoopCount (GifFileType* gif)
{
    for (int fi = 0; fi < gif->ImageCount; ++fi)
    {
        const auto& frame = gif->SavedImages[fi];
        for (int i = 0; i < frame.ExtensionBlockCount - 1; ++i)
        {
            const auto& ext = frame.ExtensionBlocks[i];
            if (ext.Function == APPLICATION_EXT_FUNC_CODE
                && ext.ByteCount == 11
                && std::memcmp (ext.Bytes, "NETSCAPE2.0", 11) == 0)
            {
                const auto& sub = frame.ExtensionBlocks[i + 1];
                if (sub.ByteCount >= 3 && sub.Bytes[0] == 1)
                    return static_cast<int> (static_cast<uint8_t> (sub.Bytes[1]))
                         | (static_cast<int> (static_cast<uint8_t> (sub.Bytes[2])) << 8);
            }
        }
    }
    return 1;
}

// Map a GIF palette index to ARGB using local or global color map, and transparency.
uint32_t mapGifPixel (int index, const ColorMapObject* colorMap, bool hasTransparency, int transparentIndex)
{
    if (hasTransparency && index == transparentIndex)
        return 0x00000000u;
    if (colorMap == nullptr || index >= colorMap->ColorCount)
        return 0xFF000000u;
    const auto& c = colorMap->Colors[index];
    return (0xFFu << 24) | (static_cast<uint32_t> (c.Red) << 16)
         | (static_cast<uint32_t> (c.Green) << 8) | static_cast<uint32_t> (c.Blue);
}

} // anonymous namespace

//==============================================================================
// GifImageFormatReader
//==============================================================================

GifImageFormatReader::GifImageFormatReader (InputStream* stream)
    : ImageFormatReader (stream, "GIF Image")
{
    // Load entire stream into memory
    uint8_t chunk[4096];
    long bytesRead;
    while ((bytesRead = input->read (chunk, sizeof (chunk))) > 0)
        fileData.insert (fileData.end(), chunk, chunk + bytesRead);

    if (fileData.empty())
        return;

    // Open GIF decoder via memory callback
    GifMemorySource src { fileData.data(), fileData.size(), 0 };
    int error = 0;
    GifFileType* rawGif = DGifOpen (&src, gifReadCallback, &error);
    if (rawGif == nullptr)
        return;

    gifFile.reset (rawGif);

    if (DGifSlurp (gifFile.get()) != GIF_OK)
    {
        gifFile.reset();
        return;
    }

    width = gifFile->SWidth;
    height = gifFile->SHeight;
    pixelFormat = PixelFormat::RGBA;

    // Pre-parse per-frame metadata
    const int count = gifFile->ImageCount;
    frameDelaysMs.resize (static_cast<size_t> (count), 0);
    disposalMethods.resize (static_cast<size_t> (count), 0);

    for (int fi = 0; fi < count; ++fi)
    {
        int delayMs = 0, disposal = 0, transparentIndex = -1;
        bool hasTransparency = false;
        parseGce (gifFile->SavedImages[fi], delayMs, disposal, hasTransparency, transparentIndex);
        frameDelaysMs[static_cast<size_t> (fi)] = delayMs;
        disposalMethods[static_cast<size_t> (fi)] = disposal;
    }

    loopCount = parseLoopCount (gifFile.get());

    // Initialise canvas to screen background
    if (width > 0 && height > 0)
    {
        canvas = Image (width, height, PixelFormat::RGBA);
        canvas.fill (0x00000000u);
    }
}

Image GifImageFormatReader::readImage()
{
    Image dest;
    readFrame (0, dest);
    return dest;
}

Image GifImageFormatReader::readFrame (int frameIndex)
{
    Image dest;
    readFrame (frameIndex, dest);
    return dest;
}

bool GifImageFormatReader::readFrame (int frameIndex, Image& dest)
{
    if (gifFile == nullptr || frameIndex < 0 || frameIndex >= gifFile->ImageCount)
        return false;

    if (width <= 0 || height <= 0)
        return false;

    // Ensure canvas is the right size
    if (! canvas.isValid() || canvas.getWidth() != width || canvas.getHeight() != height)
    {
        canvas = Image (width, height, PixelFormat::RGBA);
        canvas.fill (0x00000000u);
        lastRenderedFrame = -1;
    }

    // Seek strategy:
    // Forward sequential: composite only the missing frames.
    // Backward or random: reset and composite from 0.
    if (frameIndex < lastRenderedFrame || (frameIndex > lastRenderedFrame + 1 && frameIndex != 0))
        resetCanvas();

    const int startFrame = lastRenderedFrame + 1;

    for (int fi = startFrame; fi <= frameIndex; ++fi)
        compositeFrame (fi);

    // Reuse dest allocation when dimensions and format already match
    const bool compatible = dest.isValid()
                         && dest.getWidth() == width
                         && dest.getHeight() == height
                         && dest.getPixelFormat() == PixelFormat::RGBA;

    if (! compatible)
        dest = Image (width, height, PixelFormat::RGBA);

    // Copy composited canvas to dest
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            dest.setPixel (x, y, canvas.getPixel (x, y));

    return true;
}

void GifImageFormatReader::compositeFrame (int frameIndex)
{
    if (frameIndex < 0 || frameIndex >= gifFile->ImageCount)
        return;

    const auto& frame = gifFile->SavedImages[frameIndex];
    const auto& desc = frame.ImageDesc;

    // Apply previous frame's disposal method before drawing current frame
    if (frameIndex > 0)
    {
        const int prevDisposal = disposalMethods[static_cast<size_t> (frameIndex - 1)];
        if (prevDisposal == 2)
        {
            // Restore to background (transparent)
            const auto& prevFrame = gifFile->SavedImages[frameIndex - 1];
            const auto& prevDesc = prevFrame.ImageDesc;
            for (int y = prevDesc.Top; y < prevDesc.Top + prevDesc.Height && y < height; ++y)
                for (int x = prevDesc.Left; x < prevDesc.Left + prevDesc.Width && x < width; ++x)
                    canvas.setPixel (x, y, 0x00000000u);
        }
        else if (prevDisposal == 3 && previousCanvas.isValid())
        {
            // Restore to previous canvas state
            canvas = previousCanvas;
        }
    }

    // Save current canvas before drawing if this frame uses disposal=3
    const int thisDisposal = disposalMethods[static_cast<size_t> (frameIndex)];
    if (thisDisposal == 3)
        previousCanvas = canvas;

    // Parse GCE for this frame to find transparency
    int delayMs = 0, disposal = 0, transparentIndex = -1;
    bool hasTransparency = false;
    parseGce (frame, delayMs, disposal, hasTransparency, transparentIndex);

    // Select color map: local takes priority over global
    const ColorMapObject* colorMap = desc.ColorMap != nullptr ? desc.ColorMap : gifFile->SColorMap;

    // Render frame sub-rectangle onto canvas
    const GifByteType* raster = frame.RasterBits;
    for (int row = 0; row < desc.Height; ++row)
    {
        const int canvasY = desc.Top + row;
        if (canvasY < 0 || canvasY >= height)
            continue;

        for (int col = 0; col < desc.Width; ++col)
        {
            const int canvasX = desc.Left + col;
            if (canvasX < 0 || canvasX >= width)
                continue;

            const int index = static_cast<int> (raster[row * desc.Width + col]);
            if (hasTransparency && index == transparentIndex)
                continue; // leave canvas pixel as-is

            canvas.setPixel (canvasX, canvasY, mapGifPixel (index, colorMap, false, -1));
        }
    }

    lastRenderedFrame = frameIndex;
}

void GifImageFormatReader::resetCanvas()
{
    canvas.fill (0x00000000u);
    previousCanvas = {};
    lastRenderedFrame = -1;
}

bool GifImageFormatReader::isAnimated() const
{
    return gifFile != nullptr && gifFile->ImageCount > 1;
}

int GifImageFormatReader::getFrameCount() const
{
    return gifFile != nullptr ? gifFile->ImageCount : 0;
}

int GifImageFormatReader::getLoopCount() const
{
    return loopCount;
}

int GifImageFormatReader::getFrameDelayMs (int frameIndex) const
{
    if (frameIndex < 0 || static_cast<size_t> (frameIndex) >= frameDelaysMs.size())
        return 0;
    return frameDelaysMs[static_cast<size_t> (frameIndex)];
}

//==============================================================================
// GifImageFormatWriter
//==============================================================================

GifImageFormatWriter::GifImageFormatWriter (OutputStream* stream, PixelFormat fmt)
    : ImageFormatWriter (stream, "GIF Image", fmt)
{
}

GifImageFormatWriter::~GifImageFormatWriter() = default;

bool GifImageFormatWriter::writeImage (const Image& image)
{
    if (output == nullptr || ! image.isValid())
        return false;

    const int w = image.getWidth();
    const int h = image.getHeight();
    if (w <= 0 || h <= 0)
        return false;

    int error = 0;
    GifFileType* gif = EGifOpen (output.get(), gifWriteCallback, &error);
    if (gif == nullptr)
        return false;

    std::unique_ptr<GifFileType, GifDeleter> gifOwned (gif);
    EGifSetGifVersion (gif, true); // GIF89a

    if (! writeFrameInternal (gif, image, 0, true, w, h))
        return false;

    return EGifCloseFile (gifOwned.release(), &error) == GIF_OK;
}

bool GifImageFormatWriter::beginAnimation (int loopCount)
{
    if (output == nullptr)
        return false;

    animWidth = 0;
    animHeight = 0;
    animGif.reset();

    int error = 0;
    GifFileType* gif = EGifOpen (output.get(), gifWriteCallback, &error);
    if (gif == nullptr)
        return false;

    EGifSetGifVersion (gif, true); // GIF89a
    animGif.reset (gif);

    // Defer EGifPutScreenDesc until the first writeFrame call so we know dimensions.
    // Use animWidth = -(loopCount+1) as sentinel: negative means "header not yet written".
    animWidth = -(loopCount + 1); // sentinel: < 0 means header pending
    animHeight = loopCount;       // store loop count here for use in writeFrame
    return true;
}

bool GifImageFormatWriter::writeFrame (const Image& frame, int delayMs)
{
    if (animGif == nullptr || ! frame.isValid())
        return false;

    const int w = frame.getWidth();
    const int h = frame.getHeight();
    if (w <= 0 || h <= 0)
        return false;

    const bool isFirstFrame = (animWidth < 0);

    if (isFirstFrame)
    {
        const int storedLoopCount = animHeight; // saved in beginAnimation

        // Build a temporary palette from the first frame for the global colour table
        const int pixelCount = w * h;
        std::vector<GifByteType> r (static_cast<size_t> (pixelCount));
        std::vector<GifByteType> g (static_cast<size_t> (pixelCount));
        std::vector<GifByteType> b (static_cast<size_t> (pixelCount));

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const auto argb = frame.getPixel (x, y);
                const size_t idx = static_cast<size_t> (y * w + x);
                r[idx] = static_cast<GifByteType> ((argb >> 16) & 0xFF);
                g[idx] = static_cast<GifByteType> ((argb >> 8) & 0xFF);
                b[idx] = static_cast<GifByteType> ((argb >> 0) & 0xFF);
            }
        }

        int colorCount = 256;
        std::vector<GifByteType> indices (static_cast<size_t> (pixelCount));
        std::vector<GifColorType> palette (256);
        GifQuantizeBuffer (static_cast<unsigned> (w), static_cast<unsigned> (h), &colorCount, r.data(), g.data(), b.data(), indices.data(), palette.data());

        ColorMapObject* globalMap = GifMakeMapObject (256, palette.data());
        EGifPutScreenDesc (animGif.get(), w, h, 8, 0, globalMap);
        GifFreeMapObject (globalMap);

        // Write NETSCAPE2.0 loop block when loopCount != 1
        if (storedLoopCount != 1)
        {
            const uint8_t netscape[] = { 'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0' };
            EGifPutExtensionLeader (animGif.get(), APPLICATION_EXT_FUNC_CODE);
            EGifPutExtensionBlock (animGif.get(), 11, netscape);
            const uint8_t loopBlock[] = {
                0x01,
                static_cast<uint8_t> (storedLoopCount & 0xFF),
                static_cast<uint8_t> ((storedLoopCount >> 8) & 0xFF)
            };
            EGifPutExtensionBlock (animGif.get(), 3, loopBlock);
            EGifPutExtensionTrailer (animGif.get());
        }

        animWidth = w;
        animHeight = h;
    }

    return writeFrameInternal (animGif.get(), frame, delayMs, false, animWidth, animHeight);
}

bool GifImageFormatWriter::endAnimation()
{
    if (animGif == nullptr)
        return false;

    int error = 0;
    const bool ok = (EGifCloseFile (animGif.release(), &error) == GIF_OK);
    return ok;
}

bool GifImageFormatWriter::writeFrameInternal (GifFileType* gif, const Image& frame, int delayMs, bool writeScreenDesc, int globalWidth, int globalHeight)
{
    const int w = frame.getWidth();
    const int h = frame.getHeight();

    // Separate RGB channels and detect fully-transparent pixels
    const int pixelCount = w * h;
    std::vector<GifByteType> r (static_cast<size_t> (pixelCount));
    std::vector<GifByteType> g (static_cast<size_t> (pixelCount));
    std::vector<GifByteType> b (static_cast<size_t> (pixelCount));

    bool hasTransparentPixels = false;

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const auto argb = frame.getPixel (x, y);
            const size_t idx = static_cast<size_t> (y * w + x);
            r[idx] = static_cast<GifByteType> ((argb >> 16) & 0xFF);
            g[idx] = static_cast<GifByteType> ((argb >> 8) & 0xFF);
            b[idx] = static_cast<GifByteType> ((argb >> 0) & 0xFF);
            if (((argb >> 24) & 0xFF) == 0)
                hasTransparentPixels = true;
        }
    }

    // Quantize to 256-colour palette (or 255 to reserve one slot for transparency)
    int colorCount = hasTransparentPixels ? 255 : 256;
    std::vector<GifByteType> indices (static_cast<size_t> (pixelCount));
    std::vector<GifColorType> palette (256);
    GifQuantizeBuffer (static_cast<unsigned> (w), static_cast<unsigned> (h), &colorCount, r.data(), g.data(), b.data(), indices.data(), palette.data());

    // Reserve index 255 for transparency (if needed)
    const int transparentIndex = hasTransparentPixels ? 255 : -1;
    if (hasTransparentPixels)
    {
        palette[255] = { 0, 0, 0 };
        // Re-map fully-transparent pixels to index 255
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const auto argb = frame.getPixel (x, y);
                if (((argb >> 24) & 0xFF) == 0)
                    indices[static_cast<size_t> (y * w + x)] = 255;
            }
        }
    }

    ColorMapObject* localMap = GifMakeMapObject (256, palette.data());

    if (writeScreenDesc)
        EGifPutScreenDesc (gif, globalWidth, globalHeight, 8, 0, localMap);

    // Write GCE
    const int centiseconds = delayMs / 10;
    uint8_t gce[4];
    gce[0] = static_cast<uint8_t> ((2 << 2) | (hasTransparentPixels ? 1 : 0)); // disposal=2, transparency flag
    gce[1] = static_cast<uint8_t> (centiseconds & 0xFF);
    gce[2] = static_cast<uint8_t> ((centiseconds >> 8) & 0xFF);
    gce[3] = static_cast<uint8_t> (hasTransparentPixels ? transparentIndex : 0);
    EGifPutExtension (gif, GRAPHICS_EXT_FUNC_CODE, 4, gce);

    // Write image descriptor + raster
    EGifPutImageDesc (gif, 0, 0, w, h, false, localMap);
    GifFreeMapObject (localMap);

    for (int row = 0; row < h; ++row)
    {
        const GifByteType* rowPtr = indices.data() + static_cast<size_t> (row * w);
        if (EGifPutLine (gif, const_cast<GifByteType*> (rowPtr), w) != GIF_OK)
            return false;
    }

    return true;
}

//==============================================================================
// GifImageFormat
//==============================================================================

GifImageFormat::GifImageFormat()
    : formatName ("GIF Image")
{
}

const String& GifImageFormat::getFormatName() const
{
    return formatName;
}

Array<String> GifImageFormat::getFileExtensions (Mode /*mode*/) const
{
    return { ".gif" };
}

bool GifImageFormat::canHandleStream (InputStream& stream, Mode /*mode*/) const
{
    uint8_t sig[6] = {};
    stream.read (sig, 6);
    stream.setPosition (0);
    return (std::memcmp (sig, "GIF87a", 6) == 0 || std::memcmp (sig, "GIF89a", 6) == 0);
}

std::unique_ptr<ImageFormatReader> GifImageFormat::createReaderFor (InputStream* sourceStream)
{
    return std::make_unique<GifImageFormatReader> (sourceStream);
}

std::unique_ptr<ImageFormatWriter> GifImageFormat::createWriterFor (OutputStream* destStream,
                                                                    PixelFormat pixelFormat,
                                                                    const StringPairArray& /*metadataValues*/,
                                                                    int /*qualityOptionIndex*/)
{
    return std::make_unique<GifImageFormatWriter> (destStream, pixelFormat);
}

Array<PixelFormat> GifImageFormat::getPossiblePixelFormats() const
{
    return { PixelFormat::RGBA };
}

} // namespace yup

#endif // YUP_IMAGE_FORMAT_GIF
