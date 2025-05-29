#include <gtest/gtest.h>
#include <yup_audio_formats/yup_audio_formats.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace yup;

class WAVAudioFormatTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        format = std::make_unique<WAVAudioFormat>();
    }

    void TearDown() override
    {
        // Clean up any test files created
        cleanupTestFiles();
    }

    void cleanupTestFiles()
    {
        File testWav = getTestWavFile();
        if (testWav.exists())
            testWav.deleteFile();

        File testRf64 = getTestRf64File();
        if (testRf64.exists())
            testRf64.deleteFile();
    }

    File getTestWavFile()
    {
        return File::getCurrentWorkingDirectory().getChildFile ("test_output.wav");
    }

    File getTestRf64File()
    {
        return File::getCurrentWorkingDirectory().getChildFile ("test_output.rf64");
    }

    File getTestDataFile (const String& filename)
    {
        return File::getCurrentWorkingDirectory()
            .getChildFile ("tests")
            .getChildFile ("data")
            .getChildFile ("sounds")
            .getChildFile (filename);
    }

    // Helper to create a minimal valid WAV file header
    void createMinimalWavFile (const File& file, int sampleRate = 44100, int numChannels = 2, int numSamples = 1000, int bitsPerSample = 16)
    {
        FileOutputStream stream (file);
        if (! stream.openedOk())
            return;

        const int bytesPerSample = bitsPerSample / 8;
        const int frameSize = numChannels * bytesPerSample;

        // RIFF header
        stream.write ("RIFF", 4);
        uint32 fileSize = 36 + (numSamples * frameSize); // Header + data
        stream.writeInt (fileSize - 8);
        stream.write ("WAVE", 4);

        // fmt chunk
        stream.write ("fmt ", 4);
        stream.writeInt (16);  // Format chunk size
        stream.writeShort (1); // PCM format
        stream.writeShort (static_cast<short> (numChannels));
        stream.writeInt (sampleRate);
        stream.writeInt (sampleRate * frameSize);           // Byte rate
        stream.writeShort (static_cast<short> (frameSize)); // Block align
        stream.writeShort (static_cast<short> (bitsPerSample));

        // data chunk
        stream.write ("data", 4);
        stream.writeInt (numSamples * frameSize);

        // Write some test audio data based on bit depth
        for (int i = 0; i < numSamples * numChannels; ++i)
        {
            switch (bitsPerSample)
            {
                case 8:
                {
                    uint8 value = static_cast<uint8> (128 + (i % 127)); // 8-bit unsigned
                    stream.writeByte (value);
                    break;
                }
                case 16:
                {
                    int16 value = static_cast<int16> (i % 32767); // 16-bit signed
                    stream.writeShort (value);
                    break;
                }
                case 24:
                {
                    int32 value = (i % 8388607) << 8; // 24-bit signed (stored as 32-bit)
                    stream.writeByte (static_cast<char> (value & 0xFF));
                    stream.writeByte (static_cast<char> ((value >> 8) & 0xFF));
                    stream.writeByte (static_cast<char> ((value >> 16) & 0xFF));
                    break;
                }
                case 32:
                {
                    float value = 1.0f;
                    stream.writeFloat (value);
                    break;
                }
            }
        }

        stream.flush();
    }

    // Helper to create a corrupted WAV file for testing error handling
    void createCorruptedWavFile (const File& file, const String& corruptionType)
    {
        FileOutputStream stream (file);
        if (! stream.openedOk())
            return;

        if (corruptionType == "invalid_header")
        {
            stream.write ("INVALID", 8);
            stream.write ("WAVE", 4);
        }
        else if (corruptionType == "truncated_header")
        {
            stream.write ("RIFF", 4);
            stream.writeInt (100);
            // Missing WAVE identifier
        }
        else if (corruptionType == "no_fmt_chunk")
        {
            stream.write ("RIFF", 4);
            stream.writeInt (100);
            stream.write ("WAVE", 4);
            // Missing fmt chunk, go straight to data
            stream.write ("data", 4);
            stream.writeInt (10);
        }
        else if (corruptionType == "invalid_format")
        {
            stream.write ("RIFF", 4);
            stream.writeInt (100);
            stream.write ("WAVE", 4);
            stream.write ("fmt ", 4);
            stream.writeInt (16);
            stream.writeShort (3); // Unsupported format (not PCM)
            stream.writeShort (2);
            stream.writeInt (44100);
            stream.writeInt (176400);
            stream.writeShort (4);
            stream.writeShort (16);
        }

        stream.flush();
    }

    std::unique_ptr<WAVAudioFormat> format;
};

// === Format Properties Tests ===

