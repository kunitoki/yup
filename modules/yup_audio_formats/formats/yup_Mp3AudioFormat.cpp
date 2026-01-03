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

#if YUP_MODULE_AVAILABLE_hmp3_library
static E_CONTROL makeMp3EncoderControl (double sampleRate,
                                        int numberOfChannels,
                                        int qualityOptionIndex)
{
    E_CONTROL ec = {};
    ec.mode = (numberOfChannels == 1) ? 3 : 1;
    ec.bitrate = -1;
    ec.samprate = (int) sampleRate;
    ec.nsbstereo = -1;
    ec.filter_select = -1;
    ec.nsb_limit = -1;
    ec.freq_limit = 24000;
    ec.cr_bit = 1;
    ec.original = 1;
    ec.layer = 3;
    ec.hf_flag = 0;
    ec.vbr_flag = 1;
    ec.vbr_mnr = jlimit (0, 150, qualityOptionIndex > 0 ? qualityOptionIndex : 50);
    ec.vbr_br_limit = 160;
    ec.chan_add_f0 = 24000;
    ec.chan_add_f1 = 24000;
    ec.sparse_scale = -1;
    ec.vbr_delta_mnr = 0;
    ec.cpu_select = 0;
    ec.quick = -1;
    ec.test1 = -1;
    ec.test2 = 0;
    ec.test3 = 0;
    ec.short_block_threshold = 700;

    for (int i = 0; i < 21; ++i)
        ec.mnr_adjust[i] = 0;

    return ec;
}

class Mp3AudioFormatWriter : public AudioFormatWriter
{
public:
    Mp3AudioFormatWriter (OutputStream* destStream,
                          double sampleRate,
                          int numberOfChannels,
                          int bitsPerSample,
                          const StringPairArray& metadataValues,
                          int qualityOptionIndex);
    ~Mp3AudioFormatWriter() override;

    bool write (const float* const* samplesToWrite, int numSamples) override;
    bool flush() override;

    bool isValid() const { return isOpen; }

private:
    bool encodeAvailableInput();
    void compactPcmBuffer();

    CMp3Enc encoder;
    E_CONTROL control = {};

    std::vector<uint8> pcmBuffer;
    size_t pcmReadOffset = 0;

    HeapBlock<float> interleavedBuffer;
    size_t interleavedCapacity = 0;

    std::vector<uint8> outputBuffer;
    std::vector<uint8> zeroBuffer;

    int minInputBytes = 0;
    int64 framesExpected = 0;
    bool isOpen = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Mp3AudioFormatWriter)
};
#endif

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
        numChannels = mp3.channels;
        usesFloatingPointData = false; // MP3 outputs 16-bit integer samples

        const drmp3_uint64 unknownFrameCount = (drmp3_uint64) -1;
        drmp3_uint64 totalFrames = mp3.totalPCMFrameCount;

        if (totalFrames == 0 || totalFrames == unknownFrameCount)
        {
            const auto scannedFrames = drmp3_get_pcm_frame_count (&mp3);
            if (scannedFrames != 0 && scannedFrames != unknownFrameCount)
                totalFrames = scannedFrames;

            drmp3_seek_to_pcm_frame (&mp3, 0);
            currentPCMFrame = 0;
        }

        if (totalFrames == 0 || totalFrames == unknownFrameCount || totalFrames > 0x7fffffffffffffffULL)
            lengthInSamples = 0;
        else
            lengthInSamples = (int64) totalFrames;

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

#if YUP_MODULE_AVAILABLE_hmp3_library
Mp3AudioFormatWriter::Mp3AudioFormatWriter (OutputStream* destStream,
                                            double sampleRate,
                                            int numberOfChannels,
                                            int bitsPerSample,
                                            const StringPairArray& metadataValues,
                                            int qualityOptionIndex)
    : AudioFormatWriter (destStream, "MP3 file", sampleRate, numberOfChannels, bitsPerSample)
{
    ignoreUnused (metadataValues);

    control = makeMp3EncoderControl (sampleRate, numberOfChannels, qualityOptionIndex);
    minInputBytes = encoder.MP3_audio_encode_init (&control, 32, 1, 0, 0);

    if (minInputBytes > 0)
    {
        outputBuffer.resize (128u * 1024u);
        zeroBuffer.resize ((size_t) minInputBytes, 0);
        isOpen = true;
    }
}

Mp3AudioFormatWriter::~Mp3AudioFormatWriter()
{
    flush();
}

