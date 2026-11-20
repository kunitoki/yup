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

#include <yup_expression/yup_expression.h>

#include <gtest/gtest.h>

using namespace yup;

//==============================================================================
class ExpressionCacheTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        context.addInput<float> ("x");
    }

    ExpressionContext context;
};

//==============================================================================
TEST_F (ExpressionCacheTests, ReturnsTheSameProgramForTheSameText)
{
    ExpressionCache cache (context);

    auto first = cache.compile ("x * 2");
    auto second = cache.compile ("x * 2");
    auto other = cache.compile ("x * 3");

    ASSERT_TRUE (first.wasOk());
    ASSERT_TRUE (second.wasOk());
    ASSERT_TRUE (other.wasOk());
    EXPECT_TRUE (first.getValue() == second.getValue());
    EXPECT_FALSE (first.getValue() == other.getValue());
    EXPECT_EQ (2, cache.size());
}

TEST_F (ExpressionCacheTests, CompilesAgainstASnapshotOfTheContext)
{
    ExpressionCache cache (context);
    context.addInput<float> ("y");

    EXPECT_TRUE (cache.compile ("x + 1").wasOk());

    auto unknown = cache.compile ("y + 1");
    ASSERT_TRUE (unknown.failed());
    EXPECT_TRUE (unknown.getErrorMessage().contains ("Unknown symbol 'y'"));
    EXPECT_EQ (unknown.getErrorMessage(), cache.compile ("y + 1").getErrorMessage());
    EXPECT_EQ (2, cache.size());
}

TEST_F (ExpressionCacheTests, RemovesUnusedProgramsAndErrors)
{
    ExpressionCache cache (context);

    auto kept = cache.compile ("x * 2");
    cache.compile ("x * 3");
    cache.compile ("x *");
    EXPECT_EQ (3, cache.size());

    EXPECT_EQ (2, cache.removeUnused());
    EXPECT_EQ (1, cache.size());
    EXPECT_TRUE (kept.getValue() == cache.compile ("x * 2").getValue());

    cache.clear();
    EXPECT_EQ (0, cache.size());
}
