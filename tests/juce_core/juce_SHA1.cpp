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

#include <juce_core/juce_core.h>

using namespace juce;

TEST (SHA1Tests, All)
{
    SHA1 hash ("", std::strlen (""));
    EXPECT_EQ (hash.toHexString(), String ("da39a3ee5e6b4b0d3255bfef95601890afd80709"));

    CharPointer_UTF8 utf8 ("The quick brown fox jumps over the lazy dog");
    SHA1 hash (utf8);
    EXPECT_EQ (hash.toHexString(), String ("2fd4e1c67a2d28fced849ee1bb76e7391b93eb12"));

    MemoryInputStream m ("The quick brown fox jumps over the lazy dog", std::strlen ("The quick brown fox jumps over the lazy dog"), false);
    SHA1 hash (m);
    EXPECT_EQ (hash.toHexString(), String ("408d94384216f890ff7a0c3528e8bed1e0b01621"));
}
