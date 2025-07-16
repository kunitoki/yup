/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#include <yup_audio_basics/yup_audio_basics.h>

#include <gtest/gtest.h>

using namespace yup;

namespace
{
// Test implementation of AudioPlayHead for testing
class TestAudioPlayHead : public AudioPlayHead
{
public:
    Optional<PositionInfo> getPosition() const override
    {
        return testPosition;
    }

    void setTestPosition (const PositionInfo& pos)
    {
        testPosition = pos;
    }

    void clearTestPosition()
    {
        testPosition = nullopt;
    }

private:
    Optional<PositionInfo> testPosition;
};
} // namespace

class AudioPlayHeadTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        playHead = std::make_unique<TestAudioPlayHead>();
    }

    std::unique_ptr<TestAudioPlayHead> playHead;
};

TEST_F (AudioPlayHeadTests, DefaultTransportControlMethodsExist)
{
    // Test that default implementations exist and don't crash
    EXPECT_FALSE (playHead->canControlTransport());

    // These should not crash
    playHead->transportPlay (true);
    playHead->transportPlay (false);
    playHead->transportRecord (true);
    playHead->transportRecord (false);
    playHead->transportRewind();
}

TEST_F (AudioPlayHeadTests, FrameRateConstructorAndGetters)
{
    // Test default constructor
    AudioPlayHead::FrameRate defaultRate;
    EXPECT_EQ (0, defaultRate.getBaseRate());
    EXPECT_FALSE (defaultRate.isDrop());
    EXPECT_FALSE (defaultRate.isPullDown());
    EXPECT_EQ (AudioPlayHead::FrameRateType::fpsUnknown, defaultRate.getType());
    EXPECT_EQ (0.0, defaultRate.getEffectiveRate());

    // Test constructor from FrameRateType
    AudioPlayHead::FrameRate fps24 (AudioPlayHead::FrameRateType::fps24);
    EXPECT_EQ (24, fps24.getBaseRate());
    EXPECT_FALSE (fps24.isDrop());
    EXPECT_FALSE (fps24.isPullDown());
    EXPECT_EQ (AudioPlayHead::FrameRateType::fps24, fps24.getType());
    EXPECT_EQ (24.0, fps24.getEffectiveRate());

    // Test fps23976 (pulldown)
    AudioPlayHead::FrameRate fps23976 (AudioPlayHead::FrameRateType::fps23976);
    EXPECT_EQ (24, fps23976.getBaseRate());
    EXPECT_FALSE (fps23976.isDrop());
    EXPECT_TRUE (fps23976.isPullDown());
    EXPECT_EQ (AudioPlayHead::FrameRateType::fps23976, fps23976.getType());
    EXPECT_NEAR (24.0 / 1.001, fps23976.getEffectiveRate(), 0.001);
}

TEST_F (AudioPlayHeadTests, FrameRateWithMethods)
{
    AudioPlayHead::FrameRate rate;

    // Test withBaseRate
    auto rate30 = rate.withBaseRate (30);
    EXPECT_EQ (30, rate30.getBaseRate());
    EXPECT_EQ (0, rate.getBaseRate()); // Original unchanged

    // Test withDrop
    auto rateDrop = rate30.withDrop (true);
    EXPECT_TRUE (rateDrop.isDrop());
    EXPECT_FALSE (rate30.isDrop()); // Original unchanged

    // Test withPullDown
    auto ratePulldown = rate30.withPullDown (true);
    EXPECT_TRUE (ratePulldown.isPullDown());
    EXPECT_FALSE (rate30.isPullDown()); // Original unchanged

    // Test chaining
    auto complex = rate.withBaseRate (30).withDrop (true).withPullDown (true);
    EXPECT_EQ (30, complex.getBaseRate());
    EXPECT_TRUE (complex.isDrop());
    EXPECT_TRUE (complex.isPullDown());
    EXPECT_EQ (AudioPlayHead::FrameRateType::fps2997drop, complex.getType());
}

TEST_F (AudioPlayHeadTests, FrameRateEquality)
{
    AudioPlayHead::FrameRate rate1;
    AudioPlayHead::FrameRate rate2;

    EXPECT_TRUE (rate1 == rate2);
    EXPECT_FALSE (rate1 != rate2);

    auto rate3 = rate1.withBaseRate (24);
    EXPECT_FALSE (rate1 == rate3);
    EXPECT_TRUE (rate1 != rate3);

    auto rate4 = rate1.withBaseRate (24);
    EXPECT_TRUE (rate3 == rate4);
    EXPECT_FALSE (rate3 != rate4);
}