bool Mp3AudioFormatWriter::write (const float* const* samplesToWrite, int numSamples)
{
    if (! isOpen || numSamples <= 0)
        return false;

    const auto numChannels = getNumChannels();
    const size_t totalSamples = (size_t) numSamples * (size_t) numChannels;

    if (totalSamples > interleavedCapacity)
    {
        interleavedCapacity = totalSamples;
        interleavedBuffer.allocate (interleavedCapacity, false);
    }

    using SourceFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
    using DestFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;

    AudioData::interleaveSamples (AudioData::NonInterleavedSource<SourceFormat> { samplesToWrite, (int) numChannels },
                                  AudioData::InterleavedDest<DestFormat> { interleavedBuffer.getData(), (int) numChannels },
                                  numSamples);

    const size_t bytesToAdd = totalSamples * sizeof (float);
    const auto* bytes = reinterpret_cast<const uint8*> (interleavedBuffer.getData());
    pcmBuffer.insert (pcmBuffer.end(), bytes, bytes + bytesToAdd);

    return encodeAvailableInput();
}

bool Mp3AudioFormatWriter::flush()
{
    if (! isOpen)
        return false;

    if (minInputBytes > 0)
    {
        const size_t availableBytes = pcmBuffer.size() - pcmReadOffset;
        if (availableBytes > 0 && availableBytes < (size_t) minInputBytes)
        {
            const size_t padBytes = (size_t) minInputBytes - availableBytes;
            pcmBuffer.insert (pcmBuffer.end(), padBytes, 0);
        }
    }

    if (! encodeAvailableInput())
        return false;

    int64 expectedFrames = framesExpected;
    if (control.samprate < 32000)
        expectedFrames *= 2;

    int safetyCounter = 0;
    while (encoder.L3_audio_encode_get_frames() < (unsigned int) expectedFrames && safetyCounter < 4096)
    {
        auto io = encoder.MP3_audio_encode (zeroBuffer.data(), outputBuffer.data());
        if (io.out_bytes > 0)
        {
            if (! output->write (outputBuffer.data(), (size_t) io.out_bytes))
                return false;
        }

        if (io.in_bytes <= 0 && io.out_bytes <= 0)
            break;

        ++safetyCounter;
    }

    return true;
}

bool Mp3AudioFormatWriter::encodeAvailableInput()
{
    if (minInputBytes <= 0 || pcmBuffer.size() <= pcmReadOffset)
        return true;

    while (pcmBuffer.size() - pcmReadOffset >= (size_t) minInputBytes)
    {
        auto io = encoder.MP3_audio_encode (pcmBuffer.data() + pcmReadOffset, outputBuffer.data());
        if (io.in_bytes <= 0 && io.out_bytes <= 0)
            break;

        ++framesExpected;

        if (io.in_bytes > 0)
            pcmReadOffset += (size_t) io.in_bytes;

        if (io.out_bytes > 0)
        {
            if (! output->write (outputBuffer.data(), (size_t) io.out_bytes))
                return false;
        }

        compactPcmBuffer();
    }

    return true;
}

void Mp3AudioFormatWriter::compactPcmBuffer()
{
    if (pcmReadOffset == 0)
        return;

    if (pcmReadOffset >= pcmBuffer.size())
    {
        pcmBuffer.clear();
        pcmReadOffset = 0;
        return;
    }

    if (pcmReadOffset > 4096)
    {
        pcmBuffer.erase (pcmBuffer.begin(), pcmBuffer.begin() + (ptrdiff_t) pcmReadOffset);
        pcmReadOffset = 0;
    }
}
#endif

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

Array<String> Mp3AudioFormat::getFileExtensions ([[maybe_unused]] Mode handleMode) const
{
    if (handleMode == Mode::forReading)
        return { ".mp3" };

#if YUP_MODULE_AVAILABLE_hmp3_library
    return { ".mp3" };
#else
    return {};
#endif
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
#if YUP_MODULE_AVAILABLE_hmp3_library
    if (streamToWriteTo == nullptr)
        return nullptr;

    if (numberOfChannels < 1 || numberOfChannels > 2)
        return nullptr;

    if (sampleRate < 8000.0 || sampleRate > 48000.0)
        return nullptr;

    auto writer = std::make_unique<Mp3AudioFormatWriter> (streamToWriteTo,
                                                          sampleRate,
                                                          numberOfChannels,
                                                          bitsPerSample,
                                                          metadataValues,
                                                          qualityOptionIndex);
    if (writer->isValid())
        return writer;
#else
    ignoreUnused (streamToWriteTo,
                  sampleRate,
                  numberOfChannels,
                  bitsPerSample,
                  metadataValues,
                  qualityOptionIndex);
#endif

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
