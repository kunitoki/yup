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

class MemoryBlockTests : public ::testing::Test
{
protected:
    static constexpr uint8 sentinel = 0xEE;
};

TEST_F (MemoryBlockTests, CopyFromClipsOffsetBeyondSize)
{
    MemoryBlock block (4, true);
    const uint8 src[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };

    block.copyFrom (src, 6, 4);

    for (size_t i = 0; i < block.getSize(); ++i)
        EXPECT_EQ (static_cast<const uint8*> (block.getData())[i], 0);
}

TEST_F (MemoryBlockTests, CopyFromNegativeOffsetSkipsLeadingSourceBytes)
{
    MemoryBlock block (4, true);
    const uint8 src[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };

    block.copyFrom (src, -2, 4);

    const auto* data = static_cast<const uint8*> (block.getData());
    EXPECT_EQ (data[0], 3);
    EXPECT_EQ (data[1], 4);
    EXPECT_EQ (data[2], 0);
    EXPECT_EQ (data[3], 0);
}

TEST_F (MemoryBlockTests, CopyToZeroFillsOffsetBeyondSize)
{
    const uint8 content[4] = { 1, 2, 3, 4 };
    MemoryBlock block (content, sizeof (content));
    uint8 dst[4] = { sentinel, sentinel, sentinel, sentinel };

    block.copyTo (dst, 6, 4);

    for (auto value : dst)
        EXPECT_EQ (value, 0);
}

TEST_F (MemoryBlockTests, CopyToNegativeOffsetDoesNotWritePastDestination)
{
    const uint8 content[4] = { 1, 2, 3, 4 };
    MemoryBlock block (content, sizeof (content));
    uint8 dst[4] = { sentinel, sentinel, sentinel, sentinel };

    block.copyTo (dst, -4, 2);

    EXPECT_EQ (dst[0], 0);
    EXPECT_EQ (dst[1], 0);
    EXPECT_EQ (dst[2], sentinel);
    EXPECT_EQ (dst[3], sentinel);
}

TEST_F (MemoryBlockTests, CopyToZeroFillsNegativeOffsetLeadingBytes)
{
    const uint8 content[4] = { 1, 2, 3, 4 };
    MemoryBlock block (content, sizeof (content));
    uint8 dst[4] = { sentinel, sentinel, sentinel, sentinel };

    block.copyTo (dst, -2, 3);

    EXPECT_EQ (dst[0], 0);
    EXPECT_EQ (dst[1], 0);
    EXPECT_EQ (dst[2], 1);
    EXPECT_EQ (dst[3], sentinel);
}

TEST_F (MemoryBlockTests, FromBase64EncodingRejectsNegativeSize)
{
    MemoryBlock block (8, true);

    EXPECT_FALSE (block.fromBase64Encoding ("-1."));
    EXPECT_EQ (block.getSize(), 0);
}

TEST_F (MemoryBlockTests, FromBase64EncodingRejectsSizeLargerThanPayload)
{
    MemoryBlock block;

    EXPECT_FALSE (block.fromBase64Encoding ("2000000000.AAAA"));
    EXPECT_EQ (block.getSize(), 0);
}

TEST_F (MemoryBlockTests, Base64EncodingRoundTrips)
{
    const char* text = "hello";
    MemoryBlock source (text, std::strlen (text));

    MemoryBlock decoded;

    EXPECT_TRUE (decoded.fromBase64Encoding (source.toBase64Encoding()));
    EXPECT_EQ (decoded.getSize(), source.getSize());
    EXPECT_EQ (std::memcmp (decoded.getData(), text, 5), 0);
}
