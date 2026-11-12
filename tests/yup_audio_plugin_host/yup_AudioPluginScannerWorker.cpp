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

#if YUP_MAC || YUP_WINDOWS || YUP_LINUX

using namespace yup;

namespace yup
{
std::function<bool (const String&)>& getGoogleTestChildProcessEntryPoint();
} // namespace yup

class AudioPluginScannerWorkerTests : public ::testing::Test
{
public:
    static constexpr const char* workerID = "yupScannerWorkerTests";

    /** Opening behaves according to the file name: crash, hang, broken or a 1 in / 6 out instance. */
    class FakeFormat : public AudioPluginFormat
    {
    public:
        AudioPluginFormatType getFormatType() const override { return AudioPluginFormatType::clap; }

        String getFormatName() const override { return "Fake"; }

        StringArray getFileExtensions() const override { return { ".fakeplugin" }; }

        FileSearchPath getDefaultSearchPaths() const override { return {}; }

        ResultValue<std::vector<AudioPluginDescription>> scanFile (const File& file) override
        {
            AudioPluginDescription description;
            description.formatType = AudioPluginFormatType::clap;
            description.name = file.getFileNameWithoutExtension();
            description.identifier = "test." + description.name;
            description.fileOrBundlePath = file.getFullPathName();
            description.numInputChannels = 2;
            description.numOutputChannels = 2;
            return makeResultValueOk (std::vector<AudioPluginDescription> { description });
        }

        ResultValue<std::unique_ptr<AudioPluginInstance>> loadPlugin (const AudioPluginDescription& description,
                                                                      const AudioPluginHostContext&) override
        {
            if (description.name == "crash")
                Process::terminate();

            if (description.name == "hang")
                Thread::sleep (60000);

            if (description.name == "broken")
                return makeResultValueFail ("cannot open " + description.name);

            return makeResultValueOk (std::unique_ptr<AudioPluginInstance> (std::make_unique<OpenedInstance> (description)));
        }
    };

    class OpenedInstance : public AudioPluginInstance
    {
    public:
        explicit OpenedInstance (const AudioPluginDescription& description)
            : AudioPluginInstance (description,
                                   AudioBusLayout ({ AudioBus ("Input", AudioBus::Type::Audio, AudioBus::Direction::Input, 1) },
                                                   { AudioBus ("Output", AudioBus::Type::Audio, AudioBus::Direction::Output, 6) }))
        {
        }

        void prepareToPlay (const AudioSpec&) override {}

        void releaseResources() override {}

        void processBlock (AudioProcessContext<float>&) override {}

        int getCurrentPreset() const noexcept override { return 0; }

        void setCurrentPreset (int) noexcept override {}

        int getNumPresets() const override { return 0; }

        String getPresetName (int) const override { return {}; }

        void setPresetName (int, StringRef) override {}

        Result loadStateFromMemory (const MemoryBlock&) override { return Result::ok(); }

        Result saveStateIntoMemory (MemoryBlock&) override { return Result::ok(); }

        bool hasEditor() const override { return false; }
    };

    static bool runWorker (const String& commandLine)
    {
        static std::unique_ptr<AudioPluginScannerWorker> worker;

        auto newWorker = std::make_unique<AudioPluginScannerWorker>();
        newWorker->addFormat (std::make_unique<FakeFormat>());

        if (! newWorker->initialiseFromCommandLine (commandLine, workerID))
            return false;

        worker = std::move (newWorker);
        return true;
    }

protected:
    void SetUp() override
    {
        directory = File::createTempFile ("yup_scanner_worker");
        directory.createDirectory();

        scanner.addFormat (std::make_unique<FakeFormat>());
        scanner.setOutOfProcessScanning (makeOptions (File::getSpecialLocation (File::currentExecutableFile)));

        ASSERT_TRUE (database->openInMemory().wasOk());
        scanner.setDatabase (database);
    }

    void TearDown() override
    {
        directory.deleteRecursively();
    }

    static AudioPluginScanner::OutOfProcessOptions makeOptions (const File& executable)
    {
        AudioPluginScanner::OutOfProcessOptions options;
        options.executable = executable;
        options.commandLineUniqueID = workerID;
        options.timeoutMs = 3000;
        return options;
    }

    File addPlugin (const String& name)
    {
        auto file = directory.getChildFile (name + ".fakeplugin");
        file.replaceWithText (name);
        return file;
    }