TEST_F (WAVAudioFormatTest, GetFormatName)
{
    EXPECT_EQ (format->getFormatName(), "WAV/RF64");
}

TEST_F (WAVAudioFormatTest, GetSupportedFileExtensions)
{
    StringArray extensions = format->getSupportedFileExtensions();
    EXPECT_EQ (extensions.size(), 2);
    EXPECT_TRUE (extensions.contains (".wav"));
    EXPECT_TRUE (extensions.contains (".rf64"));
}

TEST_F (WAVAudioFormatTest, GetSupportedBitsPerSample)
{
    Array<int> bitsPerSample = format->getSupportedBitsPerSample();
    EXPECT_EQ (bitsPerSample.size(), 4);
    EXPECT_TRUE (bitsPerSample.contains (8));
    EXPECT_TRUE (bitsPerSample.contains (16));
    EXPECT_TRUE (bitsPerSample.contains (24));
    EXPECT_TRUE (bitsPerSample.contains (32));
}

TEST_F (WAVAudioFormatTest, GetSupportedSampleRates)
{
    Array<int> sampleRates = format->getSupportedSampleRates();
    EXPECT_GT (sampleRates.size(), 0);

    // Check for common sample rates
    EXPECT_TRUE (sampleRates.contains (8000));
    EXPECT_TRUE (sampleRates.contains (11025));
    EXPECT_TRUE (sampleRates.contains (16000));
    EXPECT_TRUE (sampleRates.contains (22050));
    EXPECT_TRUE (sampleRates.contains (44100));
    EXPECT_TRUE (sampleRates.contains (48000));
    EXPECT_TRUE (sampleRates.contains (88200));
    EXPECT_TRUE (sampleRates.contains (96000));
    EXPECT_TRUE (sampleRates.contains (176400));
    EXPECT_TRUE (sampleRates.contains (192000));
}

TEST_F (WAVAudioFormatTest, CanHandleFile_ValidExtensions)
{
    File wavFile = File::getCurrentWorkingDirectory().getChildFile ("test.wav");
    File rf64File = File::getCurrentWorkingDirectory().getChildFile ("test.rf64");

    // Create the files so they exist
    wavFile.create();
    rf64File.create();

    EXPECT_TRUE (format->canHandleFile (wavFile));
    EXPECT_TRUE (format->canHandleFile (rf64File));

    // Clean up
    wavFile.deleteFile();
    rf64File.deleteFile();
}

TEST_F (WAVAudioFormatTest, CanHandleFile_InvalidExtensions)
{
    File mp3File = File::getCurrentWorkingDirectory().getChildFile ("test.mp3");
    File txtFile = File::getCurrentWorkingDirectory().getChildFile ("test.txt");

    // Create the files so they exist
    mp3File.create();
    txtFile.create();

    EXPECT_FALSE (format->canHandleFile (mp3File));
    EXPECT_FALSE (format->canHandleFile (txtFile));

    // Clean up
    mp3File.deleteFile();
    txtFile.deleteFile();
}

TEST_F (WAVAudioFormatTest, CanHandleFile_NonExistentFile)
{
    File nonExistentFile = File::getCurrentWorkingDirectory().getChildFile ("nonexistent.wav");
    EXPECT_FALSE (format->canHandleFile (nonExistentFile));
}

// === Reader Tests ===

TEST_F (WAVAudioFormatTest, CreateReaderFor_NullStream)
{
    auto reader = format->createReaderFor (nullptr);
    EXPECT_EQ (reader, nullptr);
}

TEST_F (WAVAudioFormatTest, CreateReaderFor_ValidWavFile)
{
    File testFile = getTestWavFile();
    createMinimalWavFile (testFile, 44100, 2, 1000);

    FileInputStream stream (testFile);
    ASSERT_TRUE (stream.openedOk());

    auto reader = format->createReaderFor (&stream);
    ASSERT_NE (reader, nullptr);

    EXPECT_EQ (reader->getSampleRate(), 44100);
    EXPECT_EQ (reader->getNumChannels(), 2);
    EXPECT_EQ (reader->getTotalSamples(), 1000);
}

TEST_F (WAVAudioFormatTest, CreateReaderFor_RealTestFiles)
{
    // Test with actual test files from tests/data/sounds/
    File testSample = getTestDataFile ("test_sample.wav");
    if (testSample.exists())
    {
        FileInputStream stream (testSample);
        ASSERT_TRUE (stream.openedOk());

        auto reader = format->createReaderFor (&stream);
        ASSERT_NE (reader, nullptr);

        EXPECT_GT (reader->getSampleRate(), 0);
        EXPECT_GT (reader->getNumChannels(), 0);
        EXPECT_GT (reader->getTotalSamples(), 0);

        // Try reading some samples
        AudioSampleBuffer buffer (reader->getNumChannels(), 100);
        bool success = reader->readSamples (buffer, 0, 100);
        EXPECT_TRUE (success);
    }

    File guitarSustain = getTestDataFile ("guitar_sustain.wav");
    if (guitarSustain.exists())
    {
        FileInputStream stream (guitarSustain);
        ASSERT_TRUE (stream.openedOk());

        auto reader = format->createReaderFor (&stream);
        ASSERT_NE (reader, nullptr);

        EXPECT_GT (reader->getSampleRate(), 0);
        EXPECT_GT (reader->getNumChannels(), 0);
        EXPECT_GT (reader->getTotalSamples(), 0);
    }
}

