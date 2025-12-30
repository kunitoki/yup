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

struct OggPage
{
    uint8 headerType = 0;
    int64 granulePosition = -1;
    uint32 serialNumber = 0;
    uint32 sequenceNumber = 0;
    std::vector<uint8> segmentTable;
    std::vector<uint8> data;
};

struct OpusHeader
{
    int channels = 0;
    int preSkip = 0;
    int inputSampleRate = 48000;
    int outputGain = 0;
    int mappingFamily = 0;
};

static uint16 readLittleEndian16 (const uint8* data)
{
    return (uint16) data[0] | (uint16) (data[1] << 8);
}

static uint32 readLittleEndian32 (const uint8* data)
{
    return (uint32) data[0]
         | (uint32) (data[1] << 8)
         | (uint32) (data[2] << 16)
         | (uint32) (data[3] << 24);
}

static int64 readLittleEndian64 (const uint8* data)
{
    const uint64 lo = readLittleEndian32 (data);
    const uint64 hi = readLittleEndian32 (data + 4);
    return (int64) (lo | (hi << 32));
}

static bool readExact (InputStream& input, void* dest, size_t numBytes)
{
    return input.read (dest, (int) numBytes) == (int) numBytes;
}

static bool readOggPage (InputStream& input, OggPage& page)
{
    uint8 header[27] = {};
    if (! readExact (input, header, sizeof (header)))
        return false;

    if (std::memcmp (header, "OggS", 4) != 0)
        return false;

    if (header[4] != 0)
        return false;

    page.headerType = header[5];
    page.granulePosition = readLittleEndian64 (header + 6);
    page.serialNumber = readLittleEndian32 (header + 14);
    page.sequenceNumber = readLittleEndian32 (header + 18);
    const uint8 segmentCount = header[26];

    page.segmentTable.resize (segmentCount);
    if (segmentCount > 0 && ! readExact (input, page.segmentTable.data(), segmentCount))
        return false;

    uint32 bodySize = 0;
    for (auto segmentSize : page.segmentTable)
        bodySize += segmentSize;

    page.data.resize (bodySize);
    if (bodySize > 0 && ! readExact (input, page.data.data(), bodySize))
        return false;

    return true;
}

static void appendPacketsFromPage (const OggPage& page, std::vector<uint8>& currentPacket, std::vector<std::vector<uint8>>& packets)
{
    size_t offset = 0;

    for (auto segmentSize : page.segmentTable)
    {
        if (offset + segmentSize > page.data.size())
            break;

        if (segmentSize > 0)
        {
            const auto* segmentStart = page.data.data() + offset;
            currentPacket.insert (currentPacket.end(), segmentStart, segmentStart + segmentSize);
            offset += segmentSize;
        }

        if (segmentSize < 255)
        {
            packets.push_back (std::move (currentPacket));
            currentPacket.clear();
        }
    }
}

static bool parseOpusHead (const std::vector<uint8>& packet, OpusHeader& header)
{
    if (packet.size() < 19)
        return false;

    if (std::memcmp (packet.data(), "OpusHead", 8) != 0)
        return false;

    header.channels = packet[9];
    header.preSkip = (int) readLittleEndian16 (packet.data() + 10);
    header.inputSampleRate = (int) readLittleEndian32 (packet.data() + 12);
    header.outputGain = (int) readLittleEndian16 (packet.data() + 16);
    header.mappingFamily = packet[18];

    if (header.channels < 1 || header.channels > 2)
        return false;

    if (header.mappingFamily != 0)
        return false;

    return true;
}

static bool parseOpusTags (const std::vector<uint8>& packet, StringPairArray& metadataValues)
{
    if (packet.size() < 16)
        return false;

    if (std::memcmp (packet.data(), "OpusTags", 8) != 0)
        return false;

    size_t offset = 8;
    const auto size = packet.size();

    if (offset + 4 > size)
        return false;

    const uint32 vendorLength = readLittleEndian32 (packet.data() + offset);
    offset += 4;
    if (offset + vendorLength > size)
        return false;

    offset += vendorLength;
    if (offset + 4 > size)
        return false;

    const uint32 commentCount = readLittleEndian32 (packet.data() + offset);
    offset += 4;

    for (uint32 i = 0; i < commentCount; ++i)
    {
        if (offset + 4 > size)
            return false;

        const uint32 commentLength = readLittleEndian32 (packet.data() + offset);
        offset += 4;

        if (offset + commentLength > size)
            return false;

        const auto* commentData = reinterpret_cast<const char*> (packet.data() + offset);
        const auto comment = String::fromUTF8 (commentData, (int) commentLength);
        offset += commentLength;

        const auto separatorIndex = comment.indexOfChar ('=');
        if (separatorIndex > 0)
        {
            const auto key = comment.substring (0, separatorIndex).toLowerCase();
            const auto value = comment.substring (separatorIndex + 1);
            if (key.isNotEmpty())
                metadataValues.set (key, value);
        }
    }

    return true;
}

