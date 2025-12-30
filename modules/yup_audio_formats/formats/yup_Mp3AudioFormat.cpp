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

class Mp3AudioFormatReader : public AudioFormatReader
{
public:
    Mp3AudioFormatReader (InputStream* sourceStream);
    ~Mp3AudioFormatReader() override;

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

    static drmp3_bool32 seekCallback (void* pUserData, int offset, drmp3_seek_origin origin)
    {
        auto* stream = static_cast<InputStream*> (pUserData);

        if (origin == DRMP3_SEEK_SET)
            return stream->setPosition (offset) ? DRMP3_TRUE : DRMP3_FALSE;
        else if (origin == DRMP3_SEEK_CUR)
            return stream->setPosition (stream->getPosition() + offset) ? DRMP3_TRUE : DRMP3_FALSE;

        return DRMP3_FALSE;
    }

    static drmp3_bool32 tellCallback (void* pUserData, drmp3_int64* pCursor)
    {
        auto* stream = static_cast<InputStream*> (pUserData);
        *pCursor = stream->getPosition();
        return DRMP3_TRUE;
    }

    static void metaCallback (void* pUserData, const drmp3_metadata* pMetadata)
    {
        auto* reader = static_cast<Mp3AudioFormatReader*> (pUserData);
        if (reader && pMetadata && pMetadata->pRawData)
        {
            // Handle metadata based on type
            switch (pMetadata->type)
            {
                case DRMP3_METADATA_TYPE_ID3V1:
                case DRMP3_METADATA_TYPE_ID3V2:
                case DRMP3_METADATA_TYPE_APE:
                {
                    // For now, we'll just store the raw metadata. In a real implementation,
                    // you would parse the metadata and extract useful information like
                    // title, artist, album, etc.
                    break;
                }
                case DRMP3_METADATA_TYPE_XING:
                case DRMP3_METADATA_TYPE_VBRI:
                {
                    // Xing/VBRI headers contain VBR information and seek tables
                    break;
                }
                default:
                    break;
            }
        }
    }

    drmp3 mp3 = {};
    HeapBlock<float> tempBuffer;
    size_t tempBufferSize = 0;
    bool isOpen = false;
    int64 currentPCMFrame = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Mp3AudioFormatReader)
};

Mp3AudioFormatReader::Mp3AudioFormatReader (InputStream* sourceStream)
    : AudioFormatReader (sourceStream, "MP3 file")
{
    if (sourceStream == nullptr)
        return;

    isOpen = drmp3_init (&mp3, readCallback, seekCallback, tellCallback, metaCallback, sourceStream, nullptr) == DRMP3_TRUE;

    if (isOpen)
    {
        sampleRate = mp3.sampleRate;
        bitsPerSample = 16; // MP3 always outputs 16-bit samples
        lengthInSamples = mp3.totalPCMFrameCount;
        numChannels = mp3.channels;
        usesFloatingPointData = false; // MP3 outputs 16-bit integer samples

        // Allocate temp buffer for reading
        const auto bytesPerFrame = numChannels * (bitsPerSample / 8);
        tempBufferSize = bytesPerFrame * 4096;
        tempBuffer.allocate (tempBufferSize / sizeof (float), true);
    }
}

Mp3AudioFormatReader::~Mp3AudioFormatReader()
{
    if (isOpen)
        drmp3_uninit (&mp3);
}

bool Mp3AudioFormatReader::readSamples (float* const* destChannels,
                                        int numDestChannels,
                                        int startOffsetInDestBuffer,
                                        int64 startSampleInFile,
                                        int numSamples)
{
    if (! isOpen)
        return false;

    if (numSamples <= 0)
        return true;

    // Seek to the start position if needed
    if (startSampleInFile != currentPCMFrame)
    {
        if (! drmp3_seek_to_pcm_frame (&mp3, startSampleInFile))
            return false;
        currentPCMFrame = startSampleInFile;
    }

    const auto numChannelsToRead = jmin (numDestChannels, numChannels);
    const auto bytesPerSample = bitsPerSample / 8;
    const auto bytesPerFrame = numChannels * bytesPerSample;

    // Create output channel pointers offset by the start position
    HeapBlock<float*> offsetDestChannels;
    offsetDestChannels.malloc (numDestChannels);

    for (int ch = 0; ch < numDestChannels; ++ch)
    {
        offsetDestChannels[ch] = destChannels[ch] + startOffsetInDestBuffer;
    }

    drmp3_uint64 framesRead = 0;
    int samplesToRead = numSamples;

    while (samplesToRead > 0)
    {
        const auto framesToRead = jmin (samplesToRead, (int) (tempBufferSize / (numChannels * sizeof (float))));

        if (framesToRead <= 0)
            break;

        // Read MP3 frames into temp buffer
        auto framesJustRead = drmp3_read_pcm_frames_f32 (&mp3, framesToRead, tempBuffer.getData());

        if (framesJustRead == 0)
            break;

        // Convert and deinterleave the samples
        using SourceFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
        using DestFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;

        AudioData::deinterleaveSamples (AudioData::InterleavedSource<SourceFormat> { tempBuffer.getData(), numChannels },
                                        AudioData::NonInterleavedDest<DestFormat> { offsetDestChannels.getData(), numChannelsToRead },
                                        (int) framesJustRead);

        // Fill remaining channels with copies if requested
        for (int ch = numChannelsToRead; ch < numDestChannels; ++ch)
        {
            if (offsetDestChannels[ch] != nullptr)
                zeromem (offsetDestChannels[ch], sizeof (float) * framesJustRead);
        }

        // Update pointers and counters
        for (int ch = 0; ch < numDestChannels; ++ch)
        {
            if (offsetDestChannels[ch] != nullptr)
                offsetDestChannels[ch] += framesJustRead;
        }

        framesRead += framesJustRead;
        samplesToRead -= (int) framesJustRead;
        currentPCMFrame += framesJustRead;
    }

    return framesRead > 0;
}

} // namespace

//==============================================================================
// Mp3AudioFormat implementation
Mp3AudioFormat::Mp3AudioFormat()
    : formatName ("MP3 file")
{
}

Mp3AudioFormat::~Mp3AudioFormat() = default;

const String& Mp3AudioFormat::getFormatName() const
{
    return formatName;
}

Array<String> Mp3AudioFormat::getFileExtensions() const
{
    return { ".mp3" };
}

std::unique_ptr<AudioFormatReader> Mp3AudioFormat::createReaderFor (InputStream* sourceStream)
{
    auto reader = std::make_unique<Mp3AudioFormatReader> (sourceStream);

    if (reader->sampleRate > 0 && reader->numChannels > 0)
        return reader;

    return nullptr;
}

std::unique_ptr<AudioFormatWriter> Mp3AudioFormat::createWriterFor (OutputStream* streamToWriteTo,
                                                                    double sampleRate,
                                                                    int numberOfChannels,
                                                                    int bitsPerSample,
                                                                    const StringPairArray& metadataValues,
                                                                    int qualityOptionIndex)
{
    // MP3 encoding is not implemented in this version
    return nullptr;
}

Array<int> Mp3AudioFormat::getPossibleBitDepths() const
{
    return { 16 };
}

Array<int> Mp3AudioFormat::getPossibleSampleRates() const
{
    return { 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000 };
}

} // namespace yup