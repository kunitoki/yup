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

ImageFormatManager::ImageFormatManager()
{
}

void ImageFormatManager::registerDefaultFormats (ImageFormatType types)
{
#if YUP_IMAGE_FORMAT_BMP
    if (hasBitValueSet (types, ImageFormatType::bmp))
        registerFormat (std::make_unique<BmpImageFormat>());
#endif

#if YUP_IMAGE_FORMAT_PPM
    if (hasBitValueSet (types, ImageFormatType::ppm))
        registerFormat (std::make_unique<PpmImageFormat>());
#endif

#if YUP_IMAGE_FORMAT_PNG
    if (hasBitValueSet (types, ImageFormatType::png))
        registerFormat (std::make_unique<PngImageFormat>());
#endif

#if YUP_IMAGE_FORMAT_JPEG
    if (hasBitValueSet (types, ImageFormatType::jpeg))
        registerFormat (std::make_unique<JpegImageFormat>());
#endif

#if YUP_IMAGE_FORMAT_WEBP
    if (hasBitValueSet (types, ImageFormatType::webp))
        registerFormat (std::make_unique<WebPImageFormat>());
#endif

#if YUP_IMAGE_FORMAT_GIF
    if (hasBitValueSet (types, ImageFormatType::gif))
        registerFormat (std::make_unique<GifImageFormat>());
#endif

#if YUP_IMAGE_FORMAT_TGA
    if (hasBitValueSet (types, ImageFormatType::tga))
        registerFormat (std::make_unique<TgaImageFormat>());
#endif

#if YUP_IMAGE_FORMAT_TIFF
    if (hasBitValueSet (types, ImageFormatType::tiff))
        registerFormat (std::make_unique<TiffImageFormat>());
#endif
}

void ImageFormatManager::registerFormat (std::unique_ptr<ImageFormat> format)
{
    if (format != nullptr)
        formats.push_back (std::move (format));
}

std::unique_ptr<ImageFormatReader> ImageFormatManager::createReaderFor (const File& file,
                                                                        const ImageFormat::Options& options)
{
    auto stream = file.createInputStream();
    if (stream == nullptr)
        return nullptr;

    for (auto& format : formats)
    {
        if (! format->canHandleFile (file, ImageFormat::forReading))
            continue;

        stream->setPosition (0);

        if (auto reader = format->createReaderFor (stream.release(), options))
            return reader;
        else
            stream = file.createInputStream();
    }

    return nullptr;
}

std::unique_ptr<ImageFormatReader> ImageFormatManager::createReaderFor (InputStream* stream,
                                                                        const ImageFormat::Options& options)
{
    if (stream == nullptr)
        return nullptr;

    std::unique_ptr<InputStream> ownedStream (stream);

    for (auto& fmt : formats)
    {
        if (fmt->canHandleStream (*ownedStream, ImageFormat::forReading))
            return fmt->createReaderFor (ownedStream.release(), options);
    }

    return nullptr;
}

std::unique_ptr<ImageFormatWriter> ImageFormatManager::createWriterFor (const File& file,
                                                                        PixelFormat pixelFormat,
                                                                        const StringPairArray& metadataValues,
                                                                        int qualityOptionIndex)
{
    auto stream = file.createOutputStream();
    if (stream == nullptr)
        return nullptr;

    for (auto& format : formats)
    {
        if (! format->canHandleFile (file, ImageFormat::forWriting))
            continue;

        if (auto writer = format->createWriterFor (stream.release(),
                                                   pixelFormat,
                                                   metadataValues,
                                                   qualityOptionIndex))
        {
            return writer;
        }
        else
        {
            stream = file.createOutputStream();
        }
    }

    return nullptr;
}

StringArray ImageFormatManager::getFormatFileExtensions() const
{
    StringArray result;

    for (const auto& format : formats)
    {
        for (auto mode : { ImageFormat::forReading, ImageFormat::forWriting })
        {
            for (const auto& ext : format->getFileExtensions (mode))
                result.addIfNotAlreadyThere (ext);
        }
    }

    return result;
}

} // namespace yup
