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

#if YUP_IMAGE_FORMAT_WEBP

namespace yup
{

//==============================================================================
// WebPImageFormatReader
//==============================================================================

WebPImageFormatReader::WebPImageFormatReader (InputStream* stream, const ImageFormat::Options& options)
    : ImageFormatReader (stream, "WebP Image", options)
{
    if (options.parseMetadata || options.parseRawChunks)
        metadata = ImageMetadata::create();

    uint8 chunk[4096];
    long bytesRead;

    while ((bytesRead = input->read (chunk, sizeof (chunk))) > 0)
        fileData.insert (fileData.end(), chunk, chunk + bytesRead);

    if (fileData.empty())
        return;

    // Try to parse as a demux object (handles both static and animated WebP)
    WebPData webpData;
    webpData.bytes = fileData.data();
    webpData.size = fileData.size();

    WebPDemuxer* demux = WebPDemux (&webpData);
    if (demux != nullptr)
    {
        width = static_cast<int> (WebPDemuxGetI (demux, WEBP_FF_CANVAS_WIDTH));
        height = static_cast<int> (WebPDemuxGetI (demux, WEBP_FF_CANVAS_HEIGHT));
        const uint32_t frameCount = WebPDemuxGetI (demux, WEBP_FF_FRAME_COUNT);
        loopCount = static_cast<int> (WebPDemuxGetI (demux, WEBP_FF_LOOP_COUNT));

        // Extract metadata chunks
        if (getOptions().parseRawChunks)
        {
            auto extractChunk = [&] (const char* fourCC, const String& key)
            {
                WebPChunkIterator chunkIter;
                if (WebPDemuxGetChunk (demux, fourCC, 1, &chunkIter))
                {
                    metadata->rawChunks[key] = MemoryBlock (chunkIter.chunk.bytes, chunkIter.chunk.size);
                    WebPDemuxReleaseChunkIterator (&chunkIter);
                }
            };

            extractChunk ("EXIF", "webp/EXIF");
            extractChunk ("ICCP", "webp/ICCP");
            extractChunk ("XMP ", "webp/XMP");
        }

        if (frameCount > 1)
        {
            // Animated: always RGBA for compositing
            pixelFormat = PixelFormat::RGBA;

            frames.reserve (frameCount);
            WebPIterator iter;

            if (WebPDemuxGetFrame (demux, 1, &iter))
            {
                do
                {
                    FrameInfo info;
                    info.xOffset = iter.x_offset;
                    info.yOffset = iter.y_offset;
                    info.frameWidth = iter.width;
                    info.frameHeight = iter.height;
                    info.durationMs = iter.duration;
                    info.disposeMethod = static_cast<int> (iter.dispose_method);
                    info.blendMethod = static_cast<int> (iter.blend_method);
                    info.fragmentData.assign (iter.fragment.bytes,
                                              iter.fragment.bytes + iter.fragment.size);
                    frames.push_back (std::move (info));
                } while (WebPDemuxNextFrame (&iter));

                WebPDemuxReleaseIterator (&iter);
            }
        }
        else
        {
            // Static image: use WebPGetFeatures for correct pixel format
            WebPBitstreamFeatures features = {};

            if (WebPGetFeatures (fileData.data(), fileData.size(), &features) == VP8_STATUS_OK)
                pixelFormat = features.has_alpha ? PixelFormat::RGBA : PixelFormat::RGB;
            else
                pixelFormat = PixelFormat::RGBA;
        }

        WebPDemuxDelete (demux);
    }
    else
    {
        // Fallback to static image detection via WebPGetFeatures
        WebPBitstreamFeatures features = {};

        if (WebPGetFeatures (fileData.data(), fileData.size(), &features) == VP8_STATUS_OK)
        {
            width = features.width;
            height = features.height;
            pixelFormat = features.has_alpha ? PixelFormat::RGBA : PixelFormat::RGB;
        }
    }

    // Initialize compositing canvas
    if (width > 0 && height > 0)
    {
        canvas = Image (width, height, PixelFormat::RGBA);
        canvas.fill (0x00000000u);
    }
}

Image WebPImageFormatReader::readImage()
{
    if (! frames.empty())
    {
        Image dest;
        readFrame (0, dest);
        return dest;
    }

    // Static image path
    if (width <= 0 || height <= 0 || fileData.empty())
        return {};

    const auto* data = fileData.data();
    const auto dataSize = fileData.size();

    int w = 0, h = 0;

    if (pixelFormat == PixelFormat::RGBA)
    {
        auto* decoded = WebPDecodeRGBA (data, dataSize, &w, &h);
        if (decoded == nullptr)
            return {};

        Image image (w, h, PixelFormat::RGBA);

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const uint8_t* p = decoded + (y * w + x) * 4;
                image.setPixel (x, y, (uint32 (p[3]) << 24) | (uint32 (p[0]) << 16) | (uint32 (p[1]) << 8) | uint32 (p[2]));
            }
        }

