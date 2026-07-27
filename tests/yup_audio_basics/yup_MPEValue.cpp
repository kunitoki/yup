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

namespace
{
void expectValuesConsistent (MPEValue value,
                             int expectedValueAs7BitInt,
                             int expectedValueAs14BitInt,
                             float expectedValueAsSignedFloat,
                             float expectedValueAsUnsignedFloat)
{
    EXPECT_EQ (value.as7BitInt(), expectedValueAs7BitInt);
    EXPECT_EQ (value.as14BitInt(), expectedValueAs14BitInt);

    const float maxRelativeError = 0.0001f;
    const float maxAbsoluteErrorSigned = jmax (1.0f, std::abs (expectedValueAsSignedFloat)) * maxRelativeError;
    const float maxAbsoluteErrorUnsigned = jmax (1.0f, std::abs (expectedValueAsUnsignedFloat)) * maxRelativeError;

    EXPECT_LT (std::abs (expectedValueAsSignedFloat - value.asSignedFloat()), maxAbsoluteErrorSigned);
    EXPECT_LT (std::abs (expectedValueAsUnsignedFloat - value.asUnsignedFloat()), maxAbsoluteErrorUnsigned);
}
} // namespace

TEST (MPEValueTests, ComparisonOperator)
{
    MPEValue value1 = MPEValue::from7BitInt (7);
    MPEValue value2 = MPEValue::from7BitInt (7);
    MPEValue value3 = MPEValue::from7BitInt (8);

    EXPECT_TRUE (value1 == value1);
    EXPECT_TRUE (value1 == value2);
    EXPECT_TRUE (value1 != value3);
}

TEST (MPEValueTests, SpecialValues)
{
    EXPECT_EQ (MPEValue::minValue().as7BitInt(), 0);
    EXPECT_EQ (MPEValue::minValue().as14BitInt(), 0);

    EXPECT_EQ (MPEValue::centreValue().as7BitInt(), 64);
    EXPECT_EQ (MPEValue::centreValue().as14BitInt(), 8192);

    EXPECT_EQ (MPEValue::maxValue().as7BitInt(), 127);
    EXPECT_EQ (MPEValue::maxValue().as14BitInt(), 16383);
}

TEST (MPEValueTests, ZeroMinimumValue)
{
    expectValuesConsistent (MPEValue::from7BitInt (0), 0, 0, -1.0f, 0.0f);
    expectValuesConsistent (MPEValue::from14BitInt (0), 0, 0, -1.0f, 0.0f);
    expectValuesConsistent (MPEValue::fromUnsignedFloat (0.0f), 0, 0, -1.0f, 0.0f);
    expectValuesConsistent (MPEValue::fromSignedFloat (-1.0f), 0, 0, -1.0f, 0.0f);
}

TEST (MPEValueTests, MaximumValue)
{
    expectValuesConsistent (MPEValue::from7BitInt (127), 127, 16383, 1.0f, 1.0f);
    expectValuesConsistent (MPEValue::from14BitInt (16383), 127, 16383, 1.0f, 1.0f);
    expectValuesConsistent (MPEValue::fromUnsignedFloat (1.0f), 127, 16383, 1.0f, 1.0f);
    expectValuesConsistent (MPEValue::fromSignedFloat (1.0f), 127, 16383, 1.0f, 1.0f);
}

TEST (MPEValueTests, CentreValue)
{
    expectValuesConsistent (MPEValue::from7BitInt (64), 64, 8192, 0.0f, 0.5f);
    expectValuesConsistent (MPEValue::from14BitInt (8192), 64, 8192, 0.0f, 0.5f);
    expectValuesConsistent (MPEValue::fromUnsignedFloat (0.5f), 64, 8192, 0.0f, 0.5f);
    expectValuesConsistent (MPEValue::fromSignedFloat (0.0f), 64, 8192, 0.0f, 0.5f);
}

TEST (MPEValueTests, ValueHalfwayBetweenMinAndCentre)
{
    expectValuesConsistent (MPEValue::from7BitInt (32), 32, 4096, -0.5f, 0.25f);
    expectValuesConsistent (MPEValue::from14BitInt (4096), 32, 4096, -0.5f, 0.25f);
    expectValuesConsistent (MPEValue::fromUnsignedFloat (0.25f), 32, 4096, -0.5f, 0.25f);
    expectValuesConsistent (MPEValue::fromSignedFloat (-0.5f), 32, 4096, -0.5f, 0.25f);
}