TEST_F (AudioPlayHeadTests, TimeSignatureDefaultsAndEquality)
{
    AudioPlayHead::TimeSignature sig1;
    EXPECT_EQ (4, sig1.numerator);
    EXPECT_EQ (4, sig1.denominator);

    AudioPlayHead::TimeSignature sig2;
    EXPECT_TRUE (sig1 == sig2);
    EXPECT_FALSE (sig1 != sig2);

    AudioPlayHead::TimeSignature sig3 { 3, 4 };
    EXPECT_FALSE (sig1 == sig3);
    EXPECT_TRUE (sig1 != sig3);

    AudioPlayHead::TimeSignature sig4 { 3, 4 };
    EXPECT_TRUE (sig3 == sig4);
    EXPECT_FALSE (sig3 != sig4);
}

TEST_F (AudioPlayHeadTests, LoopPointsDefaultsAndEquality)
{
    AudioPlayHead::LoopPoints loop1;
    EXPECT_EQ (0.0, loop1.ppqStart);
    EXPECT_EQ (0.0, loop1.ppqEnd);

    AudioPlayHead::LoopPoints loop2;
    EXPECT_TRUE (loop1 == loop2);
    EXPECT_FALSE (loop1 != loop2);

    AudioPlayHead::LoopPoints loop3 { 1.0, 5.0 };
    EXPECT_FALSE (loop1 == loop3);
    EXPECT_TRUE (loop1 != loop3);

    AudioPlayHead::LoopPoints loop4 { 1.0, 5.0 };
    EXPECT_TRUE (loop3 == loop4);
    EXPECT_FALSE (loop3 != loop4);
}

TEST_F (AudioPlayHeadTests, CurrentPositionInfoDefaults)
{
    AudioPlayHead::CurrentPositionInfo info;

    EXPECT_EQ (120.0, info.bpm);
    EXPECT_EQ (4, info.timeSigNumerator);
    EXPECT_EQ (4, info.timeSigDenominator);
    EXPECT_EQ (0, info.timeInSamples);
    EXPECT_EQ (0.0, info.timeInSeconds);
    EXPECT_EQ (0.0, info.editOriginTime);
    EXPECT_EQ (0.0, info.ppqPosition);
    EXPECT_EQ (0.0, info.ppqPositionOfLastBarStart);
    EXPECT_EQ (AudioPlayHead::FrameRateType::fps23976, info.frameRate.getType());
    EXPECT_FALSE (info.isPlaying);
    EXPECT_FALSE (info.isRecording);
    EXPECT_EQ (0.0, info.ppqLoopStart);
    EXPECT_EQ (0.0, info.ppqLoopEnd);
    EXPECT_FALSE (info.isLooping);
}

TEST_F (AudioPlayHeadTests, CurrentPositionInfoEquality)
{
    AudioPlayHead::CurrentPositionInfo info1;
    AudioPlayHead::CurrentPositionInfo info2;

    EXPECT_TRUE (info1 == info2);
    EXPECT_FALSE (info1 != info2);

    info2.bpm = 140.0;
    EXPECT_FALSE (info1 == info2);
    EXPECT_TRUE (info1 != info2);

    info1.bpm = 140.0;
    EXPECT_TRUE (info1 == info2);
    EXPECT_FALSE (info1 != info2);
}

TEST_F (AudioPlayHeadTests, CurrentPositionInfoResetToDefault)
{
    AudioPlayHead::CurrentPositionInfo info;
    info.bpm = 140.0;
    info.isPlaying = true;
    info.timeInSamples = 1000;

    info.resetToDefault();

    EXPECT_EQ (120.0, info.bpm);
    EXPECT_FALSE (info.isPlaying);
    EXPECT_EQ (0, info.timeInSamples);
}

