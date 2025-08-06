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

//==============================================================================
// WaveAudioFormat implementation
WaveAudioFormat::WaveAudioFormat()
    : formatName ("Wave file")
{
}

WaveAudioFormat::~WaveAudioFormat() = default;

const String& WaveAudioFormat::getFormatName() const
{
    return formatName;
}

Array<String> WaveAudioFormat::getFileExtensions() const
{
    return { ".wav", ".wave", ".bwf" };
}

std::unique_ptr<AudioFormatReader> WaveAudioFormat::createReaderFor (InputStream* sourceStream)
{
    auto reader = std::make_unique<WaveAudioFormatReader> (sourceStream);

    if (reader->sampleRate > 0 && reader->numChannels > 0)
        return reader;

    return nullptr;
}

std::unique_ptr<AudioFormatWriter> WaveAudioFormat::createWriterFor (OutputStream* streamToWriteTo,
                                                                     double sampleRate,
                                                                     unsigned int numberOfChannels,
                                                                     int bitsPerSample,
                                                                     const StringPairArray& metadataValues,
                                                                     int qualityOptionIndex)
{
    if (streamToWriteTo == nullptr)
        return nullptr;

    // Check supported configurations
    if (numberOfChannels == 0 || numberOfChannels > 64)
        return nullptr;

    if (sampleRate <= 0 || sampleRate > 192000)
        return nullptr;

    if (bitsPerSample != 8 && bitsPerSample != 16 && bitsPerSample != 24 && bitsPerSample != 32)
        return nullptr;

    return std::make_unique<WaveAudioFormatWriter> (streamToWriteTo, sampleRate,
                                                    numberOfChannels, bitsPerSample,
                                                    metadataValues);
}

Array<int> WaveAudioFormat::getPossibleBitDepths() const
{
    return { 8, 16, 24, 32 };
}

Array<int> WaveAudioFormat::getPossibleSampleRates() const
{
    return { 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 88200, 96000, 176400, 192000 };
}

//==============================================================================
// WaveAudioFormatReader implementation
struct WaveAudioFormatReader::Impl
{
    drwav wav = {};
    HeapBlock<uint8> tempBuffer;
    size_t tempBufferSize = 0;
    bool isOpen = false;

    static size_t readCallback (void* pUserData, void* pBufferOut, size_t bytesToRead)
    {
        auto* stream = static_cast<InputStream*> (pUserData);
        return (size_t) stream->read (pBufferOut, (int) bytesToRead);
    }

    static drwav_bool32 seekCallback (void* pUserData, int offset, drwav_seek_origin origin)
    {
        auto* stream = static_cast<InputStream*> (pUserData);

        if (origin == DRWAV_SEEK_SET)
            return stream->setPosition (offset) ? DRWAV_TRUE : DRWAV_FALSE;
        else if (origin == DRWAV_SEEK_CUR)
            return stream->setPosition (stream->getPosition() + offset) ? DRWAV_TRUE : DRWAV_FALSE;

        return DRWAV_FALSE;
    }
};

