/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

    StringArray getFileExtensions() const override { return { ".vst3" }; }

    FileSearchPath getDefaultSearchPaths() const override { return {}; }

    ResultValue<std::vector<AudioPluginDescription>> scanFile (const File& file) override
    {
        ++numScans;

        if (file.getFileName() == "bad.vst3")
            return makeResultValueFail ("unsupported");

        AudioPluginDescription desc;
        desc.formatType = fakeType;
        desc.name = file.getFileNameWithoutExtension();
        desc.identifier = "fake." + file.getFileNameWithoutExtension();
        return makeResultValueOk (std::vector<AudioPluginDescription> { desc });
    }

    ResultValue<std::unique_ptr<AudioPluginInstance>> loadPlugin (
        const AudioPluginDescription&,
        const AudioPluginHostContext&) override
    {
        return makeResultValueFail ("not implemented");
    }

    std::atomic<int> numScans { 0 };

private:
    AudioPluginFormatType fakeType;
};

/** Scans files with configurable extensions and default search path, or the registry when there are no extensions. */
class ConfigurableScannerFormat : public AudioPluginFormat
{
public:
    ConfigurableScannerFormat (AudioPluginFormatType type, StringArray extensionsToUse, FileSearchPath defaultPaths = {}, bool shouldFail = false)
        : formatType (type)
        , extensions (std::move (extensionsToUse))
        , searchPaths (std::move (defaultPaths))
        , fails (shouldFail)
    {
    }

    AudioPluginFormatType getFormatType() const override { return formatType; }

    String getFormatName() const override { return "Configurable"; }

    StringArray getFileExtensions() const override { return extensions; }

    FileSearchPath getDefaultSearchPaths() const override { return searchPaths; }

    ResultValue<std::vector<AudioPluginDescription>> scanFile (const File& file) override
    {
        if (fails)
            return makeResultValueFail ("unavailable");

        AudioPluginDescription desc;
        desc.formatType = formatType;
        desc.name = file == File() ? String ("Registered") : file.getFileNameWithoutExtension();
        return makeResultValueOk (std::vector<AudioPluginDescription> { desc });
    }

