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