        WebPFree (decoded);
        return image;
    }

    // RGB
    auto* decoded = WebPDecodeRGB (data, dataSize, &w, &h);
    if (decoded == nullptr)
        return {};

    Image image (w, h, PixelFormat::RGB);

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const uint8_t* p = decoded + (y * w + x) * 3;
            image.setPixel (x, y, 0xFF000000u | (uint32 (p[0]) << 16) | (uint32 (p[1]) << 8) | uint32 (p[2]));
        }
    }

    WebPFree (decoded);
    return image;
}

Image WebPImageFormatReader::readFrame (int frameIndex)
{
    Image dest;
    readFrame (frameIndex, dest);
    return dest;
}

bool WebPImageFormatReader::readFrame (int frameIndex, Image& dest)
{
    if (frameIndex < 0 || static_cast<size_t> (frameIndex) >= frames.size())
        return false;

    if (width <= 0 || height <= 0)
        return false;

    // Ensure canvas is valid and correct size
    if (! canvas.isValid() || canvas.getWidth() != width || canvas.getHeight() != height)
    {
        canvas = Image (width, height, PixelFormat::RGBA);
        canvas.fill (0x00000000u);
        lastRenderedFrame = -1;
    }

    // Seek strategy: backward or non-sequential → reset and composite from 0
    if (frameIndex < lastRenderedFrame || frameIndex > lastRenderedFrame + 1)
        resetCanvas();

    const int startFrame = lastRenderedFrame + 1;

    for (int fi = startFrame; fi <= frameIndex; ++fi)
        compositeFrame (fi);

    // Copy composited canvas to dest (reuse allocation when possible)
    const bool compatible = dest.isValid()
                         && dest.getWidth() == width
                         && dest.getHeight() == height
                         && dest.getPixelFormat() == PixelFormat::RGBA;

    if (! compatible)
        dest = Image (width, height, PixelFormat::RGBA);

    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            dest.setPixel (x, y, canvas.getPixel (x, y));

    return true;
}

void WebPImageFormatReader::compositeFrame (int frameIndex)
{
    const auto& frame = frames[static_cast<size_t> (frameIndex)];

    // Apply previous frame's disposal method
    if (frameIndex > 0)
    {
        const auto& prevFrame = frames[static_cast<size_t> (frameIndex - 1)];

        if (prevFrame.disposeMethod == WEBP_MUX_DISPOSE_BACKGROUND)
        {
            // Clear previous frame's rectangle to transparent
            for (int y = prevFrame.yOffset; y < prevFrame.yOffset + prevFrame.frameHeight && y < height; ++y)
                for (int x = prevFrame.xOffset; x < prevFrame.xOffset + prevFrame.frameWidth && x < width; ++x)
                    canvas.setPixel (x, y, 0x00000000u);
        }
        // WEBP_MUX_DISPOSE_NONE: leave pixels as-is
    }

    // Decode the current frame fragment
    int w = 0, h = 0;
    auto* decoded = WebPDecodeRGBA (frame.fragmentData.data(), frame.fragmentData.size(), &w, &h);

    if (decoded == nullptr)
        return;

    // Apply to canvas based on blend method
    for (int row = 0; row < frame.frameHeight; ++row)
    {
        const int canvasY = frame.yOffset + row;
        if (canvasY < 0 || canvasY >= height)
            continue;

        for (int col = 0; col < frame.frameWidth; ++col)
        {
            const int canvasX = frame.xOffset + col;
            if (canvasX < 0 || canvasX >= width)
                continue;

            const uint8_t* p = decoded + (row * frame.frameWidth + col) * 4;
            const uint8_t srcA = p[3];
            const uint32_t srcPixel = (uint32 (srcA) << 24) | (uint32 (p[0]) << 16) | (uint32 (p[1]) << 8) | uint32 (p[2]);

            if (frame.blendMethod == WEBP_MUX_NO_BLEND || srcA == 255)
            {
                // Replace
                canvas.setPixel (canvasX, canvasY, srcPixel);
            }
            else if (srcA == 0)
            {
                // Fully transparent — leave canvas unchanged
                continue;
            }
            else
            {
                // Alpha blend (over operator)
                const uint32_t dstPixel = canvas.getPixel (canvasX, canvasY);
                const uint32_t dstA = (dstPixel >> 24) & 0xFF;
                const uint32_t dstR = (dstPixel >> 16) & 0xFF;
                const uint32_t dstG = (dstPixel >> 8) & 0xFF;
                const uint32_t dstB = dstPixel & 0xFF;

                const uint32_t srcR = (srcPixel >> 16) & 0xFF;
                const uint32_t srcG = (srcPixel >> 8) & 0xFF;
                const uint32_t srcB = srcPixel & 0xFF;

                const uint32_t outA = srcA + ((dstA * (255 - srcA)) / 255);
                const uint32_t outR = (srcR * srcA + dstR * dstA * (255 - srcA) / 255) / outA;
                const uint32_t outG = (srcG * srcA + dstG * dstA * (255 - srcA) / 255) / outA;
                const uint32_t outB = (srcB * srcA + dstB * dstA * (255 - srcA) / 255) / outA;

                canvas.setPixel (canvasX, canvasY, (outA << 24) | (outR << 16) | (outG << 8) | outB);
            }
        }
    }

    WebPFree (decoded);
    lastRenderedFrame = frameIndex;
}

