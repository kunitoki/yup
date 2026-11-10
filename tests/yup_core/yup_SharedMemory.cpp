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

#if ! (YUP_WASM || YUP_ANDROID)

using namespace yup;

class SharedMemoryTests : public ::testing::Test
{
protected:
    String name = SharedMemory::createUniqueName ("test");
};

TEST_F (SharedMemoryTests, CreatedMemoryIsZeroedAndSharedWithOpenedMapping)
{
    SharedMemory created;
    ASSERT_TRUE (created.create (name, 4096).wasOk());
    EXPECT_TRUE (created.isOpen());
    EXPECT_EQ (4096u, created.getSize());

    auto* createdBytes = static_cast<uint8*> (created.getData());
    EXPECT_EQ (0, createdBytes[0]);
    EXPECT_EQ (0, createdBytes[4095]);

    SharedMemory opened;
    ASSERT_TRUE (opened.open (name, 4096).wasOk());
    EXPECT_NE (created.getData(), opened.getData());

    createdBytes[10] = 42;
    static_cast<uint8*> (opened.getData())[20] = 7;

    EXPECT_EQ (42, static_cast<uint8*> (opened.getData())[10]);
    EXPECT_EQ (7, createdBytes[20]);
}

TEST_F (SharedMemoryTests, CreateFailsWhenNameIsInUse)
{
    SharedMemory first, second;
    ASSERT_TRUE (first.create (name, 64).wasOk());

    EXPECT_TRUE (second.create (name, 64).failed());
    EXPECT_FALSE (second.isOpen());
}

TEST_F (SharedMemoryTests, OpenFailsForUnknownNameAndInvalidArguments)
{
    SharedMemory memory;

    EXPECT_TRUE (memory.open (name, 64).failed());
    EXPECT_TRUE (memory.create ({}, 64).failed());
    EXPECT_TRUE (memory.create (name, 0).failed());
    EXPECT_FALSE (memory.isOpen());
}

TEST_F (SharedMemoryTests, CloseByCreatorReleasesTheName)
{
    SharedMemory memory;
    ASSERT_TRUE (memory.create (name, 64).wasOk());

    memory.close();
    EXPECT_FALSE (memory.isOpen());

    SharedMemory opened;
    EXPECT_TRUE (opened.open (name, 64).failed());
    EXPECT_TRUE (memory.create (name, 64).wasOk());
}

TEST_F (SharedMemoryTests, MoveTransfersTheMapping)
{
    SharedMemory memory;
    ASSERT_TRUE (memory.create (name, 64).wasOk());
    auto* data = memory.getData();

    SharedMemory moved (std::move (memory));
    EXPECT_FALSE (memory.isOpen());
    EXPECT_EQ (data, moved.getData());
    EXPECT_EQ (name, moved.getName());
}

TEST_F (SharedMemoryTests, MoveAssignmentClosesTargetAndTransfersMapping)
{
    const auto otherName = SharedMemory::createUniqueName ("test");

    SharedMemory source, target;
    ASSERT_TRUE (source.create (name, 64).wasOk());
    ASSERT_TRUE (target.create (otherName, 64).wasOk());

    auto* data = source.getData();
    static_cast<uint8*> (data)[0] = 99;

    target = std::move (source);

    EXPECT_FALSE (source.isOpen());
    EXPECT_EQ (nullptr, source.getData());
    EXPECT_EQ (0u, source.getSize());

    EXPECT_TRUE (target.isOpen());
    EXPECT_EQ (data, target.getData());
    EXPECT_EQ (64u, target.getSize());
    EXPECT_EQ (name, target.getName());
    EXPECT_EQ (99, static_cast<uint8*> (target.getData())[0]);

    SharedMemory reopened;
    EXPECT_TRUE (reopened.open (otherName, 64).failed());

    target.close();
    EXPECT_TRUE (reopened.open (name, 64).failed());
}

TEST_F (SharedMemoryTests, OpenFailsWithInvalidArguments)
{
    SharedMemory created;
    ASSERT_TRUE (created.create (name, 64).wasOk());

    SharedMemory memory;
    EXPECT_TRUE (memory.open ({}, 64).failed());
    EXPECT_TRUE (memory.open (name, 0).failed());
    EXPECT_FALSE (memory.isOpen());
}

TEST_F (SharedMemoryTests, OpenFailsWhenRequestingMoreThanWasCreated)
{
    SharedMemory created;
    ASSERT_TRUE (created.create (name, 64).wasOk());

    SharedMemory opened;
    EXPECT_TRUE (opened.open (name, 1024 * 1024).failed());
    EXPECT_FALSE (opened.isOpen());
}

#if ! YUP_WINDOWS
TEST_F (SharedMemoryTests, CreateFailsAndReleasesNameForUnmappableSizes)
{
    SharedMemory memory;

    // Overflows off_t, so sizing the block fails
    EXPECT_TRUE (memory.create (name, std::numeric_limits<size_t>::max()).failed());
    EXPECT_FALSE (memory.isOpen());

    // Larger than any process address space, so mapping (or sizing) fails
    EXPECT_TRUE (memory.create (name, (size_t) 1 << 62).failed());
    EXPECT_FALSE (memory.isOpen());

    EXPECT_TRUE (memory.create (name, 64).wasOk());
}
#endif

TEST_F (SharedMemoryTests, UniqueNamesAreShortAndDistinct)
{
    const auto other = SharedMemory::createUniqueName ("test");

    EXPECT_NE (name, other);
    EXPECT_TRUE (name.startsWith ("test"));
    EXPECT_LE (name.length(), 24);
}

#endif
