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

static float flacIntToFloat (FLAC__int32 sample, int bitsPerSample)
{
    if (bitsPerSample <= 0)
        return 0.0f;

    const double scale = 1.0 / (double) (1ull << (bitsPerSample - 1));
    return (float) ((double) sample * scale);
}

static FLAC__int32 floatToFlacInt (float sample, int bitsPerSample)
{
    if (bitsPerSample <= 0)
        return 0;

    if (sample >= 1.0f)
        return (FLAC__int32) ((1ull << (bitsPerSample - 1)) - 1);
    if (sample <= -1.0f)
        return (FLAC__int32) (-(int64) (1ull << (bitsPerSample - 1)));

    const double scale = (double) ((1ull << (bitsPerSample - 1)) - 1);
    return (FLAC__int32) roundToIntAccurate ((double) sample * scale);
}

class FlacAudioFormatReader : public AudioFormatReader
{
public:
    FlacAudioFormatReader (InputStream* sourceStream);
    ~FlacAudioFormatReader() override;

    bool readSamples (float* const* destChannels,
                      int numDestChannels,
                      int startOffsetInDestBuffer,
                      int64 startSampleInFile,
                      int numSamples) override;

private:
    struct ReadState
    {
        float* const* destChannels = nullptr;
        int numDestChannels = 0;
        int startOffset = 0;
        int numSamples = 0;
        int samplesWritten = 0;
    };

    static FLAC__StreamDecoderReadStatus readCallback (const FLAC__StreamDecoder*,
                                                       FLAC__byte buffer[],
                                                       size_t* bytes,
                                                       void* clientData)
    {
        auto* reader = static_cast<FlacAudioFormatReader*> (clientData);
        if (reader == nullptr || reader->input == nullptr || bytes == nullptr || *bytes == 0)
            return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

        const auto bytesRequested = (int) *bytes;
        const int bytesRead = reader->input->read (buffer, bytesRequested);

        if (bytesRead > 0)
        {
            *bytes = (size_t) bytesRead;
            return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
        }

        *bytes = 0;
        return reader->input->isExhausted() ? FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM
                                            : FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    }

    static FLAC__StreamDecoderSeekStatus seekCallback (const FLAC__StreamDecoder*,
                                                       FLAC__uint64 absoluteByteOffset,
                                                       void* clientData)
    {
        auto* reader = static_cast<FlacAudioFormatReader*> (clientData);
        if (reader == nullptr || reader->input == nullptr)
            return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;

        return reader->input->setPosition ((int64) absoluteByteOffset)
                 ? FLAC__STREAM_DECODER_SEEK_STATUS_OK
                 : FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
    }

    static FLAC__StreamDecoderTellStatus tellCallback (const FLAC__StreamDecoder*,
                                                       FLAC__uint64* absoluteByteOffset,
                                                       void* clientData)
    {
        auto* reader = static_cast<FlacAudioFormatReader*> (clientData);
        if (reader == nullptr || reader->input == nullptr || absoluteByteOffset == nullptr)
            return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;

        *absoluteByteOffset = (FLAC__uint64) reader->input->getPosition();
        return FLAC__STREAM_DECODER_TELL_STATUS_OK;
    }

    static FLAC__StreamDecoderLengthStatus lengthCallback (const FLAC__StreamDecoder*,
                                                           FLAC__uint64* streamLength,
                                                           void* clientData)
    {
        auto* reader = static_cast<FlacAudioFormatReader*> (clientData);
        if (reader == nullptr || reader->input == nullptr || streamLength == nullptr)
            return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;

        const auto length = reader->input->getTotalLength();
        if (length < 0)
            return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;

        *streamLength = (FLAC__uint64) length;
        return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
    }

    static FLAC__bool eofCallback (const FLAC__StreamDecoder*, void* clientData)
    {
        auto* reader = static_cast<FlacAudioFormatReader*> (clientData);
        return reader != nullptr && reader->input != nullptr && reader->input->isExhausted();
    }

