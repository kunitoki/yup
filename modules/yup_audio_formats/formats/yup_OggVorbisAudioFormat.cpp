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

class OggVorbisAudioFormatReader : public AudioFormatReader
{
public:
    OggVorbisAudioFormatReader (InputStream* sourceStream);
    ~OggVorbisAudioFormatReader() override;

    bool readSamples (float* const* destChannels,
                      int numDestChannels,
                      int startOffsetInDestBuffer,
                      int64 startSampleInFile,
                      int numSamples) override;

private:
    static size_t readCallback (void* buffer, size_t size, size_t nmemb, void* datasource)
    {
        auto* reader = static_cast<OggVorbisAudioFormatReader*> (datasource);
        if (reader == nullptr || reader->input == nullptr || size == 0)
            return 0;

        const size_t bytesToRead = size * nmemb;
        const int bytesRead = reader->input->read (buffer, (int) bytesToRead);
        if (bytesRead <= 0)
            return 0;

        return (size_t) bytesRead / size;
    }

    static int seekCallback (void* datasource, ogg_int64_t offset, int whence)
    {
        auto* reader = static_cast<OggVorbisAudioFormatReader*> (datasource);
        if (reader == nullptr || reader->input == nullptr)
            return -1;

        int64 target = 0;

        switch (whence)
        {
            case SEEK_SET:
                target = (int64) offset;
                break;
            case SEEK_CUR:
                target = reader->input->getPosition() + (int64) offset;
                break;
            case SEEK_END:
            {
                const auto length = reader->input->getTotalLength();
                if (length < 0)
                    return -1;
                target = length + (int64) offset;
                break;
            }
            default:
                return -1;
        }

        return reader->input->setPosition (target) ? 0 : -1;
    }

    static int closeCallback (void*)
    {
        return 0;
    }

    static long tellCallback (void* datasource)
    {
        auto* reader = static_cast<OggVorbisAudioFormatReader*> (datasource);
        if (reader == nullptr || reader->input == nullptr)
            return -1;

        return (long) reader->input->getPosition();
    }

    void readMetadata()
    {
        auto* comment = ov_comment (&vorbisFile, -1);
        if (comment == nullptr || comment->comments <= 0)
            return;

        for (int i = 0; i < comment->comments; ++i)
        {
            const char* entry = comment->user_comments[i];
            const auto length = comment->comment_lengths[i];
            if (entry == nullptr || length <= 0)
                continue;

            const auto text = String::fromUTF8 (entry, length);
            const auto separatorIndex = text.indexOfChar ('=');
            if (separatorIndex <= 0)
                continue;

            const auto key = text.substring (0, separatorIndex).toLowerCase();
            const auto value = text.substring (separatorIndex + 1);
            if (key.isNotEmpty())
                metadataValues.set (key, value);
        }
    }

    OggVorbis_File vorbisFile = {};
    ogg_int64_t currentSample = 0;
    bool isOpen = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OggVorbisAudioFormatReader)
};

OggVorbisAudioFormatReader::OggVorbisAudioFormatReader (InputStream* sourceStream)
    : AudioFormatReader (sourceStream, "Ogg Vorbis")
{
    if (input == nullptr)
        return;

    input->setPosition (0);

    ov_callbacks callbacks;
    callbacks.read_func = &readCallback;
    callbacks.seek_func = &seekCallback;
    callbacks.close_func = &closeCallback;
    callbacks.tell_func = &tellCallback;

    if (ov_open_callbacks (this, &vorbisFile, nullptr, 0, callbacks) < 0)
        return;

    const auto* info = ov_info (&vorbisFile, -1);
    if (info == nullptr)
        return;

    sampleRate = (double) info->rate;
    numChannels = (int) info->channels;
    bitsPerSample = 32;
    usesFloatingPointData = true;

    const auto totalSamples = ov_pcm_total (&vorbisFile, -1);
    if (totalSamples > 0)
        lengthInSamples = (int64) totalSamples;

    readMetadata();

    isOpen = (sampleRate > 0 && numChannels > 0);
    currentSample = 0;
}

OggVorbisAudioFormatReader::~OggVorbisAudioFormatReader()
{
    if (isOpen)
        ov_clear (&vorbisFile);
}

