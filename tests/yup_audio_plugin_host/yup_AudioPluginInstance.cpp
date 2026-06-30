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

    void prepareToPlay (const AudioSpec&) override { prepared = true; }

    void releaseResources() override { prepared = false; }

    void processBlock (AudioProcessContext<float>& context) override
    {
        context.audio.clear();
        processCallCount++;
    }

    void processBlock (AudioProcessContext<double>& context) override
    {
        context.audio.clear();
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

class CountOnlyPluginInstance : public AudioPluginInstance
{
public:
    CountOnlyPluginInstance()
        : AudioPluginInstance (makeDescription(),
                               AudioBusLayout ({ AudioBus ("MIDI Input", AudioBus::Type::Midi, AudioBus::Direction::Input, 1) },
                                               { AudioBus ("MIDI Output", AudioBus::Type::Midi, AudioBus::Direction::Output, 1) }))
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

private:
    static AudioPluginDescription makeDescription()
    {
        AudioPluginDescription d;
        d.name = "CountOnlyPlugin";
        d.identifier = "count.only";
        d.formatType = AudioPluginFormatType::clap;
        return d;
    }
};

class BypassPluginInstance : public AudioPluginInstance
{
public:
    BypassPluginInstance (int numInputChannels, int numOutputChannels)
        : AudioPluginInstance (makeDescription (numInputChannels, numOutputChannels),
                               makeLayout (numInputChannels, numOutputChannels))
    {
    }

    void prepareToPlay (const AudioSpec&) override {}

    void releaseResources() override {}

    void processBlock (AudioProcessContext<float>& context) override
    {
        if (isBypassed())
        {
            processBlockBypassed (context);
            return;
        }

        context.audio.clear();
    }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    Result loadStateFromMemory (const MemoryBlock&) override { return Result::ok(); }

    Result saveStateIntoMemory (MemoryBlock&) override { return Result::ok(); }

    bool hasEditor() const override { return false; }

private:
    static AudioPluginDescription makeDescription (int numInputChannels, int numOutputChannels)
    {
        AudioPluginDescription d;
        d.name = "BypassPlugin";
        d.identifier = "bypass.plugin";
        d.formatType = AudioPluginFormatType::vst3;
        d.numInputChannels = numInputChannels;
        d.numOutputChannels = numOutputChannels;
        return d;
    }

    static AudioBusLayout makeLayout (int numInputChannels, int numOutputChannels)
    {
        std::vector<AudioBus> inputs;
        std::vector<AudioBus> outputs;

        if (numInputChannels > 0)
            inputs.emplace_back ("Input", AudioBus::Type::Audio, AudioBus::Direction::Input, numInputChannels);

        if (numOutputChannels > 0)
            outputs.emplace_back ("Output", AudioBus::Type::Audio, AudioBus::Direction::Output, numOutputChannels);

        return AudioBusLayout (std::move (inputs), std::move (outputs));
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
    instance.prepareToPlay (AudioSpec (44100.0f, 512));
    instance.releaseResources();
    EXPECT_FALSE (instance.prepared);
}

TEST_F (AudioPluginInstanceTests, ProcessBlockIncrementsCounter)
{
    AudioBuffer<float> audio (2, 512);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    instance.prepareToPlay (AudioSpec (44100.0f, 512));
    AudioProcessContext<float> ctx { audio, midi, params };
    instance.processBlock (ctx);

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
    ParameterChangeBuffer params;

    audio.setSample (0, 0, 1.0);

    doublePrecisionInstance.setProcessingPrecision (AudioProcessor::ProcessingPrecision::doublePrecision);
    doublePrecisionInstance.prepareToPlay (AudioSpec (44100.0f, 512));
    AudioProcessContext<double> ctx { audio, midi, params };
    doublePrecisionInstance.processBlock (ctx);

    EXPECT_EQ (0, doublePrecisionInstance.processCallCount);
    EXPECT_EQ (1, doublePrecisionInstance.doubleProcessCallCount);
    EXPECT_DOUBLE_EQ (0.0, audio.getSample (0, 0));
}

TEST_F (AudioPluginInstanceTests, BusLayoutMatchesConstructorArg)
{
    EXPECT_EQ (1, instance.getNumAudioInputs());
    EXPECT_EQ (1, instance.getNumAudioOutputs());
}

TEST_F (AudioPluginInstanceTests, DefaultsToRealtimeAndNotBypassed)
{
    EXPECT_FALSE (instance.isNonRealtime());
    EXPECT_FALSE (instance.isBypassed());
}

TEST_F (AudioPluginInstanceTests, RenderModeAndBypassCanBeSetByHost)
{
    instance.setNonRealtime (true);
    instance.setBypassed (true);

    EXPECT_TRUE (instance.isNonRealtime());
    EXPECT_TRUE (instance.isBypassed());
}

TEST (AudioPluginInstanceBusLayoutTests, AudioInputAndOutputCountsIgnoreMidiBuses)
{
    CountOnlyPluginInstance instanceWithMidiBuses;

    EXPECT_EQ (0, instanceWithMidiBuses.getNumAudioInputs());
    EXPECT_EQ (0, instanceWithMidiBuses.getNumAudioOutputs());
}

TEST (AudioPluginInstanceBypassTests, CopiesMatchingInputChannelsAndClearsExtraOutputs)
{
    BypassPluginInstance bypassInstance (1, 2);
    AudioBuffer<float> audio (2, 8);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    audio.setSample (0, 0, 0.75f);
    audio.setSample (1, 0, 0.5f);

    bypassInstance.setBypassed (true);
    AudioProcessContext<float> ctx { audio, midi, params };
    bypassInstance.processBlock (ctx);

    EXPECT_FLOAT_EQ (0.75f, audio.getSample (0, 0));
    EXPECT_FLOAT_EQ (0.0f, audio.getSample (1, 0));
}

TEST (AudioPluginInstanceBypassTests, ClearsInstrumentOutputsWithoutInputs)
{
    BypassPluginInstance bypassInstance (0, 2);
    AudioBuffer<float> audio (2, 8);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    audio.setSample (0, 0, 0.75f);
    audio.setSample (1, 0, 0.5f);

    bypassInstance.setBypassed (true);
    AudioProcessContext<float> ctx { audio, midi, params };
    bypassInstance.processBlock (ctx);

    EXPECT_FLOAT_EQ (0.0f, audio.getSample (0, 0));
    EXPECT_FLOAT_EQ (0.0f, audio.getSample (1, 0));
}

TEST (AudioPluginInstanceBypassTests, SupportsDoublePrecisionBypass)
{
    BypassPluginInstance bypassInstance (1, 2);
    AudioBuffer<double> audio (2, 8);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    audio.setSample (0, 0, 0.75);
    audio.setSample (1, 0, 0.5);

    AudioProcessContext<double> ctx { audio, midi, params };
    bypassInstance.processBlockBypassed (ctx);

    EXPECT_DOUBLE_EQ (0.75, audio.getSample (0, 0));
    EXPECT_DOUBLE_EQ (0.0, audio.getSample (1, 0));
}