TEST_F (WAVAudioFormatTest, CreateReaderFor_InvalidTestFile)
{
    File invalidFile = getTestDataFile ("invalid.wav");
    if (invalidFile.exists())
    {
        FileInputStream stream (invalidFile);
        ASSERT_TRUE (stream.openedOk());

        auto reader = format->createReaderFor (&stream);
        // The "invalid.wav" file actually has a valid WAV structure,
        // so it should be readable (just with unusual data content)
        EXPECT_NE (reader, nullptr);

        if (reader != nullptr)
        {
            // Verify we can get basic properties
            EXPECT_GT (reader->getSampleRate(), 0);
            EXPECT_GT (reader->getNumChannels(), 0);
            EXPECT_GE (reader->getTotalSamples(), 0);
        }
    }
}

TEST_F (WAVAudioFormatTest, CreateReaderFor_DifferentSampleRates)
{
    struct TestCase
    {
        int sampleRate;
        int numChannels;
        int numSamples;
    };

    TestCase testCases[] = {
        { 22050, 1, 500 },
        { 48000, 2, 2000 },
        { 96000, 6, 100 }
    };

    for (const auto& testCase : testCases)
    {
        File testFile = getTestWavFile();
        createMinimalWavFile (testFile, testCase.sampleRate, testCase.numChannels, testCase.numSamples);

        FileInputStream stream (testFile);
        ASSERT_TRUE (stream.openedOk());

        auto reader = format->createReaderFor (&stream);
        ASSERT_NE (reader, nullptr);

        EXPECT_EQ (reader->getSampleRate(), testCase.sampleRate);
        EXPECT_EQ (reader->getNumChannels(), testCase.numChannels);
        EXPECT_EQ (reader->getTotalSamples(), testCase.numSamples);

        testFile.deleteFile();
    }
}

TEST_F (WAVAudioFormatTest, CreateReaderFor_CorruptedFiles)
{
    String corruptionTypes[] = {
        "invalid_header",
        "truncated_header",
        "no_fmt_chunk",
        "invalid_format"
    };

    for (const auto& corruptionType : corruptionTypes)
    {
        File testFile = getTestWavFile();
        createCorruptedWavFile (testFile, corruptionType);

        FileInputStream stream (testFile);
        ASSERT_TRUE (stream.openedOk());

        auto reader = format->createReaderFor (&stream);
        EXPECT_EQ (reader, nullptr) << "Should fail for corruption type: " << corruptionType;

        testFile.deleteFile();
    }
}

TEST_F (WAVAudioFormatTest, CreateReaderFor_InvalidFile)
{
    File testFile = getTestWavFile();
    FileOutputStream stream (testFile);
    stream.write ("INVALID_HEADER", 14);
    stream.flush();

    FileInputStream readStream (testFile);
    ASSERT_TRUE (readStream.openedOk());

    auto reader = format->createReaderFor (&readStream);
    EXPECT_EQ (reader, nullptr);
}

TEST_F (WAVAudioFormatTest, ReaderReadSamples)
{
    File testFile = getTestWavFile();
    createMinimalWavFile (testFile, 44100, 2, 1000);

    FileInputStream stream (testFile);
    ASSERT_TRUE (stream.openedOk());

    auto reader = format->createReaderFor (&stream);
    ASSERT_NE (reader, nullptr);

    AudioSampleBuffer buffer (2, 100);
    buffer.clear();

    bool success = reader->readSamples (buffer, 0, 100);
    EXPECT_TRUE (success);

    // Verify that data was actually read (should not be all zeros)
    bool hasNonZeroData = false;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            if (buffer.getSample (channel, sample) != 0.0f)
            {
                hasNonZeroData = true;
                break;
            }
        }
        if (hasNonZeroData)
            break;
    }
    EXPECT_TRUE (hasNonZeroData);
}