class OpusAudioFormatReader : public AudioFormatReader
{
public:
    OpusAudioFormatReader (InputStream* sourceStream);
    ~OpusAudioFormatReader() override = default;

    bool readSamples (float* const* destChannels,
                      int numDestChannels,
                      int startOffsetInDestBuffer,
                      int64 startSampleInFile,
                      int numSamples) override;

private:
    bool decodeStream();

    std::vector<float> interleavedSamples;
    bool isValid = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpusAudioFormatReader)
};

OpusAudioFormatReader::OpusAudioFormatReader (InputStream* sourceStream)
    : AudioFormatReader (sourceStream, "Opus audio")
{
    usesFloatingPointData = true;
    bitsPerSample = 32;

    if (sourceStream == nullptr)
        return;

    isValid = decodeStream();
}

bool OpusAudioFormatReader::decodeStream()
{
    if (input == nullptr)
        return false;

    input->setPosition (0);

    std::vector<std::vector<uint8>> packets;
    std::vector<uint8> currentPacket;
    OpusHeader header;
    bool hasHeader = false;
    bool hasTags = false;
    uint32 streamSerial = 0;
    int64 lastGranulePosition = -1;

    while (! input->isExhausted())
    {
        OggPage page;
        if (! readOggPage (*input, page))
            break;

        if (! hasHeader)
        {
            if (page.segmentTable.empty())
                continue;

            std::vector<std::vector<uint8>> pagePackets;
            appendPacketsFromPage (page, currentPacket, pagePackets);
            if (pagePackets.empty())
                continue;

            if (! parseOpusHead (pagePackets.front(), header))
                return false;

            streamSerial = page.serialNumber;
            hasHeader = true;

            for (size_t i = 1; i < pagePackets.size(); ++i)
                packets.push_back (std::move (pagePackets[i]));
        }
        else if (page.serialNumber == streamSerial)
        {
            appendPacketsFromPage (page, currentPacket, packets);

            if (page.granulePosition >= 0)
                lastGranulePosition = page.granulePosition;

            if ((page.headerType & 0x04) != 0)
                break;
        }
    }

    if (! hasHeader || packets.empty())
        return false;

    if (! packets.empty())
        hasTags = parseOpusTags (packets.front(), metadataValues);

    const int decodeSampleRate = 48000;
    int opusError = OPUS_OK;
    OpusDecoder* decoder = opus_decoder_create (decodeSampleRate, header.channels, &opusError);

    if (decoder == nullptr || opusError != OPUS_OK)
        return false;

    const int maxFrameSize = 5760;
    std::vector<float> decodeBuffer ((size_t) maxFrameSize * (size_t) header.channels);
    int64 samplesToSkip = header.preSkip;

    for (size_t i = hasTags ? 1 : 0; i < packets.size(); ++i)
    {
        const auto& packet = packets[i];
        if (packet.empty())
            continue;

        const int decodedSamples = opus_decode_float (decoder,
                                                      packet.data(),
                                                      (opus_int32) packet.size(),
                                                      decodeBuffer.data(),
                                                      maxFrameSize,
                                                      0);

        if (decodedSamples < 0)
        {
            opus_decoder_destroy (decoder);
            return false;
        }

        int decodedOffset = 0;
        int decodedAvailable = decodedSamples;

        if (samplesToSkip > 0)
        {
            const int skipNow = (int) jmin<int64> (samplesToSkip, decodedSamples);
            decodedOffset = skipNow;
            decodedAvailable -= skipNow;
            samplesToSkip -= skipNow;
        }

        if (decodedAvailable > 0)
        {
            const size_t offsetSamples = (size_t) decodedOffset * (size_t) header.channels;
            const size_t availableSamples = (size_t) decodedAvailable * (size_t) header.channels;
            const auto* start = decodeBuffer.data() + offsetSamples;
            interleavedSamples.insert (interleavedSamples.end(), start, start + availableSamples);
        }
    }

    opus_decoder_destroy (decoder);

    const int64 totalSamples = (int64) interleavedSamples.size() / header.channels;
    if (lastGranulePosition > 0 && lastGranulePosition < totalSamples)
        interleavedSamples.resize ((size_t) lastGranulePosition * (size_t) header.channels);

    sampleRate = decodeSampleRate;
    numChannels = header.channels;
    lengthInSamples = (int64) interleavedSamples.size() / numChannels;

    return lengthInSamples > 0;
}