    ResultValue<std::unique_ptr<AudioPluginInstance>> loadPlugin (const AudioPluginDescription&,
                                                                  const AudioPluginHostContext&) override
    {
        return makeResultValueFail ("not implemented");
    }

private:
    AudioPluginFormatType formatType;
    StringArray extensions;
    FileSearchPath searchPaths;
    bool fails = false;
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

TEST_F (AudioPluginScannerTests, EmptyDirectoryReturnsNoResults)
{
    File tmpDir = File::getSpecialLocation (File::tempDirectory).getChildFile ("yup_scanner_empty");
    tmpDir.createDirectory();

    FileSearchPath path;
    path.add (tmpDir);

    auto result = scanner.scan (path);
    EXPECT_TRUE (result.discovered.empty());
    EXPECT_TRUE (result.failedPaths.empty());

    tmpDir.deleteRecursively();
}

TEST_F (AudioPluginScannerTests, MultipleGoodPluginsAllDiscovered)
{
    File tmpDir = File::getSpecialLocation (File::tempDirectory).getChildFile ("yup_scanner_multi");
    tmpDir.createDirectory();
    tmpDir.getChildFile ("SynthA.vst3").create();
    tmpDir.getChildFile ("SynthB.vst3").create();

    FileSearchPath path;
    path.add (tmpDir);

    auto result = scanner.scan (path);
    EXPECT_EQ (2, (int) result.discovered.size());
    EXPECT_TRUE (result.failedPaths.empty());

    tmpDir.deleteRecursively();
}

TEST_F (AudioPluginScannerTests, MixedGoodAndBadPluginsSeparateLists)
{
    File tmpDir = File::getSpecialLocation (File::tempDirectory).getChildFile ("yup_scanner_mixed");
    tmpDir.createDirectory();
    tmpDir.getChildFile ("GoodSynth.vst3").create();
    tmpDir.getChildFile ("bad.vst3").create();

    FileSearchPath path;
    path.add (tmpDir);

    auto result = scanner.scan (path);
    EXPECT_EQ (1, (int) result.discovered.size());
    EXPECT_EQ (1, (int) result.failedPaths.size());

    tmpDir.deleteRecursively();
}

TEST_F (AudioPluginScannerTests, GetNumFormatsReturnsCorrectCount)
{
    EXPECT_EQ (1, scanner.getNumFormats());
}

TEST_F (AudioPluginScannerTests, NonVst3FilesAreIgnored)
{
    File tmpDir = File::getSpecialLocation (File::tempDirectory).getChildFile ("yup_scanner_nonvst3");
    tmpDir.createDirectory();
    tmpDir.getChildFile ("SomeLib.so").create();
    tmpDir.getChildFile ("readme.txt").create();

    FileSearchPath path;
    path.add (tmpDir);

    auto result = scanner.scan (path);
    EXPECT_TRUE (result.discovered.empty());
    EXPECT_TRUE (result.failedPaths.empty());

    tmpDir.deleteRecursively();
}

TEST_F (AudioPluginScannerTests, DatabaseScansOnlyNewOrChangedFiles)
{
    File tmpDir = File::getSpecialLocation (File::tempDirectory).getChildFile ("yup_scanner_database");
    tmpDir.deleteRecursively();
    tmpDir.createDirectory();
    tmpDir.getChildFile ("Good.vst3").replaceWithText ("good");
    tmpDir.getChildFile ("bad.vst3").replaceWithText ("bad");
    tmpDir.getChildFile ("Crashy.vst3").replaceWithText ("crashy");

    auto format = std::make_unique<FakeFormat> (AudioPluginFormatType::vst3);
    auto& fakeFormat = *format;
    scanner.addFormat (std::move (format));

    auto database = std::make_shared<AudioPluginDatabase>();
    ASSERT_TRUE (database->openInMemory().wasOk());
    ASSERT_TRUE (database->storeScanFailure (tmpDir.getChildFile ("Crashy.vst3"), AudioPluginFormatType::vst3, AudioPluginDatabase::FailureReason::crash, "crashed").wasOk());
    scanner.setDatabase (database);

    FileSearchPath path;
    path.add (tmpDir);

    const auto first = scanner.scan (path);
    EXPECT_EQ (fakeFormat.numScans.load(), 2);
    EXPECT_EQ (first.discovered.size(), 1u);
    EXPECT_EQ (first.failedPaths.size(), 1u);
    EXPECT_EQ (first.blacklistedPaths.size(), 1u);

    const auto second = scanner.scan (path);
    EXPECT_EQ (fakeFormat.numScans.load(), 2);
    ASSERT_EQ (second.discovered.size(), 1u);
    EXPECT_EQ (second.discovered.front().name, "Good");
    EXPECT_EQ (second.failedPaths.size(), 1u);
    EXPECT_EQ (second.blacklistedPaths.size(), 1u);

    tmpDir.getChildFile ("Good.vst3").replaceWithText ("good v2");
    scanner.scan (path);
    EXPECT_EQ (fakeFormat.numScans.load(), 3);

    tmpDir.deleteRecursively();
}

TEST_F (AudioPluginScannerTests, FormatsCanBeLookedUpByIndexAndType)
{
    ASSERT_NE (nullptr, scanner.getFormat (0));
    EXPECT_EQ (nullptr, scanner.getFormat (-1));
    EXPECT_EQ (nullptr, scanner.getFormat (1));

    EXPECT_EQ (scanner.getFormat (0), scanner.getFormatForType (AudioPluginFormatType::vst3));
    EXPECT_EQ (nullptr, scanner.getFormatForType (AudioPluginFormatType::clap));
}

TEST_F (AudioPluginScannerTests, DatabaseAndOutOfProcessOptionsAreStored)
{
    EXPECT_EQ (nullptr, scanner.getDatabase());
    EXPECT_FALSE (scanner.getOutOfProcessScanning().has_value());

    auto database = std::make_shared<AudioPluginDatabase>();
    scanner.setDatabase (database);
    EXPECT_EQ (database, scanner.getDatabase());

    AudioPluginScanner::OutOfProcessOptions options;
    options.timeoutMs = 1234;
    scanner.setOutOfProcessScanning (options);
    ASSERT_TRUE (scanner.getOutOfProcessScanning().has_value());
    EXPECT_EQ (1234, scanner.getOutOfProcessScanning()->timeoutMs);
}

TEST_F (AudioPluginScannerTests, RegistryFormatsAreScannedWithoutSearchPaths)
{
    scanner.addFormat (std::make_unique<ConfigurableScannerFormat> (AudioPluginFormatType::audioUnit, StringArray()));
    scanner.addFormat (std::make_unique<ConfigurableScannerFormat> (AudioPluginFormatType::lv2, StringArray(), FileSearchPath(), true));

    const auto result = scanner.scan (FileSearchPath {});

    ASSERT_EQ (1u, result.discovered.size());
    EXPECT_EQ ("Registered", result.discovered[0].name);
    EXPECT_EQ (AudioPluginFormatType::audioUnit, result.discovered[0].formatType);
    EXPECT_TRUE (result.failedPaths.empty());
}

TEST_F (AudioPluginScannerTests, ScanDefaultsUsesEveryExtensionOfTheDefaultPaths)
{
    File tmpDir = File::getSpecialLocation (File::tempDirectory).getChildFile ("yup_scanner_defaults");
    tmpDir.deleteRecursively();
    tmpDir.createDirectory();
    tmpDir.getChildFile ("First.clap").create();
    tmpDir.getChildFile ("Second.clp").create();
    tmpDir.getChildFile ("Ignored.txt").create();

    FileSearchPath defaults;
    defaults.add (tmpDir);
    scanner.addFormat (std::make_unique<ConfigurableScannerFormat> (AudioPluginFormatType::clap, StringArray { ".clap", ".clp" }, defaults));

    const auto result = scanner.scanDefaults();

    EXPECT_EQ (2u, result.discovered.size());
    EXPECT_TRUE (result.failedPaths.empty());

    tmpDir.deleteRecursively();
}

TEST_F (AudioPluginScannerTests, AsyncScansDeliverResultsToTheCallback)
{
    File tmpDir = File::getSpecialLocation (File::tempDirectory).getChildFile ("yup_scanner_async");
    tmpDir.deleteRecursively();
    tmpDir.createDirectory();
    tmpDir.getChildFile ("Async.clap").create();

    FileSearchPath defaults;
    defaults.add (tmpDir);
    scanner.addFormat (std::make_unique<ConfigurableScannerFormat> (AudioPluginFormatType::clap, StringArray { ".clap" }, defaults));

    WaitableEvent scanned;
    std::atomic<int> numDiscovered { -1 };
    scanner.scanAsync (defaults, [&] (AudioPluginScanner::ScanResult result)
    {
        numDiscovered = static_cast<int> (result.discovered.size());
        scanned.signal();
    });

    ASSERT_TRUE (scanned.wait (10000));
    EXPECT_EQ (1, numDiscovered.load());

    numDiscovered = -1;
    scanner.scanDefaultsAsync ([&] (AudioPluginScanner::ScanResult result)
    {
        numDiscovered = static_cast<int> (result.discovered.size());
        scanned.signal();
    });

    ASSERT_TRUE (scanned.wait (10000));
    EXPECT_EQ (1, numDiscovered.load());

    scanner.cancelPendingScans();
    tmpDir.deleteRecursively();
}

TEST_F (AudioPluginScannerTests, MissingWorkerExecutableScansManyFilesInProcess)
{
    File tmpDir = File::getSpecialLocation (File::tempDirectory).getChildFile ("yup_scanner_missing_worker");
    tmpDir.deleteRecursively();
    tmpDir.createDirectory();

    for (int i = 0; i < 12; ++i)
        tmpDir.getChildFile ("Synth" + String (i) + ".vst3").create();

    AudioPluginScanner::OutOfProcessOptions options;
    options.executable = tmpDir.getChildFile ("missing_worker");
    scanner.setOutOfProcessScanning (options);

    FileSearchPath path;
    path.add (tmpDir);

    const auto result = scanner.scan (path);

    EXPECT_EQ (12u, result.discovered.size());
    EXPECT_TRUE (result.failedPaths.empty());

    tmpDir.deleteRecursively();
}
