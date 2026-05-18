#include <gtest/gtest.h>

#include <yup_audio_plugin_host/yup_audio_plugin_host.h>

using namespace yup;

namespace
{

class FakePluginInstance : public AudioPluginInstance
{
public:
    explicit FakePluginInstance (bool supportsDoublePrecision = false)
        : AudioPluginInstance (makeDescription(),
                               AudioBusLayout ({ AudioBus ("Input", AudioBus::Type::Audio, AudioBus::Direction::Input, 2) },
                                               { AudioBus ("Output", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) }))
        , supportsDoublePrecision (supportsDoublePrecision)
    {
    }

    void prepareToPlay (float, int) override { prepared = true; }

    void releaseResources() override { prepared = false; }

    void processBlock (AudioBuffer<float>& audio, MidiBuffer&) override
    {
        audio.clear();
        processCallCount++;
    }

    void processBlock (AudioBuffer<double>& audio, MidiBuffer&) override
    {
        audio.clear();
        doubleProcessCallCount++;
    }

    bool supportsDoublePrecisionProcessing() const override { return supportsDoublePrecision; }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    Result loadStateFromMemory (const MemoryBlock& block) override
    {
        lastLoadedState = block;
        return Result::ok();
    }

    Result saveStateIntoMemory (MemoryBlock& block) override
    {
        block = lastLoadedState;
        return Result::ok();
    }

    bool hasEditor() const override { return false; }

    bool prepared = false;
    int processCallCount = 0;
    int doubleProcessCallCount = 0;
    MemoryBlock lastLoadedState;

private:
    bool supportsDoublePrecision = false;

    static AudioPluginDescription makeDescription()
    {
        AudioPluginDescription d;
        d.name = "FakePlugin";
        d.identifier = "fake.plugin";
        d.formatType = AudioPluginFormatType::vst3;
        return d;
    }
};

} // namespace

class AudioPluginInstanceTests : public ::testing::Test
{
protected:
    FakePluginInstance instance;
};

TEST_F (AudioPluginInstanceTests, DescriptionIsStoredCorrectly)
{
    EXPECT_EQ ("FakePlugin", instance.getDescription().name);
    EXPECT_EQ (AudioPluginFormatType::vst3, instance.getDescription().formatType);
}

TEST_F (AudioPluginInstanceTests, PrepareToPlaySetsPreparedFlag)
{
    instance.setPlaybackConfiguration (44100.0f, 512);
    EXPECT_TRUE (instance.prepared);
    EXPECT_FLOAT_EQ (44100.0f, instance.getSampleRate());
    EXPECT_EQ (512, instance.getSamplesPerBlock());
}

TEST_F (AudioPluginInstanceTests, ReleaseResourcesClearsPreparedFlag)
{
    instance.prepareToPlay (44100.0f, 512);
    instance.releaseResources();
    EXPECT_FALSE (instance.prepared);
}

TEST_F (AudioPluginInstanceTests, ProcessBlockIncrementsCounter)
{
    AudioBuffer<float> audio (2, 512);
    MidiBuffer midi;

    instance.prepareToPlay (44100.0f, 512);
    instance.processBlock (audio, midi);

    EXPECT_EQ (1, instance.processCallCount);
}

TEST_F (AudioPluginInstanceTests, DefaultsToSinglePrecision)
{
    EXPECT_FALSE (instance.supportsDoublePrecisionProcessing());
    EXPECT_EQ (AudioProcessor::ProcessingPrecision::singlePrecision, instance.getProcessingPrecision());
    EXPECT_FALSE (instance.isUsingDoublePrecision());
}

TEST_F (AudioPluginInstanceTests, UsesDoublePrecisionWhenSupported)
{
    FakePluginInstance doublePrecisionInstance (true);

    doublePrecisionInstance.setProcessingPrecision (AudioProcessor::ProcessingPrecision::doublePrecision);

    EXPECT_TRUE (doublePrecisionInstance.supportsDoublePrecisionProcessing());
    EXPECT_EQ (AudioProcessor::ProcessingPrecision::doublePrecision, doublePrecisionInstance.getProcessingPrecision());
    EXPECT_TRUE (doublePrecisionInstance.isUsingDoublePrecision());
}

TEST_F (AudioPluginInstanceTests, CanReturnToSinglePrecisionAfterUsingDoublePrecision)
{
    FakePluginInstance doublePrecisionInstance (true);

    doublePrecisionInstance.setProcessingPrecision (AudioProcessor::ProcessingPrecision::doublePrecision);
    doublePrecisionInstance.setProcessingPrecision (AudioProcessor::ProcessingPrecision::singlePrecision);

    EXPECT_EQ (AudioProcessor::ProcessingPrecision::singlePrecision, doublePrecisionInstance.getProcessingPrecision());
    EXPECT_FALSE (doublePrecisionInstance.isUsingDoublePrecision());
}

TEST_F (AudioPluginInstanceTests, DoublePrecisionProcessBlockUsesDoublePath)
{
    FakePluginInstance doublePrecisionInstance (true);
    AudioBuffer<double> audio (2, 512);
    MidiBuffer midi;

    audio.setSample (0, 0, 1.0);

    doublePrecisionInstance.setProcessingPrecision (AudioProcessor::ProcessingPrecision::doublePrecision);
    doublePrecisionInstance.prepareToPlay (44100.0f, 512);
    doublePrecisionInstance.processBlock (audio, midi);

    EXPECT_EQ (0, doublePrecisionInstance.processCallCount);
    EXPECT_EQ (1, doublePrecisionInstance.doubleProcessCallCount);
    EXPECT_DOUBLE_EQ (0.0, audio.getSample (0, 0));
}

TEST_F (AudioPluginInstanceTests, BusLayoutMatchesConstructorArg)
{
    EXPECT_EQ (1, instance.getNumAudioInputs());
    EXPECT_EQ (1, instance.getNumAudioOutputs());
}
