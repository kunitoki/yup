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

namespace
{

//==============================================================================

class WaveAudioFormatReader : public AudioFormatReader
{
public:
    WaveAudioFormatReader (InputStream* sourceStream);

    ~WaveAudioFormatReader() override;

    bool readSamples (float* const* destChannels,
                      int numDestChannels,
                      int startOffsetInDestBuffer,
                      int64 startSampleInFile,
                      int numSamples) override;

private:
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

    drwav wav = {};
    HeapBlock<uint8> tempBuffer;
    size_t tempBufferSize = 0;
    bool isOpen = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveAudioFormatReader)
};

WaveAudioFormatReader::WaveAudioFormatReader (InputStream* sourceStream)
    : AudioFormatReader (sourceStream, "Wave file")
{
    if (sourceStream == nullptr)
        return;

    isOpen = drwav_init_with_metadata (&wav,
                                       readCallback,
                                       seekCallback,
                                       nullptr,
                                       sourceStream,
                                       DRWAV_WITH_METADATA,
                                       nullptr) == DRWAV_TRUE;

    if (isOpen)
    {
        sampleRate = wav.sampleRate;
        bitsPerSample = wav.bitsPerSample;
        lengthInSamples = (int64) wav.totalPCMFrameCount;
        numChannels = wav.channels;
        usesFloatingPointData = wav.translatedFormatTag == DR_WAVE_FORMAT_IEEE_FLOAT;

        // Extract metadata
        for (drwav_uint32 i = 0; i < wav.metadataCount; ++i)
        {
            auto& metadata = wav.pMetadata[i];

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
        tempBufferSize = bytesPerFrame * 4096;
        tempBuffer.allocate (tempBufferSize, true);
    }
}

WaveAudioFormatReader::~WaveAudioFormatReader()
{
    if (isOpen)
        drwav_uninit (&wav);
}

bool WaveAudioFormatReader::readSamples (float* const* destChannels,
                                         int numDestChannels,
                                         int startOffsetInDestBuffer,
                                         int64 startSampleInFile,
                                         int numSamples)
{
    if (! isOpen)
        return false;

    if (numSamples <= 0)
        return true;

    // Seek to the start position
    if (! drwav_seek_to_pcm_frame (&wav, (drwav_uint64) startSampleInFile))
        return false;

    const auto numChannelsToRead = jmin (numDestChannels, (int) numChannels);
    const auto bytesPerSample = bitsPerSample / 8;
    const auto bytesPerFrame = numChannels * bytesPerSample;

    // Read the data
    const auto framesToRead = (drwav_uint64) numSamples;
    const auto bytesToRead = framesToRead * bytesPerFrame;

    if (bytesToRead > tempBufferSize)
    {
        tempBufferSize = bytesToRead;
        tempBuffer.allocate (bytesToRead, false);
    }

    const auto framesRead = drwav_read_pcm_frames (&wav, framesToRead, tempBuffer.getData());

    if (framesRead == 0)
        return false;

    // Create output channel pointers offset by the start position
    HeapBlock<float*> offsetDestChannels;
    offsetDestChannels.malloc (numDestChannels);
    
    for (int ch = 0; ch < numDestChannels; ++ch)
    {
        offsetDestChannels[ch] = destChannels[ch] + startOffsetInDestBuffer;
    }

    // Use AudioData::deinterleaveSamples to convert and deinterleave in one step
    if (bitsPerSample == 8)
    {
        using SourceFormat = AudioData::Format<AudioData::UInt8, AudioData::LittleEndian>;
        using DestFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
        
        AudioData::deinterleaveSamples (AudioData::InterleavedSource<SourceFormat> { reinterpret_cast<const uint8*> (tempBuffer.getData()), (int) numChannels },
                                        AudioData::NonInterleavedDest<DestFormat> { offsetDestChannels.getData(), numDestChannels },
                                        (int) framesRead);
    }
    else if (bitsPerSample == 16)
    {
        using SourceFormat = AudioData::Format<AudioData::Int16, AudioData::LittleEndian>;
        using DestFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
        
        AudioData::deinterleaveSamples (AudioData::InterleavedSource<SourceFormat> { reinterpret_cast<const uint16*> (tempBuffer.getData()), (int) numChannels },
                                        AudioData::NonInterleavedDest<DestFormat> { offsetDestChannels.getData(), numDestChannels },
                                        (int) framesRead);
    }
    else if (bitsPerSample == 24)
    {
        using SourceFormat = AudioData::Format<AudioData::Int24, AudioData::LittleEndian>;
        using DestFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
        
        AudioData::deinterleaveSamples (AudioData::InterleavedSource<SourceFormat> { reinterpret_cast<const char*> (tempBuffer.getData()), (int) numChannels },
                                        AudioData::NonInterleavedDest<DestFormat> { offsetDestChannels.getData(), numDestChannels },
                                        (int) framesRead);
    }
    else if (bitsPerSample == 32)
    {
        if (usesFloatingPointData)
        {
            using SourceFormat = AudioData::Format<AudioData::Float32, AudioData::LittleEndian>;
            using DestFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
            
            AudioData::deinterleaveSamples (AudioData::InterleavedSource<SourceFormat> { reinterpret_cast<const float*> (tempBuffer.getData()), (int) numChannels },
                                            AudioData::NonInterleavedDest<DestFormat> { offsetDestChannels.getData(), numDestChannels },
                                            (int) framesRead);
        }
        else
        {
            using SourceFormat = AudioData::Format<AudioData::Int32, AudioData::LittleEndian>;
            using DestFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
            
            AudioData::deinterleaveSamples (AudioData::InterleavedSource<SourceFormat> { reinterpret_cast<const uint32*> (tempBuffer.getData()), (int) numChannels },
                                            AudioData::NonInterleavedDest<DestFormat> { offsetDestChannels.getData(), numDestChannels },
                                            (int) framesRead);
        }
    }
    else if (bitsPerSample == 64 && usesFloatingPointData)
    {
        // Handle 64-bit double precision float samples using AudioData
        using SourceFormat = AudioData::Format<AudioData::Float64, AudioData::LittleEndian>;
        using DestFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
        
        AudioData::deinterleaveSamples (AudioData::InterleavedSource<SourceFormat> { reinterpret_cast<const double*> (tempBuffer.getData()), (int) numChannels },
                                        AudioData::NonInterleavedDest<DestFormat> { offsetDestChannels.getData(), numDestChannels },
                                        (int) framesRead);
    }
    else
    {
        return false;
    }

    return true;
}

//==============================================================================
class WaveAudioFormatWriter : public AudioFormatWriter
{
public:
    WaveAudioFormatWriter (OutputStream* destStream,
                           double sampleRate,
                           int numberOfChannels,
                           int bitsPerSample,
                           const StringPairArray& metadataValues);

    ~WaveAudioFormatWriter() override;

    bool write (const float* const* samplesToWrite, int numSamples) override;

    bool flush() override;

private:
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

    drwav wav = {};
    HeapBlock<uint8> tempBuffer;
    size_t tempBufferSize = 0;
    bool isOpen = false;
    int64 samplesWritten = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveAudioFormatWriter)
};

