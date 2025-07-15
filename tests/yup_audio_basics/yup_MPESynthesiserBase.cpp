/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <yup_audio_basics/yup_audio_basics.h>
#include <gtest/gtest.h>

using namespace yup;

namespace
{
enum class CallbackKind
{
    process,
    midi
};

struct StartAndLength
{
    StartAndLength (int s, int l)
        : start (s)
        , length (l)
    {
    }

    int start = 0;
    int length = 0;

    std::tuple<const int&, const int&> tie() const noexcept { return std::tie (start, length); }

    bool operator== (const StartAndLength& other) const noexcept { return tie() == other.tie(); }

    bool operator!= (const StartAndLength& other) const noexcept { return tie() != other.tie(); }

    bool operator< (const StartAndLength& other) const noexcept { return tie() < other.tie(); }
};

struct Events
{
    std::vector<StartAndLength> blocks;
    std::vector<MidiMessage> messages;
    std::vector<CallbackKind> order;
};

class MockSynthesiser final : public MPESynthesiserBase
{
public:
    Events events;

    void handleMidiEvent (const MidiMessage& m) override
    {
        events.messages.emplace_back (m);
        events.order.emplace_back (CallbackKind::midi);
    }

private:
    using MPESynthesiserBase::renderNextSubBlock;

    void renderNextSubBlock (AudioBuffer<float>&,
                             int startSample,
                             int numSamples) override
    {
        events.blocks.push_back ({ startSample, numSamples });
        events.order.emplace_back (CallbackKind::process);
    }
};

MidiBuffer makeTestBuffer (const int bufferLength)
{
    MidiBuffer result;

    for (int i = 0; i != bufferLength; ++i)
        result.addEvent ({}, i);

    return result;
}

int sumBlockLengths (const std::vector<StartAndLength>& b)
{
    const auto addBlock = [] (int acc, const StartAndLength& info)
    {
        return acc + info.length;
    };
    return std::accumulate (b.begin(), b.end(), 0, addBlock);
}

bool blockLengthsAreValid (const std::vector<StartAndLength>& info, int minLength, bool strict)
{
    if (info.size() <= 1)
        return true;

    const auto lengthIsValid = [&] (const StartAndLength& s)
    {
        return minLength <= s.length;
    };
    const auto begin = strict ? info.begin() : std::next (info.begin());
    // The final block is allowed to be shorter than the minLength
    return std::all_of (begin, std::prev (info.end()), lengthIsValid);
}
} // namespace

class MPESynthesiserBaseTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup common test data if needed
    }
};

TEST_F (MPESynthesiserBaseTest, RenderingSparseSubblocksWorks)
{
    const int blockSize = 512;
    const auto midi = [&]
    {
        MidiBuffer b;
        b.addEvent ({}, blockSize / 2);
        return b;
    }();
    AudioBuffer<float> audio (1, blockSize);

    const auto processEvents = [&] (int start, int length)
    {
        MockSynthesiser synth;
        synth.setMinimumRenderingSubdivisionSize (1, false);
        synth.setCurrentPlaybackSampleRate (44100);
        synth.renderNextBlock (audio, midi, start, length);
        return synth.events;
    };

    {
        const auto e = processEvents (0, blockSize);
        EXPECT_EQ (e.blocks.size(), 2u);
        EXPECT_EQ (e.messages.size(), 1u);
        EXPECT_TRUE (std::is_sorted (e.blocks.begin(), e.blocks.end()));
        EXPECT_EQ (sumBlockLengths (e.blocks), blockSize);
        EXPECT_EQ (e.order, (std::vector<CallbackKind> { CallbackKind::process, CallbackKind::midi, CallbackKind::process }));
    }
}