TEST_F (WAVAudioFormatTest, ReaderReadSamples_InvalidBuffer)
{
    File testFile = getTestWavFile();
    createMinimalWavFile (testFile, 44100, 2, 1000);

    FileInputStream stream (testFile);
    ASSERT_TRUE (stream.openedOk());

    auto reader = format->createReaderFor (&stream);
    ASSERT_NE (reader, nullptr);

    // Try reading with wrong number of channels
    AudioSampleBuffer wrongBuffer (1, 100); // Reader expects 2 channels
    bool success = reader->readSamples (wrongBuffer, 0, 100);
    EXPECT_FALSE (success);
}

TEST_F (WAVAudioFormatTest, ReaderReadSamples_ZeroChannelBuffer)
{
    File testFile = getTestWavFile();
    createMinimalWavFile (testFile, 44100, 2, 1000);

    FileInputStream stream (testFile);
    ASSERT_TRUE (stream.openedOk());

    auto reader = format->createReaderFor (&stream);
    ASSERT_NE (reader, nullptr);

    // Try reading with zero channels
    AudioSampleBuffer zeroBuffer (0, 100);
    bool success = reader->readSamples (zeroBuffer, 0, 100);
    EXPECT_FALSE (success);
}

// === Writer Tests ===

TEST_F (WAVAudioFormatTest, CreateWriterFor_NullStream)
{
    auto writer = format->createWriterFor (nullptr, 44100, 2, 16);
    EXPECT_EQ (writer, nullptr);
}

TEST_F (WAVAudioFormatTest, CreateWriterFor_InvalidParameters)
{
    File testFile = getTestWavFile();
    FileOutputStream stream (testFile);
    ASSERT_TRUE (stream.openedOk());

    // Test invalid sample rate
    auto writer1 = format->createWriterFor (&stream, 0, 2, 16);
    EXPECT_EQ (writer1, nullptr);

    // Test negative sample rate
    auto writer1b = format->createWriterFor (&stream, -44100, 2, 16);
    EXPECT_EQ (writer1b, nullptr);

    // Test invalid channels
    auto writer2 = format->createWriterFor (&stream, 44100, 0, 16);
    EXPECT_EQ (writer2, nullptr);

    // Test negative channels
    auto writer2b = format->createWriterFor (&stream, 44100, -2, 16);
    EXPECT_EQ (writer2b, nullptr);

    // Test unsupported bit depths (not 8, 16, 24, or 32)
    auto writer3 = format->createWriterFor (&stream, 44100, 2, 12);
    EXPECT_EQ (writer3, nullptr);

    auto writer4 = format->createWriterFor (&stream, 44100, 2, 20);
    EXPECT_EQ (writer4, nullptr);

    auto writer5 = format->createWriterFor (&stream, 44100, 2, 64);
    EXPECT_EQ (writer5, nullptr);
}

TEST_F (WAVAudioFormatTest, CreateWriterFor_ValidParameters)
{
    File testFile = getTestWavFile();
    FileOutputStream stream (testFile);
    ASSERT_TRUE (stream.openedOk());

    auto writer = format->createWriterFor (&stream, 44100, 2, 16);
    EXPECT_NE (writer, nullptr);
}

TEST_F (WAVAudioFormatTest, CreateWriterFor_DifferentValidParameters)
{
    struct TestCase
    {
        int sampleRate;
        int numChannels;
    };

    TestCase testCases[] = {
        { 8000, 1 },
        { 22050, 2 },
        { 44100, 1 },
        { 44100, 2 },
        { 48000, 6 },
        { 96000, 8 }
    };

    for (const auto& testCase : testCases)
    {
        File testFile = getTestWavFile();
        FileOutputStream stream (testFile);
        ASSERT_TRUE (stream.openedOk());

        auto writer = format->createWriterFor (&stream, testCase.sampleRate, testCase.numChannels, 16);
        EXPECT_NE (writer, nullptr) << "Failed for " << testCase.sampleRate << "Hz, " << testCase.numChannels << " channels";

        testFile.deleteFile();
    }
}

TEST_F (WAVAudioFormatTest, CreateWriterFor_AllSupportedBitDepths)
{
    Array<int> supportedBits = format->getSupportedBitsPerSample();

    for (int bits : supportedBits)
    {
        File testFile = getTestWavFile();
        FileOutputStream stream (testFile);
        ASSERT_TRUE (stream.openedOk());

        auto writer = format->createWriterFor (&stream, 44100, 2, bits);
        EXPECT_NE (writer, nullptr) << "Failed to create writer for " << bits << "-bit";

        testFile.deleteFile();
    }
}

TEST_F (WAVAudioFormatTest, CreateWriterFor_UnsupportedBitDepths)
{
    Array<int> unsupportedBits = { 12, 20, 48, 64 };

    for (int bits : unsupportedBits)
    {
        File testFile = getTestWavFile();
        FileOutputStream stream (testFile);
        ASSERT_TRUE (stream.openedOk());

        auto writer = format->createWriterFor (&stream, 44100, 2, bits);
        EXPECT_EQ (writer, nullptr) << "Should not create writer for unsupported " << bits << "-bit";

        testFile.deleteFile();
    }
}

