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
*/

#include <gtest/gtest.h>

#include <juce_core/juce_core.h>

using namespace yup;

TEST (StatisticsAccumulatorTests, DefaultConstructor)
{
    StatisticsAccumulator<double> accumulator;
    EXPECT_EQ (accumulator.getCount(), 0);
    EXPECT_EQ (accumulator.getAverage(), 0);
    EXPECT_EQ (accumulator.getVariance(), 0);
    EXPECT_EQ (accumulator.getStandardDeviation(), 0);
    EXPECT_EQ (accumulator.getMinValue(), std::numeric_limits<double>::infinity());
    EXPECT_EQ (accumulator.getMaxValue(), -std::numeric_limits<double>::infinity());
    EXPECT_EQ (accumulator.getEnergy(), 0);
}

TEST (StatisticsAccumulatorTests, AddValue)
{
    StatisticsAccumulator<double> accumulator;
    accumulator.addValue (1.0);
    accumulator.addValue (2.0);
    accumulator.addValue (3.0);

    EXPECT_EQ (accumulator.getCount(), 3);
    EXPECT_DOUBLE_EQ (accumulator.getAverage(), 2.0);
    EXPECT_DOUBLE_EQ (accumulator.getVariance(), 0.66666666666666663);
    EXPECT_DOUBLE_EQ (accumulator.getStandardDeviation(), 0.81649658092772603);
    EXPECT_DOUBLE_EQ (accumulator.getMinValue(), 1.0);
    EXPECT_DOUBLE_EQ (accumulator.getMaxValue(), 3.0);
    EXPECT_DOUBLE_EQ (accumulator.getEnergy(), 14.0);
}

TEST (StatisticsAccumulatorTests, Reset)
{
    StatisticsAccumulator<double> accumulator;
    accumulator.addValue (1.0);
    accumulator.addValue (2.0);
    accumulator.addValue (3.0);
    accumulator.reset();

    EXPECT_EQ (accumulator.getCount(), 0);
    EXPECT_EQ (accumulator.getAverage(), 0);
    EXPECT_EQ (accumulator.getVariance(), 0);
    EXPECT_EQ (accumulator.getStandardDeviation(), 0);
    EXPECT_EQ (accumulator.getMinValue(), std::numeric_limits<double>::infinity());
    EXPECT_EQ (accumulator.getMaxValue(), -std::numeric_limits<double>::infinity());
    EXPECT_EQ (accumulator.getEnergy(), 0);
}

TEST (StatisticsAccumulatorTests, SingleValue)
{
    StatisticsAccumulator<double> accumulator;
    accumulator.addValue (5.0);

    EXPECT_EQ (accumulator.getCount(), 1);
    EXPECT_DOUBLE_EQ (accumulator.getAverage(), 5.0);
    EXPECT_DOUBLE_EQ (accumulator.getVariance(), 0.0);
    EXPECT_DOUBLE_EQ (accumulator.getStandardDeviation(), 0.0);
    EXPECT_DOUBLE_EQ (accumulator.getMinValue(), 5.0);
    EXPECT_DOUBLE_EQ (accumulator.getMaxValue(), 5.0);
    EXPECT_DOUBLE_EQ (accumulator.getEnergy(), 25.0);
}

TEST (StatisticsAccumulatorTests, MultipleValues)
{
    StatisticsAccumulator<double> accumulator;
    accumulator.addValue (4.0);
    accumulator.addValue (7.0);
    accumulator.addValue (13.0);
    accumulator.addValue (16.0);
    accumulator.addValue (19.0);

    EXPECT_EQ (accumulator.getCount(), 5);
    EXPECT_DOUBLE_EQ (accumulator.getAverage(), 11.8);
    EXPECT_NEAR (accumulator.getVariance(), 30.95999999999999, 1e-5);
    EXPECT_NEAR (accumulator.getStandardDeviation(), std::sqrt (30.95999999999999), 1e-5);
    EXPECT_DOUBLE_EQ (accumulator.getMinValue(), 4.0);
    EXPECT_DOUBLE_EQ (accumulator.getMaxValue(), 19.0);
    EXPECT_DOUBLE_EQ (accumulator.getEnergy(), 851.0);
}

TEST (StatisticsAccumulatorTests, AddNegativeValues)
{
    StatisticsAccumulator<double> accumulator;
    accumulator.addValue (-1.0);
    accumulator.addValue (-2.0);
    accumulator.addValue (-3.0);

    EXPECT_EQ (accumulator.getCount(), 3);
    EXPECT_DOUBLE_EQ (accumulator.getAverage(), -2.0);
    EXPECT_DOUBLE_EQ (accumulator.getVariance(), 0.66666666666666663);
    EXPECT_DOUBLE_EQ (accumulator.getStandardDeviation(), 0.81649658092772603);
    EXPECT_DOUBLE_EQ (accumulator.getMinValue(), -3.0);
    EXPECT_DOUBLE_EQ (accumulator.getMaxValue(), -1.0);
    EXPECT_DOUBLE_EQ (accumulator.getEnergy(), 14.0);
}

TEST (StatisticsAccumulatorTests, AddMixedValues)
{
    StatisticsAccumulator<double> accumulator;
    accumulator.addValue (-2.0);
    accumulator.addValue (3.0);
    accumulator.addValue (-4.0);
    accumulator.addValue (5.0);

    EXPECT_EQ (accumulator.getCount(), 4);
    EXPECT_DOUBLE_EQ (accumulator.getAverage(), 0.5);
    EXPECT_NEAR (accumulator.getVariance(), 13.25, 1e-5);
    EXPECT_NEAR (accumulator.getStandardDeviation(), std::sqrt (13.25), 1e-5);
    EXPECT_DOUBLE_EQ (accumulator.getMinValue(), -4.0);
    EXPECT_DOUBLE_EQ (accumulator.getMaxValue(), 5.0);
    EXPECT_DOUBLE_EQ (accumulator.getEnergy(), 54.0);
}
