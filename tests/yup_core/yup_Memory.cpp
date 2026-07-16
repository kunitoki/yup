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

#include <gtest/gtest.h>

#include <yup_core/yup_core.h>

#include <type_traits>
#include <utility>

using namespace yup;

namespace
{

struct TrackedObject
{
    TrackedObject (int valueToStore, int* destructionCounter) noexcept
        : value (valueToStore)
        , counter (destructionCounter)
    {
    }

    ~TrackedObject()
    {
        if (counter != nullptr)
            ++(*counter);
    }

    int value = 0;
    int* counter = nullptr;
};

} // namespace

class MemoryConstructAtTests : public ::testing::Test
{
};

TEST_F (MemoryConstructAtTests, VoidifyReturnsAddressAsVoidPointer)
{
    int value = 42;

    EXPECT_EQ (voidify (value), static_cast<void*> (&value));
}

TEST_F (MemoryConstructAtTests, ConstructAtBuildsScalarInPlace)
{
    alignas (int) unsigned char storage[sizeof (int)] = {};

    auto* ptr = constructAt (reinterpret_cast<int*> (storage), 123);

    ASSERT_NE (ptr, nullptr);
    EXPECT_EQ (*ptr, 123);
    EXPECT_EQ (ptr, reinterpret_cast<int*> (storage));

    destroyAt (ptr);
}

TEST_F (MemoryConstructAtTests, ConstructAtForwardsMultipleArguments)
{
    alignas (TrackedObject) unsigned char storage[sizeof (TrackedObject)] = {};
    int destructions = 0;

    auto* ptr = constructAt (reinterpret_cast<TrackedObject*> (storage), 7, &destructions);

    ASSERT_NE (ptr, nullptr);
    EXPECT_EQ (ptr->value, 7);
    EXPECT_EQ (destructions, 0);

    destroyAt (ptr);
    EXPECT_EQ (destructions, 1);
}

TEST_F (MemoryConstructAtTests, ConstructAtReturnsSameLocation)
{
    alignas (double) unsigned char storage[sizeof (double)] = {};
    auto* location = reinterpret_cast<double*> (storage);

    EXPECT_EQ (constructAt (location, 1.5), location);

    destroyAt (location);
}

TEST_F (MemoryConstructAtTests, DestroyAtInvokesDestructor)
{
    alignas (TrackedObject) unsigned char storage[sizeof (TrackedObject)] = {};
    int destructions = 0;

    auto* ptr = constructAt (reinterpret_cast<TrackedObject*> (storage), 1, &destructions);
    destroyAt (ptr);

    EXPECT_EQ (destructions, 1);
}

TEST_F (MemoryConstructAtTests, ConstructAtIsConstexpr)
{
    static_assert (std::is_same_v<decltype (constructAt (std::declval<int*>(), 0)), int*>);
    SUCCEED();
}