    static FLAC__StreamDecoderWriteStatus writeCallback (const FLAC__StreamDecoder*,
                                                         const FLAC__Frame* frame,
                                                         const FLAC__int32* const buffer[],
                                                         void* clientData)
    {
        auto* reader = static_cast<FlacAudioFormatReader*> (clientData);
        if (reader == nullptr || frame == nullptr || buffer == nullptr || reader->currentReadState == nullptr)
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

        auto& state = *reader->currentReadState;
        const int samplesAvailable = (int) frame->header.blocksize;
        const int samplesRemaining = state.numSamples - state.samplesWritten;
        const int samplesToCopy = jmin (samplesAvailable, samplesRemaining);

        if (samplesToCopy <= 0)
            return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;

        const int numChannelsToCopy = jmin (state.numDestChannels, reader->numChannels);

        for (int ch = 0; ch < numChannelsToCopy; ++ch)
        {
            if (state.destChannels[ch] == nullptr)
                continue;

            float* dest = state.destChannels[ch] + state.startOffset + state.samplesWritten;
            const FLAC__int32* src = buffer[ch];

            for (int i = 0; i < samplesToCopy; ++i)
                dest[i] = flacIntToFloat (src[i], reader->bitsPerSample);
        }

        state.samplesWritten += samplesToCopy;
        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    }

    static void metadataCallback (const FLAC__StreamDecoder*,
                                  const FLAC__StreamMetadata* metadata,
                                  void* clientData)
    {
        auto* reader = static_cast<FlacAudioFormatReader*> (clientData);
        if (reader == nullptr || metadata == nullptr)
            return;

        if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
        {
            const auto& info = metadata->data.stream_info;
            reader->sampleRate = (double) info.sample_rate;
            reader->bitsPerSample = (int) info.bits_per_sample;
            reader->numChannels = (int) info.channels;
            reader->lengthInSamples = (int64) info.total_samples;
        }
        else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
        {
            const auto& comment = metadata->data.vorbis_comment;
            for (FLAC__uint32 i = 0; i < comment.num_comments; ++i)
            {
                const auto& entry = comment.comments[i];
                if (entry.entry == nullptr || entry.length == 0)
                    continue;

                const auto text = String::fromUTF8 (reinterpret_cast<const char*> (entry.entry),
                                                    (int) entry.length);
                const auto separatorIndex = text.indexOfChar ('=');
                if (separatorIndex > 0)
                {
                    const auto key = text.substring (0, separatorIndex).toLowerCase();
                    const auto value = text.substring (separatorIndex + 1);
                    if (key.isNotEmpty())
                        reader->metadataValues.set (key, value);
                }
            }
        }
    }

    static void errorCallback (const FLAC__StreamDecoder*,
                               FLAC__StreamDecoderErrorStatus,
                               void*)
    {
    }

    FLAC__StreamDecoder* decoder = nullptr;
    ReadState* currentReadState = nullptr;
    bool isOpen = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FlacAudioFormatReader)
};

FlacAudioFormatReader::FlacAudioFormatReader (InputStream* sourceStream)
    : AudioFormatReader (sourceStream, "FLAC audio")
{
    usesFloatingPointData = false;

    if (sourceStream == nullptr)
        return;

    decoder = FLAC__stream_decoder_new();
    if (decoder == nullptr)
        return;

    FLAC__stream_decoder_set_metadata_respond (decoder, FLAC__METADATA_TYPE_STREAMINFO);
    FLAC__stream_decoder_set_metadata_respond (decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT);

    const auto initStatus = FLAC__stream_decoder_init_stream (decoder,
                                                              readCallback,
                                                              seekCallback,
                                                              tellCallback,
                                                              lengthCallback,
                                                              eofCallback,
                                                              writeCallback,
                                                              metadataCallback,
                                                              errorCallback,
                                                              this);

    if (initStatus != FLAC__STREAM_DECODER_INIT_STATUS_OK)
        return;

    if (! FLAC__stream_decoder_process_until_end_of_metadata (decoder))
        return;

    isOpen = sampleRate > 0 && numChannels > 0 && bitsPerSample > 0;
}

FlacAudioFormatReader::~FlacAudioFormatReader()
{
    if (decoder != nullptr)
    {
        FLAC__stream_decoder_finish (decoder);
        FLAC__stream_decoder_delete (decoder);
    }
}

