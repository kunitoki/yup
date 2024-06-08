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

#include <gtest/gtest.h>

#include <juce_core/juce_core.h>

using namespace juce;

TEST(UuidTests, DefaultConstructorCreatesNonNullUuid)
{
    Uuid uuid;
    EXPECT_FALSE(uuid.isNull());
}

TEST(UuidTests, NullUuid)
{
    Uuid nullUuid = Uuid::null();
    EXPECT_TRUE(nullUuid.isNull());
    EXPECT_EQ(nullUuid.toString(), "00000000000000000000000000000000");
}

TEST(UuidTests, CopyConstructor)
{
    Uuid uuid1;
    Uuid uuid2 = uuid1;
    EXPECT_EQ(uuid1, uuid2);
}

TEST(UuidTests, CopyAssignment)
{
    Uuid uuid1;
    Uuid uuid2;
    uuid2 = uuid1;
    EXPECT_EQ(uuid1, uuid2);
}

TEST(UuidTests, MoveConstructor)
{
    Uuid uuid1;
    Uuid uuid2 = std::move(uuid1);
    EXPECT_FALSE(uuid2.isNull());
}

TEST(UuidTests, MoveAssignment)
{
    Uuid uuid1;
    Uuid uuid2;
    uuid2 = std::move(uuid1);
    EXPECT_FALSE(uuid2.isNull());
}

TEST(UuidTests, StringConstructor)
{
    String uuidStr = "12345678123456781234567812345678";
    Uuid uuid(uuidStr);
    EXPECT_EQ(uuid.toString(), uuidStr);
}

TEST(UuidTests, StringAssignment)
{
    String uuidStr = "12345678123456781234567812345678";
    Uuid uuid;
    uuid = uuidStr;
    EXPECT_EQ(uuid.toString(), uuidStr);
}

TEST(UuidTests, ToString)
{
    Uuid uuid;
    String uuidStr = uuid.toString();
    EXPECT_EQ(uuidStr.length(), 32);
}

TEST(UuidTests, ToDashedString)
{
    Uuid uuid;
    String dashedStr = uuid.toDashedString();
    EXPECT_EQ(dashedStr.length(), 36);
    EXPECT_EQ(dashedStr[8], '-');
    EXPECT_EQ(dashedStr[13], '-');
    EXPECT_EQ(dashedStr[18], '-');
    EXPECT_EQ(dashedStr[23], '-');
}

TEST(UuidTests, ComparisonOperators)
{
    Uuid uuid1;
    Uuid uuid2;

    EXPECT_NE(uuid1, uuid2);
    EXPECT_TRUE(uuid1 < uuid2 || uuid2 < uuid1);
    EXPECT_TRUE(uuid1 > uuid2 || uuid2 > uuid1);
    EXPECT_TRUE(uuid1 <= uuid2 || uuid2 <= uuid1);
    EXPECT_TRUE(uuid1 >= uuid2 || uuid2 >= uuid1);
}

TEST(UuidTests, GetTimeLow)
{
    Uuid uuid;
    uint32 timeLow = uuid.getTimeLow();
    EXPECT_NE(timeLow, 0);
}

TEST(UuidTests, GetTimeMid)
{
    Uuid uuid;
    uint16 timeMid = uuid.getTimeMid();
    EXPECT_NE(timeMid, 0);
}

TEST(UuidTests, GetTimeHighAndVersion)
{
    Uuid uuid;
    uint16 timeHighAndVersion = uuid.getTimeHighAndVersion();
    EXPECT_NE(timeHighAndVersion, 0);
}

TEST(UuidTests, GetClockSeqAndReserved)
{
    Uuid uuid;
    uint8 clockSeqAndReserved = uuid.getClockSeqAndReserved();
    EXPECT_NE(clockSeqAndReserved, 0);
}

TEST(UuidTests, GetClockSeqLow)
{
    Uuid uuid;
    uint8 clockSeqLow = uuid.getClockSeqLow();
    EXPECT_NE(clockSeqLow, 0);
}

TEST(UuidTests, GetNode)
{
    Uuid uuid;
    uint64 node = uuid.getNode();
    EXPECT_NE(node, 0);
}

TEST(UuidTests, GetRawData)
{
    Uuid uuid;
    const uint8* rawData = uuid.getRawData();
    EXPECT_NE(rawData, nullptr);
}

TEST(UuidTests, RawDataConstructor)
{
    uint8 rawData[16] = {0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0x0a, 0xbc, 0xde};
    Uuid uuid(rawData);
    EXPECT_EQ(uuid.getTimeLow(), 0x12345678);
    EXPECT_EQ(uuid.getNode(), 0x00004567890abcde);
}

TEST(UuidTests, RawDataAssignment)
{
    uint8 rawData[16] = {0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0x0a, 0xbc, 0xde};
    Uuid uuid;
    uuid = rawData;
    EXPECT_EQ(uuid.getTimeLow(), 0x12345678);
    EXPECT_EQ(uuid.getNode(), 0x00004567890abcde);
}

TEST(UuidTests, Hash)
{
    Uuid uuid;
    uint64 hash = uuid.hash();
    EXPECT_NE(hash, 0);
}