TEST_F (WAVAudioFormatTest, WriterWriteSamples)
{
    File testFile = getTestWavFile();
    FileOutputStream stream (testFile);
    ASSERT_TRUE (stream.openedOk());

    auto writer = format->createWriterFor (&stream, 44100, 2, 16);
    ASSERT_NE (writer, nullptr);

    // Create test audio data
    const int numSamples = 100;
    AudioSampleBuffer buffer (2, numSamples);

    // Fill with test data
    for (int channel = 0; channel < 2; ++channel)
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float value = std::sin (2.0f * M_PI * 440.0f * sample / 44100.0f); // 440Hz sine wave
            buffer.setSample (channel, sample, value * 0.5f);
        }
    }

    bool success = writer->writeSamples (buffer, numSamples);
    EXPECT_TRUE (success);

    success = writer->finalize();
    EXPECT_TRUE (success);
}

TEST_F (WAVAudioFormatTest, WriterWriteSamples_InvalidBuffer)
{
    File testFile = getTestWavFile();
    FileOutputStream stream (testFile);
    ASSERT_TRUE (stream.openedOk());

    auto writer = format->createWriterFor (&stream, 44100, 2, 16);
    ASSERT_NE (writer, nullptr);

    // Try writing with wrong number of channels
    AudioSampleBuffer wrongBuffer (1, 100); // Writer expects 2 channels
    bool success = writer->writeSamples (wrongBuffer, 100);
    EXPECT_FALSE (success);
}

TEST_F (WAVAudioFormatTest, WriterWriteSamples_ClampingBehavior)
{
    File testFile = getTestWavFile();
    FileOutputStream stream (testFile);
    ASSERT_TRUE (stream.openedOk());

    auto writer = format->createWriterFor (&stream, 44100, 1, 16);
    ASSERT_NE (writer, nullptr);

    // Test values that need clamping
    AudioSampleBuffer buffer (1, 4);
    buffer.setSample (0, 0, -2.0f); // Should clamp to -1.0
    buffer.setSample (0, 1, 2.0f);  // Should clamp to 1.0
    buffer.setSample (0, 2, 0.0f);  // Normal value
    buffer.setSample (0, 3, 0.5f);  // Normal value

    bool success = writer->writeSamples (buffer, 4);
    EXPECT_TRUE (success);

    success = writer->finalize();
    EXPECT_TRUE (success);

    // Read back and verify clamping occurred
    FileInputStream readStream (testFile);
    auto reader = format->createReaderFor (&readStream);
    ASSERT_NE (reader, nullptr);

    AudioSampleBuffer readBuffer (1, 4);
    success = reader->readSamples (readBuffer, 0, 4);
    EXPECT_TRUE (success);

    // Allow for quantization tolerance
    const float tolerance = 1.0f / 32768.0f * 2.0f;
    EXPECT_NEAR (readBuffer.getSample (0, 0), -1.0f, tolerance);
    EXPECT_NEAR (readBuffer.getSample (0, 1), 1.0f, tolerance);
}

// === Round Trip Tests ===

TEST_F (WAVAudioFormatTest, WriteAndReadRoundTrip)
{
    File testFile = getTestWavFile();
    const int sampleRate = 44100;
    const int numChannels = 2;
    const int numSamples = 1000;

    // Create test data
    AudioSampleBuffer originalBuffer (numChannels, numSamples);
    for (int channel = 0; channel < numChannels; ++channel)
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float value = std::sin (2.0f * M_PI * (440.0f + channel * 100.0f) * sample / sampleRate);
            originalBuffer.setSample (channel, sample, value * 0.5f);
        }
    }

    // Write the file
    {
        FileOutputStream stream (testFile);
        ASSERT_TRUE (stream.openedOk());

        auto writer = format->createWriterFor (&stream, sampleRate, numChannels, 16);
        ASSERT_NE (writer, nullptr);

        bool success = writer->writeSamples (originalBuffer, numSamples);
        ASSERT_TRUE (success);

        success = writer->finalize();
        ASSERT_TRUE (success);
    }

    // Read the file back
    {
        FileInputStream stream (testFile);
        ASSERT_TRUE (stream.openedOk());

        auto reader = format->createReaderFor (&stream);
        ASSERT_NE (reader, nullptr);

        EXPECT_EQ (reader->getSampleRate(), sampleRate);
        EXPECT_EQ (reader->getNumChannels(), numChannels);
        EXPECT_EQ (reader->getTotalSamples(), numSamples);

        AudioSampleBuffer readBuffer (numChannels, numSamples);
        bool success = reader->readSamples (readBuffer, 0, numSamples);
        ASSERT_TRUE (success);

        // Compare data (with some tolerance for 16-bit quantization)
        const float tolerance = 1.0f / 32768.0f * 2.0f; // Allow for quantization error

        for (int channel = 0; channel < numChannels; ++channel)
        {
            for (int sample = 0; sample < numSamples; ++sample)
            {
                float original = originalBuffer.getSample (channel, sample);
                float read = readBuffer.getSample (channel, sample);

                EXPECT_NEAR (original, read, tolerance)
                    << "Mismatch at channel " << channel << ", sample " << sample;
            }
        }
    }
}