void WebPImageFormatReader::resetCanvas()
{
    canvas.fill (0x00000000u);
    lastRenderedFrame = -1;
}

bool WebPImageFormatReader::isAnimated() const
{
    return frames.size() > 1;
}

int WebPImageFormatReader::getFrameCount() const
{
    return static_cast<int> (frames.size());
}

int WebPImageFormatReader::getLoopCount() const
{
    return loopCount;
}

int WebPImageFormatReader::getFrameDelayMs (int frameIndex) const
{
    if (frameIndex < 0 || static_cast<size_t> (frameIndex) >= frames.size())
        return 0;
    return frames[static_cast<size_t> (frameIndex)].durationMs;
}

//==============================================================================
// WebPImageFormatWriter
//==============================================================================

WebPImageFormatWriter::WebPImageFormatWriter (OutputStream* stream, PixelFormat fmt, int qualityIndex_)
    : ImageFormatWriter (stream, "WebP Image", fmt)
    , qualityIndex (qualityIndex_)
{
}

WebPImageFormatWriter::~WebPImageFormatWriter()
{
    if (animEncoder != nullptr)
        endAnimation();
}

void WebPImageFormatWriter::WebPAnimEncoderDeleter::operator() (::WebPAnimEncoder* p) const noexcept
{
    if (p != nullptr)
        WebPAnimEncoderDelete (p);
}

bool WebPImageFormatWriter::writeImage (const Image& image)
{
    if (output == nullptr || ! image.isValid())
        return false;

    const int w = image.getWidth();
    const int h = image.getHeight();

    if (w <= 0 || h <= 0)
        return false;

    const bool isRGBA = (getPixelFormat() == PixelFormat::RGBA
                         || image.getPixelFormat() == PixelFormat::RGBA);
    const int channels = isRGBA ? 4 : 3;
    const int stride = w * channels;

    std::vector<uint8_t> pixelBuffer (static_cast<size_t> (h * stride));

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            auto argb = image.getPixel (x, y);
            auto* dst = pixelBuffer.data() + (y * w + x) * channels;

            dst[0] = static_cast<uint8_t> ((argb >> 16) & 0xFF); // R
            dst[1] = static_cast<uint8_t> ((argb >> 8) & 0xFF);  // G
            dst[2] = static_cast<uint8_t> ((argb >> 0) & 0xFF);  // B

            if (isRGBA)
                dst[3] = static_cast<uint8_t> ((argb >> 24) & 0xFF); // A
        }
    }

    uint8_t* encoded = nullptr;
    size_t encodedSize = 0;

    if (qualityIndex == 0) // lossless
    {
        if (isRGBA)
            encodedSize = WebPEncodeLosslessRGBA (pixelBuffer.data(), w, h, stride, &encoded);
        else
            encodedSize = WebPEncodeLosslessRGB (pixelBuffer.data(), w, h, stride, &encoded);
    }
    else
    {
        static const float qualityValues[] = { 90.0f, 80.0f, 60.0f, 40.0f };
        const int qi = std::min (qualityIndex - 1, 3);
        const float quality = qualityValues[qi];

        if (isRGBA)
            encodedSize = WebPEncodeRGBA (pixelBuffer.data(), w, h, stride, quality, &encoded);
        else
            encodedSize = WebPEncodeRGB (pixelBuffer.data(), w, h, stride, quality, &encoded);
    }

    if (encoded == nullptr || encodedSize == 0)
        return false;

    output->write (encoded, encodedSize);
    WebPFree (encoded);
    return true;
}

bool WebPImageFormatWriter::beginAnimation (int loopCount)
{
    if (output == nullptr)
        return false;

    animEncoder.reset();
    animWidth = 0;
    animHeight = 0;
    animTimestampMs = 0;
    animLoopCount = loopCount;
    return true;
}