bool FlacAudioFormatReader::readSamples (float* const* destChannels,
                                         int numDestChannels,
                                         int startOffsetInDestBuffer,
                                         int64 startSampleInFile,
                                         int numSamples)
{
    if (! isOpen || decoder == nullptr)
        return false;

    if (numSamples <= 0)
        return true;

    if (startSampleInFile < 0)
        return false;

    int64 availableSamples = lengthInSamples > 0 ? (lengthInSamples - startSampleInFile) : numSamples;
    const int samplesToRead = (int) jmax<int64> (0, jmin<int64> (availableSamples, numSamples));

    if (samplesToRead <= 0)
        return false;

    if (! FLAC__stream_decoder_seek_absolute (decoder, (FLAC__uint64) startSampleInFile))
        return false;

    ReadState state;
    state.destChannels = destChannels;
    state.numDestChannels = numDestChannels;
    state.startOffset = startOffsetInDestBuffer;
    state.numSamples = samplesToRead;
    state.samplesWritten = 0;
    currentReadState = &state;

    while (state.samplesWritten < samplesToRead)
    {
        if (! FLAC__stream_decoder_process_single (decoder))
            break;

        const auto decoderState = FLAC__stream_decoder_get_state (decoder);
        if (decoderState == FLAC__STREAM_DECODER_END_OF_STREAM)
            break;
    }

    currentReadState = nullptr;

    const int numChannelsToCopy = jmin (numDestChannels, numChannels);
    const int missingSamples = samplesToRead - state.samplesWritten;

    if (missingSamples > 0)
    {
        for (int ch = 0; ch < numChannelsToCopy; ++ch)
            if (destChannels[ch] != nullptr)
                zeromem (destChannels[ch] + startOffsetInDestBuffer + state.samplesWritten,
                         sizeof (float) * (size_t) missingSamples);
    }

    if (numSamples > samplesToRead)
    {
        const int remainder = numSamples - samplesToRead;
        for (int ch = 0; ch < numDestChannels; ++ch)
            if (destChannels[ch] != nullptr)
                zeromem (destChannels[ch] + startOffsetInDestBuffer + samplesToRead,
                         sizeof (float) * (size_t) remainder);
    }

    if (numDestChannels > numChannelsToCopy)
    {
        for (int ch = numChannelsToCopy; ch < numDestChannels; ++ch)
            if (destChannels[ch] != nullptr)
                zeromem (destChannels[ch] + startOffsetInDestBuffer,
                         sizeof (float) * (size_t) numSamples);
    }

    return state.samplesWritten > 0;
}

//==============================================================================
class FlacAudioFormatWriter : public AudioFormatWriter
{
public:
    FlacAudioFormatWriter (OutputStream* destStream,
                           double sampleRate,
                           int numberOfChannels,
                           int bitsPerSample,
                           const StringPairArray& metadataValues,
                           int qualityOptionIndex);
    ~FlacAudioFormatWriter() override;

    bool write (const float* const* samplesToWrite, int numSamples) override;

    bool flush() override;

private:
    static FLAC__StreamEncoderWriteStatus writeCallback (const FLAC__StreamEncoder*,
                                                         const FLAC__byte buffer[],
                                                         size_t bytes,
                                                         unsigned,
                                                         unsigned,
                                                         void* clientData)
    {
        auto* writer = static_cast<FlacAudioFormatWriter*> (clientData);
        if (writer == nullptr || writer->output == nullptr)
            return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;

        return writer->output->write (buffer, bytes)
                 ? FLAC__STREAM_ENCODER_WRITE_STATUS_OK
                 : FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
    }

    static FLAC__StreamEncoderSeekStatus seekCallback (const FLAC__StreamEncoder*,
                                                       FLAC__uint64 absoluteByteOffset,
                                                       void* clientData)
    {
        auto* writer = static_cast<FlacAudioFormatWriter*> (clientData);
        if (writer == nullptr || writer->output == nullptr)
            return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;

        if (! writer->output->setPosition ((int64) absoluteByteOffset))
            return FLAC__STREAM_ENCODER_SEEK_STATUS_UNSUPPORTED;

        return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
    }

    static FLAC__StreamEncoderTellStatus tellCallback (const FLAC__StreamEncoder*,
                                                       FLAC__uint64* absoluteByteOffset,
                                                       void* clientData)
    {
        auto* writer = static_cast<FlacAudioFormatWriter*> (clientData);
        if (writer == nullptr || writer->output == nullptr || absoluteByteOffset == nullptr)
            return FLAC__STREAM_ENCODER_TELL_STATUS_ERROR;

        *absoluteByteOffset = (FLAC__uint64) writer->output->getPosition();
        return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
    }

    FLAC__StreamEncoder* encoder = nullptr;
    std::vector<FLAC__int32> interleavedBuffer;
    bool isOpen = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FlacAudioFormatWriter)
};

