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

#include <functional>
#include <memory>
#include <stdexcept>

using namespace juce;

struct ConstructCounts
{
    int constructions = 0;
    int copies = 0;
    int moves = 0;
    int calls = 0;
    int destructions = 0;

    auto tie() const noexcept { return std::tie(constructions, copies, moves, calls, destructions); }

    ConstructCounts withConstructions(int i) const noexcept { auto c = *this; c.constructions = i; return c; }
    ConstructCounts withCopies(int i) const noexcept { auto c = *this; c.copies = i; return c; }
    ConstructCounts withMoves(int i) const noexcept { auto c = *this; c.moves = i; return c; }
    ConstructCounts withCalls(int i) const noexcept { auto c = *this; c.calls = i; return c; }
    ConstructCounts withDestructions(int i) const noexcept { auto c = *this; c.destructions = i; return c; }

    bool operator==(const ConstructCounts& other) const noexcept { return tie() == other.tie(); }
    bool operator!=(const ConstructCounts& other) const noexcept { return tie() != other.tie(); }
};

struct ConstructCounter
{
    explicit ConstructCounter(ConstructCounts& countsIn)
        : counts(countsIn) {}

    ConstructCounter(const ConstructCounter& c)
        : counts(c.counts)
    {
        counts.copies += 1;
    }

    ConstructCounter(ConstructCounter&& c) noexcept
        : counts(c.counts)
    {
        counts.moves += 1;
    }

    ~ConstructCounter() noexcept { counts.destructions += 1; }

    void operator()() const noexcept { counts.calls += 1; }

    ConstructCounts& counts;
};

TEST(FixedSizeFunctionTests, DefaultConstructor)
{
    FixedSizeFunction<64, void()> fn;
    EXPECT_FALSE(fn);
}

TEST(FixedSizeFunctionTests, NullptrConstructor)
{
    FixedSizeFunction<64, void()> fn(nullptr);
    EXPECT_FALSE(fn);
}

TEST(FixedSizeFunctionTests, CallableConstructor)
{
    int called = 0;
    FixedSizeFunction<64, void()> fn([&] { ++called; });
    EXPECT_TRUE(fn);
    fn();
    EXPECT_EQ(called, 1);
}

TEST(FixedSizeFunctionTests, MoveConstructor)
{
    int called = 0;
    FixedSizeFunction<64, void()> fn1([&] { ++called; });
    FixedSizeFunction<64, void()> fn2(std::move(fn1));
    EXPECT_TRUE(fn2);
    fn2();
    EXPECT_EQ(called, 1);
}

TEST(FixedSizeFunctionTests, MoveAssignment)
{
    int called = 0;
    FixedSizeFunction<64, void()> fn1([&] { ++called; });
    FixedSizeFunction<64, void()> fn2;
    fn2 = std::move(fn1);
    EXPECT_TRUE(fn2);
    fn2();
    EXPECT_EQ(called, 1);
}

TEST(FixedSizeFunctionTests, CallOperator)
{
    int called = 0;
    FixedSizeFunction<64, void()> fn([&] { ++called; });
    fn();
    EXPECT_EQ(called, 1);
}

TEST(FixedSizeFunctionTests, ThrowOnCallEmpty)
{
    FixedSizeFunction<64, void()> fn;
    EXPECT_THROW(fn(), std::bad_function_call);
}

TEST(FixedSizeFunctionTests, CallableWithArguments)
{
    FixedSizeFunction<64, int(int, int)> fn([](int a, int b) { return a + b; });
    EXPECT_EQ(fn(2, 3), 5);
}

TEST(FixedSizeFunctionTests, AssignCallable)
{
    int called = 0;
    FixedSizeFunction<64, void()> fn;
    fn = [&] { ++called; };
    fn();
    EXPECT_EQ(called, 1);
}

TEST(FixedSizeFunctionTests, ClearFunction)
{
    int called = 0;
    FixedSizeFunction<64, void()> fn([&] { ++called; });
    fn = nullptr;
    EXPECT_FALSE(fn);
    EXPECT_THROW(fn(), std::bad_function_call);
}