bool OggVorbisAudioFormatReader::readSamples (float* const* destChannels,
                                              int numDestChannels,
                                              int startOffsetInDestBuffer,
                                              int64 startSampleInFile,
                                              int numSamples)
{
    if (! isOpen)
        return false;

    if (numSamples <= 0)
        return true;

    if (startSampleInFile < 0)
        return false;

    if (lengthInSamples > 0 && startSampleInFile >= lengthInSamples)
        return false;

    if (startSampleInFile != currentSample)
    {
        if (ov_pcm_seek (&vorbisFile, (ogg_int64_t) startSampleInFile) != 0)
            return false;

        currentSample = startSampleInFile;
    }

    HeapBlock<float*> offsetDestChannels;
    offsetDestChannels.malloc (numDestChannels);

    for (int ch = 0; ch < numDestChannels; ++ch)
        offsetDestChannels[ch] = destChannels[ch] + startOffsetInDestBuffer;

    int samplesRemaining = numSamples;
    int samplesReadTotal = 0;

    while (samplesRemaining > 0)
    {
        float** pcm = nullptr;
        int currentSection = 0;
        const long samplesRead = ov_read_float (&vorbisFile, &pcm, samplesRemaining, &currentSection);

        if (samplesRead == 0)
            break;

        if (samplesRead < 0)
            return false;

        const auto numChannelsToCopy = jmin (numDestChannels, numChannels);

        for (int ch = 0; ch < numChannelsToCopy; ++ch)
        {
            if (offsetDestChannels[ch] != nullptr)
                FloatVectorOperations::copy (offsetDestChannels[ch], pcm[ch], (int) samplesRead);
        }

        for (int ch = numChannelsToCopy; ch < numDestChannels; ++ch)
        {
            if (offsetDestChannels[ch] != nullptr)
                zeromem (offsetDestChannels[ch], sizeof (float) * (size_t) samplesRead);
        }

        for (int ch = 0; ch < numDestChannels; ++ch)
        {
            if (offsetDestChannels[ch] != nullptr)
                offsetDestChannels[ch] += samplesRead;
        }

        samplesRemaining -= (int) samplesRead;
        samplesReadTotal += (int) samplesRead;
        currentSample += samplesRead;
    }

    if (samplesReadTotal < numSamples)
    {
        const auto remaining = numSamples - samplesReadTotal;
        for (int ch = 0; ch < numDestChannels; ++ch)
            if (destChannels[ch] != nullptr)
                zeromem (destChannels[ch] + startOffsetInDestBuffer + samplesReadTotal,
                         sizeof (float) * (size_t) remaining);
    }

    return samplesReadTotal > 0;
}

//==============================================================================

class OggVorbisAudioFormatWriter : public AudioFormatWriter
{
public:
    OggVorbisAudioFormatWriter (OutputStream* destStream,
                                double sampleRate,
                                int numberOfChannels,
                                int bitsPerSample,
                                const StringPairArray& metadataValues,
                                int qualityOptionIndex);
    ~OggVorbisAudioFormatWriter() override;

    bool write (const float* const* samplesToWrite, int numSamples) override;
    bool flush() override;

    bool isValid() const { return isOpen; }

private:
    bool writePage (const ogg_page& page)
    {
        if (output == nullptr)
            return false;

        if (! output->write (page.header, (size_t) page.header_len))
            return false;

        if (! output->write (page.body, (size_t) page.body_len))
            return false;

        return true;
    }

    bool flushPages (bool force)
    {
        ogg_page page;

        while (true)
        {
            const int result = force ? ogg_stream_flush (&oggStream, &page)
                                     : ogg_stream_pageout (&oggStream, &page);

            if (result == 0)
                break;

            if (result < 0)
                return false;

            if (! writePage (page))
                return false;
        }

        return true;
    }

    bool processBlocks (bool forceFlush)
    {
        while (vorbis_analysis_blockout (&dspState, &block) == 1)
        {
            vorbis_analysis (&block, nullptr);
            vorbis_bitrate_addblock (&block);

            ogg_packet packet;
            while (vorbis_bitrate_flushpacket (&dspState, &packet))
            {
                ogg_stream_packetin (&oggStream, &packet);
                if (! flushPages (forceFlush))
                    return false;
            }
        }

        return true;
    }

    bool finalizeEncoding()
    {
        if (finished)
            return true;

        if (vorbis_analysis_wrote (&dspState, 0) != 0)
            return false;

        if (! processBlocks (true))
            return false;

        finished = true;
        return true;
    }

    bool writeHeaders()
    {
        ogg_packet header = {};
        ogg_packet headerComment = {};
        ogg_packet headerCode = {};

        if (vorbis_analysis_headerout (&dspState, &comment, &header, &headerComment, &headerCode) != 0)
            return false;

        ogg_stream_packetin (&oggStream, &header);
        ogg_stream_packetin (&oggStream, &headerComment);
        ogg_stream_packetin (&oggStream, &headerCode);

        return flushPages (true);
    }

    void clearState()
    {
        if (oggStreamInitialized)
            ogg_stream_clear (&oggStream);

        if (blockInitialized)
            vorbis_block_clear (&block);

        if (dspInitialized)
            vorbis_dsp_clear (&dspState);

        if (commentInitialized)
            vorbis_comment_clear (&comment);

        if (infoInitialized)
            vorbis_info_clear (&info);
    }

    vorbis_info info = {};
    vorbis_dsp_state dspState = {};
    vorbis_block block = {};
    vorbis_comment comment = {};
    ogg_stream_state oggStream = {};

    bool infoInitialized = false;
    bool commentInitialized = false;
    bool dspInitialized = false;
    bool blockInitialized = false;
    bool oggStreamInitialized = false;
    bool finished = false;
    bool isOpen = false;
    int channelCount = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OggVorbisAudioFormatWriter)
};

