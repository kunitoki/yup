/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

AudioFormatManager::AudioFormatManager()
{
}

void AudioFormatManager::registerDefaultFormats()
{
    // Register Wave format
    registerFormat (std::make_unique<WaveAudioFormat>());

    // TODO: Add other formats like:
    // registerFormat (std::make_unique<AiffAudioFormat>());
    // registerFormat (std::make_unique<FlacAudioFormat>());
    // registerFormat (std::make_unique<OggVorbisAudioFormat>());
    // registerFormat (std::make_unique<MP3AudioFormat>());
}

void AudioFormatManager::registerFormat (std::unique_ptr<AudioFormat> format)
{
    if (format != nullptr)
        formats.push_back (std::move (format));
}

std::unique_ptr<AudioFormatReader> AudioFormatManager::createReaderFor (const File& file)
{
    // Try to open the file
    auto stream = file.createInputStream();

    if (stream == nullptr)
        return nullptr;

    // Try each format
    for (auto& format : formats)
    {
        if (format->canHandleFile (file))
        {
            stream->setPosition (0);

            if (auto reader = format->createReaderFor (stream.release()))
                return reader;
        }
    }

    return nullptr;
}

std::unique_ptr<AudioFormatWriter> AudioFormatManager::createWriterFor (const File& file,
                                                                        int sampleRate,
                                                                        int numChannels,
                                                                        int bitsPerSample)
{
    // Try to create the output file
    auto stream = file.createOutputStream();

    if (stream == nullptr)
        return nullptr;

    // Try each format
    for (auto& format : formats)
    {
        if (format->canHandleFile (file))
        {
            StringPairArray metadataValues;

            if (auto writer = format->createWriterFor (stream.release(),
                                                        sampleRate,
                                                        numChannels,
                                                        bitsPerSample,
                                                        metadataValues,
                                                        0))
                return writer;
        }
    }

    return nullptr;
}

} // namespace yup