bool OpusAudioFormatReader::readSamples (float* const* destChannels,
                                         int numDestChannels,
                                         int startOffsetInDestBuffer,
                                         int64 startSampleInFile,
                                         int numSamples)
{
    if (! isValid || numSamples <= 0)
        return false;

    if (startSampleInFile < 0 || startSampleInFile >= lengthInSamples)
        return false;

    const auto numChannelsToRead = jmin (numDestChannels, numChannels);
    const auto availableSamples = (int64) lengthInSamples - startSampleInFile;
    const auto samplesToCopy = (int) jmin<int64> (availableSamples, numSamples);

    if (samplesToCopy <= 0)
        return false;

    HeapBlock<float*> offsetDestChannels;
    offsetDestChannels.malloc (numDestChannels);

    for (int ch = 0; ch < numDestChannels; ++ch)
        offsetDestChannels[ch] = destChannels[ch] + startOffsetInDestBuffer;

    const size_t interleavedOffset = (size_t) startSampleInFile * (size_t) numChannels;
    const float* interleavedStart = interleavedSamples.data() + interleavedOffset;

    using SourceFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
    using DestFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;

    AudioData::deinterleaveSamples (AudioData::InterleavedSource<SourceFormat> { interleavedStart, (int) numChannels },
                                    AudioData::NonInterleavedDest<DestFormat> { offsetDestChannels.getData(), numChannelsToRead },
                                    samplesToCopy);

    if (numDestChannels > numChannelsToRead)
    {
        for (int ch = numChannelsToRead; ch < numDestChannels; ++ch)
            if (offsetDestChannels[ch] != nullptr)
                zeromem (offsetDestChannels[ch], sizeof (float) * (size_t) samplesToCopy);
    }

    if (samplesToCopy < numSamples)
    {
        const auto remaining = numSamples - samplesToCopy;
        for (int ch = 0; ch < numDestChannels; ++ch)
            if (offsetDestChannels[ch] != nullptr)
                zeromem (offsetDestChannels[ch] + samplesToCopy, sizeof (float) * (size_t) remaining);
    }

    return true;
}

class OpusAudioFormatWriter : public AudioFormatWriter
{
public:
    OpusAudioFormatWriter (OutputStream* destStream,
                           double sampleRate,
                           int numberOfChannels,
                           int bitsPerSample,
                           const StringPairArray& metadataValues,
                           int qualityOptionIndex);
    ~OpusAudioFormatWriter() override;

    bool write (const float* const* samplesToWrite, int numSamples) override;
    bool flush() override;

private:
    struct OggPageWriter
    {
        explicit OggPageWriter (OutputStream* outputStream, uint32 serial)
            : output (outputStream)
            , serialNumber (serial)
        {
            initCrcTable();
        }

        bool writePacket (const uint8* data, size_t size, int64 granulePosition, bool isBOS, bool isEOS)
        {
            if (output == nullptr)
                return false;

            size_t offset = 0;
            bool firstPage = true;

            while (offset < size)
            {
                const size_t remaining = size - offset;
                const size_t maxPageBytes = 255u * 255u;
                const size_t pageBytes = remaining > maxPageBytes ? maxPageBytes : remaining;
                const size_t segmentCount = (pageBytes + 254u) / 255u;

                uint8 segmentTable[255] = {};
                for (size_t i = 0; i < segmentCount; ++i)
                {
                    const size_t segmentSize = (i + 1 == segmentCount) ? (pageBytes - (i * 255u)) : 255u;
                    segmentTable[i] = (uint8) segmentSize;
                }

                uint8 headerType = 0;
                if (! firstPage)
                    headerType |= 0x01;
                if (firstPage && isBOS)
                    headerType |= 0x02;

                const bool isLastPage = (offset + pageBytes) == size;
                if (isLastPage && isEOS)
                    headerType |= 0x04;

                const int64 pageGranule = isLastPage ? granulePosition : -1;

                if (! writePage (segmentTable,
                                 (uint8) segmentCount,
                                 data + offset,
                                 pageBytes,
                                 headerType,
                                 pageGranule))
                    return false;

                offset += pageBytes;
                firstPage = false;
            }

            return true;
        }