    AudioPluginScanner::ScanResult scan()
    {
        return scanner.scan (FileSearchPath (directory.getFullPathName()));
    }

    File directory;
    AudioPluginScanner scanner;
    std::shared_ptr<AudioPluginDatabase> database = std::make_shared<AudioPluginDatabase>();
};

namespace
{
const bool scannerWorkerEntryPointRegistered = []
{
    auto& entryPoint = yup::getGoogleTestChildProcessEntryPoint();

    entryPoint = [previous = entryPoint] (const String& commandLine)
    {
        return AudioPluginScannerWorkerTests::runWorker (commandLine) || (previous && previous (commandLine));
    };

    return true;
}();
} // namespace

TEST_F (AudioPluginScannerWorkerTests, OpensPluginsInWorkerProcess)
{
    addPlugin ("synth");

    const auto result = scan();

    ASSERT_EQ (1u, result.discovered.size());
    EXPECT_EQ ("synth", result.discovered[0].name);
    EXPECT_EQ (1, result.discovered[0].numInputChannels);
    EXPECT_EQ (6, result.discovered[0].numOutputChannels);
    EXPECT_TRUE (result.failedPaths.empty());
}

TEST_F (AudioPluginScannerWorkerTests, CrashingPluginIsBlacklistedAndScanContinues)
{
    const auto crashing = addPlugin ("crash");
    addPlugin ("synth");

    const auto result = scan();

    ASSERT_EQ (1u, result.discovered.size());
    EXPECT_EQ ("synth", result.discovered[0].name);
    ASSERT_EQ (1u, result.failedPaths.size());
    EXPECT_EQ (crashing.getFullPathName(), result.failedPaths[0]);
    EXPECT_EQ (AudioPluginDatabase::FileState::blacklisted, database->getFileState (crashing, AudioPluginFormatType::clap));

    const auto failures = database->getFailures();
    ASSERT_EQ (1u, failures.size());
    EXPECT_EQ (AudioPluginDatabase::FailureReason::crash, failures[0].reason);

    const auto rescan = scan();
    ASSERT_EQ (1u, rescan.blacklistedPaths.size());
    EXPECT_TRUE (rescan.failedPaths.empty());
}

TEST_F (AudioPluginScannerWorkerTests, HangingPluginTimesOutAndIsBlacklisted)
{
    const auto hanging = addPlugin ("hang");

    const auto result = scan();

    EXPECT_TRUE (result.discovered.empty());
    ASSERT_EQ (1u, result.failedPaths.size());
    EXPECT_EQ (AudioPluginDatabase::FileState::blacklisted, database->getFileState (hanging, AudioPluginFormatType::clap));

    const auto failures = database->getFailures();
    ASSERT_EQ (1u, failures.size());
    EXPECT_EQ (AudioPluginDatabase::FailureReason::timeout, failures[0].reason);
}

TEST_F (AudioPluginScannerWorkerTests, PluginFailingToOpenIsNotBlacklisted)
{
    const auto broken = addPlugin ("broken");

    const auto result = scan();

    EXPECT_TRUE (result.discovered.empty());
    ASSERT_EQ (1u, result.failedPaths.size());
    EXPECT_EQ (AudioPluginDatabase::FileState::failed, database->getFileState (broken, AudioPluginFormatType::clap));
}

TEST_F (AudioPluginScannerWorkerTests, MissingWorkerFallsBackToInProcessScan)
{
    scanner.setOutOfProcessScanning (makeOptions (directory.getChildFile ("missing_worker")));
    addPlugin ("crash");

    const auto result = scan();

    ASSERT_EQ (1u, result.discovered.size());
    EXPECT_EQ (2, result.discovered[0].numOutputChannels);
}

TEST_F (AudioPluginScannerWorkerTests, NonWorkerCommandLineIsIgnored)
{
    AudioPluginScannerWorker worker;

    EXPECT_FALSE (worker.initialiseFromCommandLine ("--gtest_filter=*", workerID));
}

TEST_F (AudioPluginScannerWorkerTests, ReplacingFormatsAndSettingTheContextKeepsTheWorkerIdle)
{
    AudioPluginScannerWorker worker;
    worker.addFormat (std::make_unique<FakeFormat>());
    worker.addFormat (std::make_unique<FakeFormat>());

    AudioPluginHostContext context;
    context.sampleRate = 48000.0f;
    worker.setHostContext (context);

    EXPECT_FALSE (worker.initialiseFromCommandLine ("--gtest_filter=*", workerID));
}

#endif
