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

using namespace juce;

static String operator"" _S(const char* chars, size_t)
{
    return String{ chars };
}

TEST (ResultTests, CreateSuccessResult)
{
    Result success = Result::ok();
    EXPECT_TRUE(success.wasOk());
    EXPECT_FALSE(success.failed());
    EXPECT_EQ(success.getErrorMessage(), ""_S);
}

TEST (ResultTests, CreateFailureResultWithMessage)
{
    Result failure = Result::fail("An error occurred");
    EXPECT_FALSE(failure.wasOk());
    EXPECT_TRUE(failure.failed());
    EXPECT_EQ(failure.getErrorMessage(), "An error occurred"_S);
}

TEST (ResultTests, CreateFailureResultWithEmptyMessage)
{
    Result failure = Result::fail(""_S);
    EXPECT_FALSE(failure.wasOk());
    EXPECT_TRUE(failure.failed());
    EXPECT_EQ(failure.getErrorMessage(), "Unknown Error"_S);
}

TEST (ResultTests, ConversionOperators)
{
    Result success = Result::ok();
    Result failure = Result::fail("Error");

    EXPECT_TRUE(success);
    EXPECT_FALSE(failure);
    EXPECT_FALSE(!success);
    EXPECT_TRUE(!failure);
}

TEST (ResultTests, CopyConstructor)
{
    Result original = Result::fail("Original error");
    Result copy = original;

    EXPECT_EQ(copy.failed(), original.failed());
    EXPECT_EQ(copy.getErrorMessage(), original.getErrorMessage());
}

TEST (ResultTests, MoveConstructor)
{
    Result original = Result::fail("Original error");
    Result moved = std::move(original);

    EXPECT_TRUE(moved.failed());
    EXPECT_EQ(moved.getErrorMessage(), "Original error"_S);

    // After moving, the original may still be valid, but its state is unspecified
    // For this class, the expected behavior is that the original is left in a default state
    EXPECT_FALSE(original.failed());
    EXPECT_EQ(original.getErrorMessage(), ""_S);
}

TEST (ResultTests, CopyAssignmentOperator)
{
    Result original = Result::fail("Original error");
    Result copy = Result::ok();
    copy = original;

    EXPECT_EQ(copy.failed(), original.failed());
    EXPECT_EQ(copy.getErrorMessage(), original.getErrorMessage());
}

TEST (ResultTests, MoveAssignmentOperator)
{
    Result original = Result::fail("Original error");
    Result moved = Result::ok();
    moved = std::move(original);

    EXPECT_TRUE(moved.failed());
    EXPECT_EQ(moved.getErrorMessage(), "Original error"_S);

    // After moving, the original may still be valid, but its state is unspecified
    // For this class, the expected behavior is that the original is left in a default state
    EXPECT_FALSE(original.failed());
    EXPECT_EQ(original.getErrorMessage(), ""_S);
}

TEST (ResultTests, EqualityOperator)
{
    Result success1 = Result::ok();
    Result success2 = Result::ok();
    Result failure1 = Result::fail("Error 1");
    Result failure2 = Result::fail("Error 1");
    Result failure3 = Result::fail("Error 2");

    EXPECT_TRUE(success1 == success2);
    EXPECT_TRUE(failure1 == failure2);
    EXPECT_FALSE(success1 == failure1);
    EXPECT_FALSE(failure1 == failure3);
}

TEST (ResultTests, InequalityOperator)
{
    Result success1 = Result::ok();
    Result success2 = Result::ok();
    Result failure1 = Result::fail("Error 1");
    Result failure2 = Result::fail("Error 1");
    Result failure3 = Result::fail("Error 2");

    EXPECT_FALSE(success1 != success2);
    EXPECT_FALSE(failure1 != failure2);
    EXPECT_TRUE(success1 != failure1);
    EXPECT_TRUE(failure1 != failure3);
}