TEST_F (AudioPlayHeadTests, PositionInfoGettersReturnNulloptByDefault)
{
    AudioPlayHead::PositionInfo info;

    EXPECT_FALSE (info.getTimeInSamples().hasValue());
    EXPECT_FALSE (info.getTimeInSeconds().hasValue());
    EXPECT_FALSE (info.getBpm().hasValue());
    EXPECT_FALSE (info.getTimeSignature().hasValue());
    EXPECT_FALSE (info.getLoopPoints().hasValue());
    EXPECT_FALSE (info.getBarCount().hasValue());
    EXPECT_FALSE (info.getPpqPositionOfLastBarStart().hasValue());
    EXPECT_FALSE (info.getFrameRate().hasValue());
    EXPECT_FALSE (info.getPpqPosition().hasValue());
    EXPECT_FALSE (info.getEditOriginTime().hasValue());
    EXPECT_FALSE (info.getHostTimeNs().hasValue());
    EXPECT_FALSE (info.getContinuousTimeInSamples().hasValue());

    // Boolean flags should default to false
    EXPECT_FALSE (info.getIsPlaying());
    EXPECT_FALSE (info.getIsRecording());
    EXPECT_FALSE (info.getIsLooping());
}

TEST_F (AudioPlayHeadTests, PositionInfoSettersAndGetters)
{
    AudioPlayHead::PositionInfo info;

    // Test setTimeInSamples/getTimeInSamples
    info.setTimeInSamples (1000);
    EXPECT_TRUE (info.getTimeInSamples().hasValue());
    EXPECT_EQ (1000, *info.getTimeInSamples());

    // Test setBpm/getBpm
    info.setBpm (120.0);
    EXPECT_TRUE (info.getBpm().hasValue());
    EXPECT_EQ (120.0, *info.getBpm());

    // Test setTimeSignature/getTimeSignature
    AudioPlayHead::TimeSignature sig { 3, 4 };
    info.setTimeSignature (sig);
    EXPECT_TRUE (info.getTimeSignature().hasValue());
    EXPECT_EQ (sig, *info.getTimeSignature());

    // Test boolean flags
    info.setIsPlaying (true);
    EXPECT_TRUE (info.getIsPlaying());

    info.setIsRecording (true);
    EXPECT_TRUE (info.getIsRecording());

    info.setIsLooping (true);
    EXPECT_TRUE (info.getIsLooping());
}

TEST_F (AudioPlayHeadTests, PositionInfoSetOptionalValues)
{
    AudioPlayHead::PositionInfo info;

    // Set values using Optional
    info.setTimeInSamples (makeOptional<int64_t> (2000));
    EXPECT_TRUE (info.getTimeInSamples().hasValue());
    EXPECT_EQ (2000, *info.getTimeInSamples());

    // Clear values using nullopt
    info.setTimeInSamples (nullopt);
    EXPECT_FALSE (info.getTimeInSamples().hasValue());

    // Test with FrameRate
    AudioPlayHead::FrameRate rate (AudioPlayHead::FrameRateType::fps30);
    info.setFrameRate (rate);
    EXPECT_TRUE (info.getFrameRate().hasValue());
    EXPECT_EQ (rate, *info.getFrameRate());
}

TEST_F (AudioPlayHeadTests, PositionInfoEquality)
{
    AudioPlayHead::PositionInfo info1;
    AudioPlayHead::PositionInfo info2;

    EXPECT_TRUE (info1 == info2);
    EXPECT_FALSE (info1 != info2);

    info1.setTimeInSamples (1000);
    EXPECT_FALSE (info1 == info2);
    EXPECT_TRUE (info1 != info2);

    info2.setTimeInSamples (1000);
    EXPECT_TRUE (info1 == info2);
    EXPECT_FALSE (info1 != info2);
}

TEST_F (AudioPlayHeadTests, PlayHeadPositionReturnsNulloptByDefault)
{
    playHead->clearTestPosition();
    auto position = playHead->getPosition();
    EXPECT_FALSE (position.hasValue());
}

TEST_F (AudioPlayHeadTests, PlayHeadCanReturnPositionInfo)
{
    AudioPlayHead::PositionInfo testInfo;
    testInfo.setTimeInSamples (5000);
    testInfo.setBpm (140.0);
    testInfo.setIsPlaying (true);

    playHead->setTestPosition (testInfo);

    auto position = playHead->getPosition();
    EXPECT_TRUE (position.hasValue());
    EXPECT_EQ (testInfo, *position);
}