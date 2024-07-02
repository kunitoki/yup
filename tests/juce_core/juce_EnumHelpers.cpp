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

#include <juce_core/juce_core.h>

using namespace juce;

namespace
{
enum class TestEnum
{
    one = 1 << 0,
    four = 1 << 1,
    other = 1 << 2
};

JUCE_DECLARE_SCOPED_ENUM_BITWISE_OPERATORS (TestEnum)
} // namespace

TEST (ScopedEnumBitwiseOperatorsTests, BitwiseOrOperator)
{
    TestEnum e = TestEnum::one | TestEnum::four;
    EXPECT_EQ (e, TestEnum::one | TestEnum::four);
}

TEST (ScopedEnumBitwiseOperatorsTests, BitwiseAndOperator)
{
    TestEnum e = TestEnum::one | TestEnum::four;
    EXPECT_EQ (e & TestEnum::one, TestEnum::one);
    EXPECT_EQ (e & TestEnum::four, TestEnum::four);
    EXPECT_EQ (e & TestEnum::other, TestEnum {});
}

TEST (ScopedEnumBitwiseOperatorsTests, BitwiseNotOperator)
{
    TestEnum e = ~TestEnum::one;
    EXPECT_EQ (e, static_cast<TestEnum> (~static_cast<int> (TestEnum::one)));
}

TEST (ScopedEnumBitwiseOperatorsTests, BitwiseOrAssignmentOperator)
{
    TestEnum e = TestEnum::one;
    e |= TestEnum::four;
    EXPECT_EQ (e, TestEnum::one | TestEnum::four);
}

TEST (ScopedEnumBitwiseOperatorsTests, BitwiseAndAssignmentOperator)
{
    TestEnum e = TestEnum::one | TestEnum::four;
    e &= TestEnum::one;
    EXPECT_EQ (e, TestEnum::one);
}

TEST (ScopedEnumBitwiseOperatorsTests, HasBitValueSet)
{
    TestEnum e = TestEnum::one | TestEnum::four;
    EXPECT_TRUE (hasBitValueSet (e, TestEnum::one));
    EXPECT_TRUE (hasBitValueSet (e, TestEnum::four));
    EXPECT_FALSE (hasBitValueSet (e, TestEnum::other));
}

TEST (ScopedEnumBitwiseOperatorsTests, WithBitValueSet)
{
    TestEnum e = TestEnum::one;
    e = withBitValueSet (e, TestEnum::four);
    EXPECT_EQ (e, TestEnum::one | TestEnum::four);
}

TEST (ScopedEnumBitwiseOperatorsTests, WithBitValueCleared)
{
    TestEnum e = TestEnum::one | TestEnum::four;
    e = withBitValueCleared (e, TestEnum::four);
    EXPECT_EQ (e, TestEnum::one);
}

TEST (ScopedEnumBitwiseOperatorsTests, DefaultInitializedEnumIsNone)
{
    TestEnum e = {};
    EXPECT_EQ (e, TestEnum {});
    EXPECT_FALSE (hasBitValueSet (e, TestEnum {}));
}

TEST (ScopedEnumBitwiseOperatorsTests, WithBitValueSetCorrectBitOnEmptyEnum)
{
    TestEnum e = {};
    e = withBitValueSet (e, TestEnum::other);
    EXPECT_EQ (e, TestEnum::other);
    EXPECT_TRUE (hasBitValueSet (e, TestEnum::other));
}

TEST (ScopedEnumBitwiseOperatorsTests, WithBitValueSetCorrectBitOnNonEmptyEnum)
{
    TestEnum e = withBitValueSet (TestEnum::other, TestEnum::one);
    EXPECT_TRUE (hasBitValueSet (e, TestEnum::one));
    EXPECT_TRUE (hasBitValueSet (e, TestEnum::other));
}

TEST (ScopedEnumBitwiseOperatorsTests, WithBitValueClearedCorrectBit)
{
    TestEnum e = withBitValueSet (TestEnum::other, TestEnum::one);
    e = withBitValueCleared (e, TestEnum::one);
    EXPECT_FALSE (hasBitValueSet (e, TestEnum::one));
    EXPECT_TRUE (hasBitValueSet (e, TestEnum::other));
}

TEST (ScopedEnumBitwiseOperatorsTests, OperatorsWorkAsExpected)
{
    TestEnum e = TestEnum::one;
    EXPECT_NE ((e & TestEnum::one), TestEnum {});
    e |= TestEnum::other;
    EXPECT_NE ((e & TestEnum::other), TestEnum {});

    e &= ~TestEnum::one;
    EXPECT_EQ ((e & TestEnum::one), TestEnum {});
    EXPECT_NE ((e & TestEnum::other), TestEnum {});
}
