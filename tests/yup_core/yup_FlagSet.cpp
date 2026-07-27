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

#include <yup_core/yup_core.h>

namespace
{
namespace Detail
{
struct verboseLog;
struct noErrorLog;
} // namespace Detail

using LogOption = yup::FlagSet<yup::uint32, Detail::verboseLog, Detail::noErrorLog>;
static inline constexpr LogOption defaultLog = LogOption();
static inline constexpr LogOption verboseLog = LogOption::declareValue<Detail::verboseLog>();
static inline constexpr LogOption noErrorLog = LogOption::declareValue<Detail::noErrorLog>();
} // namespace

TEST (FlagSetTests, Default_Constructed)
{
    LogOption option;
    EXPECT_FALSE (option.test (verboseLog));
    EXPECT_FALSE (option.test (noErrorLog));
}

TEST (FlagSetTests, Default_Constructed_From_Default)
{
    LogOption option = defaultLog;
    EXPECT_FALSE (option.test (verboseLog));
    EXPECT_FALSE (option.test (noErrorLog));
}

TEST (FlagSetTests, Default_Constructed_From_Value)
{
    LogOption option = verboseLog;
    EXPECT_TRUE (option.test (verboseLog));
    EXPECT_FALSE (option.test (noErrorLog));
}

TEST (FlagSetTests, Default_Constructed_From_Values)
{
    LogOption option = verboseLog | noErrorLog;
    EXPECT_TRUE (option.test (verboseLog));
    EXPECT_TRUE (option.test (noErrorLog));
}

TEST (FlagSetTests, To_String)
{
    EXPECT_EQ (yup::String ("00"), defaultLog.toString());
    EXPECT_EQ (yup::String ("10"), verboseLog.toString());
    EXPECT_EQ (yup::String ("01"), noErrorLog.toString());

    LogOption option = verboseLog | noErrorLog;
    EXPECT_EQ (yup::String ("11"), option.toString());
}

TEST (FlagSetTests, From_String)
{
    EXPECT_EQ (defaultLog, LogOption::fromString ("00"));
    EXPECT_EQ (verboseLog, LogOption::fromString ("10"));
    EXPECT_EQ (noErrorLog, LogOption::fromString ("01"));
    EXPECT_EQ ((verboseLog | noErrorLog), LogOption::fromString ("11"));
}

TEST (FlagSetTests, DeclareValue)
{
    constexpr auto value1 = LogOption::declareValue<Detail::verboseLog>();
    constexpr auto value2 = LogOption::declareValue<Detail::noErrorLog>();
    constexpr auto valueBoth = LogOption::declareValue<Detail::verboseLog, Detail::noErrorLog>();

    EXPECT_TRUE (value1.test (verboseLog));
    EXPECT_FALSE (value1.test (noErrorLog));

    EXPECT_FALSE (value2.test (verboseLog));
    EXPECT_TRUE (value2.test (noErrorLog));

    EXPECT_TRUE (valueBoth.test (verboseLog));
    EXPECT_TRUE (valueBoth.test (noErrorLog));
}

TEST (FlagSetTests, SetMethod)
{
    LogOption option;
    EXPECT_FALSE (option.test (verboseLog));

    option.set (verboseLog);
    EXPECT_TRUE (option.test (verboseLog));
    EXPECT_FALSE (option.test (noErrorLog));

    option.set (noErrorLog);
    EXPECT_TRUE (option.test (verboseLog));
    EXPECT_TRUE (option.test (noErrorLog));
}

TEST (FlagSetTests, WithSet)
{
    LogOption option = verboseLog;
    auto newOption = option.withSet (noErrorLog);

    // Original unchanged
    EXPECT_TRUE (option.test (verboseLog));
    EXPECT_FALSE (option.test (noErrorLog));

    // New has both
    EXPECT_TRUE (newOption.test (verboseLog));
    EXPECT_TRUE (newOption.test (noErrorLog));
}

TEST (FlagSetTests, UnsetMethod)
{
    LogOption option = verboseLog | noErrorLog;
    EXPECT_TRUE (option.test (verboseLog));
    EXPECT_TRUE (option.test (noErrorLog));

    option.unset (verboseLog);
    EXPECT_FALSE (option.test (verboseLog));
    EXPECT_TRUE (option.test (noErrorLog));

    option.unset (noErrorLog);
    EXPECT_FALSE (option.test (verboseLog));
    EXPECT_FALSE (option.test (noErrorLog));
}

TEST (FlagSetTests, WithUnset)
{
    LogOption option = verboseLog | noErrorLog;
    auto newOption = option.withUnset (verboseLog);

    // Original unchanged
    EXPECT_TRUE (option.test (verboseLog));
    EXPECT_TRUE (option.test (noErrorLog));

    // New has only noErrorLog
    EXPECT_FALSE (newOption.test (verboseLog));
    EXPECT_TRUE (newOption.test (noErrorLog));
}

