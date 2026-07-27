#include <gtest/gtest.h>

#include <yup_audio_plugin_host/yup_audio_plugin_host.h>

using namespace yup;

class LV2FormatTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        format = std::make_unique<LV2Format>();
    }

    void TearDown() override
    {
        format.reset();
    }

    std::unique_ptr<LV2Format> format;
};

TEST_F (LV2FormatTests, ReturnsCorrectFormatType)
{
    EXPECT_EQ (AudioPluginFormatType::lv2, format->getFormatType());
}

TEST_F (LV2FormatTests, ReturnsCorrectFormatName)
{
    EXPECT_EQ ("LV2", format->getFormatName());
}

TEST_F (LV2FormatTests, ReturnsLV2BundleExtension)
{
    const auto extensions = format->getFileExtensions();
    EXPECT_FALSE (extensions.isEmpty());
    EXPECT_TRUE (extensions.contains (".lv2"));
}

TEST_F (LV2FormatTests, ReturnsNonEmptyDefaultSearchPaths)
{
    const auto paths = format->getDefaultSearchPaths();
    EXPECT_GT (paths.getNumPaths(), 0);
}

TEST_F (LV2FormatTests, FailsOnInvalidPath)
{
    auto result = format->scanFile (File { "/invalid/nonexistent/path" });

    EXPECT_FALSE (result.wasOk());
}

TEST_F (LV2FormatTests, FailsOnNonDirectory)
{
    auto tempDir = File::getSpecialLocation (File::SpecialLocationType::tempDirectory);
    auto tempFile = tempDir.getChildFile ("lv2_test_file.txt");
    tempFile.create();

    auto result = format->scanFile (tempFile);
    EXPECT_FALSE (result.wasOk());

    tempFile.deleteFile();
}

TEST_F (LV2FormatTests, FailsLoadUnknownPlugin)
{
    AudioPluginDescription desc;
    desc.formatType = AudioPluginFormatType::lv2;
    desc.identifier = "http://example.com/nonexistent";
    desc.fileOrBundlePath = "/nonexistent/test.lv2";

    AudioPluginHostContext ctx;
    ctx.sampleRate = 44100.0;
    ctx.maxBlockSize = 512;

    auto result = format->loadPlugin (desc, ctx);
    EXPECT_FALSE (result.wasOk());
}