WaveAudioFormatReader::WaveAudioFormatReader (InputStream* sourceStream)
    : AudioFormatReader (sourceStream, "Wave file")
    , impl (std::make_unique<Impl>())
{
    if (sourceStream == nullptr)
        return;

    impl->isOpen = drwav_init_with_metadata (&impl->wav,
                                             Impl::readCallback,
                                             Impl::seekCallback,
                                             nullptr,
                                             sourceStream,
                                             DRWAV_WITH_METADATA,
                                             nullptr) == DRWAV_TRUE;

    if (impl->isOpen)
    {
        sampleRate = impl->wav.sampleRate;
        bitsPerSample = impl->wav.bitsPerSample;
        lengthInSamples = (int64) impl->wav.totalPCMFrameCount;
        numChannels = impl->wav.channels;
        usesFloatingPointData = impl->wav.translatedFormatTag == DR_WAVE_FORMAT_IEEE_FLOAT;

        // Extract metadata
        for (drwav_uint32 i = 0; i < impl->wav.metadataCount; ++i)
        {
            auto& metadata = impl->wav.pMetadata[i];

            if (metadata.type == drwav_metadata_type_list_info_title && metadata.data.infoText.pString)
                metadataValues.set ("title", String::fromUTF8 (metadata.data.infoText.pString));
            else if (metadata.type == drwav_metadata_type_list_info_artist && metadata.data.infoText.pString)
                metadataValues.set ("artist", String::fromUTF8 (metadata.data.infoText.pString));
            else if (metadata.type == drwav_metadata_type_list_info_album && metadata.data.infoText.pString)
                metadataValues.set ("album", String::fromUTF8 (metadata.data.infoText.pString));
            else if (metadata.type == drwav_metadata_type_list_info_date && metadata.data.infoText.pString)
                metadataValues.set ("year", String::fromUTF8 (metadata.data.infoText.pString));
            else if (metadata.type == drwav_metadata_type_list_info_genre && metadata.data.infoText.pString)
                metadataValues.set ("genre", String::fromUTF8 (metadata.data.infoText.pString));
            else if (metadata.type == drwav_metadata_type_list_info_comment && metadata.data.infoText.pString)
                metadataValues.set ("comment", String::fromUTF8 (metadata.data.infoText.pString));
            else if (metadata.type == drwav_metadata_type_list_info_tracknumber && metadata.data.infoText.pString)
                metadataValues.set ("tracknumber", String::fromUTF8 (metadata.data.infoText.pString));
        }

        // Allocate temp buffer for reading
        const auto bytesPerFrame = numChannels * (bitsPerSample / 8);
        impl->tempBufferSize = bytesPerFrame * 4096;
        impl->tempBuffer.allocate (impl->tempBufferSize, true);
    }
}

WaveAudioFormatReader::~WaveAudioFormatReader()
{
    if (impl->isOpen)
        drwav_uninit (&impl->wav);
}

bool WaveAudioFormatReader::readSamples (int* const* destChannels,
                                          int numDestChannels,
                                          int startOffsetInDestBuffer,
                                          int64 startSampleInFile,
                                          int numSamples)
{
    if (! impl->isOpen)
        return false;

    if (numSamples <= 0)
        return true;

    // Seek to the start position
    if (! drwav_seek_to_pcm_frame (&impl->wav, (drwav_uint64) startSampleInFile))
        return false;

    const auto numChannelsToRead = jmin (numDestChannels, (int) numChannels);
    const auto bytesPerSample = bitsPerSample / 8;
    const auto bytesPerFrame = numChannels * bytesPerSample;

    // Read the data
    const auto framesToRead = (drwav_uint64) numSamples;
    const auto bytesToRead = framesToRead * bytesPerFrame;

    if (bytesToRead > impl->tempBufferSize)
    {
        impl->tempBufferSize = bytesToRead;
        impl->tempBuffer.allocate (bytesToRead, false);
    }

    const auto framesRead = drwav_read_pcm_frames (&impl->wav, framesToRead, impl->tempBuffer.getData());

    if (framesRead == 0)
        return false;

    // Deinterleave and convert to int
    if (bitsPerSample == 8)
    {
        const auto* src = impl->tempBuffer.getData();

        for (int sample = 0; sample < (int) framesRead; ++sample)
        {
            for (int ch = 0; ch < numChannelsToRead; ++ch)
            {
                const auto value = (int) (src[sample * numChannels + ch] - 128) << 24;
                destChannels[ch][startOffsetInDestBuffer + sample] = value;
            }
        }
    }
    else if (bitsPerSample == 16)
    {
        const auto* src = reinterpret_cast<const drwav_int16*> (impl->tempBuffer.getData());

        for (int sample = 0; sample < (int) framesRead; ++sample)
        {
            for (int ch = 0; ch < numChannelsToRead; ++ch)
            {
                destChannels[ch][startOffsetInDestBuffer + sample] = ((int) src[sample * numChannels + ch]) << 16;
            }
        }
    }
    else if (bitsPerSample == 24)
    {
        const auto* src = impl->tempBuffer.getData();

        for (int sample = 0; sample < (int) framesRead; ++sample)
        {
            for (int ch = 0; ch < numChannelsToRead; ++ch)
            {
                const auto* samplePtr = src + (sample * numChannels + ch) * 3;
                const int value = (((int) samplePtr[2]) << 24) |
                                  (((int) samplePtr[1]) << 16) |
                                  (((int) samplePtr[0]) << 8);
                destChannels[ch][startOffsetInDestBuffer + sample] = value;
            }
        }
    }
    else if (bitsPerSample == 32)
    {
        if (usesFloatingPointData)
        {
            const auto* src = reinterpret_cast<const float*> (impl->tempBuffer.getData());

            for (int sample = 0; sample < (int) framesRead; ++sample)
            {
                for (int ch = 0; ch < numChannelsToRead; ++ch)
                {
                    const auto value = jlimit (-1.0f, 1.0f, src[sample * numChannels + ch]);
                    destChannels[ch][startOffsetInDestBuffer + sample] = (int) (value * 0x7fffffff);
                }
            }
        }
        else
        {
            const auto* src = reinterpret_cast<const drwav_int32*> (impl->tempBuffer.getData());

            for (int sample = 0; sample < (int) framesRead; ++sample)
            {
                for (int ch = 0; ch < numChannelsToRead; ++ch)
                {
                    destChannels[ch][startOffsetInDestBuffer + sample] = src[sample * numChannels + ch];
                }
            }
        }
    }

    return true;
}