    private:
        OutputStream* output = nullptr;
        uint32 serialNumber = 0;
        uint32 sequenceNumber = 0;

        static uint32 crcTable[256];
        static bool crcTableInitialized;

        static void initCrcTable()
        {
            if (crcTableInitialized)
                return;

            for (uint32 i = 0; i < 256; ++i)
            {
                uint32 r = i << 24;
                for (int j = 0; j < 8; ++j)
                    r = (r & 0x80000000u) ? ((r << 1) ^ 0x04C11DB7u) : (r << 1);
                crcTable[i] = r;
            }

            crcTableInitialized = true;
        }

        static uint32 updateCrc (uint32 crc, const uint8* data, size_t size)
        {
            for (size_t i = 0; i < size; ++i)
                crc = (crc << 8) ^ crcTable[((crc >> 24) & 0xff) ^ data[i]];
            return crc;
        }

        bool writePage (const uint8* segmentTable,
                        uint8 segmentCount,
                        const uint8* data,
                        size_t dataSize,
                        uint8 headerType,
                        int64 granulePosition)
        {
            const size_t headerSize = 27u + segmentCount;
            std::vector<uint8> header (headerSize, 0);

            std::memcpy (header.data(), "OggS", 4);
            header[4] = 0;
            header[5] = headerType;

            const uint64 granule = (granulePosition < 0) ? 0xffffffffffffffffull : (uint64) granulePosition;
            for (int i = 0; i < 8; ++i)
                header[6 + i] = (uint8) ((granule >> (8 * i)) & 0xff);

            for (int i = 0; i < 4; ++i)
                header[14 + i] = (uint8) ((serialNumber >> (8 * i)) & 0xff);

            for (int i = 0; i < 4; ++i)
                header[18 + i] = (uint8) ((sequenceNumber >> (8 * i)) & 0xff);

            header[26] = segmentCount;
            if (segmentCount > 0)
                std::memcpy (header.data() + 27, segmentTable, segmentCount);

            uint32 crc = 0;
            crc = updateCrc (crc, header.data(), header.size());
            crc = updateCrc (crc, data, dataSize);

            for (int i = 0; i < 4; ++i)
                header[22 + i] = (uint8) ((crc >> (8 * i)) & 0xff);

            ++sequenceNumber;

            if (! output->write (header.data(), header.size()))
                return false;

            if (dataSize > 0 && ! output->write (data, dataSize))
                return false;

            return true;
        }
    };

    bool writeHeaders (const StringPairArray& metadataValues);
    bool encodeFrames (bool flushPending);
    bool writeOpusPacket (const uint8* data, size_t size, int64 granulePosition, bool isEOS);

    OpusEncoder* encoder = nullptr;
    std::vector<float> pendingInterleaved;
    size_t pendingOffset = 0;
    std::vector<uint8> packetBuffer;
    OggPageWriter oggWriter;
    int frameSize = 960;
    int numChannelsInternal = 0;
    int preSkip = 0;
    int64 totalInputSamples = 0;
    int64 totalEncodedSamples = 0;
    bool wroteHeaders = false;
    bool wroteAudio = false;
    bool finished = false;
    bool isOpen = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpusAudioFormatWriter)
};

uint32 OpusAudioFormatWriter::OggPageWriter::crcTable[256] = {};
bool OpusAudioFormatWriter::OggPageWriter::crcTableInitialized = false;

static void appendLE16 (std::vector<uint8>& buffer, uint16 value)
{
    buffer.push_back ((uint8) (value & 0xff));
    buffer.push_back ((uint8) ((value >> 8) & 0xff));
}

static void appendLE32 (std::vector<uint8>& buffer, uint32 value)
{
    buffer.push_back ((uint8) (value & 0xff));
    buffer.push_back ((uint8) ((value >> 8) & 0xff));
    buffer.push_back ((uint8) ((value >> 16) & 0xff));
    buffer.push_back ((uint8) ((value >> 24) & 0xff));
}