TEST_F (WAVAudioFormatTest, WriteAndReadRoundTrip_DifferentConfigurations)
{
    struct TestConfig
    {
        int sampleRate;
        int numChannels;
        int numSamples;
    };

    TestConfig configs[] = {
        { 22050, 1, 256 },
        { 44100, 2, 512 },
        { 48000, 6, 1024 },
        { 96000, 1, 128 }
    };

    for (const auto& config : configs)
    {
        File testFile = getTestWavFile();

        // Create test data with different frequencies per channel
        AudioSampleBuffer originalBuffer (config.numChannels, config.numSamples);
        for (int channel = 0; channel < config.numChannels; ++channel)
        {
            float frequency = 440.0f + channel * 110.0f; // A4, A4+D5, etc.
            for (int sample = 0; sample < config.numSamples; ++sample)
            {
                float value = std::sin (2.0f * M_PI * frequency * sample / config.sampleRate);
                originalBuffer.setSample (channel, sample, value * 0.3f);
            }
        }

        // Write and read back
        {
            FileOutputStream stream (testFile);
            ASSERT_TRUE (stream.openedOk());

            auto writer = format->createWriterFor (&stream, config.sampleRate, config.numChannels, 16);
            ASSERT_NE (writer, nullptr);

            bool success = writer->writeSamples (originalBuffer, config.numSamples);
            ASSERT_TRUE (success);
            success = writer->finalize();
            ASSERT_TRUE (success);
        }

        {
            FileInputStream stream (testFile);
            ASSERT_TRUE (stream.openedOk());

            auto reader = format->createReaderFor (&stream);
            ASSERT_NE (reader, nullptr);

            EXPECT_EQ (reader->getSampleRate(), config.sampleRate);
            EXPECT_EQ (reader->getNumChannels(), config.numChannels);
            EXPECT_EQ (reader->getTotalSamples(), config.numSamples);

            AudioSampleBuffer readBuffer (config.numChannels, config.numSamples);
            bool success = reader->readSamples (readBuffer, 0, config.numSamples);
            ASSERT_TRUE (success);

            // Verify data integrity
            const float tolerance = 1.0f / 32768.0f * 2.0f;
            for (int channel = 0; channel < config.numChannels; ++channel)
            {
                for (int sample = 0; sample < config.numSamples; ++sample)
                {
                    float original = originalBuffer.getSample (channel, sample);
                    float read = readBuffer.getSample (channel, sample);
                    EXPECT_NEAR (original, read, tolerance);
                }
            }
        }

        testFile.deleteFile();
    }
}