//==============================================================================
// WaveAudioFormatWriter implementation
struct WaveAudioFormatWriter::Impl
{
    drwav wav = {};
    HeapBlock<uint8> tempBuffer;
    size_t tempBufferSize = 0;
    bool isOpen = false;
    int64 samplesWritten = 0;

    static size_t writeCallback (void* pUserData, const void* pData, size_t bytesToWrite)
    {
        auto* stream = static_cast<OutputStream*> (pUserData);
        return stream->write (pData, bytesToWrite) ? bytesToWrite : 0;
    }

    static drwav_bool32 seekCallback (void* pUserData, int offset, drwav_seek_origin origin)
    {
        auto* stream = static_cast<OutputStream*> (pUserData);

        if (origin == DRWAV_SEEK_SET)
            return stream->setPosition (offset) ? DRWAV_TRUE : DRWAV_FALSE;
        else if (origin == DRWAV_SEEK_CUR)
            return stream->setPosition (stream->getPosition() + offset) ? DRWAV_TRUE : DRWAV_FALSE;

        return DRWAV_FALSE;
    }
};

WaveAudioFormatWriter::WaveAudioFormatWriter (OutputStream* destStream,
                                              double sampleRate,
                                              unsigned int numberOfChannels,
                                              unsigned int bitsPerSample,
                                              const StringPairArray& metadataValues)
    : AudioFormatWriter (destStream, "Wave file", sampleRate, numberOfChannels, bitsPerSample)
    , impl (std::make_unique<Impl>())
{
    drwav_data_format format = {};
    format.container = drwav_container_riff;
    format.format = (bitsPerSample == 32) ? DR_WAVE_FORMAT_IEEE_FLOAT : DR_WAVE_FORMAT_PCM;
    format.channels = (drwav_uint32) numberOfChannels;
    format.sampleRate = (drwav_uint32) sampleRate;
    format.bitsPerSample = (drwav_uint32) bitsPerSample;

    // Prepare metadata
    std::vector<drwav_metadata> metadata;

    auto addStringMetadata = [&] (const String& key, drwav_metadata_type type)
    {
        if (metadataValues.containsKey (key))
        {
            auto value = metadataValues.getValue (key, "");
            if (value.isNotEmpty())
            {
                drwav_metadata meta = {};
                meta.type = type;
                meta.data.infoText.stringLength = (drwav_uint32) value.length();
                meta.data.infoText.pString = const_cast<char*> (value.toRawUTF8());
                metadata.push_back (meta);
            }
        }
    };

    addStringMetadata ("title", drwav_metadata_type_list_info_title);
    addStringMetadata ("artist", drwav_metadata_type_list_info_artist);
    addStringMetadata ("album", drwav_metadata_type_list_info_album);
    addStringMetadata ("year", drwav_metadata_type_list_info_date);
    addStringMetadata ("genre", drwav_metadata_type_list_info_genre);
    addStringMetadata ("comment", drwav_metadata_type_list_info_comment);
    addStringMetadata ("tracknumber", drwav_metadata_type_list_info_tracknumber);

    impl->isOpen = drwav_init_write_with_metadata (&impl->wav,
                                                    &format,
                                                    Impl::writeCallback,
                                                    Impl::seekCallback,
                                                    destStream,
                                                    nullptr,
                                                    metadata.empty() ? nullptr : metadata.data(),
                                                    (drwav_uint32) metadata.size()) == DRWAV_TRUE;

    if (impl->isOpen)
    {
        // Allocate temp buffer for writing
        const auto bytesPerFrame = numberOfChannels * (bitsPerSample / 8);
        impl->tempBufferSize = bytesPerFrame * 4096;
        impl->tempBuffer.allocate (impl->tempBufferSize, true);
    }
}

