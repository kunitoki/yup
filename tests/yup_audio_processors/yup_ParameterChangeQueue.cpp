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

#include <yup_audio_processors/yup_audio_processors.h>

#include <gtest/gtest.h>

#include <memory>
#include <thread>
#include <vector>

using namespace yup;

//==============================================================================
class ParameterChangeQueueTests : public ::testing::Test
{
protected:
    struct TestProcessor final : AudioProcessorBase
    {
        TestProcessor()
            : AudioProcessorBase ("Queue Test")
        {
        }

        int getCurrentPreset() const noexcept override { return 0; }

        void setCurrentPreset (int) noexcept override {}

        int getNumPresets() const override { return 0; }

        String getPresetName (int) const override { return {}; }

        void setPresetName (int, StringRef) override {}
    };

    static std::vector<int> drain (ParameterChangeQueue& queue)
    {
        std::vector<int> result;
        queue.readChanges ([&result] (int index) { result.push_back (index); });
        return result;
    }
};

TEST_F (ParameterChangeQueueTests, StartsEmpty)
{
    ParameterChangeQueue queue (4);

    EXPECT_EQ (4, queue.getNumParameters());
    EXPECT_TRUE (drain (queue).empty());
}

TEST_F (ParameterChangeQueueTests, ReportsMarkedIndicesInOrderAndClears)
{
    ParameterChangeQueue queue (70);
    queue.markChanged (69);
    queue.markChanged (0);
    queue.markChanged (33);

    EXPECT_EQ ((std::vector<int> { 0, 33, 69 }), drain (queue));
    EXPECT_TRUE (drain (queue).empty());
}

TEST_F (ParameterChangeQueueTests, CoalescesRepeatedMarks)
{
    ParameterChangeQueue queue (4);
    for (int i = 0; i < 100; ++i)
        queue.markChanged (1);

    EXPECT_EQ ((std::vector<int> { 1 }), drain (queue));
}

TEST_F (ParameterChangeQueueTests, IgnoresOutOfRangeIndices)
{
    ParameterChangeQueue queue (4);
    queue.markChanged (-1);
    queue.markChanged (4);

    EXPECT_TRUE (drain (queue).empty());
}

TEST_F (ParameterChangeQueueTests, ResizeClearsPendingChanges)
{
    ParameterChangeQueue queue (4);
    queue.markChanged (2);
    queue.resize (8);

    EXPECT_EQ (8, queue.getNumParameters());
    EXPECT_TRUE (drain (queue).empty());
}

TEST_F (ParameterChangeQueueTests, GrowthPreservesPendingChangesAcrossWordBoundaries)
{
    ParameterChangeQueue queue (1);
    queue.markChanged (0);
    queue.ensureSize (32);
    queue.markChanged (31);
    queue.ensureSize (33);
    queue.markChanged (32);
    queue.ensureSize (129);
    queue.markChanged (128);
    queue.ensureSize (2);

    EXPECT_EQ (129, queue.getNumParameters());
    EXPECT_EQ ((std::vector<int> { 0, 31, 32, 128 }), drain (queue));
    EXPECT_TRUE (drain (queue).empty());
}

TEST_F (ParameterChangeQueueTests, ChangesMarkedDuringDispatchRemainPending)
{
    ParameterChangeQueue queue (2);
    queue.markChanged (0);
    queue.readChanges ([&queue] (int)
    {
        queue.markChanged (0);
        queue.markChanged (1);
    });

    EXPECT_EQ ((std::vector<int> { 0, 1 }), drain (queue));
    EXPECT_TRUE (drain (queue).empty());
}

TEST_F (ParameterChangeQueueTests, ConcurrentProducersLoseNoIndex)
{
    constexpr int numParameters = 256;
    constexpr int numProducers = 4;

    ParameterChangeQueue queue (numParameters);

    std::vector<std::thread> producers;
    for (int producer = 0; producer < numProducers; ++producer)
    {
        producers.emplace_back ([&queue, producer]
        {
            for (int index = producer; index < numParameters; index += numProducers)
                queue.markChanged (index);
        });
    }

    std::vector<bool> seen (numParameters, false);
    const auto collect = [&]
    {
        queue.readChanges ([&seen] (int index) { seen[static_cast<std::size_t> (index)] = true; });
    };

    for (int i = 0; i < 1000; ++i)
        collect();

    for (auto& producer : producers)
        producer.join();

    collect();

    for (int index = 0; index < numParameters; ++index)
        EXPECT_TRUE (seen[static_cast<std::size_t> (index)]) << "index " << index;
}

TEST_F (ParameterChangeQueueTests, ParameterMarksOnlyRealChanges)
{
    TestProcessor processor;
    auto parameter = AudioParameterBuilder().withID ("p").withRange (0.0f, 1.0f).withDefault (0.0f).build();
    processor.addParameter (parameter);
    auto& queue = processor.getParameterChangeQueue();

    parameter->setValue (0.0f);
    EXPECT_TRUE (drain (queue).empty());

    parameter->setValue (0.5f);
    EXPECT_EQ ((std::vector<int> { 0 }), drain (queue));

    parameter->setValue (0.5f);
    EXPECT_TRUE (drain (queue).empty());
}

TEST_F (ParameterChangeQueueTests, RegisteringMoreParametersPreservesPendingChanges)
{
    TestProcessor processor;
    auto parameter = AudioParameterBuilder().withID ("p").withRange (0.0f, 1.0f).build();
    processor.addParameter (parameter);
    parameter->setValue (0.5f);

    for (int index = 0; index < 70; ++index)
        processor.addParameter (AudioParameterBuilder().withID (String ("extra") + String (index)).build());

    EXPECT_EQ ((std::vector<int> { 0 }), drain (processor.getParameterChangeQueue()));
}

TEST_F (ParameterChangeQueueTests, SharedParameterKeepsEachOwnersIndexAndSurvivesItsFirstOwner)
{
    auto first = std::make_unique<TestProcessor>();
    TestProcessor second;
    auto parameter = AudioParameterBuilder().withID ("shared").withRange (0.0f, 1.0f).build();
    first->addParameter (parameter);
    second.addParameter (AudioParameterBuilder().withID ("extra").build());
    second.addParameter (parameter);

    parameter->setValue (0.5f);
    EXPECT_EQ ((std::vector<int> { 0 }), drain (first->getParameterChangeQueue()));
    EXPECT_EQ ((std::vector<int> { 1 }), drain (second.getParameterChangeQueue()));

    first.reset();
    parameter->setValue (0.75f);
    EXPECT_EQ ((std::vector<int> { 1 }), drain (second.getParameterChangeQueue()));
}

TEST_F (ParameterChangeQueueTests, SharedParameterSurvivesItsLastRegisteredOwner)
{
    TestProcessor first;
    auto second = std::make_unique<TestProcessor>();
    auto parameter = AudioParameterBuilder().withID ("shared").withRange (0.0f, 1.0f).build();
    first.addParameter (parameter);
    second->addParameter (parameter);
    second.reset();

    parameter->setValue (0.5f);
    EXPECT_EQ ((std::vector<int> { 0 }), drain (first.getParameterChangeQueue()));
}
