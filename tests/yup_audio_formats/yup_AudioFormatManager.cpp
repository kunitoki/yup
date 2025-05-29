#include <gtest/gtest.h>
#include <yup_audio_formats/yup_audio_formats.h>

using namespace yup;

class AudioFormatManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        manager.registerFormat (std::make_unique<WAVAudioFormat>());
    }

    AudioFormatManager manager;
};

TEST_F (AudioFormatManagerTest, RegisterFormat)
{
    // Test that registering a format doesn't crash
    auto format = std::make_unique<WAVAudioFormat>();
    EXPECT_NO_THROW (manager.registerFormat (std::move (format)));
}

TEST_F (AudioFormatManagerTest, CreateReaderForNonExistentFile)
{
    // Test with a non-existent file
    auto reader = manager.createReaderFor (File::getCurrentWorkingDirectory().getChildFile ("nonexistent.wav"));
    EXPECT_EQ (reader, nullptr);
}

TEST_F (AudioFormatManagerTest, CreateWriterForInvalidPath)
{
    // Test with invalid parameters
    auto writer = manager.createWriterFor (File::getCurrentWorkingDirectory(), 44100, 2, 16);
    EXPECT_EQ (writer, nullptr);
}