WaveAudioFormatWriter::~WaveAudioFormatWriter()
{
    if (impl->isOpen)
        drwav_uninit (&impl->wav);
}

bool WaveAudioFormatWriter::write (const int** samplesToWrite, int numSamples)
{
    if (! impl->isOpen || numSamples <= 0)
        return false;

    const auto bytesPerSample = getBitsPerSample() / 8;
    const auto bytesPerFrame = getNumChannels() * bytesPerSample;
    const auto bytesToWrite = numSamples * bytesPerFrame;

    if (bytesToWrite > impl->tempBufferSize)
    {
        impl->tempBufferSize = bytesToWrite;
        impl->tempBuffer.allocate (bytesToWrite, false);
    }

    // Interleave the samples
    if (getBitsPerSample() == 8)
    {
        auto* dest = impl->tempBuffer.getData();

        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (unsigned int ch = 0; ch < getNumChannels(); ++ch)
            {
                const auto value = (samplesToWrite[ch][sample] >> 24) + 128;
                dest[sample * getNumChannels() + ch] = (drwav_uint8) jlimit (0, 255, value);
            }
        }
    }
    else if (getBitsPerSample() == 16)
    {
        auto* dest = reinterpret_cast<drwav_int16*> (impl->tempBuffer.getData());

        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (unsigned int ch = 0; ch < getNumChannels(); ++ch)
            {
                dest[sample * getNumChannels() + ch] = (drwav_int16) (samplesToWrite[ch][sample] >> 16);
            }
        }
    }
    else if (getBitsPerSample() == 24)
    {
        auto* dest = impl->tempBuffer.getData();

        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (unsigned int ch = 0; ch < getNumChannels(); ++ch)
            {
                const auto value = samplesToWrite[ch][sample] >> 8;
                auto* samplePtr = dest + (sample * getNumChannels() + ch) * 3;
                samplePtr[0] = (drwav_uint8) value;
                samplePtr[1] = (drwav_uint8) (value >> 8);
                samplePtr[2] = (drwav_uint8) (value >> 16);
            }
        }
    }
    else if (getBitsPerSample() == 32)
    {
        if (isFloatingPoint())
        {
            auto* dest = reinterpret_cast<float*> (impl->tempBuffer.getData());

            for (int sample = 0; sample < numSamples; ++sample)
            {
                for (unsigned int ch = 0; ch < getNumChannels(); ++ch)
                {
                    dest[sample * getNumChannels() + ch] = samplesToWrite[ch][sample] / (float) 0x7fffffff;
                }
            }
        }
        else
        {
            auto* dest = reinterpret_cast<drwav_int32*> (impl->tempBuffer.getData());

            for (int sample = 0; sample < numSamples; ++sample)
            {
                for (unsigned int ch = 0; ch < getNumChannels(); ++ch)
                {
                    dest[sample * getNumChannels() + ch] = samplesToWrite[ch][sample];
                }
            }
        }
    }

    const auto framesWritten = drwav_write_pcm_frames (&impl->wav, (drwav_uint64) numSamples,
                                                        impl->tempBuffer.getData());

    if (framesWritten > 0)
    {
        impl->samplesWritten += framesWritten;
        return true;
    }

    return false;
}

bool WaveAudioFormatWriter::flush()
{
    if (impl->isOpen && output != nullptr)
    {
        output->flush();
        return true;
    }
    return false;
}

} // namespace yup