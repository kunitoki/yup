#include <gtest/gtest.h>

#include <yup_audio_plugin_host/yup_audio_plugin_host.h>

using namespace yup;

TEST (AudioPluginDescriptionTests, DefaultFormatTypeIsUnknown)
{
    AudioPluginDescription desc;
    EXPECT_EQ (AudioPluginFormatType::unknown, desc.formatType);
}

TEST (AudioPluginDescriptionTests, IsInstrumentDefaultsFalse)
{
    AudioPluginDescription desc;
    EXPECT_FALSE (desc.isInstrument);
}

TEST (AudioPluginDescriptionTests, EqualityMatchesAllFields)
{
    AudioPluginDescription a;
    a.name = "Synth";
    a.identifier = "com.vendor.synth";
    a.formatType = AudioPluginFormatType::vst3;

    AudioPluginDescription b = a;
    EXPECT_EQ (a, b);

    b.version = "2.0";
    EXPECT_NE (a, b);
}

TEST (AudioPluginHostContextTests, DefaultSampleRate)
{
    AudioPluginHostContext ctx;
    EXPECT_FLOAT_EQ (44100.0f, ctx.sampleRate);
}

TEST (AudioPluginHostContextTests, DefaultMaxBlockSize)
{
    AudioPluginHostContext ctx;
    EXPECT_EQ (512, ctx.maxBlockSize);
}
