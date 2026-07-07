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

WebPImageFormatReader::WebPImageFormatReader (InputStream* stream)
    : ImageFormatReader (stream, "WebP Image")
{
    uint8 chunk[4096];
    long bytesRead;

    while ((bytesRead = input->read (chunk, sizeof (chunk))) > 0)
        fileData.insert (fileData.end(), chunk, chunk + bytesRead);

    WebPBitstreamFeatures features = {};
    if (WebPGetFeatures (fileData.data(), fileData.size(), &features) == VP8_STATUS_OK)
    {
        width = features.width;
        height = features.height;
        pixelFormat = features.has_alpha ? PixelFormat::RGBA : PixelFormat::RGB;
    }
}

Image WebPImageFormatReader::readImage()
{
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
    else // RGB
    {
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
}

//==============================================================================
// WebPImageFormatWriter
//==============================================================================

WebPImageFormatWriter::WebPImageFormatWriter (OutputStream* stream, PixelFormat fmt, int qualityIndex_)
    : ImageFormatWriter (stream, "WebP Image", fmt)
    , qualityIndex (qualityIndex_)
{
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

Array<String> WebPImageFormat::getFileExtensions (Mode /*mode*/) const
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

std::unique_ptr<ImageFormatReader> WebPImageFormat::createReaderFor (InputStream* sourceStream)
{
    return std::make_unique<WebPImageFormatReader> (sourceStream);
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