OggVorbisAudioFormatWriter::OggVorbisAudioFormatWriter (OutputStream* destStream,
                                                        double sampleRate,
                                                        int numberOfChannels,
                                                        int bitsPerSample,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex)
    : AudioFormatWriter (destStream, "Ogg Vorbis", sampleRate, numberOfChannels, bitsPerSample)
{
    if (destStream == nullptr)
        return;

    channelCount = numberOfChannels;

    const float quality = jlimit (0.0f, 1.0f, (float) qualityOptionIndex / 10.0f);

    vorbis_info_init (&info);
    infoInitialized = true;

    if (vorbis_encode_init_vbr (&info, numberOfChannels, (long) sampleRate, quality) != 0)
        return;

    vorbis_comment_init (&comment);
    commentInitialized = true;

    for (const auto& pair : metadataValues)
    {
        const auto key = String (pair.key).trim().toUpperCase();
        const auto value = String (pair.value);
        if (key.isNotEmpty() && value.isNotEmpty())
            vorbis_comment_add_tag (&comment, key.toRawUTF8(), value.toRawUTF8());
    }

    if (vorbis_analysis_init (&dspState, &info) != 0)
        return;

    dspInitialized = true;

    if (vorbis_block_init (&dspState, &block) != 0)
        return;

    blockInitialized = true;

    if (ogg_stream_init (&oggStream, (int) Random::getSystemRandom().nextInt()) != 0)
        return;

    oggStreamInitialized = true;

    if (! writeHeaders())
        return;

    isOpen = true;
}

OggVorbisAudioFormatWriter::~OggVorbisAudioFormatWriter()
{
    flush();
    clearState();
}

bool OggVorbisAudioFormatWriter::write (const float* const* samplesToWrite, int numSamples)
{
    if (! isOpen || numSamples <= 0)
        return false;

    auto** buffer = vorbis_analysis_buffer (&dspState, numSamples);
    if (buffer == nullptr)
        return false;

    for (int ch = 0; ch < channelCount; ++ch)
    {
        auto* dest = buffer[ch];
        const auto* src = samplesToWrite[ch];
        if (src != nullptr)
            FloatVectorOperations::copy (dest, src, numSamples);
        else
            zeromem (dest, sizeof (float) * (size_t) numSamples);
    }

    if (vorbis_analysis_wrote (&dspState, numSamples) != 0)
        return false;

    return processBlocks (false);
}

bool OggVorbisAudioFormatWriter::flush()
{
    if (! isOpen)
        return false;

    if (! finalizeEncoding())
        return false;

    output->flush();
    return true;
}

} // namespace

//==============================================================================

OggVorbisAudioFormat::OggVorbisAudioFormat()
    : formatName ("Ogg Vorbis")
{
}

OggVorbisAudioFormat::~OggVorbisAudioFormat() = default;

const String& OggVorbisAudioFormat::getFormatName() const
{
    return formatName;
}

Array<String> OggVorbisAudioFormat::getFileExtensions ([[maybe_unused]] Mode handleMode) const
{
    return { ".ogg" };
}

std::unique_ptr<AudioFormatReader> OggVorbisAudioFormat::createReaderFor (InputStream* sourceStream)
{
    auto reader = std::make_unique<OggVorbisAudioFormatReader> (sourceStream);

    if (reader->sampleRate > 0 && reader->numChannels > 0)
        return reader;

    return nullptr;
}

std::unique_ptr<AudioFormatWriter> OggVorbisAudioFormat::createWriterFor (OutputStream* streamToWriteTo,
                                                                          double sampleRate,
                                                                          int numberOfChannels,
                                                                          int bitsPerSample,
                                                                          const StringPairArray& metadataValues,
                                                                          int qualityOptionIndex)
{
    if (streamToWriteTo == nullptr)
        return nullptr;

    if (numberOfChannels < 1 || numberOfChannels > 8)
        return nullptr;

    if (sampleRate <= 0 || sampleRate > 192000.0)
        return nullptr;

    auto writer = std::make_unique<OggVorbisAudioFormatWriter> (streamToWriteTo,
                                                                sampleRate,
                                                                numberOfChannels,
                                                                bitsPerSample,
                                                                metadataValues,
                                                                qualityOptionIndex);
    if (writer->isValid())
        return writer;

    return nullptr;
}

Array<int> OggVorbisAudioFormat::getPossibleBitDepths() const
{
    return { 16 };
}

Array<int> OggVorbisAudioFormat::getPossibleSampleRates() const
{
    return { 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 88200, 96000, 176400, 192000 };
}

StringArray OggVorbisAudioFormat::getQualityOptions() const
{
    return { "0 - Lowest", "1 - Very low", "2 - Low", "3 - Medium low", "4 - Medium", "5 - Medium high", "6 - High", "7 - Very high", "8 - Higher", "9 - Very high", "10 - Highest" };
}

} // namespace yup
