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

TEST (AudioPluginStateTests, EmptyStateRoundTrip)
{
    StatefulFakeInstance instance;
    MemoryBlock empty;

    EXPECT_TRUE (instance.loadStateFromMemory (empty).wasOk());

    MemoryBlock readBack;
    EXPECT_TRUE (instance.saveStateIntoMemory (readBack).wasOk());

    EXPECT_EQ (readBack.getSize(), 0u);
}

TEST (AudioPluginStateTests, LargeStateRoundTrip)
{
    StatefulFakeInstance instance;
    MemoryBlock large (4096, true);
    for (int i = 0; i < 4096; ++i)
        static_cast<char*> (large.getData())[i] = (char) (i & 0xFF);

    EXPECT_TRUE (instance.loadStateFromMemory (large).wasOk());

    MemoryBlock readBack;
    EXPECT_TRUE (instance.saveStateIntoMemory (readBack).wasOk());
    EXPECT_EQ (large, readBack);
}

TEST (AudioPluginStateTests, GetNumPresetsReturnsCorrectCount)
{
    StatefulFakeInstance instance;
    EXPECT_EQ (3, instance.getNumPresets());
}

TEST (AudioPluginStateTests, HasEditorReturnsFalse)
{
    StatefulFakeInstance instance;
    EXPECT_FALSE (instance.hasEditor());
}

TEST (AudioPluginStateTests, DefaultCurrentPresetIsZero)
{
    StatefulFakeInstance instance;
    EXPECT_EQ (0, instance.getCurrentPreset());
}

TEST (AudioPluginStateTests, MultipleSavesPreserveLastLoad)
{
    StatefulFakeInstance instance;

    const unsigned char firstData[] = { 0x01, 0x02 };
    EXPECT_TRUE (instance.loadStateFromMemory ({ firstData, sizeof (firstData) }).wasOk());

    const unsigned char secondData[] = { 0xAA, 0xBB, 0xCC };
    EXPECT_TRUE (instance.loadStateFromMemory ({ secondData, sizeof (secondData) }).wasOk());

    MemoryBlock readBack;
    EXPECT_TRUE (instance.saveStateIntoMemory (readBack).wasOk());

    EXPECT_EQ (readBack.getSize(), sizeof (secondData));
    EXPECT_EQ (memcmp (readBack.getData(), secondData, sizeof (secondData)), 0);
}
