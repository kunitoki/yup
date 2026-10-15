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

class BinaryArchiveTests : public ::testing::Test
{
};

TEST_F (BinaryArchiveTests, VectorRoundTrips)
{
    const std::vector<int> original { 1, 2, 3 };
    MemoryOutputStream out;
    BinaryOutputArchive writer (out);
    EXPECT_TRUE (writer (original));

    MemoryInputStream in (out.getData(), out.getDataSize(), false);
    BinaryInputArchive reader (in);
    std::vector<int> restored;
    EXPECT_TRUE (reader (restored));
    EXPECT_EQ (restored, original);
}

TEST_F (BinaryArchiveTests, StringRoundTrips)
{
    MemoryOutputStream out;
    BinaryOutputArchive writer (out);
    EXPECT_TRUE (writer (String ("yup")));

    MemoryInputStream in (out.getData(), out.getDataSize(), false);
    BinaryInputArchive reader (in);
    String restored;
    EXPECT_TRUE (reader (restored));
    EXPECT_EQ (restored, String ("yup"));
}

TEST_F (BinaryArchiveTests, StringLengthBeyondInputIsRejected)
{
    MemoryOutputStream out;
    out.writeInt (1 << 30);

    MemoryInputStream in (out.getData(), out.getDataSize(), false);
    BinaryInputArchive reader (in);
    String restored;
    EXPECT_FALSE (reader (restored));
}

TEST_F (BinaryArchiveTests, NegativeStringLengthIsRejected)
{
    MemoryOutputStream out;
    out.writeInt (-1);

    MemoryInputStream in (out.getData(), out.getDataSize(), false);
    BinaryInputArchive reader (in);
    String restored;
    EXPECT_FALSE (reader (restored));
}

TEST_F (BinaryArchiveTests, ContainerCountBeyondInputIsRejected)
{
    MemoryOutputStream out;
    out.writeInt64 ((int64) 1 << 40);
    out.writeInt (7);

    MemoryInputStream in (out.getData(), out.getDataSize(), false);
    BinaryInputArchive reader (in);
    std::vector<int> restored;
    reader (restored);
    EXPECT_TRUE (restored.empty());
}

TEST_F (BinaryArchiveTests, NegativeContainerCountIsRejected)
{
    MemoryOutputStream out;
    out.writeInt64 (-5);

    MemoryInputStream in (out.getData(), out.getDataSize(), false);
    BinaryInputArchive reader (in);
    std::vector<int> restored;
    reader (restored);
    EXPECT_TRUE (restored.empty());
}