bool WebPImageFormatWriter::writeFrame (const Image& frame, int delayMs)
{
    if (output == nullptr || ! frame.isValid())
        return false;

    const int w = frame.getWidth();
    const int h = frame.getHeight();

    if (w <= 0 || h <= 0)
        return false;

    // Lazily create the encoder on the first frame
    if (animEncoder == nullptr)
    {
        WebPAnimEncoderOptions encOptions;
        WebPAnimEncoderOptionsInit (&encOptions);
        encOptions.anim_params.loop_count = animLoopCount;
        encOptions.anim_params.bgcolor = 0x00000000u; // transparent black

        animEncoder.reset (WebPAnimEncoderNew (w, h, &encOptions));

        if (animEncoder == nullptr)
            return false;

        animWidth = w;
        animHeight = h;
    }

    // Convert Image to RGBA buffer
    const int stride = w * 4;
    std::vector<uint8_t> rgba (static_cast<size_t> (h * stride));

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const auto argb = frame.getPixel (x, y);
            auto* dst = rgba.data() + (y * w + x) * 4;
            dst[0] = static_cast<uint8_t> ((argb >> 16) & 0xFF); // R
            dst[1] = static_cast<uint8_t> ((argb >> 8) & 0xFF);  // G
            dst[2] = static_cast<uint8_t> ((argb >> 0) & 0xFF);  // B
            dst[3] = static_cast<uint8_t> ((argb >> 24) & 0xFF); // A
        }
    }

    // Create WebPPicture and import RGBA data
    WebPPicture pic;
    WebPPictureInit (&pic);
    pic.width = w;
    pic.height = h;
    pic.use_argb = 1;

    if (! WebPPictureImportRGBA (&pic, rgba.data(), stride))
    {
        WebPPictureFree (&pic);
        return false;
    }

    // Configure encoding
    WebPConfig config;
    WebPConfigInit (&config);

    if (qualityIndex == 0)
        config.lossless = 1;
    else
    {
        static const float qualityValues[] = { 90.0f, 80.0f, 60.0f, 40.0f };
        const int qi = std::min (qualityIndex - 1, 3);
        config.quality = qualityValues[qi];
    }

    const bool ok = WebPAnimEncoderAdd (animEncoder.get(), &pic, animTimestampMs, &config) != 0;
    WebPPictureFree (&pic);

    if (! ok)
        return false;

    animTimestampMs += delayMs;
    return true;
}

bool WebPImageFormatWriter::endAnimation()
{
    if (animEncoder == nullptr)
        return false;

    // Finalise with a NULL frame to signal end of animation
    WebPAnimEncoderAdd (animEncoder.get(), nullptr, animTimestampMs, nullptr);

    WebPData assembledData;
    WebPDataInit (&assembledData);

    if (! WebPAnimEncoderAssemble (animEncoder.get(), &assembledData))
    {
        animEncoder.reset();
        return false;
    }

    output->write (assembledData.bytes, assembledData.size);
    WebPDataClear (&assembledData);
    animEncoder.reset();
    return true;
}

//==============================================================================
// WebPImageFormat
//==============================================================================

WebPImageFormat::WebPImageFormat()
    : formatName ("WebP Image")
{
}

const String& WebPImageFormat::getFormatName() const
{
    return formatName;
}

StringArray WebPImageFormat::getFileExtensions (Mode /*mode*/) const
{
    return { ".webp" };
}

bool WebPImageFormat::canHandleStream (InputStream& stream, Mode /*mode*/) const
{
    uint8 sig[12] = {};
    stream.read (sig, 12);
    stream.setPosition (0);
    return sig[0] == 'R' && sig[1] == 'I' && sig[2] == 'F' && sig[3] == 'F'
        && sig[8] == 'W' && sig[9] == 'E' && sig[10] == 'B' && sig[11] == 'P';
}

StringArray WebPImageFormat::getQualityOptions() const
{
    return { "Lossless", "Quality 90", "Quality 80", "Quality 60", "Quality 40" };
}

std::unique_ptr<ImageFormatReader> WebPImageFormat::createReaderFor (InputStream* sourceStream, const ImageFormat::Options& options)
{
    return std::make_unique<WebPImageFormatReader> (sourceStream, options);
}

std::unique_ptr<ImageFormatWriter> WebPImageFormat::createWriterFor (OutputStream* destStream,
                                                                     PixelFormat pixelFormat,
                                                                     const StringPairArray& /*metadataValues*/,
                                                                     int qualityOptionIndex)
{
    return std::make_unique<WebPImageFormatWriter> (destStream, pixelFormat, qualityOptionIndex);
}

Array<PixelFormat> WebPImageFormat::getPossiblePixelFormats() const
{
    return { PixelFormat::RGB, PixelFormat::RGBA };
}

} // namespace yup

#endif // YUP_IMAGE_FORMAT_WEBP
