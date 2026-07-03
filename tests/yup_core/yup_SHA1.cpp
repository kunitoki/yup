/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

TEST (SHA1Tests, All)
{
    SHA1 hash1 ("", std::strlen (""));
    EXPECT_EQ (hash1.toHexString(), String ("da39a3ee5e6b4b0d3255bfef95601890afd80709"));

    CharPointer_UTF8 utf8 ("The quick brown fox jumps over the lazy dog");
    SHA1 hash2 (utf8);
    EXPECT_EQ (hash2.toHexString(), String ("2fd4e1c67a2d28fced849ee1bb76e7391b93eb12"));

    MemoryInputStream m ("The quick brown fox jumps over the lazy dog", std::strlen ("The quick brown fox jumps over the lazy dog"), false);
    SHA1 hash3 (m);
    EXPECT_EQ (hash3.toHexString(), String ("2fd4e1c67a2d28fced849ee1bb76e7391b93eb12"));
}

TEST (SHA1Tests, EqualityOperator)
{
    SHA1 a ("hello", 5);
    SHA1 b ("hello", 5);
    SHA1 c ("world", 5);

    EXPECT_TRUE (a == b);
    EXPECT_FALSE (a == c);
    EXPECT_FALSE (a != b);
    EXPECT_TRUE (a != c);
}

TEST (SHA1Tests, HashViaMemoryBlock)
{
    const char* data = "The quick brown fox jumps over the lazy dog";
    MemoryBlock block (data, std::strlen (data));
    SHA1 hash (block.getData(), block.getSize());
    EXPECT_EQ (hash.toHexString(), String ("2fd4e1c67a2d28fced849ee1bb76e7391b93eb12"));
}

TEST (SHA1Tests, HexStringLengthIs40Characters)
{
    SHA1 hash ("any input", 9);
    EXPECT_EQ (hash.toHexString().length(), 40);
}

TEST (SHA1Tests, CopyConstructorProducesSameHash)
{
    SHA1 original ("test data", 9);
    SHA1 copy (original);
    EXPECT_EQ (original.toHexString(), copy.toHexString());
    EXPECT_TRUE (original == copy);
}

TEST (SHA1Tests, KnownVectorAbc)
{
    SHA1 hash ("abc", 3);
    EXPECT_EQ (hash.toHexString(), String ("a9993e364706816aba3e25717850c26c9cd0d89d"));
}

TEST (SHA1Tests, MultiBlockInputViaStream)
{
    // NIST SHA-1 test vector: 112-byte input spanning two 64-byte blocks
    const char* input = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
    const size_t len = std::strlen (input);
    EXPECT_GT (len, (size_t) 64); // Verify truly multi-block
    SHA1 hash (input, len);
    EXPECT_EQ (hash.toHexString(), String ("a49b2446a02c645bf419f995b67091253a04a259"));
}

TEST (SHA1Tests, SameInputAlwaysProducesSameHash)
{
    const char* data = "deterministic input";
    const size_t len = std::strlen (data);
    SHA1 hash1 (data, len);
    SHA1 hash2 (data, len);
    EXPECT_EQ (hash1.toHexString(), hash2.toHexString());
    EXPECT_TRUE (hash1 == hash2);
}