OpusAudioFormatWriter::OpusAudioFormatWriter (OutputStream* destStream,
                                              double sampleRate,
                                              int numberOfChannels,
                                              int bitsPerSample,
                                              const StringPairArray& metadataValues,
                                              int qualityOptionIndex)
    : AudioFormatWriter (destStream, "Opus audio", sampleRate, numberOfChannels, bitsPerSample)
    , packetBuffer (4000)
    , oggWriter (destStream, (uint32) Random::getSystemRandom().nextInt())
    , numChannelsInternal (numberOfChannels)
{
    ignoreUnused (qualityOptionIndex);

    if (destStream == nullptr || numberOfChannels < 1 || numberOfChannels > 2)
        return;

    if (bitsPerSample != 32 || sampleRate != 48000.0)
        return;

    int opusError = OPUS_OK;
    encoder = opus_encoder_create ((opus_int32) sampleRate, numberOfChannels, OPUS_APPLICATION_AUDIO, &opusError);
    if (encoder == nullptr || opusError != OPUS_OK)
        return;

    opus_encoder_ctl (encoder, OPUS_SET_VBR (1));
    opus_encoder_ctl (encoder, OPUS_SET_VBR_CONSTRAINT (0));
    opus_encoder_ctl (encoder, OPUS_SET_COMPLEXITY (10));
    opus_encoder_ctl (encoder, OPUS_SET_SIGNAL (OPUS_SIGNAL_MUSIC));

    const int targetBitrate = 128000 * numberOfChannels;
    opus_encoder_ctl (encoder, OPUS_SET_BITRATE (targetBitrate));

    int lookahead = 0;
    if (opus_encoder_ctl (encoder, OPUS_GET_LOOKAHEAD (&lookahead)) == OPUS_OK)
        preSkip = lookahead;

    isOpen = writeHeaders (metadataValues);
}

OpusAudioFormatWriter::~OpusAudioFormatWriter()
{
    if (isOpen && ! finished)
        flush();

    if (encoder != nullptr)
        opus_encoder_destroy (encoder);
}

bool OpusAudioFormatWriter::writeHeaders (const StringPairArray& metadataValues)
{
    std::vector<uint8> headerPacket;
    headerPacket.insert (headerPacket.end(), { 'O', 'p', 'u', 's', 'H', 'e', 'a', 'd' });
    headerPacket.push_back (1);
    headerPacket.push_back ((uint8) numChannelsInternal);
    appendLE16 (headerPacket, (uint16) preSkip);
    appendLE32 (headerPacket, (uint32) getSampleRate());
    appendLE16 (headerPacket, 0);
    headerPacket.push_back (0);

    if (! oggWriter.writePacket (headerPacket.data(), headerPacket.size(), 0, true, false))
        return false;

    std::vector<uint8> tagsPacket;
    tagsPacket.insert (tagsPacket.end(), { 'O', 'p', 'u', 's', 'T', 'a', 'g', 's' });

    const String vendorString ("YUP");
    const auto vendorBytes = vendorString.toUTF8();
    appendLE32 (tagsPacket, (uint32) vendorBytes.sizeInBytes() - 1);
    tagsPacket.insert (tagsPacket.end(),
                       vendorBytes.getAddress(),
                       vendorBytes.getAddress() + (vendorBytes.sizeInBytes() - 1));

    std::vector<String> comments;
    comments.reserve ((size_t) metadataValues.size() + 1);

    for (const auto& pair : metadataValues)
    {
        if (pair.key.isNotEmpty() && pair.value.isNotEmpty())
            comments.push_back (pair.key + "=" + pair.value);
    }

    comments.push_back ("ENCODER=YUP");

    appendLE32 (tagsPacket, (uint32) comments.size());

    for (const auto& comment : comments)
    {
        const auto commentBytes = comment.toUTF8();
        appendLE32 (tagsPacket, (uint32) commentBytes.sizeInBytes() - 1);
        tagsPacket.insert (tagsPacket.end(),
                           commentBytes.getAddress(),
                           commentBytes.getAddress() + (commentBytes.sizeInBytes() - 1));
    }

    if (! oggWriter.writePacket (tagsPacket.data(), tagsPacket.size(), 0, false, false))
        return false;

    wroteHeaders = true;
    return true;
}

