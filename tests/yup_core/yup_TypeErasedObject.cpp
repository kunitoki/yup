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

#include <string>
#include <utility>

using namespace yup;

namespace
{

struct LifetimeTracker
{
    explicit LifetimeTracker (int* destructionCounter) noexcept
        : counter (destructionCounter)
    {
    }

    LifetimeTracker (LifetimeTracker&& other) noexcept
        : counter (std::exchange (other.counter, nullptr))
    {
    }

    LifetimeTracker& operator= (LifetimeTracker&& other) noexcept
    {
        counter = std::exchange (other.counter, nullptr);
        return *this;
    }

    LifetimeTracker (const LifetimeTracker&) = delete;
    LifetimeTracker& operator= (const LifetimeTracker&) = delete;

    ~LifetimeTracker()
    {
        if (counter != nullptr)
            ++(*counter);
    }

    int* counter = nullptr;
};

struct Payload
{
    int value = 0;
    double factor = 0.0;
};

} // namespace

class TypeErasedObjectTests : public ::testing::Test
{
};

TEST_F (TypeErasedObjectTests, DefaultConstructedHasNoPayload)
{
    TypeErasedObject<64> object;

    EXPECT_EQ (object.getPayload<int>(), nullptr);
    EXPECT_EQ (object.getPayload<Payload>(), nullptr);
}

TEST_F (TypeErasedObjectTests, StoresAndRetrievesValue)
{
    TypeErasedObject<64> object (Payload { 42, 1.5 });

    auto* payload = object.getPayload<Payload>();
    ASSERT_NE (payload, nullptr);
    EXPECT_EQ (payload->value, 42);
    EXPECT_EQ (payload->factor, 1.5);
}

TEST_F (TypeErasedObjectTests, StoresPrimitiveValue)
{
    TypeErasedObject<64> object (123);

    auto* payload = object.getPayload<int>();
    ASSERT_NE (payload, nullptr);
    EXPECT_EQ (*payload, 123);
}

TEST_F (TypeErasedObjectTests, GetPayloadWithWrongTypeReturnsNull)
{
    TypeErasedObject<64> object (Payload { 1, 2.0 });

    EXPECT_EQ (object.getPayload<int>(), nullptr);
    EXPECT_EQ (object.getPayload<double>(), nullptr);
    EXPECT_NE (object.getPayload<Payload>(), nullptr);
}

TEST_F (TypeErasedObjectTests, ConstGetPayloadRetrievesValue)
{
    const TypeErasedObject<64> object (Payload { 7, 3.0 });

    const auto* payload = object.getPayload<Payload>();
    ASSERT_NE (payload, nullptr);
    EXPECT_EQ (payload->value, 7);
    EXPECT_EQ (payload->factor, 3.0);
}

TEST_F (TypeErasedObjectTests, ConstGetPayloadWithWrongTypeReturnsNull)
{
    const TypeErasedObject<64> object (Payload { 7, 3.0 });

    EXPECT_EQ (object.getPayload<int>(), nullptr);
}

TEST_F (TypeErasedObjectTests, NonConstPayloadIsMutable)
{
    TypeErasedObject<64> object (Payload { 10, 1.0 });

    auto* payload = object.getPayload<Payload>();
    ASSERT_NE (payload, nullptr);
    payload->value = 99;

    EXPECT_EQ (object.getPayload<Payload>()->value, 99);
}

TEST_F (TypeErasedObjectTests, MoveConstructionTransfersPayload)
{
    TypeErasedObject<64> source (Payload { 55, 2.5 });
    TypeErasedObject<64> destination (std::move (source));

    auto* payload = destination.getPayload<Payload>();
    ASSERT_NE (payload, nullptr);
    EXPECT_EQ (payload->value, 55);
    EXPECT_EQ (payload->factor, 2.5);

    EXPECT_EQ (source.getPayload<Payload>(), nullptr);
}

TEST_F (TypeErasedObjectTests, MoveAssignmentTransfersPayload)
{
    TypeErasedObject<64> source (Payload { 12, 4.0 });
    TypeErasedObject<64> destination;

    destination = std::move (source);

    auto* payload = destination.getPayload<Payload>();
    ASSERT_NE (payload, nullptr);
    EXPECT_EQ (payload->value, 12);

    EXPECT_EQ (source.getPayload<Payload>(), nullptr);
}

TEST_F (TypeErasedObjectTests, DestructorInvokesStoredDeleter)
{
    int destructions = 0;

    {
        TypeErasedObject<64> object (LifetimeTracker { &destructions });
        EXPECT_EQ (destructions, 0);
    }

    EXPECT_EQ (destructions, 1);
}

TEST_F (TypeErasedObjectTests, MoveConstructionDoesNotDoubleDestroy)
{
    int destructions = 0;

    {
        TypeErasedObject<64> source (LifetimeTracker { &destructions });
        TypeErasedObject<64> destination (std::move (source));

        EXPECT_EQ (destructions, 0);
    }

    EXPECT_EQ (destructions, 1);
}

TEST_F (TypeErasedObjectTests, MoveAssignmentDestroysExistingPayload)
{
    int firstDestructions = 0;
    int secondDestructions = 0;

    {
        TypeErasedObject<64> destination (LifetimeTracker { &firstDestructions });
        TypeErasedObject<64> source (LifetimeTracker { &secondDestructions });

        destination = std::move (source);

        EXPECT_EQ (firstDestructions, 1);
        EXPECT_EQ (secondDestructions, 0);
    }

    EXPECT_EQ (firstDestructions, 1);
    EXPECT_EQ (secondDestructions, 1);
}

