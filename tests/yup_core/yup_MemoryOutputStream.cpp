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

#include <yup_core/yup_core.h>

using namespace yup;

TEST (MemoryOutputStreamTests, WriteTextUtf16SupportsFullUnicodeCodepoints)
{
    static constexpr yup_wchar stringA[] { 0x1F600, 0x00 };               // Grinning face emoji
    static constexpr yup_wchar stringB[] { 0xA, 0xB, 0xC, 0x0 };          // ASCII
    static constexpr yup_wchar stringC[] { 0xAAAA, 0xBBBB, 0xCCCC, 0x0 }; // two-byte characters

    CharPointer_UTF32 pointers[] { CharPointer_UTF32 (stringA),
                                   CharPointer_UTF32 (stringB),
                                   CharPointer_UTF32 (stringC) };

    for (auto originalPtr : pointers)
    {
        MemoryOutputStream stream;
        EXPECT_TRUE (stream.writeText (String (originalPtr), true, false, "\n"));
        EXPECT_NE (stream.getDataSize(), (size_t) 0);

        CharPointer_UTF16 writtenPtr { reinterpret_cast<const CharPointer_UTF16::CharType*> (stream.getData()) };

        for (auto currentOriginal = originalPtr; ! currentOriginal.isEmpty(); ++currentOriginal, ++writtenPtr)
            EXPECT_EQ (*currentOriginal, *writtenPtr);
    }
}