TEST_F (MPESynthesiserBaseTest, RenderingSubblocksProcessesOnlyContainedMidiEvents)
{
    const int blockSize = 512;
    const auto midi = makeTestBuffer (blockSize);
    AudioBuffer<float> audio (1, blockSize);

    const auto processEvents = [&] (int start, int length)
    {
        MockSynthesiser synth;
        synth.setMinimumRenderingSubdivisionSize (1, false);
        synth.setCurrentPlaybackSampleRate (44100);
        synth.renderNextBlock (audio, midi, start, length);
        return synth.events;
    };

    {
        const int subBlockLength = 0;
        const auto e = processEvents (0, subBlockLength);
        EXPECT_EQ (e.blocks.size(), 0u);
        EXPECT_EQ (e.messages.size(), 0u);
        EXPECT_TRUE (std::is_sorted (e.blocks.begin(), e.blocks.end()));
        EXPECT_EQ (sumBlockLengths (e.blocks), subBlockLength);
    }

    {
        const int subBlockLength = 0;
        const auto e = processEvents (1, subBlockLength);
        EXPECT_EQ (e.blocks.size(), 0u);
        EXPECT_EQ (e.messages.size(), 0u);
        EXPECT_TRUE (std::is_sorted (e.blocks.begin(), e.blocks.end()));
        EXPECT_EQ (sumBlockLengths (e.blocks), subBlockLength);
    }

    {
        const int subBlockLength = 1;
        const auto e = processEvents (1, subBlockLength);
        EXPECT_EQ (e.blocks.size(), 1u);
        EXPECT_EQ (e.messages.size(), 1u);
        EXPECT_TRUE (std::is_sorted (e.blocks.begin(), e.blocks.end()));
        EXPECT_EQ (sumBlockLengths (e.blocks), subBlockLength);
        EXPECT_EQ (e.order, (std::vector<CallbackKind> { CallbackKind::midi, CallbackKind::process }));
    }

    {
        const auto e = processEvents (0, blockSize);
        EXPECT_EQ (e.blocks.size(), blockSize);
        EXPECT_EQ (e.messages.size(), blockSize);
        EXPECT_TRUE (std::is_sorted (e.blocks.begin(), e.blocks.end()));
        EXPECT_EQ (sumBlockLengths (e.blocks), blockSize);
        EXPECT_EQ (e.order.front(), CallbackKind::midi);
    }
}

TEST_F (MPESynthesiserBaseTest, SubblocksRespectTheirMinimumSize)
{
    const int blockSize = 512;
    const auto midi = makeTestBuffer (blockSize);
    AudioBuffer<float> audio (1, blockSize);

    for (auto strict : { false, true })
    {
        for (auto subblockSize : { 1, 16, 32, 64, 1024 })
        {
            MockSynthesiser synth;
            synth.setMinimumRenderingSubdivisionSize (subblockSize, strict);
            synth.setCurrentPlaybackSampleRate (44100);
            synth.renderNextBlock (audio, midi, 0, blockSize);

            const auto& e = synth.events;
            EXPECT_NEAR (float (e.blocks.size()),
                         std::ceil ((float) blockSize / (float) subblockSize),
                         1.0f);
            EXPECT_EQ (e.messages.size(), blockSize);
            EXPECT_TRUE (std::is_sorted (e.blocks.begin(), e.blocks.end()));
            EXPECT_EQ (sumBlockLengths (e.blocks), blockSize);
            EXPECT_TRUE (blockLengthsAreValid (e.blocks, subblockSize, strict));
        }
    }

    {
        MockSynthesiser synth;
        synth.setMinimumRenderingSubdivisionSize (32, true);
        synth.setCurrentPlaybackSampleRate (44100);
        synth.renderNextBlock (audio, MidiBuffer {}, 0, 16);

        EXPECT_EQ (synth.events.blocks, (std::vector<StartAndLength> { { 0, 16 } }));
        EXPECT_EQ (synth.events.order, (std::vector<CallbackKind> { CallbackKind::process }));
        EXPECT_TRUE (synth.events.messages.empty());
    }
}
