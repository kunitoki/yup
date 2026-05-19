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

using namespace yup;

TEST (ResultValueTests, CreateSuccessResult)
{
    auto result = ResultValue<int>::ok (42);
    EXPECT_TRUE (result.wasOk());
    EXPECT_FALSE (result.failed());
    EXPECT_EQ (result.getValue(), 42);
    EXPECT_EQ (result.getReference(), 42);
    EXPECT_EQ (std::as_const (result).getReference(), 42);
}

TEST (ResultValueTests, CreateFailureResultWithMessage)
{
    auto result = ResultValue<int>::fail ("An error occurred");
    EXPECT_FALSE (result.wasOk());
    EXPECT_TRUE (result.failed());
    EXPECT_EQ (result.getErrorMessage(), "An error occurred");
}

TEST (ResultValueTests, CreateFailureResultWithEmptyMessage)
{
    auto result = ResultValue<int>::fail ("");
    EXPECT_FALSE (result.wasOk());
    EXPECT_TRUE (result.failed());
    EXPECT_EQ (result.getErrorMessage(), "Unknown Error");
}

TEST (ResultValueTests, ConversionOperators)
{
    auto success = ResultValue<int>::ok (42);
    auto failure = ResultValue<int>::fail ("Error");

    EXPECT_TRUE (static_cast<bool> (success));
    EXPECT_FALSE (static_cast<bool> (failure));
    EXPECT_FALSE (! success);
    EXPECT_TRUE (! failure);
}

TEST (ResultValueTests, CopyConstructor)
{
    auto original = ResultValue<int>::fail ("Original error");
    auto copy = original;

    EXPECT_EQ (copy.failed(), original.failed());
    EXPECT_EQ (copy.getErrorMessage(), original.getErrorMessage());
}

TEST (ResultValueTests, MoveConstructor)
{
    auto original = ResultValue<int>::fail ("Original error");
    auto moved = std::move (original);

    EXPECT_TRUE (moved.failed());
    EXPECT_EQ (moved.getErrorMessage(), "Original error");

    // After moving, the original may still be valid, but its state is unspecified
    // For this class, the expected behavior is that the original is left in a default state
    //EXPECT_FALSE(original.wasOk());
    //EXPECT_EQ(original.getErrorMessage(), "Unknown Error");
}

TEST (ResultValueTests, CopyAssignmentOperator)
{
    auto original = ResultValue<int>::fail ("Original error");
    ResultValue<int> copy = ResultValue<int>::ok (42);
    copy = original;

    EXPECT_EQ (copy.failed(), original.failed());
    EXPECT_EQ (copy.getErrorMessage(), original.getErrorMessage());
}

TEST (ResultValueTests, MoveAssignmentOperator)
{
    auto original = ResultValue<int>::fail ("Original error");
    ResultValue<int> moved = ResultValue<int>::ok (42);
    moved = std::move (original);

    EXPECT_TRUE (moved.failed());
    EXPECT_EQ (moved.getErrorMessage(), "Original error");

    // After moving, the original may still be valid, but its state is unspecified
    // For this class, the expected behavior is that the original is left in a default state
    //EXPECT_FALSE(original.wasOk());
    //EXPECT_EQ(original.getErrorMessage(), "Unknown Error");
}

TEST (ResultValueTests, EqualityOperator)
{
    auto success1 = ResultValue<int>::ok (42);
    auto success2 = ResultValue<int>::ok (42);
    auto failure1 = ResultValue<int>::fail ("Error 1");
    auto failure2 = ResultValue<int>::fail ("Error 1");
    auto failure3 = ResultValue<int>::fail ("Error 2");

    EXPECT_TRUE (success1 == success2);
    EXPECT_TRUE (failure1 == failure2);
    EXPECT_FALSE (success1 == failure1);
    EXPECT_FALSE (failure1 == failure3);
}

TEST (ResultValueTests, InequalityOperator)
{
    auto success1 = ResultValue<int>::ok (42);
    auto success2 = ResultValue<int>::ok (42);
    auto failure1 = ResultValue<int>::fail ("Error 1");
    auto failure2 = ResultValue<int>::fail ("Error 1");
    auto failure3 = ResultValue<int>::fail ("Error 2");

    EXPECT_FALSE (success1 != success2);
    EXPECT_FALSE (failure1 != failure2);
    EXPECT_TRUE (success1 != failure1);
    EXPECT_TRUE (failure1 != failure3);
}

TEST (ResultValueTests, MakeResultValueOkDeducesTypeFromArgument)
{
    ResultValue<int> result = makeResultValueOk (42);
    EXPECT_TRUE (result.wasOk());
    EXPECT_FALSE (result.failed());
    EXPECT_EQ (result.getValue(), 42);
}

TEST (ResultValueTests, MakeResultValueOkWorksWithReturnDeduction)
{
    auto makeInt = []() -> ResultValue<int>
    {
        return makeResultValueOk (1337);
    };
    auto makeStr = []() -> ResultValue<String>
    {
        return makeResultValueOk (String ("hello"));
    };

    auto intResult = makeInt();
    EXPECT_TRUE (intResult.wasOk());
    EXPECT_EQ (intResult.getValue(), 1337);

    auto strResult = makeStr();
    EXPECT_TRUE (strResult.wasOk());
    EXPECT_EQ (strResult.getValue(), String ("hello"));
}

TEST (ResultValueTests, MakeResultValueOkWithImplicitConversion)
{
    ResultValue<double> result = makeResultValueOk (42);
    EXPECT_TRUE (result.wasOk());
    EXPECT_DOUBLE_EQ (result.getValue(), 42.0);
}

TEST (ResultValueTests, MakeResultValueFailDeducesForAnyType)
{
    ResultValue<int> intResult = makeResultValueFail ("int error");
    EXPECT_FALSE (intResult.wasOk());
    EXPECT_TRUE (intResult.failed());
    EXPECT_EQ (intResult.getErrorMessage(), "int error");

    ResultValue<String> strResult = makeResultValueFail ("string error");
    EXPECT_FALSE (strResult.wasOk());
    EXPECT_EQ (strResult.getErrorMessage(), "string error");
}

TEST (ResultValueTests, MakeResultValueFailWorksWithReturnDeduction)
{
    auto makeFailure = []() -> ResultValue<int>
    {
        return makeResultValueFail ("something went wrong");
    };

    auto result = makeFailure();
    EXPECT_FALSE (result.wasOk());
    EXPECT_EQ (result.getErrorMessage(), "something went wrong");
}

TEST (ResultValueTests, MakeResultValueFailWithEmptyMessageUsesDefault)
{
    ResultValue<int> result = makeResultValueFail ("");
    EXPECT_FALSE (result.wasOk());
    EXPECT_EQ (result.getErrorMessage(), "Unknown Error");
}

TEST (ResultValueTests, MakeResultValueOkAndFailProduceEqualResults)
{
    ResultValue<int> a = makeResultValueOk (7);
    ResultValue<int> b = ResultValue<int>::ok (7);
    EXPECT_EQ (a, b);

    ResultValue<int> c = makeResultValueFail ("err");
    ResultValue<int> d = ResultValue<int>::fail ("err");
    EXPECT_EQ (c, d);
}