bool OpusAudioFormatWriter::write (const float* const* samplesToWrite, int numSamples)
{
    if (! isOpen || encoder == nullptr || numSamples <= 0)
        return false;

    const size_t framesToAppend = (size_t) numSamples;
    const size_t interleavedCount = framesToAppend * (size_t) numChannelsInternal;
    const size_t oldSize = pendingInterleaved.size() - pendingOffset;

    if (pendingOffset > 0 && oldSize > 0)
    {
        pendingInterleaved.erase (pendingInterleaved.begin(), pendingInterleaved.begin() + (int) pendingOffset);
        pendingOffset = 0;
    }

    const size_t startOffset = pendingInterleaved.size();
    pendingInterleaved.resize (startOffset + interleavedCount);

    using SourceFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
    using DestFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;

    AudioData::interleaveSamples (AudioData::NonInterleavedSource<SourceFormat> { samplesToWrite, numChannelsInternal },
                                  AudioData::InterleavedDest<DestFormat> { pendingInterleaved.data() + startOffset, numChannelsInternal },
                                  numSamples);

    totalInputSamples += numSamples;

    return encodeFrames (false);
}

bool OpusAudioFormatWriter::encodeFrames (bool flushPending)
{
    const size_t samplesPerFrame = (size_t) frameSize * (size_t) numChannelsInternal;

    while (true)
    {
        const size_t available = pendingInterleaved.size() - pendingOffset;
        if (available == 0)
            break;

        if (! flushPending && available < samplesPerFrame)
            break;

        const bool isLast = flushPending && available < samplesPerFrame;
        std::vector<float> tempFrame;
        const float* frameData = pendingInterleaved.data() + pendingOffset;

        if (isLast)
        {
            tempFrame.resize (samplesPerFrame, 0.0f);
            std::memcpy (tempFrame.data(), frameData, available * sizeof (float));
            frameData = tempFrame.data();
        }

        const int encodedBytes = opus_encode_float (encoder,
                                                    frameData,
                                                    frameSize,
                                                    packetBuffer.data(),
                                                    (opus_int32) packetBuffer.size());

        if (encodedBytes < 0)
            return false;

        if (! isLast)
            pendingOffset += samplesPerFrame;
        else
            pendingOffset += available;

        totalEncodedSamples += frameSize;
        wroteAudio = true;

        const int64 granulePosition = preSkip + (isLast ? totalInputSamples : totalEncodedSamples);
        if (! writeOpusPacket (packetBuffer.data(), (size_t) encodedBytes, granulePosition, isLast))
            return false;

        if (isLast)
            break;
    }

    if (pendingOffset > 0 && pendingOffset >= pendingInterleaved.size())
    {
        pendingInterleaved.clear();
        pendingOffset = 0;
    }

    return true;
}

bool OpusAudioFormatWriter::writeOpusPacket (const uint8* data, size_t size, int64 granulePosition, bool isEOS)
{
    return oggWriter.writePacket (data, size, granulePosition, false, isEOS);
}

bool OpusAudioFormatWriter::flush()
{
    if (! isOpen || finished)
        return false;

    const bool ok = encodeFrames (true);
    finished = true;

    if (output != nullptr)
        output->flush();

    return ok;
}

} // namespace

//==============================================================================
// OpusAudioFormat implementation
OpusAudioFormat::OpusAudioFormat()
    : formatName ("Opus audio")
{
}

OpusAudioFormat::~OpusAudioFormat() = default;

const String& OpusAudioFormat::getFormatName() const
{
    return formatName;
}

Array<String> OpusAudioFormat::getFileExtensions() const
{
    return { ".opus" };
}

std::unique_ptr<AudioFormatReader> OpusAudioFormat::createReaderFor (InputStream* sourceStream)
{
    auto reader = std::make_unique<OpusAudioFormatReader> (sourceStream);

    if (reader->sampleRate > 0 && reader->numChannels > 0)
        return reader;

    return nullptr;
}

std::unique_ptr<AudioFormatWriter> OpusAudioFormat::createWriterFor (OutputStream* streamToWriteTo,
                                                                     double sampleRate,
                                                                     int numberOfChannels,
                                                                     int bitsPerSample,
                                                                     const StringPairArray& metadataValues,
                                                                     int qualityOptionIndex)
{
    if (streamToWriteTo == nullptr)
        return nullptr;

    if (numberOfChannels < 1 || numberOfChannels > 2)
        return nullptr;

    if (sampleRate != 48000.0)
        return nullptr;

    if (bitsPerSample != 32)
        return nullptr;

    return std::make_unique<OpusAudioFormatWriter> (streamToWriteTo,
                                                    sampleRate,
                                                    numberOfChannels,
                                                    bitsPerSample,
                                                    metadataValues,
                                                    qualityOptionIndex);
}

Array<int> OpusAudioFormat::getPossibleBitDepths() const
{
    return { 32 };
}

Array<int> OpusAudioFormat::getPossibleSampleRates() const
{
    return { 48000 };
}

} // namespace yup
