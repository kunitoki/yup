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

#include <string>

using namespace juce;

class StringPoolTest : public ::testing::Test
{
protected:
    StringPool pool;

    void addStringsToPool(const std::vector<String>& strings)
    {
        for (const auto& s : strings)
            pool.getPooledString(s);
    }
};

TEST_F(StringPoolTest, ReturnsSameInstanceForDuplicateString)
{
    String str = "testString";
    auto pooled1 = pool.getPooledString(str);
    auto pooled2 = pool.getPooledString(str);

    // Check if the pointers for the same text are equal
    EXPECT_EQ(pooled1.getCharPointer().getAddress(), pooled2.getCharPointer().getAddress());
}

TEST_F(StringPoolTest, ReturnsSameInstanceForDifferentInputTypes)
{
    const char* cstr = "anotherTest";
    String str(cstr);
    StringRef strRef(str);

    auto pooled1 = pool.getPooledString(cstr);
    auto pooled2 = pool.getPooledString(str);
    auto pooled3 = pool.getPooledString(strRef);

    EXPECT_EQ(pooled1.getCharPointer().getAddress(), pooled2.getCharPointer().getAddress());
    EXPECT_EQ(pooled1.getCharPointer().getAddress(), pooled3.getCharPointer().getAddress());
}

TEST_F(StringPoolTest, DifferentStringsDifferentInstances)
{
    auto pooled1 = pool.getPooledString("stringOne");
    auto pooled2 = pool.getPooledString("stringTwo");

    EXPECT_NE(pooled1.getCharPointer().getAddress(), pooled2.getCharPointer().getAddress());
}

TEST_F(StringPoolTest, GarbageCollectFreesUnreferencedStrings)
{
    std::string a{ "temp1" }, b{ "temp2" }, c{ "temp3" };

    addStringsToPool({ String (a), String (b), String (c) });

    String::CharPointerType::CharType *address1, *address2;

    {
        auto pooled1 = pool.getPooledString(String{ "temp1" });
        address1 = pooled1.getCharPointer().getAddress();
    }

    pool.garbageCollect();
    addStringsToPool({ String (b), String (c) });

    {
        auto pooled2 = pool.getPooledString(std::string{ "temp1" });
        address2 = pooled2.getCharPointer().getAddress();
    }

    EXPECT_NE(address1, address2);
}

TEST_F(StringPoolTest, DifferentPoolDifferentStrings)
{
    StringPool pool1, pool2;

    auto pooled1 = pool1.getPooledString("stringOne");
    auto pooled2 = pool2.getPooledString("stringOne");

    EXPECT_NE(pooled1.getCharPointer().getAddress(), pooled2.getCharPointer().getAddress());
}

TEST_F(StringPoolTest, GlobalPoolSingletonInstance)
{
    auto& globalPool1 = StringPool::getGlobalPool();
    auto& globalPool2 = StringPool::getGlobalPool();

    EXPECT_EQ(&globalPool1, &globalPool2);

    auto pooled1 = globalPool1.getPooledString("stringOne");
    auto pooled2 = globalPool2.getPooledString("stringOne");

    EXPECT_EQ(pooled1.getCharPointer().getAddress(), pooled2.getCharPointer().getAddress());
}
