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

#if ! YUP_EXCEPTIONS_DISABLED
TEST_F (HeapBlockTests, FailedAllocationLeavesThrowingBlockEmpty)
{
    HeapBlock<char, true> mallocBlock (16);
    EXPECT_THROW (mallocBlock.malloc ((size_t) 1 << 50), std::bad_alloc);
    EXPECT_TRUE (mallocBlock == nullptr);

    HeapBlock<char, true> callocBlock (16);
    EXPECT_THROW (callocBlock.calloc ((size_t) 1 << 50), std::bad_alloc);
    EXPECT_TRUE (callocBlock == nullptr);

    HeapBlock<char, true> allocateBlock (16);
    EXPECT_THROW (allocateBlock.allocate ((size_t) 1 << 50, true), std::bad_alloc);
    EXPECT_TRUE (allocateBlock == nullptr);
}
#endif
