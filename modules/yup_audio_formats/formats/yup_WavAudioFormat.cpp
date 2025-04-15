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

namespace yup {

//==============================================================================

struct WAVHeader
{
    char chunkID[4];      // "RIFF"
    uint32 chunkSize;     // 4 + (8 + subchunk1Size) + (8 + subchunk2Size)
    char format[4];       // "WAVE"
    char subchunk1ID[4];  // "fmt "
    uint32 subchunk1Size; // 16 for PCM
    uint16 audioFormat;   // 1 for PCM
    uint16 numChannels;
    uint32 sampleRate;
    uint32 byteRate;      // sampleRate * numChannels * bitsPerSample/8
    uint16 blockAlign;    // numChannels * bitsPerSample/8
    uint16 bitsPerSample;
    char subchunk2ID[4];  // "data"
    uint32 subchunk2Size; // Number of data bytes
};

//==============================================================================

class WAVAudioFormatReader : public AudioFormatReader
{
public:
    explicit WAVAudioFormatReader (const std::string& filePath)
        : stream (filePath, std::ios::binary)
        , dataOffset (0)
    {
        if (!stream)
            throw std::runtime_error ("Could not open WAV file for reading.");

        readHeader();
    }

    double getSampleRate() const override
    {
        return header.sampleRate;
    }

    int getNumChannels() const override
    {
        return header.numChannels;
    }

    int64 getTotalSamples() const override
    {
        int blockAlign = header.numChannels * header.bitsPerSample / 8;
        return header.subchunk2Size / blockAlign;
    }

    bool readSamples (AudioSampleBuffer& buffer, int64 startSample, int64 numSamples) override
    {
        if (buffer.getNumChannels() != header.numChannels || buffer.getNumSamples() < numSamples)
            return false;

        int numCh = header.numChannels;
        std::vector<int16_t> tempBuffer (numSamples * numCh);

        if (!readSamples (tempBuffer.data(), startSample, numSamples))
            return false;

        // Deinterleave and convert to float (in range [-1, 1]).
        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            for (int channel = 0; channel < numCh; ++channel)
            {
                int16 value = tempBuffer[sampleIndex * numCh + channel];
                float floatValue = static_cast<float> (value) / 32768.0f;
                buffer.getWritePointer (channel)[sampleIndex] = floatValue;
            }
        }
        return true;
    }

private:
    bool readSamples (void* buffer, int64 startSample, int64 numSamples)
    {
        int blockAlign = header.numChannels * header.bitsPerSample / 8;
        int64 byteOffset = dataOffset + startSample * blockAlign;

        stream.clear();
        stream.seekg (byteOffset, std::ios::beg);
        if (!stream)
            return false;

        size_t bytesToRead = numSamples * blockAlign;
        stream.read(reinterpret_cast<char*> (buffer), bytesToRead);

        return stream.gcount() == static_cast<std::streamsize> (bytesToRead);
    }

    void readHeader()
    {
        stream.seekg (0, std::ios::beg);
        stream.read (reinterpret_cast<char*> (&header), sizeof (WAVHeader));

        if (stream.gcount() != sizeof (WAVHeader))
            throw std::runtime_error ("Failed to read complete WAV header.");

        // Validate header values.
        if (std::strncmp (header.chunkID, "RIFF", 4) != 0 ||
            std::strncmp (header.format, "WAVE", 4) != 0 ||
            std::strncmp (header.subchunk1ID, "fmt ", 4) != 0 ||
            std::strncmp (header.subchunk2ID, "data", 4) != 0)
        {
            throw std::runtime_error ("Invalid or unsupported WAV file.");
        }

        // Standard PCM header: sample data starts immediately after the header.
        dataOffset = sizeof (WAVHeader);
    }

    std::ifstream stream;
    WAVHeader header;
    std::streampos dataOffset;
};

//==============================================================================

class JUCE_API WAVAudioFormatWriter : public AudioFormatWriter
{
public:
    WAVAudioFormatWriter(const std::string& filePath, int sampleRate, int numChannels, int bitsPerSample)
        : stream(filePath, std::ios::binary)
        , sampleRate(sampleRate)
        , numChannels(numChannels)
        , bitsPerSample(bitsPerSample)
        , dataBytesWritten(0)
        , finalized(false)
    {
        if (!stream)
            throw std::runtime_error("Could not open WAV file for writing.");

        writeHeaderPlaceholder();
    }