TEST_F (WAVAudioFormatTest, WriteAndReadRoundTrip_AllBitDepths)
{
    Array<int> supportedBits = format->getSupportedBitsPerSample();
    const int sampleRate = 44100;
    const int numChannels = 2;
    const int numSamples = 1000;

    for (int bits : supportedBits)
    {
        File testFile = getTestWavFile();

        // Create test data with different frequencies per channel
        AudioSampleBuffer originalBuffer (numChannels, numSamples);
        for (int channel = 0; channel < numChannels; ++channel)
        {
            for (int sample = 0; sample < numSamples; ++sample)
            {
                float value = std::sin (2.0f * M_PI * (440.0f + channel * 100.0f) * sample / sampleRate);
                originalBuffer.setSample (channel, sample, value * 0.5f);
            }
        }

        // Write the file
        {
            FileOutputStream stream (testFile);
            ASSERT_TRUE (stream.openedOk());

            auto writer = format->createWriterFor (&stream, sampleRate, numChannels, bits);
            ASSERT_NE (writer, nullptr) << "Failed to create writer for " << bits << "-bit";

            bool success = writer->writeSamples (originalBuffer, numSamples);
            ASSERT_TRUE (success) << "Failed to write samples for " << bits << "-bit";

            success = writer->finalize();
            ASSERT_TRUE (success) << "Failed to finalize writer for " << bits << "-bit";
        }

        // Read the file back
        {
            FileInputStream stream (testFile);
            ASSERT_TRUE (stream.openedOk());

            auto reader = format->createReaderFor (&stream);
            ASSERT_NE (reader, nullptr) << "Failed to create reader for " << bits << "-bit";

            EXPECT_EQ (reader->getSampleRate(), sampleRate);
            EXPECT_EQ (reader->getNumChannels(), numChannels);
            EXPECT_EQ (reader->getTotalSamples(), numSamples);

            AudioSampleBuffer readBuffer (numChannels, numSamples);
            bool success = reader->readSamples (readBuffer, 0, numSamples);
            ASSERT_TRUE (success) << "Failed to read samples for " << bits << "-bit";

            // Compare data with appropriate tolerance for bit depth
            float tolerance;
            switch (bits)
            {
                case 8:
                    tolerance = 1.0f / 128.0f;
                    break; // 8-bit has lower precision
                case 16:
                    tolerance = 1.0f / 32768.0f * 2.0f;
                    break;
                case 24:
                    tolerance = 1.0f / 8388608.0f * 2.0f;
                    break;
                case 32:
                    tolerance = 1.0f / 2147483648.0f * 2.0f;
                    break;
                default:
                    tolerance = 0.01f;
                    break;
            }

            for (int channel = 0; channel < numChannels; ++channel)
            {
                for (int sample = 0; sample < numSamples; ++sample)
                {
                    float original = originalBuffer.getSample (channel, sample);
                    float read = readBuffer.getSample (channel, sample);

                    EXPECT_NEAR (original, read, tolerance)
                        << "Mismatch at " << bits << "-bit, channel " << channel << ", sample " << sample;
                }
            }
        }

        testFile.deleteFile();
    }
}

// === Edge Cases and Error Handling ===

TEST_F (WAVAudioFormatTest, ReaderReadSamples_BeyondEndOfFile)
{
    File testFile = getTestWavFile();
    createMinimalWavFile (testFile, 44100, 2, 100); // Only 100 samples

    FileInputStream stream (testFile);
    ASSERT_TRUE (stream.openedOk());

    auto reader = format->createReaderFor (&stream);
    ASSERT_NE (reader, nullptr);

    AudioSampleBuffer buffer (2, 200); // Try to read more than available
    bool success = reader->readSamples (buffer, 0, 200);
    EXPECT_FALSE (success); // Should fail when trying to read beyond EOF
}

TEST_F (WAVAudioFormatTest, ReaderReadSamples_StartBeyondEOF)
{
    File testFile = getTestWavFile();
    createMinimalWavFile (testFile, 44100, 2, 100); // Only 100 samples

    FileInputStream stream (testFile);
    ASSERT_TRUE (stream.openedOk());

    auto reader = format->createReaderFor (&stream);
    ASSERT_NE (reader, nullptr);

    AudioSampleBuffer buffer (2, 50);
    bool success = reader->readSamples (buffer, 150, 50); // Start beyond EOF
    EXPECT_FALSE (success);
}

TEST_F (WAVAudioFormatTest, ReaderReadSamples_OffsetRead)
{
    File testFile = getTestWavFile();
    createMinimalWavFile (testFile, 44100, 2, 1000);

    FileInputStream stream (testFile);
    ASSERT_TRUE (stream.openedOk());

    auto reader = format->createReaderFor (&stream);
    ASSERT_NE (reader, nullptr);

    // Read from offset
    AudioSampleBuffer buffer (2, 100);
    bool success = reader->readSamples (buffer, 500, 100); // Start from sample 500
    EXPECT_TRUE (success);
}

TEST_F (WAVAudioFormatTest, ReaderReadSamples_PartialRead)
{
    File testFile = getTestWavFile();
    createMinimalWavFile (testFile, 44100, 2, 100); // Only 100 samples

    FileInputStream stream (testFile);
    ASSERT_TRUE (stream.openedOk());

    auto reader = format->createReaderFor (&stream);
    ASSERT_NE (reader, nullptr);

    // Try to read beyond file, but starting within bounds
    AudioSampleBuffer buffer (2, 200);
    bool success = reader->readSamples (buffer, 90, 200); // Start at 90, try to read 200
    EXPECT_FALSE (success);                               // Should fail as we can't read beyond EOF
}

