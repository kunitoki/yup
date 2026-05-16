#include <gtest/gtest.h>

#include <yup_audio_plugin_host/yup_audio_plugin_host.h>

using namespace yup;

namespace
{

class FakePluginInstance : public AudioPluginInstance
{
public:
    FakePluginInstance()
        : AudioPluginInstance (makeDescription(),
                               AudioBusLayout ({ AudioBus ("Input", AudioBus::Type::Audio, AudioBus::Direction::Input, 2) },
                                               { AudioBus ("Output", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) }))
    {
    }

    void prepareToPlay (float, int) override { prepared = true; }

    void releaseResources() override { prepared = false; }

    void processBlock (AudioBuffer<float>& audio, MidiBuffer&) override
    {
        audio.clear();
        processCallCount++;
    }

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
    MemoryBlock lastLoadedState;

private:
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

TEST_F (AudioPluginInstanceTests, BusLayoutMatchesConstructorArg)
{
    EXPECT_EQ (1, instance.getNumAudioInputs());
    EXPECT_EQ (1, instance.getNumAudioOutputs());
}