FlacAudioFormatWriter::FlacAudioFormatWriter (OutputStream* destStream,
                                              double sampleRate,
                                              int numberOfChannels,
                                              int bitsPerSample,
                                              const StringPairArray&,
                                              int qualityOptionIndex)
    : AudioFormatWriter (destStream, "FLAC audio", sampleRate, numberOfChannels, bitsPerSample)
{
    if (destStream == nullptr)
        return;

    encoder = FLAC__stream_encoder_new();
    if (encoder == nullptr)
        return;

    FLAC__stream_encoder_set_channels (encoder, (unsigned) numberOfChannels);
    FLAC__stream_encoder_set_bits_per_sample (encoder, (unsigned) bitsPerSample);
    FLAC__stream_encoder_set_sample_rate (encoder, (unsigned) sampleRate);

    const auto compressionLevel = (unsigned) jlimit (0, 8, qualityOptionIndex);
    FLAC__stream_encoder_set_compression_level (encoder, compressionLevel);

    const auto initStatus = FLAC__stream_encoder_init_stream (encoder,
                                                              writeCallback,
                                                              seekCallback,
                                                              tellCallback,
                                                              nullptr,
                                                              this);

    if (initStatus != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
        return;

    isOpen = true;
}

FlacAudioFormatWriter::~FlacAudioFormatWriter()
{
    if (encoder != nullptr)
    {
        FLAC__stream_encoder_finish (encoder);
        FLAC__stream_encoder_delete (encoder);
    }
}

bool FlacAudioFormatWriter::write (const float* const* samplesToWrite, int numSamples)
{
    if (! isOpen || encoder == nullptr || numSamples <= 0)
        return false;

    const auto numChannels = getNumChannels();
    const size_t totalSamples = (size_t) numSamples * (size_t) numChannels;

    if (interleavedBuffer.size() < totalSamples)
        interleavedBuffer.resize (totalSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        const size_t baseIndex = (size_t) i * (size_t) numChannels;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float sample = samplesToWrite[ch] != nullptr ? samplesToWrite[ch][i] : 0.0f;
            interleavedBuffer[baseIndex + (size_t) ch] = floatToFlacInt (sample, getBitsPerSample());
        }
    }

    return FLAC__stream_encoder_process_interleaved (encoder,
                                                     interleavedBuffer.data(),
                                                     (unsigned) numSamples)
        == true;
}

bool FlacAudioFormatWriter::flush()
{
    if (output != nullptr)
    {
        output->flush();
        return true;
    }
    return false;
}

} // namespace

//==============================================================================
// FlacAudioFormat implementation
FlacAudioFormat::FlacAudioFormat()
    : formatName ("FLAC audio")
{
}

FlacAudioFormat::~FlacAudioFormat() = default;

const String& FlacAudioFormat::getFormatName() const
{
    return formatName;
}

Array<String> FlacAudioFormat::getFileExtensions() const
{
    return { ".flac" };
}

std::unique_ptr<AudioFormatReader> FlacAudioFormat::createReaderFor (InputStream* sourceStream)
{
    auto reader = std::make_unique<FlacAudioFormatReader> (sourceStream);

    if (reader->sampleRate > 0 && reader->numChannels > 0)
        return reader;

    return nullptr;
}

std::unique_ptr<AudioFormatWriter> FlacAudioFormat::createWriterFor (OutputStream* streamToWriteTo,
                                                                     double sampleRate,
                                                                     int numberOfChannels,
                                                                     int bitsPerSample,
                                                                     const StringPairArray& metadataValues,
                                                                     int qualityOptionIndex)
{
    if (streamToWriteTo == nullptr)
        return nullptr;

    if (numberOfChannels <= 0 || numberOfChannels > 8)
        return nullptr;

    if (sampleRate <= 0 || sampleRate > 655350)
        return nullptr;

    if (bitsPerSample != 8 && bitsPerSample != 16 && bitsPerSample != 24 && bitsPerSample != 32)
        return nullptr;

    return std::make_unique<FlacAudioFormatWriter> (streamToWriteTo,
                                                    sampleRate,
                                                    numberOfChannels,
                                                    bitsPerSample,
                                                    metadataValues,
                                                    qualityOptionIndex);
}

Array<int> FlacAudioFormat::getPossibleBitDepths() const
{
    return { 8, 16, 24, 32 };
}

Array<int> FlacAudioFormat::getPossibleSampleRates() const
{
    return { 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 88200, 96000, 176400, 192000 };
}

StringArray FlacAudioFormat::getQualityOptions() const
{
    return { "0 - Fastest", "1 - Very fast", "2 - Fast", "3 - Medium", "4 - Medium", "5 - Default", "6 - High", "7 - Very high", "8 - Highest" };
}

} // namespace yup