TEST_F (WAVAudioFormatTest, WriterMultipleWrites)
{
    File testFile = getTestWavFile();
    FileOutputStream stream (testFile);
    ASSERT_TRUE (stream.openedOk());

    auto writer = format->createWriterFor (&stream, 44100, 2, 16);
    ASSERT_NE (writer, nullptr);

    // Write multiple chunks
    const int chunkSize = 100;
    AudioSampleBuffer buffer (2, chunkSize);

    for (int chunk = 0; chunk < 5; ++chunk)
    {
        // Fill with different data for each chunk
        for (int channel = 0; channel < 2; ++channel)
        {
            for (int sample = 0; sample < chunkSize; ++sample)
            {
                float value = std::sin (2.0f * M_PI * 440.0f * (chunk * chunkSize + sample) / 44100.0f);
                buffer.setSample (channel, sample, value * 0.5f);
            }
        }

        bool success = writer->writeSamples (buffer, chunkSize);
        EXPECT_TRUE (success);
    }

    bool success = writer->finalize();
    EXPECT_TRUE (success);

    // Verify the final file
    FileInputStream readStream (testFile);
    auto reader = format->createReaderFor (&readStream);
    ASSERT_NE (reader, nullptr);
    EXPECT_EQ (reader->getTotalSamples(), 5 * chunkSize);
}

TEST_F (WAVAudioFormatTest, WriterEmptyWrite)
{
    File testFile = getTestWavFile();
    FileOutputStream stream (testFile);
    ASSERT_TRUE (stream.openedOk());

    auto writer = format->createWriterFor (&stream, 44100, 2, 16);
    ASSERT_NE (writer, nullptr);

    // Write zero samples
    AudioSampleBuffer buffer (2, 100);
    bool success = writer->writeSamples (buffer, 0);
    EXPECT_TRUE (success);

    success = writer->finalize();
    EXPECT_TRUE (success);

    // Verify file has zero samples
    FileInputStream readStream (testFile);
    auto reader = format->createReaderFor (&readStream);
    ASSERT_NE (reader, nullptr);
    EXPECT_EQ (reader->getTotalSamples(), 0);
}

TEST_F (WAVAudioFormatTest, WriterFinalizeMultipleTimes)
{
    File testFile = getTestWavFile();
    FileOutputStream stream (testFile);
    ASSERT_TRUE (stream.openedOk());

    auto writer = format->createWriterFor (&stream, 44100, 2, 16);
    ASSERT_NE (writer, nullptr);

    AudioSampleBuffer buffer (2, 100);
    buffer.clear();

    bool success = writer->writeSamples (buffer, 100);
    EXPECT_TRUE (success);

    // Finalize multiple times
    success = writer->finalize();
    EXPECT_TRUE (success);

    success = writer->finalize();
    EXPECT_TRUE (success); // Should still return true

    success = writer->finalize();
    EXPECT_TRUE (success); // Should still return true
}

TEST_F (WAVAudioFormatTest, WriterWriteAfterFinalize)
{
    File testFile = getTestWavFile();
    FileOutputStream stream (testFile);
    ASSERT_TRUE (stream.openedOk());

    auto writer = format->createWriterFor (&stream, 44100, 2, 16);
    ASSERT_NE (writer, nullptr);

    AudioSampleBuffer buffer (2, 100);
    buffer.clear();

    bool success = writer->writeSamples (buffer, 100);
    EXPECT_TRUE (success);

    success = writer->finalize();
    EXPECT_TRUE (success);

    // The current implementation allows writing after finalize
    // (though the data may not be properly formatted in the file)
    success = writer->writeSamples (buffer, 100);
    EXPECT_TRUE (success); // Current implementation allows this
}

TEST_F (WAVAudioFormatTest, CreateReaderFor_DifferentBitDepths)
{
    Array<int> supportedBits = format->getSupportedBitsPerSample();

    for (int bits : supportedBits)
    {
        File testFile = getTestWavFile();
        createMinimalWavFile (testFile, 44100, 2, 1000, bits);

        FileInputStream stream (testFile);
        ASSERT_TRUE (stream.openedOk());

        auto reader = format->createReaderFor (&stream);
        ASSERT_NE (reader, nullptr) << "Failed to create reader for " << bits << "-bit";

        EXPECT_EQ (reader->getSampleRate(), 44100);
        EXPECT_EQ (reader->getNumChannels(), 2);
        EXPECT_EQ (reader->getTotalSamples(), 1000);

        // Try reading some samples
        AudioSampleBuffer buffer (2, 100);
        bool success = reader->readSamples (buffer, 0, 100);
        EXPECT_TRUE (success) << "Failed to read samples for " << bits << "-bit";

        // Verify that data was actually read (should not be all zeros for most cases)
        bool hasNonZeroData = false;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                if (std::abs (buffer.getSample (channel, sample)) > 0.001f)
                {
                    hasNonZeroData = true;
                    break;
                }
            }
            if (hasNonZeroData)
                break;
        }
        EXPECT_TRUE (hasNonZeroData) << "No non-zero data found for " << bits << "-bit";

        testFile.deleteFile();
    }
}