TEST(FixedSizeFunctionTests, DifferentSizes)
{
    int called = 0;
    FixedSizeFunction<128, void()> fn1([&] { ++called; });
    FixedSizeFunction<256, void()> fn2(std::move(fn1));
    EXPECT_TRUE(fn2);
    fn2();
    EXPECT_EQ(called, 1);
}

TEST(FixedSizeFunctionTests, NullAssignment)
{
    int called = 0;
    FixedSizeFunction<64, void()> fn([&] { ++called; });
    fn = nullptr;
    EXPECT_FALSE(fn);
    EXPECT_THROW(fn(), std::bad_function_call);
}

TEST(FixedSizeFunctionTests, ConstructedAndCalledFromLambda)
{
    const auto result = 5;
    bool wasCalled = false;
    const auto lambda = [&] { wasCalled = true; return result; };

    const FixedSizeFunction<sizeof(lambda), int()> fn(lambda);
    const auto out = fn();

    EXPECT_TRUE(wasCalled);
    EXPECT_EQ(result, out);
}

TEST(FixedSizeFunctionTests, VoidFunctionConstructedFromReturnValueFunction)
{
    bool wasCalled = false;
    const auto lambda = [&] { wasCalled = true; return 5; };
    const FixedSizeFunction<sizeof(lambda), void()> fn(lambda);

    fn();
    EXPECT_TRUE(wasCalled);
}

TEST(FixedSizeFunctionTests, ConstructedAndCalledFromFunctionPointer)
{
    bool state = false;
    auto toggleBool = [](bool& b) { b = !b; };

    const FixedSizeFunction<sizeof(void*), void(bool&)> fn(toggleBool);

    fn(state);
    EXPECT_TRUE(state);

    fn(state);
    EXPECT_FALSE(state);

    fn(state);
    EXPECT_TRUE(state);
}

TEST(FixedSizeFunctionTests, DefaultConstructedFunctionsThrowIfCalled)
{
    const auto a = FixedSizeFunction<8, void()>();
    EXPECT_THROW(a(), std::bad_function_call);

    const auto b = FixedSizeFunction<8, void()>(nullptr);
    EXPECT_THROW(b(), std::bad_function_call);
}

TEST(FixedSizeFunctionTests, FunctionsCanBeMoved)
{
    ConstructCounts counts;

    auto a = FixedSizeFunction<sizeof(ConstructCounter), void()>(ConstructCounter{ counts });
    EXPECT_EQ(counts, ConstructCounts().withMoves(1).withDestructions(1)); // The temporary gets destroyed

    a();
    EXPECT_EQ(counts, ConstructCounts().withMoves(1).withDestructions(1).withCalls(1));

    const auto b = std::move(a);
    EXPECT_EQ(counts, ConstructCounts().withMoves(2).withDestructions(1).withCalls(1));

    b();
    EXPECT_EQ(counts, ConstructCounts().withMoves(2).withDestructions(1).withCalls(2));

    b();
    EXPECT_EQ(counts, ConstructCounts().withMoves(2).withDestructions(1).withCalls(3));
}

TEST(FixedSizeFunctionTests, FunctionsAreDestructedProperly)
{
    ConstructCounts counts;
    const ConstructCounter toCopy{ counts };

    {
        auto a = FixedSizeFunction<sizeof(ConstructCounter), void()>(toCopy);
        EXPECT_EQ(counts, ConstructCounts().withCopies(1));
    }

    EXPECT_EQ(counts, ConstructCounts().withCopies(1).withDestructions(1));
}

TEST(FixedSizeFunctionTests, AvoidDestructingFunctionsThatFailToConstruct)
{
    struct BadConstructor
    {
        explicit BadConstructor(ConstructCounts& c)
            : counts(c)
        {
            counts.constructions += 1;
            throw std::runtime_error{ "this was meant to happen" };
        }

        BadConstructor(const BadConstructor&) = default;
        BadConstructor& operator=(const BadConstructor&) = delete;

        ~BadConstructor() noexcept { counts.destructions += 1; }

        void operator()() const noexcept { counts.calls += 1; }

        ConstructCounts& counts;
    };

    ConstructCounts counts;

    EXPECT_THROW((FixedSizeFunction<sizeof(BadConstructor), void()>(BadConstructor{ counts })),
                 std::runtime_error);

    EXPECT_EQ(counts, ConstructCounts().withConstructions(1));
}

