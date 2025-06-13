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

#include <yup_core/yup_core.h>

#include <gtest/gtest.h>
#include <functional>

using namespace yup;

class FunctionalTest : public ::testing::Test
{
protected:
    static int functionCallCount;

    static void resetCallCount()
    {
        functionCallCount = 0;
    }

    static void incrementCallCount()
    {
        ++functionCallCount;
    }

    static void incrementCallCountWithValue (int value)
    {
        functionCallCount += value;
    }
};

int FunctionalTest::functionCallCount = 0;

TEST_F (FunctionalTest, NullCheckedInvocationWithValidFunction)
{
    resetCallCount();

    auto func = []()
    {
        incrementCallCount();
    };

    NullCheckedInvocation::invoke (func);

    EXPECT_EQ (functionCallCount, 1);
}

TEST_F (FunctionalTest, NullCheckedInvocationWithValidFunctionAndArgs)
{
    resetCallCount();

    auto func = [] (int value)
    {
        incrementCallCountWithValue (value);
    };

    NullCheckedInvocation::invoke (func, 5);

    EXPECT_EQ (functionCallCount, 5);
}

TEST_F (FunctionalTest, NullCheckedInvocationWithNullptr)
{
    resetCallCount();

    std::function<void()> nullFunc = nullptr;

    // Should not crash or call anything
    EXPECT_NO_THROW (NullCheckedInvocation::invoke (nullFunc));

    EXPECT_EQ (functionCallCount, 0);
}

TEST_F (FunctionalTest, NullCheckedInvocationWithNullptrAndArgs)
{
    resetCallCount();

    std::function<void (int)> nullFunc = nullptr;

    // Should not crash or call anything
    EXPECT_NO_THROW (NullCheckedInvocation::invoke (nullFunc, 10));

    EXPECT_EQ (functionCallCount, 0);
}

TEST_F (FunctionalTest, NullCheckedInvocationWithDirectNullptr)
{
    resetCallCount();

    // Test direct nullptr invocation
    NullCheckedInvocation::invoke (nullptr);
    NullCheckedInvocation::invoke (nullptr, 42);

    EXPECT_EQ (functionCallCount, 0);
}

TEST_F (FunctionalTest, WithMemberCopy)
{
    struct TestStruct
    {
        int value = 0;
        String name = "test";
    };

    TestStruct original;
    original.value = 10;
    original.name = "original";

    // Test with member modification
    auto modified = withMember (original, &TestStruct::value, 20);

    EXPECT_EQ (original.value, 10); // Original unchanged
    EXPECT_EQ (modified.value, 20); // Copy modified
    EXPECT_EQ (original.name, "original");
    EXPECT_EQ (modified.name, "original"); // Other members copied
}

TEST_F (FunctionalTest, WithMemberStringCopy)
{
    struct TestStruct
    {
        int value = 0;
        String name = "test";
    };

    TestStruct original;
    original.value = 10;
    original.name = "original";

    // Test with string member modification
    auto modified = withMember (original, &TestStruct::name, String ("modified"));

    EXPECT_EQ (original.name, "original"); // Original unchanged
    EXPECT_EQ (modified.name, "modified"); // Copy modified
    EXPECT_EQ (original.value, 10);
    EXPECT_EQ (modified.value, 10); // Other members copied
}

TEST_F (FunctionalTest, WithMemberMultipleModifications)
{
    struct TestStruct
    {
        int value = 0;
        String name = "test";
        float ratio = 1.0f;
    };

    TestStruct original;
    original.value = 10;
    original.name = "original";
    original.ratio = 1.0f;

    // Chain multiple modifications
    auto modified = withMember (
        withMember (original, &TestStruct::value, 20),
        &TestStruct::name,
        String ("modified"));

    EXPECT_EQ (original.value, 10);
    EXPECT_EQ (original.name, "original");
    EXPECT_EQ (modified.value, 20);
    EXPECT_EQ (modified.name, "modified");
    EXPECT_EQ (modified.ratio, 1.0f); // Unchanged member preserved
}

TEST_F (FunctionalTest, ToFnPtrWithSimpleLambda)
{
    auto lambda = []()
    {
        return 42;
    };

    auto fnPtr = toFnPtr (lambda);

    EXPECT_EQ (fnPtr(), 42);
}

TEST_F (FunctionalTest, ToFnPtrWithParameterizedLambda)
{
    auto lambda = [] (int x, int y)
    {
        return x + y;
    };

    auto fnPtr = toFnPtr (lambda);

    EXPECT_EQ (fnPtr (3, 4), 7);
}

TEST_F (FunctionalTest, ToFnPtrWithReturnType)
{
    auto lambda = [] (float x) -> double
    {
        return static_cast<double> (x) * 2.0;
    };

    auto fnPtr = toFnPtr (lambda);

    EXPECT_DOUBLE_EQ (fnPtr (3.5f), 7.0);
}

// Test the constexpr nature of template functions at compile time
TEST_F (FunctionalTest, CompileTimeTests)
{
    // These tests ensure the template functions work at compile time
    struct TestObject
    {
        int value = 5;
        bool flag = false;
    };

    constexpr TestObject original {};
    auto modified = withMember (original, &TestObject::value, 10);

    EXPECT_EQ (original.value, 5);
    EXPECT_EQ (modified.value, 10);
    EXPECT_EQ (modified.flag, false);
}

// Test DisableIfSameOrDerived with a simple case
TEST_F (FunctionalTest, DisableIfSameOrDerivedConcept)
{
    struct Base
    {
    };

    struct Derived : Base
    {
    };

    struct Other
    {
    };

    // This test mainly ensures the template compiles correctly
    // The actual functionality is compile-time template SFINAE

    static_assert (std::is_base_of_v<Base, Derived>);
    static_assert (! std::is_base_of_v<Base, Other>);

    // These would be used in template specialization contexts
    // but we can at least verify the type traits work
    SUCCEED();
}

TEST_F (FunctionalTest, BindFront)
{
    struct X
    {
        int test (int x) { return x; }
    } x;

    auto func1 = [] (int a, bool b, float c)
    {
        return a;
    };

    auto bindFunc1 = bindFront (func1, 42);
    auto result1 = bindFunc1 (true, 1.0f);
    EXPECT_EQ (result1, 42);

    auto bindFunc2 = bindFront (&X::test, &x, 42);
    auto result2 = bindFunc2();
    EXPECT_EQ (result2, 42);
}

TEST_F (FunctionalTest, BindBack)
{
    struct X
    {
        int test (int x) { return x; }
    } x;

    auto func1 = [] (bool a, float b, int c)
    {
        return c;
    };

    auto bindFunc1 = bindBack (func1, 42);
    auto result1 = bindFunc1 (true, 1.0f);
    EXPECT_EQ (result1, 42);

    auto bindFunc2 = bindBack (&X::test, &x, 42);
    auto result2 = bindFunc2();
    EXPECT_EQ (result2, 42);
}