TEST_F (TypeErasedObjectTests, MovedFromObjectIsReusable)
{
    TypeErasedObject<64> source (Payload { 1, 1.0 });
    TypeErasedObject<64> destination (std::move (source));

    source = TypeErasedObject<64> (Payload { 2, 2.0 });

    auto* payload = source.getPayload<Payload>();
    ASSERT_NE (payload, nullptr);
    EXPECT_EQ (payload->value, 2);
}

TEST_F (TypeErasedObjectTests, StoresTypeFillingTheBuffer)
{
    struct FullBuffer
    {
        char data[16] = {};
    };

    TypeErasedObject<16> object (FullBuffer {});

    EXPECT_NE (object.getPayload<FullBuffer>(), nullptr);
}

TEST_F (TypeErasedObjectTests, MoveConstructionFromSmallerSize)
{
    TypeErasedObject<16> source (Payload { 77, 5.5 });
    TypeErasedObject<64> destination (std::move (source));

    auto* payload = destination.getPayload<Payload>();
    ASSERT_NE (payload, nullptr);
    EXPECT_EQ (payload->value, 77);
    EXPECT_EQ (payload->factor, 5.5);

    EXPECT_EQ (source.getPayload<Payload>(), nullptr);
}

TEST_F (TypeErasedObjectTests, MoveAssignmentFromSmallerSize)
{
    TypeErasedObject<16> source (Payload { 88, 6.5 });
    TypeErasedObject<64> destination;

    destination = std::move (source);

    auto* payload = destination.getPayload<Payload>();
    ASSERT_NE (payload, nullptr);
    EXPECT_EQ (payload->value, 88);
    EXPECT_EQ (payload->factor, 6.5);

    EXPECT_EQ (source.getPayload<Payload>(), nullptr);
}

TEST_F (TypeErasedObjectTests, MoveFromSmallerSizeDoesNotDoubleDestroy)
{
    int destructions = 0;

    {
        TypeErasedObject<16> source (LifetimeTracker { &destructions });
        TypeErasedObject<64> destination (std::move (source));

        EXPECT_EQ (destructions, 0);
    }

    EXPECT_EQ (destructions, 1);
}

TEST_F (TypeErasedObjectTests, DeductionGuideSizesStorageToValue)
{
    TypeErasedObject object (Payload { 33, 7.5 });

    static_assert (std::is_same_v<decltype (object), TypeErasedObject<sizeof (Payload)>>);

    auto* payload = object.getPayload<Payload>();
    ASSERT_NE (payload, nullptr);
    EXPECT_EQ (payload->value, 33);
    EXPECT_EQ (payload->factor, 7.5);
}

TEST_F (TypeErasedObjectTests, MoveAssignmentFromSameSizedEmptyDoesNotCrash)
{
    TypeErasedObject<64> destination (Payload { 1, 1.0 });
    TypeErasedObject<64> source; // Empty - moved from elsewhere

    // Move from empty source into a destination with a live payload.
    destination = std::move (source);

    // Destination is now empty.
    EXPECT_EQ (destination.getPayload<Payload>(), nullptr);
}

TEST_F (TypeErasedObjectTests, MoveAssignmentFromSmallerSizedEmptyDoesNotCrash)
{
    TypeErasedObject<16> source; // Empty
    TypeErasedObject<64> destination (Payload { 1, 1.0 });

    // Move from smaller empty source.
    destination = std::move (source);

    EXPECT_EQ (destination.getPayload<Payload>(), nullptr);
}

TEST_F (TypeErasedObjectTests, MoveAssignmentFromEmptyDestroysExistingPayload)
{
    int destructions = 0;

    {
        TypeErasedObject<16> source; // Empty - moved from elsewhere
        TypeErasedObject<64> destination (LifetimeTracker { &destructions });

        // Before move: destination owns a LifetimeTracker.
        EXPECT_EQ (destructions, 0);

        destination = std::move (source);

        // After moving empty source: destination's LifetimeTracker should be destroyed.
        EXPECT_EQ (destructions, 1);
    }

    EXPECT_EQ (destructions, 1);
}

TEST_F (TypeErasedObjectTests, StoresDoublePrecisionValue)
{
    TypeErasedObject<64> object (3.14159265358979323846);

    auto* payload = object.getPayload<double>();
    ASSERT_NE (payload, nullptr);
    EXPECT_DOUBLE_EQ (*payload, 3.14159265358979323846);
}

TEST_F (TypeErasedObjectTests, StoresMaxSizePayloadAtBufferLimit)
{
    struct MaxPayload
    {
        char data[64] = {};
    };

    TypeErasedObject<64> object (MaxPayload {});
    ASSERT_NE (object.getPayload<MaxPayload>(), nullptr);
}

TEST_F (TypeErasedObjectTests, MoveAssignmentSelfDestroysExistingFromSmallerSize)
{
    int destructions = 0;

    {
        TypeErasedObject<16> source; // Empty
        TypeErasedObject<64> destination (LifetimeTracker { &destructions });

        destination = std::move (source);

        EXPECT_EQ (destructions, 1);
    }

    EXPECT_EQ (destructions, 1);
}

TEST_F (TypeErasedObjectTests, ConstGetPayloadOnDefaultReturnsNull)
{
    const TypeErasedObject<64> object;
    EXPECT_EQ (object.getPayload<int>(), nullptr);
    EXPECT_EQ (object.getPayload<Payload>(), nullptr);
    EXPECT_EQ (object.getPayload<double>(), nullptr);
}

TEST_F (TypeErasedObjectTests, NonConstGetPayloadOnDefaultReturnsNull)
{
    TypeErasedObject<64> object;
    EXPECT_EQ (object.getPayload<int>(), nullptr);
    EXPECT_EQ (object.getPayload<Payload>(), nullptr);
    EXPECT_EQ (object.getPayload<double>(), nullptr);
}
