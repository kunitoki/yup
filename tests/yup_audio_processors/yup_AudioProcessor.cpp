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

#include <yup_audio_processors/yup_audio_processors.h>

using namespace yup;

//==============================================================================

class AudioProcessorTests : public ::testing::Test
{
protected:
    class Processor : public AudioProcessor
    {
    public:
        explicit Processor (AudioBusLayout layout)
            : AudioProcessor ("Test", std::move (layout))
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

        void hostInfoChanged (const HostInfo& newInfo) override
        {
            ++hostInfoChangedCount;
            lastHostInfo = newInfo;
        }

        int hostInfoChangedCount = 0;
        HostInfo lastHostInfo;
    };

    class FlexibleProcessor final : public Processor
    {
    public:
        using Processor::Processor;

        bool isBusLayoutSupported (const AudioBusLayout& layout) const override
        {
            const auto inputs = layout.getAudioBusNumChannels (0, true);
            const auto outputs = layout.getAudioBusNumChannels (0, false);
            return inputs == outputs && (inputs == 1 || inputs == 2);
        }
    };

    class ChangeListener final : public AudioProcessorBase::Listener
    {
    public:
        void audioProcessorChanged (AudioProcessorBase*, const AudioProcessorBase::ChangeDetails& details) override
        {
            ++changedCount;
            lastDetails = details;
        }

        int changedCount = 0;
        AudioProcessorBase::ChangeDetails lastDetails;
    };

    static AudioBusLayout stereoLayout()
    {
        return AudioBusLayout ({ AudioBus ("In", AudioBus::Type::Audio, AudioBus::Direction::Input, 2) },
                               { AudioBus ("Out", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) });
    }

    static AudioParameter::Ptr makeParameter (StringRef id)
    {
        return AudioParameterBuilder().withID (id).withName (id).withRange (0.0f, 1.0f).build();
    }
};

//==============================================================================

TEST_F (AudioProcessorTests, BypassParameterIsSteppedBooleanAndFound)
{
    Processor processor (stereoLayout());
    EXPECT_EQ (nullptr, processor.getBypassParameter());

    auto bypass = AudioParameterBuilder().withID ("bypass").withName ("Bypass").withBypass (true).build();
    processor.addParameter (makeParameter ("gain"));
    processor.addParameter (bypass);

    EXPECT_TRUE (bypass->isBypass());
    EXPECT_TRUE (bypass->isStepped());
    EXPECT_FLOAT_EQ (0.0f, bypass->getMinimumValue());
    EXPECT_FLOAT_EQ (1.0f, bypass->getMaximumValue());
    EXPECT_EQ (bypass.get(), processor.getBypassParameter().get());
}

TEST_F (AudioProcessorTests, RegularParameterIsNotBypass)
{
    EXPECT_FALSE (makeParameter ("gain")->isBypass());
}

TEST_F (AudioProcessorTests, SetHostInfoNotifiesOnlyOnChange)
{
    Processor processor (stereoLayout());

    AudioProcessorBase::HostInfo info;
    info.trackName = "Drums";
    info.trackColor = 0xff112233u;

    processor.setHostInfo (info);
    processor.setHostInfo (info);

    EXPECT_EQ (1, processor.hostInfoChangedCount);
    EXPECT_EQ (String ("Drums"), processor.getHostInfo().trackName);
    EXPECT_EQ (0xff112233u, processor.getHostInfo().trackColor.value_or (0));
    EXPECT_TRUE (processor.lastHostInfo == info);

    info.trackColor.reset();
    processor.setHostInfo (info);

    EXPECT_EQ (2, processor.hostInfoChangedCount);
    EXPECT_FALSE (processor.getHostInfo().trackColor.has_value());
}

TEST_F (AudioProcessorTests, DefaultMidiControllerMappingIsEmpty)
{
    Processor processor (stereoLayout());
    EXPECT_EQ (-1, processor.getParameterIndexForMidiController (1, 7));
}

TEST_F (AudioProcessorTests, DefaultProcessorDoesNotOptIntoNoteExpression)
{
    Processor processor (stereoLayout());
    EXPECT_FALSE (processor.supportsNoteExpression());
    EXPECT_FALSE (processor.supportsMpe());
}

TEST_F (AudioProcessorTests, DefaultProcessorOnlySupportsCurrentLayout)
{
    Processor processor (stereoLayout());

    EXPECT_TRUE (processor.isBusLayoutSupported (stereoLayout()));
    EXPECT_TRUE (processor.setBusLayout (stereoLayout()).wasOk());

    const auto mono = stereoLayout().withAudioBusNumChannels (0, true, 1).withAudioBusNumChannels (0, false, 1);
    EXPECT_FALSE (processor.isBusLayoutSupported (mono));
    EXPECT_TRUE (processor.setBusLayout (mono).failed());
    EXPECT_EQ (2, processor.getBusLayout().getNumAudioOutputChannels());
}

TEST_F (AudioProcessorTests, SetBusLayoutAppliesSupportedLayoutAndNotifies)
{
    FlexibleProcessor processor (stereoLayout());
    ChangeListener listener;
    processor.addListener (&listener);

    const auto mono = stereoLayout().withAudioBusNumChannels (0, true, 1).withAudioBusNumChannels (0, false, 1);
    EXPECT_TRUE (processor.setBusLayout (mono).wasOk());
    EXPECT_EQ (1, processor.getBusLayout().getNumAudioOutputChannels());
    EXPECT_EQ (1, listener.changedCount);
    EXPECT_TRUE (listener.lastDetails.busLayoutChanged);

    const auto mismatched = stereoLayout().withAudioBusNumChannels (0, true, 1);
    EXPECT_TRUE (processor.setBusLayout (mismatched).failed());
    EXPECT_EQ (1, listener.changedCount);

    processor.removeListener (&listener);
}

TEST_F (AudioProcessorTests, SetBusLayoutRejectsDifferentBusStructure)
{
    FlexibleProcessor processor (stereoLayout());

    const AudioBusLayout extraBus ({ AudioBus ("In", AudioBus::Type::Audio, AudioBus::Direction::Input, 2),
                                     AudioBus ("Side", AudioBus::Type::Audio, AudioBus::Direction::Input, 2, AudioBus::Role::Auxiliary) },
                                   { AudioBus ("Out", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) });

    EXPECT_TRUE (processor.setBusLayout (extraBus).failed());
    EXPECT_EQ (1, processor.getBusLayout().getNumAudioBuses (true));
}

TEST_F (AudioProcessorTests, HostTypeIsStoredGlobally)
{
    const auto previous = AudioProcessor::getHostType();

    AudioProcessor::setHostType (PluginHostType (PluginHostType::Host::reaper));
    EXPECT_TRUE (AudioProcessor::getHostType().isReaper());

    AudioProcessor::setHostType (previous);
}
