#include <gtest/gtest.h>

#include <yup_audio_plugin_host/yup_audio_plugin_host.h>

using namespace yup;

namespace
{

class FakeFormat : public AudioPluginFormat
{
public:
    explicit FakeFormat (AudioPluginFormatType type)
        : fakeType (type)
    {
    }

    AudioPluginFormatType getFormatType() const override { return fakeType; }

    String getFormatName() const override { return "Fake"; }

    FileSearchPath getDefaultSearchPaths() const override { return {}; }

    ResultValue<std::vector<AudioPluginDescription>> scanFile (const File& file) override
    {
        if (file.getFileName() == "bad.vst3")
            return ResultValue<std::vector<AudioPluginDescription>>::fail ("unsupported");

        AudioPluginDescription desc;
        desc.formatType = fakeType;
        desc.name = file.getFileNameWithoutExtension();
        desc.identifier = "fake." + file.getFileNameWithoutExtension();
        return ResultValue<std::vector<AudioPluginDescription>>::ok (std::vector<AudioPluginDescription> { desc });
    }

    ResultValue<std::unique_ptr<AudioPluginInstance>> loadPlugin (
        const AudioPluginDescription&,
        const AudioPluginHostContext&) override
    {
        return ResultValue<std::unique_ptr<AudioPluginInstance>>::fail ("not implemented");
    }

private:
    AudioPluginFormatType fakeType;
};

} // namespace

class AudioPluginScannerTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        scanner.addFormat (std::make_unique<FakeFormat> (AudioPluginFormatType::vst3));
    }

    AudioPluginScanner scanner;
};

TEST_F (AudioPluginScannerTests, EmptyPathReturnsNoResults)
{
    auto result = scanner.scan (FileSearchPath {});
    EXPECT_TRUE (result.discovered.empty());
    EXPECT_TRUE (result.failedPaths.empty());
}

TEST_F (AudioPluginScannerTests, BadPluginAppearsInFailedPaths)
{
    File tmpDir = File::getSpecialLocation (File::tempDirectory).getChildFile ("yup_scanner_test");
    tmpDir.createDirectory();
    tmpDir.getChildFile ("bad.vst3").create();

    FileSearchPath path;
    path.add (tmpDir);

    auto result = scanner.scan (path);
    EXPECT_TRUE (result.discovered.empty());
    EXPECT_EQ (1, (int) result.failedPaths.size());

    tmpDir.deleteRecursively();
}

TEST_F (AudioPluginScannerTests, GoodPluginAppearsInDiscovered)
{
    File tmpDir = File::getSpecialLocation (File::tempDirectory).getChildFile ("yup_scanner_test2");
    tmpDir.createDirectory();
    tmpDir.getChildFile ("MySynth.vst3").create();

    FileSearchPath path;
    path.add (tmpDir);

    auto result = scanner.scan (path);
    ASSERT_EQ (1, (int) result.discovered.size());
    EXPECT_EQ ("MySynth", result.discovered[0].name);

    tmpDir.deleteRecursively();
}

TEST_F (AudioPluginScannerTests, AddingDuplicateFormatTypeReplacesExisting)
{
    scanner.addFormat (std::make_unique<FakeFormat> (AudioPluginFormatType::vst3));
    EXPECT_EQ (1, scanner.getNumFormats());
}