TEST(FixedSizeFunctionTests, EqualityChecksWork)
{
    FixedSizeFunction<8, void()> a;
    EXPECT_FALSE(bool(a));
    EXPECT_TRUE(a == nullptr);
    EXPECT_TRUE(nullptr == a);
    EXPECT_FALSE(a != nullptr);
    EXPECT_FALSE(nullptr != a);

    FixedSizeFunction<8, void()> b([] {});
    EXPECT_TRUE(bool(b));
    EXPECT_FALSE(b == nullptr);
    EXPECT_FALSE(nullptr == b);
    EXPECT_TRUE(b != nullptr);
    EXPECT_TRUE(nullptr != b);
}

TEST(FixedSizeFunctionTests, FunctionsCanBeCleared)
{
    FixedSizeFunction<8, void()> fn([] {});
    EXPECT_TRUE(bool(fn));

    fn = nullptr;
    EXPECT_FALSE(bool(fn));
}

TEST(FixedSizeFunctionTests, FunctionsCanBeAssigned)
{
    using Fn = FixedSizeFunction<8, void()>;

    int numCallsA = 0;
    int numCallsB = 0;

    Fn x;
    Fn y;
    EXPECT_FALSE(bool(x));
    EXPECT_FALSE(bool(y));

    x = [&] { numCallsA += 1; };
    y = [&] { numCallsB += 1; };
    EXPECT_TRUE(bool(x));
    EXPECT_TRUE(bool(y));

    x();
    EXPECT_EQ(numCallsA, 1);
    EXPECT_EQ(numCallsB, 0);

    y();
    EXPECT_EQ(numCallsA, 1);
    EXPECT_EQ(numCallsB, 1);

    x = std::move(y);
    EXPECT_EQ(numCallsA, 1);
    EXPECT_EQ(numCallsB, 1);

    x();
    EXPECT_EQ(numCallsA, 1);
    EXPECT_EQ(numCallsB, 2);
}

TEST(FixedSizeFunctionTests, FunctionsMayMutateInternalState)
{
    using Fn = FixedSizeFunction<64, void()>;

    Fn x;
    EXPECT_FALSE(bool(x));

    int numCalls = 0;
    x = [&numCalls, counter = 0]() mutable { counter += 1; numCalls = counter; };
    EXPECT_TRUE(bool(x));

    EXPECT_EQ(numCalls, 0);

    x();
    EXPECT_EQ(numCalls, 1);

    x();
    EXPECT_EQ(numCalls, 2);
}

TEST(FixedSizeFunctionTests, FunctionsCanSinkMoveOnlyParameters)
{
    using FnA = FixedSizeFunction<64, int(std::unique_ptr<int>)>;

    auto value = 5;
    auto ptr = std::make_unique<int>(value);

    FnA fnA = [](std::unique_ptr<int> p) { return *p; };

    EXPECT_EQ(value, fnA(std::move(ptr)));

    using FnB = FixedSizeFunction<64, void(std::unique_ptr<int>&&)>;

    FnB fnB = [&value](std::unique_ptr<int>&& p)
    {
        auto x = std::move(p);
        value = *x;
    };

    const auto newValue = 10;
    fnB(std::make_unique<int>(newValue));
    EXPECT_EQ(value, newValue);
}

TEST(FixedSizeFunctionTests, FunctionsCanBeConvertedFromSmallerFunctions)
{
    using SmallFn = FixedSizeFunction<20, void()>;
    using LargeFn = FixedSizeFunction<21, void()>;

    bool smallCalled = false;
    bool largeCalled = false;

    SmallFn small = [&smallCalled, a = std::array<char, 8>{}] { smallCalled = true; ignoreUnused(a); };
    LargeFn large = [&largeCalled, a = std::array<char, 8>{}] { largeCalled = true; ignoreUnused(a); };

    large = std::move(small);

    large();

    EXPECT_TRUE(smallCalled);
    EXPECT_FALSE(largeCalled);
}
