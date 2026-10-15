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

using namespace yup;

class HeapBlockTests : public ::testing::Test
{
};

#if ! YUP_WASM && ! YUP_EXCEPTIONS_DISABLED
TEST_F (HeapBlockTests, FailedAllocationLeavesThrowingBlockEmpty)
{
    // Too large for any address space, so the allocator must fail even when the OS overcommits memory
    const auto impossibleSize = static_cast<size_t> (-1) / 2;

    HeapBlock<char, true> mallocBlock (16);
    EXPECT_THROW (mallocBlock.malloc (impossibleSize), std::bad_alloc);
    EXPECT_TRUE (mallocBlock == nullptr);

    HeapBlock<char, true> callocBlock (16);
    EXPECT_THROW (callocBlock.calloc (impossibleSize), std::bad_alloc);
    EXPECT_TRUE (callocBlock == nullptr);

    HeapBlock<char, true> allocateBlock (16);
    EXPECT_THROW (allocateBlock.allocate (impossibleSize, true), std::bad_alloc);
    EXPECT_TRUE (allocateBlock == nullptr);
}
#endif
