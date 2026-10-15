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

#include <algorithm>
#include <array>

using namespace yup;

class SecureRandomTests : public ::testing::Test
{
protected:
    static bool allZero (const std::vector<uint8>& bytes)
    {
        return std::all_of (bytes.begin(), bytes.end(), [] (uint8 b) { return b == 0; });
    }
};

TEST_F (SecureRandomTests, FillsBufferWithNonZeroBytes)
{
    std::vector<uint8> bytes (256, 0);

    EXPECT_TRUE (SecureRandom::fillBytes (bytes.data(), bytes.size()));
    EXPECT_FALSE (allZero (bytes));
}

TEST_F (SecureRandomTests, SucceedsForLengthsAboveOneChunk)
{
    std::vector<uint8> bytes (1000, 0);

    EXPECT_TRUE (SecureRandom::fillBytes (bytes.data(), bytes.size()));
    EXPECT_FALSE (allZero (bytes));
}

TEST_F (SecureRandomTests, ZeroLengthSucceeds)
{
    EXPECT_TRUE (SecureRandom::fillBytes (nullptr, 0));
}

TEST_F (SecureRandomTests, UuidVersionAndVariantBitsAreSet)
{
    const Uuid uuid;
    const auto text = uuid.toDashedString();

    EXPECT_EQ (text.length(), 36);
    EXPECT_EQ (text[14], (yup_wchar) '4');
}