    bool writeSamples (const AudioSampleBuffer& buffer, int numSamples) override
    {
        if (buffer.getNumChannels() != numChannels || buffer.getNumSamples() < numSamples)
            return false;

        int numCh = numChannels;
        std::vector<int16> tempBuffer (numSamples * numCh);

        // Interleave and convert float to int16_t.
        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            for (int channel = 0; channel < numCh; ++channel)
            {
                float sampleFloat = buffer.getReadPointer (channel)[sampleIndex];

                // Clamp the float value between -1 and 1 if necessary.
                if (sampleFloat > 1.0f)
                    sampleFloat = 1.0f;
                else if (sampleFloat < -1.0f)
                    sampleFloat = -1.0f;

                int16 intSample = static_cast<int16> (std::round (sampleFloat * 32767.0f));
                tempBuffer[sampleIndex * numCh + channel] = intSample;
            }
        }

        return writeSamples (tempBuffer.data(), numSamples);
    }

    bool finalize() override
    {
        if (finalized)
            return true;

        finalized = true;
        stream.flush();
        std::streampos fileSize = stream.tellp();

        uint32_t subchunk2Size = static_cast<uint32_t>(dataBytesWritten);
        uint32_t chunkSize = static_cast<uint32_t>(fileSize) - 8;

        WAVHeader header;
        std::memcpy(header.chunkID, "RIFF", 4);
        header.chunkSize = chunkSize;
        std::memcpy(header.format, "WAVE", 4);

        std::memcpy(header.subchunk1ID, "fmt ", 4);
        header.subchunk1Size = 16;
        header.audioFormat = 1;
        header.numChannels = static_cast<uint16_t>(numChannels);
        header.sampleRate = static_cast<uint32_t>(sampleRate);
        header.bitsPerSample = static_cast<uint16_t>(bitsPerSample);
        header.byteRate = header.sampleRate * header.numChannels * header.bitsPerSample / 8;
        header.blockAlign = static_cast<uint16_t>(header.numChannels * header.bitsPerSample / 8);

        std::memcpy(header.subchunk2ID, "data", 4);
        header.subchunk2Size = subchunk2Size;

        stream.seekp(0, std::ios::beg);
        stream.write(reinterpret_cast<const char*>(&header), sizeof(WAVHeader));
        stream.flush();
        return stream.good();
    }

    ~WAVAudioFormatWriter()
    {
        if (!finalized)
            finalize();
    }

private:
    void writeHeaderPlaceholder()
    {
        WAVHeader header;
        std::memset(&header, 0, sizeof(WAVHeader));
        stream.write(reinterpret_cast<const char*>(&header), sizeof(WAVHeader));
    }

    bool writeSamples(const void* samples, size_t numSamples) override
    {
        size_t bytesPerSample = (bitsPerSample / 8) * numChannels;
        size_t bytesToWrite = numSamples * bytesPerSample;

        stream.write (reinterpret_cast<const char*> (samples), bytesToWrite);

        if (!stream)
            return false;

        dataBytesWritten += bytesToWrite;
        return true;
    }

    std::ofstream stream;
    int sampleRate;
    int numChannels;
    int bitsPerSample;
    size_t dataBytesWritten;
    bool finalized;
};

//================================================================================

bool WAVAudioFormat::canHandleFile (const File& filePath) const
{
    return filePath.existsAsFile() && filePath.hasFileExtension (".wav");
}

//================================================================================

std::unique_ptr<AudioFormatReader> WAVAudioFormat::createReaderFor (InputStream& stream)
{
    return std::make_unique<WAVAudioFormatReader> (stream);
}

std::unique_ptr<AudioFormatWriter> WAVAudioFormat::createWriterFor (OutputStream& stream,
                                                                    int sampleRate,
                                                                    int numChannels,
                                                                    int bitsPerSample)
{
    return std::make_unique<WAVAudioFormatWriter> (stream, sampleRate, numChannels, bitsPerSample);
}

} // namespace yup
