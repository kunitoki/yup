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

AudioFormatManager::AudioFormatManager() = default;

void AudioFormatManager::registerDefaultFormats()
{
    registerFormat (std::make_unique<WAVAudioFormat>());
}

void AudioFormatManager::registerFormat (std::unique_ptr<AudioFormat> format)
{
    formats.push_back (std::move (format));
}

std::unique_ptr<AudioFormatReader> AudioFormatManager::createReaderFor (const File& file)
{
    if (! file.existsAsFile())
        return {};

    for (auto& format : formats)
    {
        if (format->canHandleFile (file))
        {
            auto stream = std::make_unique<FileInputStream> (file);
            if (stream->openedOk())
            {
                if (auto reader = format->createReaderFor (stream.get()))
                {
                    // Reader takes ownership of the stream, so release it
                    stream.release();
                    return reader;
                }
            }
        }
    }

    return {};
}

std::unique_ptr<AudioFormatWriter> AudioFormatManager::createWriterFor (const File& file,
                                                                        int sampleRate,
                                                                        int numChannels,
                                                                        int bitsPerSample)
{
    for (auto& format : formats)
    {
        if (format->canHandleFile (file))
        {
            auto stream = std::make_unique<FileOutputStream> (file);
            if (stream->openedOk())
            {
                if (auto writer = format->createWriterFor (stream.get(), sampleRate, numChannels, bitsPerSample))
                {
                    // Writer takes ownership of the stream, so release it
                    stream.release();
                    return writer;
                }
            }
        }
    }

    return {};
}

} // namespace yup