TEST (FlagSetTests, OperatorOrEquals)
{
    LogOption option = verboseLog;
    option |= noErrorLog;

    EXPECT_TRUE (option.test (verboseLog));
    EXPECT_TRUE (option.test (noErrorLog));
}

TEST (FlagSetTests, OperatorAnd)
{
    LogOption option1 = verboseLog | noErrorLog;
    LogOption option2 = verboseLog;

    auto result = option1 & option2;

    EXPECT_TRUE (result.test (verboseLog));
    EXPECT_FALSE (result.test (noErrorLog));
}

TEST (FlagSetTests, OperatorAndEquals)
{
    LogOption option1 = verboseLog | noErrorLog;
    LogOption option2 = verboseLog;

    option1 &= option2;

    EXPECT_TRUE (option1.test (verboseLog));
    EXPECT_FALSE (option1.test (noErrorLog));
}

TEST (FlagSetTests, OperatorNot)
{
    LogOption option = verboseLog;
    auto inverted = ~option;

    // Inverted should NOT have verboseLog set in the first bit position
    // but will have other bits set (this is bitwise NOT)
    EXPECT_FALSE (inverted.test (verboseLog));

    // Note: ~verboseLog will have noErrorLog bit set (and potentially others)
    EXPECT_TRUE (inverted.test (noErrorLog));
}

TEST (FlagSetTests, SetMultipleFlags)
{
    LogOption option;
    option.set (verboseLog);
    option.set (noErrorLog);

    EXPECT_TRUE (option.test (verboseLog));
    EXPECT_TRUE (option.test (noErrorLog));
}

TEST (FlagSetTests, UnsetAllFlags)
{
    LogOption option = verboseLog | noErrorLog;
    option.unset (verboseLog | noErrorLog);

    EXPECT_FALSE (option.test (verboseLog));
    EXPECT_FALSE (option.test (noErrorLog));
}

TEST (FlagSetTests, ChainedWithSet)
{
    LogOption option;
    auto result = option.withSet (verboseLog).withSet (noErrorLog);

    EXPECT_TRUE (result.test (verboseLog));
    EXPECT_TRUE (result.test (noErrorLog));
}

TEST (FlagSetTests, ChainedWithUnset)
{
    LogOption option = verboseLog | noErrorLog;
    auto result = option.withUnset (verboseLog).withUnset (noErrorLog);

    EXPECT_FALSE (result.test (verboseLog));
    EXPECT_FALSE (result.test (noErrorLog));
}

TEST (FlagSetTests, CombineOperators)
{
    LogOption option1 = verboseLog;
    LogOption option2 = noErrorLog;

    auto combined = option1 | option2;
    EXPECT_TRUE (combined.test (verboseLog));
    EXPECT_TRUE (combined.test (noErrorLog));

    auto intersected = combined & verboseLog;
    EXPECT_TRUE (intersected.test (verboseLog));
    EXPECT_FALSE (intersected.test (noErrorLog));
}

TEST (FlagSetTests, ConstexprDeclareValue)
{
    // Test that declareValue is constexpr
    constexpr LogOption value = LogOption::declareValue<Detail::verboseLog>();
    static_assert (value.test (verboseLog), "Should be compile-time constant");
}

TEST (FlagSetTests, ConstexprOperations)
{
    // Test constexpr operations
    constexpr LogOption option1 = verboseLog;
    constexpr LogOption option2 = noErrorLog;
    constexpr LogOption combined = option1 | option2;

    static_assert (combined.test (verboseLog), "Should work at compile time");
    static_assert (combined.test (noErrorLog), "Should work at compile time");
}

TEST (FlagSetTests, AndOperatorReturnsIntersection)
{
    LogOption all = verboseLog | noErrorLog;
    LogOption some = verboseLog;

    auto intersection = all & some;

    EXPECT_EQ (intersection, verboseLog);
    EXPECT_NE (intersection, noErrorLog);
}

TEST (FlagSetTests, NotOperatorInverts)
{
    LogOption option = defaultLog; // No flags set
    auto inverted = ~option;

    // After inversion, should have flags set (exact bits depend on type width)
    EXPECT_NE (inverted, option);
}

TEST (FlagSetTests, SetIdempotent)
{
    LogOption option = verboseLog;
    option.set (verboseLog); // Set same flag again

    EXPECT_TRUE (option.test (verboseLog));
    EXPECT_EQ (option, verboseLog);
}

TEST (FlagSetTests, UnsetIdempotent)
{
    LogOption option;
    option.unset (verboseLog); // Unset flag that isn't set

    EXPECT_FALSE (option.test (verboseLog));
    EXPECT_EQ (option, defaultLog);
}
