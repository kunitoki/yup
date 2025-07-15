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

#include <gtest/gtest.h>

#include <yup_audio_basics/yup_audio_basics.h>

using namespace yup;

// Template test class for common SmoothedValue tests
template <class SmoothedValueType>
class CommonSmoothedValueTests : public ::testing::Test
{
protected:
    void runInitialStateTest()
    {
        SmoothedValueType sv;

        auto value = sv.getCurrentValue();
        EXPECT_EQ (sv.getTargetValue(), value);

        sv.getNextValue();
        EXPECT_EQ (sv.getCurrentValue(), value);
        EXPECT_FALSE (sv.isSmoothing());
    }

    void runResettingTest()
    {
        auto initialValue = 15.0f;

        SmoothedValueType sv (initialValue);
        sv.reset (3);
        EXPECT_EQ (sv.getCurrentValue(), initialValue);

        auto targetValue = initialValue + 1.0f;
        sv.setTargetValue (targetValue);
        EXPECT_EQ (sv.getTargetValue(), targetValue);
        EXPECT_EQ (sv.getCurrentValue(), initialValue);
        EXPECT_TRUE (sv.isSmoothing());

        auto currentValue = sv.getNextValue();
        EXPECT_GT (currentValue, initialValue);
        EXPECT_EQ (sv.getCurrentValue(), currentValue);
        EXPECT_EQ (sv.getTargetValue(), targetValue);
        EXPECT_TRUE (sv.isSmoothing());

        sv.reset (5);

        EXPECT_EQ (sv.getCurrentValue(), targetValue);
        EXPECT_EQ (sv.getTargetValue(), targetValue);
        EXPECT_FALSE (sv.isSmoothing());

        sv.getNextValue();
        EXPECT_EQ (sv.getCurrentValue(), targetValue);

        sv.setTargetValue (1.5f);
        sv.getNextValue();

        float newStart = 0.2f;
        sv.setCurrentAndTargetValue (newStart);
        EXPECT_EQ (sv.getNextValue(), newStart);
        EXPECT_EQ (sv.getTargetValue(), newStart);
        EXPECT_EQ (sv.getCurrentValue(), newStart);
        EXPECT_FALSE (sv.isSmoothing());
    }

    void runSampleRateTest()
    {
        SmoothedValueType svSamples { 3.0f };
        auto svTime = svSamples;

        auto numSamples = 12;

        svSamples.reset (numSamples);
        svTime.reset (numSamples * 2, 1.0);

        for (int i = 0; i < numSamples; ++i)
        {
            svTime.skip (1);
            EXPECT_NEAR (svSamples.getNextValue(), svTime.getNextValue(), 1.0e-7f);
        }
    }

    void runBlockProcessingTest()
    {
        SmoothedValueType sv (1.0f);

        sv.reset (12);
        sv.setTargetValue (2.0f);

        const auto numSamples = 15;

        AudioBuffer<float> referenceData (1, numSamples);

        for (int i = 0; i < numSamples; ++i)
            referenceData.setSample (0, i, sv.getNextValue());

        EXPECT_GT (referenceData.getSample (0, 0), 0);
        EXPECT_LT (referenceData.getSample (0, 10), sv.getTargetValue());
        EXPECT_NEAR (referenceData.getSample (0, 11), sv.getTargetValue(), 2.0e-7f);

        auto getUnitData = [] (int numSamplesToGenerate)
        {
            AudioBuffer<float> result (1, numSamplesToGenerate);

            for (int i = 0; i < numSamplesToGenerate; ++i)
                result.setSample (0, i, 1.0f);

            return result;
        };

        auto compareData = [this] (const AudioBuffer<float>& test,
                                   const AudioBuffer<float>& reference)
        {
            for (int i = 0; i < test.getNumSamples(); ++i)
                EXPECT_NEAR (test.getSample (0, i), reference.getSample (0, i), 2.0e-7f);
        };

        auto testData = getUnitData (numSamples);
        sv.setCurrentAndTargetValue (1.0f);
        sv.setTargetValue (2.0f);
        sv.applyGain (testData.getWritePointer (0), numSamples);
        compareData (testData, referenceData);

        testData = getUnitData (numSamples);
        AudioBuffer<float> destData (1, numSamples);
        sv.setCurrentAndTargetValue (1.0f);
        sv.setTargetValue (2.0f);
        sv.applyGain (destData.getWritePointer (0),
                      testData.getReadPointer (0),
                      numSamples);
        compareData (destData, referenceData);
        compareData (testData, getUnitData (numSamples));

        testData = getUnitData (numSamples);
        sv.setCurrentAndTargetValue (1.0f);
        sv.setTargetValue (2.0f);
        sv.applyGain (testData, numSamples);
        compareData (testData, referenceData);
    }

    void runSkipTest()
    {
        SmoothedValueType sv;

        sv.reset (12);
        sv.setCurrentAndTargetValue (1.0f);
        sv.setTargetValue (2.0f);

        Array<float> reference;

        for (int i = 0; i < 15; ++i)
            reference.add (sv.getNextValue());

        sv.setCurrentAndTargetValue (1.0f);
        sv.setTargetValue (2.0f);

        EXPECT_NEAR (sv.skip (1), reference[0], 1.0e-6f);
        EXPECT_NEAR (sv.skip (1), reference[1], 1.0e-6f);
        EXPECT_NEAR (sv.skip (2), reference[3], 1.0e-6f);
        sv.skip (3);
        EXPECT_NEAR (sv.getCurrentValue(), reference[6], 1.0e-6f);
        EXPECT_EQ (sv.skip (300), sv.getTargetValue());
        EXPECT_EQ (sv.getCurrentValue(), sv.getTargetValue());
    }

