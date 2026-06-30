#include <gtest/gtest.h>

#include <yup_audio_plugin_host/yup_audio_plugin_host.h>

using namespace yup;

namespace
{

class StatefulFakeInstance : public AudioPluginInstance
{
public:
    StatefulFakeInstance()
        : AudioPluginInstance (AudioPluginDescription {},
                               AudioBusLayout ({}, {}))
    {
    }

    void prepareToPlay (const AudioSpec&) override {}

    void releaseResources() override {}

    void processBlock (AudioProcessContext<float>&) override {}

    int getCurrentPreset() const noexcept override { return currentPreset; }

    void setCurrentPreset (int i) noexcept override { currentPreset = i; }

    int getNumPresets() const override { return 3; }

    String getPresetName (int i) const override { return "Preset " + String (i); }

    void setPresetName (int, StringRef) override {}

    bool hasEditor() const override { return false; }

    Result loadStateFromMemory (const MemoryBlock& block) override
    {
        storedState = block;
        return Result::ok();
    }

    Result saveStateIntoMemory (MemoryBlock& block) override
    {
        block = storedState;
        return Result::ok();
    }

    int currentPreset = 0;
    MemoryBlock storedState;
};

} // namespace

TEST (AudioPluginStateTests, RoundTripPreservesData)
{
    StatefulFakeInstance instance;

    const char rawData[] = { 0x01, 0x02, 0x03, 0x04 };
    MemoryBlock written (rawData, sizeof (rawData));

    EXPECT_TRUE (instance.loadStateFromMemory (written).wasOk());

    MemoryBlock readBack;
    EXPECT_TRUE (instance.saveStateIntoMemory (readBack).wasOk());

    EXPECT_EQ (written, readBack);
}

TEST (AudioPluginStateTests, PresetNameByIndex)
{
    StatefulFakeInstance instance;
    EXPECT_EQ ("Preset 0", instance.getPresetName (0));
    EXPECT_EQ ("Preset 2", instance.getPresetName (2));
}

TEST (AudioPluginStateTests, SetAndGetCurrentPreset)
{
    StatefulFakeInstance instance;
    instance.setCurrentPreset (2);
    EXPECT_EQ (2, instance.getCurrentPreset());
}