WaveAudioFormatWriter::WaveAudioFormatWriter (OutputStream* destStream,
                                              double sampleRate,
                                              int numberOfChannels,
                                              int bitsPerSample,
                                              const StringPairArray& metadataValues)
    : AudioFormatWriter (destStream, "Wave file", sampleRate, numberOfChannels, bitsPerSample)
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

    isOpen = drwav_init_write_with_metadata (&wav,
                                             &format,
                                             writeCallback,
                                             seekCallback,
                                             destStream,
                                             nullptr,
                                             metadata.empty() ? nullptr : metadata.data(),
                                             (drwav_uint32) metadata.size()) == DRWAV_TRUE;

    if (isOpen)
    {
        // Allocate temp buffer for writing
        const auto bytesPerFrame = numberOfChannels * (bitsPerSample / 8);
        tempBufferSize = bytesPerFrame * 4096;
        tempBuffer.allocate (tempBufferSize, true);
    }
}

WaveAudioFormatWriter::~WaveAudioFormatWriter()
{
    if (isOpen)
        drwav_uninit (&wav);
}

bool WaveAudioFormatWriter::write (const float* const* samplesToWrite, int numSamples)
{
    if (! isOpen || numSamples <= 0)
        return false;

    const auto numChannels = getNumChannels();
    const auto bytesPerSample = getBitsPerSample() / 8;
    const auto bytesPerFrame = numChannels * bytesPerSample;
    const auto bytesToWrite = numSamples * bytesPerFrame;

    if (bytesToWrite > tempBufferSize)
    {
        tempBufferSize = bytesToWrite;
        tempBuffer.allocate (bytesToWrite, false);
    }

    // Use AudioData to interleave and convert in one step
    if (getBitsPerSample() == 8)
    {
        using SourceFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
        using DestFormat = AudioData::Format<AudioData::UInt8, AudioData::LittleEndian>;
        
        AudioData::interleaveSamples (AudioData::NonInterleavedSource<SourceFormat> { samplesToWrite, (int) numChannels },
                                      AudioData::InterleavedDest<DestFormat> { reinterpret_cast<uint8*> (tempBuffer.getData()), (int) numChannels },
                                      numSamples);
    }
    else if (getBitsPerSample() == 16)
    {
        using SourceFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
        using DestFormat = AudioData::Format<AudioData::Int16, AudioData::LittleEndian>;
        
        AudioData::interleaveSamples (AudioData::NonInterleavedSource<SourceFormat> { samplesToWrite, (int) numChannels },
                                      AudioData::InterleavedDest<DestFormat> { reinterpret_cast<uint16*> (tempBuffer.getData()), (int) numChannels },
                                      numSamples);
    }
    else if (getBitsPerSample() == 24)
    {
        using SourceFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
        using DestFormat = AudioData::Format<AudioData::Int24, AudioData::LittleEndian>;
        
        AudioData::interleaveSamples (AudioData::NonInterleavedSource<SourceFormat> { samplesToWrite, (int) numChannels },
                                      AudioData::InterleavedDest<DestFormat> { reinterpret_cast<char*> (tempBuffer.getData()), (int) numChannels },
                                      numSamples);
    }
    else if (getBitsPerSample() == 32)
    {
        if (isFloatingPoint())
        {
            using SourceFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
            using DestFormat = AudioData::Format<AudioData::Float32, AudioData::LittleEndian>;
            
            AudioData::interleaveSamples (AudioData::NonInterleavedSource<SourceFormat> { samplesToWrite, (int) numChannels },
                                          AudioData::InterleavedDest<DestFormat> { reinterpret_cast<float*> (tempBuffer.getData()), (int) numChannels },
                                          numSamples);
        }
        else
        {
            using SourceFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
            using DestFormat = AudioData::Format<AudioData::Int32, AudioData::LittleEndian>;
            
            AudioData::interleaveSamples (AudioData::NonInterleavedSource<SourceFormat> { samplesToWrite, (int) numChannels },
                                          AudioData::InterleavedDest<DestFormat> { reinterpret_cast<uint32*> (tempBuffer.getData()), (int) numChannels },
                                          numSamples);
        }
    }
    else
    {
        return false;
    }

    const auto framesWritten = drwav_write_pcm_frames (&wav, (drwav_uint64) numSamples, tempBuffer.getData());

    if (framesWritten > 0)
    {
        samplesWritten += framesWritten;
        return true;
    }

    return false;
}

bool WaveAudioFormatWriter::flush()
{
    if (isOpen && output != nullptr)
    {
        output->flush();
        return true;
    }
    return false;
}

} // namespace

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
                                                                     int numberOfChannels,
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

    return std::make_unique<WaveAudioFormatWriter> (streamToWriteTo, sampleRate, numberOfChannels, bitsPerSample, metadataValues);
}

Array<int> WaveAudioFormat::getPossibleBitDepths() const
{
    return { 8, 16, 24, 32 };
}

Array<int> WaveAudioFormat::getPossibleSampleRates() const
{
    return { 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 88200, 96000, 176400, 192000 };
}

} // namespace yup