    void runNegativeTest()
    {
        SmoothedValueType sv;

        auto numValues = 12;
        sv.reset (numValues);

        std::vector<std::pair<float, float>> ranges = { { -1.0f, -2.0f },
                                                        { -100.0f, -3.0f } };

        for (auto range : ranges)
        {
            auto start = range.first, end = range.second;

            sv.setCurrentAndTargetValue (start);
            sv.setTargetValue (end);

            auto val = sv.skip (numValues / 2);

            if (end > start)
                EXPECT_TRUE (val > start && val < end);
            else
                EXPECT_TRUE (val < start && val > end);

            auto nextVal = sv.getNextValue();
            EXPECT_TRUE (end > start ? (nextVal > val) : (nextVal < val));

            auto endVal = sv.skip (500);
            EXPECT_EQ (endVal, end);
            EXPECT_EQ (sv.getNextValue(), end);
            EXPECT_EQ (sv.getCurrentValue(), end);

            sv.setCurrentAndTargetValue (start);
            sv.setTargetValue (end);

            SmoothedValueType positiveSv { -start };
            positiveSv.reset (numValues);
            positiveSv.setTargetValue (-end);

            for (int i = 0; i < numValues + 2; ++i)
                EXPECT_EQ (sv.getNextValue(), -positiveSv.getNextValue());
        }
    }
};

// Test fixture for Linear SmoothedValue
class LinearSmoothedValueTests : public CommonSmoothedValueTests<SmoothedValue<float, ValueSmoothingTypes::Linear>>
{
};

// Test fixture for Multiplicative SmoothedValue
class MultiplicativeSmoothedValueTests : public CommonSmoothedValueTests<SmoothedValue<float, ValueSmoothingTypes::Multiplicative>>
{
};

// Common tests for Linear SmoothedValue
TEST_F (LinearSmoothedValueTests, InitialState)
{
    runInitialStateTest();
}

TEST_F (LinearSmoothedValueTests, Resetting)
{
    runResettingTest();
}

TEST_F (LinearSmoothedValueTests, SampleRate)
{
    runSampleRateTest();
}

TEST_F (LinearSmoothedValueTests, BlockProcessing)
{
    runBlockProcessingTest();
}

TEST_F (LinearSmoothedValueTests, Skip)
{
    runSkipTest();
}

TEST_F (LinearSmoothedValueTests, Negative)
{
    runNegativeTest();
}

// Common tests for Multiplicative SmoothedValue
TEST_F (MultiplicativeSmoothedValueTests, InitialState)
{
    runInitialStateTest();
}

TEST_F (MultiplicativeSmoothedValueTests, Resetting)
{
    runResettingTest();
}

TEST_F (MultiplicativeSmoothedValueTests, SampleRate)
{
    runSampleRateTest();
}

TEST_F (MultiplicativeSmoothedValueTests, BlockProcessing)
{
    runBlockProcessingTest();
}

TEST_F (MultiplicativeSmoothedValueTests, Skip)
{
    runSkipTest();
}

TEST_F (MultiplicativeSmoothedValueTests, Negative)
{
    runNegativeTest();
}

// Specific tests for SmoothedValue functionality
TEST (SmoothedValueSpecificTests, LinearMovingTarget)
{
    SmoothedValue<float, ValueSmoothingTypes::Linear> sv;

    sv.reset (12);
    float initialValue = 0.0f;
    sv.setCurrentAndTargetValue (initialValue);
    sv.setTargetValue (1.0f);

    auto delta = sv.getNextValue() - initialValue;

    sv.skip (6);

    auto newInitialValue = sv.getCurrentValue();
    sv.setTargetValue (newInitialValue + 2.0f);
    auto doubleDelta = sv.getNextValue() - newInitialValue;

    EXPECT_NEAR (doubleDelta, delta * 2.0f, 1.0e-7f);
}

TEST (SmoothedValueSpecificTests, MultiplicativeCurve)
{
    SmoothedValue<double, ValueSmoothingTypes::Multiplicative> sv;

    auto numSamples = 12;
    AudioBuffer<double> values (2, numSamples + 1);

    sv.reset (numSamples);
    sv.setCurrentAndTargetValue (1.0);
    sv.setTargetValue (2.0f);

    values.setSample (0, 0, sv.getCurrentValue());

    for (int i = 1; i < values.getNumSamples(); ++i)
        values.setSample (0, i, sv.getNextValue());

    sv.setTargetValue (1.0f);
    values.setSample (1, values.getNumSamples() - 1, sv.getCurrentValue());

    for (int i = values.getNumSamples() - 2; i >= 0; --i)
        values.setSample (1, i, sv.getNextValue());

    for (int i = 0; i < values.getNumSamples(); ++i)
        EXPECT_NEAR (values.getSample (0, i), values.getSample (1, i), 1.0e-9);
}