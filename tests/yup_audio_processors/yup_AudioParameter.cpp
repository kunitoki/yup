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

namespace
{

class TestAudioProcessor final : public AudioProcessor
{
public:
    TestAudioProcessor()
        : AudioProcessor ("Test", AudioBusLayout ({}, {}))
    {
    }

    void prepareToPlay (float, int) override {}

    void releaseResources() override {}

    void processBlock (AudioProcessContext<float>& context) override
    {
        ignoreUnused (context);
    }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    Result loadStateFromMemory (const MemoryBlock&) override { return Result::ok(); }

    Result saveStateIntoMemory (MemoryBlock&) override { return Result::ok(); }

    bool hasEditor() const override { return false; }
};

AudioParameter::Ptr makeParameter (StringRef id, StringRef name)
{
    return AudioParameterBuilder()
        .withID (id)
        .withName (name)
        .withRange (0.0f, 1.0f)
        .withDefault (0.5f)
        .build();
}

} // namespace

TEST (AudioParameterTests, UsesIndexAsHostIDByDefault)
{
    TestAudioProcessor processor;
    auto first = makeParameter ("first", "First");
    auto second = makeParameter ("second", "Second");

    processor.addParameter (first);
    processor.addParameter (second);

    EXPECT_FALSE (first->hasExplicitHostParameterID());
    EXPECT_FALSE (second->hasExplicitHostParameterID());
    EXPECT_EQ (0u, first->getHostParameterID());
    EXPECT_EQ (1u, second->getHostParameterID());
    EXPECT_EQ (first.get(), processor.getParameterByHostID (0u).get());
    EXPECT_EQ (second.get(), processor.getParameterByHostID (1u).get());
}

TEST (AudioParameterTests, UsesExplicitStableHostIDWhenProvided)
{
    TestAudioProcessor processor;
    auto parameter = AudioParameterBuilder()
                         .withID ("gain")
                         .withName ("Gain")
                         .withHostID (1001u)
                         .withRange (0.0f, 1.0f)
                         .withDefault (0.5f)
                         .build();

    processor.addParameter (parameter);

    EXPECT_TRUE (parameter->hasExplicitHostParameterID());
    EXPECT_EQ (1001u, parameter->getHostParameterID());
    EXPECT_EQ (0, processor.getParameterIndexByHostID (1001u));
    EXPECT_EQ (parameter.get(), processor.getParameterByHostID (1001u).get());
    EXPECT_EQ (nullptr, processor.getParameterByHostID (0u).get());
}
