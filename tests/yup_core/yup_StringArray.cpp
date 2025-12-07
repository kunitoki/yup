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

TEST (StringArrayTests, MoveConstructorFromArray)
{
    Array<String> arr;
    arr.add ("first");
    arr.add ("second");
    arr.add ("third");

    StringArray sa (std::move (arr));

    EXPECT_EQ (sa.size(), 3);
    EXPECT_EQ (sa[0], "first");
    EXPECT_EQ (sa[1], "second");
    EXPECT_EQ (sa[2], "third");
}

TEST (StringArrayTests, ConstructorFromCharPointerArray)
{
    const char* strings[] = { "one", "two", "three", nullptr };

    StringArray sa (strings);

    EXPECT_EQ (sa.size(), 3);
    EXPECT_EQ (sa[0], "one");
    EXPECT_EQ (sa[1], "two");
    EXPECT_EQ (sa[2], "three");
}

TEST (StringArrayTests, ConstructorFromCharPointerArrayWithCount)
{
    const char* strings[] = { "one", "two", "three", "four" };

    StringArray sa (strings, 3);

    EXPECT_EQ (sa.size(), 3);
    EXPECT_EQ (sa[0], "one");
    EXPECT_EQ (sa[1], "two");
    EXPECT_EQ (sa[2], "three");
}

TEST (StringArrayTests, ConstructorFromWideCharPointerArray)
{
    const wchar_t* strings[] = { L"one", L"two", L"three", nullptr };

    StringArray sa (strings);

    EXPECT_EQ (sa.size(), 3);
    EXPECT_EQ (sa[0], "one");
    EXPECT_EQ (sa[1], "two");
    EXPECT_EQ (sa[2], "three");
}

TEST (StringArrayTests, ConstructorFromWideCharPointerArrayWithCount)
{
    const wchar_t* strings[] = { L"one", L"two", L"three", L"four" };

    StringArray sa (strings, 3);

    EXPECT_EQ (sa.size(), 3);
    EXPECT_EQ (sa[0], "one");
    EXPECT_EQ (sa[1], "two");
    EXPECT_EQ (sa[2], "three");
}

TEST (StringArrayTests, IndexOfWithNegativeStartIndex)
{
    StringArray sa;
    sa.add ("apple");
    sa.add ("banana");
    sa.add ("cherry");

    int index = sa.indexOf ("banana", false, -5);
    EXPECT_EQ (index, 1);
}

TEST (StringArrayTests, IndexOfIgnoreCaseFalse)
{
    StringArray sa;
    sa.add ("Apple");
    sa.add ("Banana");
    sa.add ("Cherry");

    EXPECT_EQ (sa.indexOf ("banana", false), -1);
    EXPECT_EQ (sa.indexOf ("Banana", false), 1);
}

TEST (StringArrayTests, IndexOfIgnoreCaseTrue)
{
    StringArray sa;
    sa.add ("Apple");
    sa.add ("Banana");
    sa.add ("Cherry");

    EXPECT_EQ (sa.indexOf ("banana", true), 1);
    EXPECT_EQ (sa.indexOf ("CHERRY", true), 2);
}

TEST (StringArrayTests, RemoveStringIgnoreCase)
{
    StringArray sa;
    sa.add ("Apple");
    sa.add ("banana");
    sa.add ("BANANA");
    sa.add ("Cherry");

    sa.removeString ("banana", true);

    EXPECT_EQ (sa.size(), 2);
    EXPECT_EQ (sa[0], "Apple");
    EXPECT_EQ (sa[1], "Cherry");
}

TEST (StringArrayTests, RemoveStringCaseSensitive)
{
    StringArray sa;
    sa.add ("Apple");
    sa.add ("banana");
    sa.add ("BANANA");
    sa.add ("Cherry");

    sa.removeString ("banana", false);

    EXPECT_EQ (sa.size(), 3);
    EXPECT_EQ (sa[0], "Apple");
    EXPECT_EQ (sa[1], "BANANA");
    EXPECT_EQ (sa[2], "Cherry");
}

TEST (StringArrayTests, RemoveEmptyStringsWithoutWhitespace)
{
    StringArray sa;
    sa.add ("apple");
    sa.add ("");
    sa.add ("banana");
    sa.add ("   ");
    sa.add ("cherry");
    sa.add ("");

    sa.removeEmptyStrings (false);

    EXPECT_EQ (sa.size(), 4);
    EXPECT_EQ (sa[0], "apple");
    EXPECT_EQ (sa[1], "banana");
    EXPECT_EQ (sa[2], "   ");
    EXPECT_EQ (sa[3], "cherry");
}

TEST (StringArrayTests, RemoveEmptyStringsWithWhitespace)
{
    StringArray sa;
    sa.add ("apple");
    sa.add ("");
    sa.add ("banana");
    sa.add ("   ");
    sa.add ("cherry");
    sa.add ("\t\n");

    sa.removeEmptyStrings (true);

    EXPECT_EQ (sa.size(), 3);
    EXPECT_EQ (sa[0], "apple");
    EXPECT_EQ (sa[1], "banana");
    EXPECT_EQ (sa[2], "cherry");
}

TEST (StringArrayTests, JoinIntoStringWithNegativeStart)
{
    StringArray sa;
    sa.add ("apple");
    sa.add ("banana");
    sa.add ("cherry");

    String result = sa.joinIntoString (", ", -5, -1);
    EXPECT_EQ (result, "apple, banana, cherry");
}

TEST (StringArrayTests, JoinIntoStringWithLimitedRange)
{
    StringArray sa;
    sa.add ("apple");
    sa.add ("banana");
    sa.add ("cherry");
    sa.add ("date");
    sa.add ("elderberry");

    String result = sa.joinIntoString (", ", 1, 3);
    EXPECT_EQ (result, "banana, cherry, date");
}

TEST (StringArrayTests, JoinIntoStringWithNumberExceedingSize)
{
    StringArray sa;
    sa.add ("apple");
    sa.add ("banana");
    sa.add ("cherry");

    String result = sa.joinIntoString (", ", 1, 100);
    EXPECT_EQ (result, "banana, cherry");
}

TEST (StringArrayTests, AppendNumbersToDuplicatesWithoutFirstInstance)
{
    StringArray sa;
    sa.add ("file.txt");
    sa.add ("document.doc");
    sa.add ("file.txt");
    sa.add ("file.txt");
    sa.add ("document.doc");

    sa.appendNumbersToDuplicates (false, false, CharPointer_UTF8 (" ("), CharPointer_UTF8 (")"));

    EXPECT_EQ (sa.size(), 5);
    EXPECT_EQ (sa[0], "file.txt");
    EXPECT_EQ (sa[1], "document.doc");
    EXPECT_EQ (sa[2], "file.txt (2)");
    EXPECT_EQ (sa[3], "file.txt (3)");
    EXPECT_EQ (sa[4], "document.doc (2)");
}

TEST (StringArrayTests, AppendNumbersToDuplicatesWithFirstInstance)
{
    StringArray sa;
    sa.add ("file.txt");
    sa.add ("document.doc");
    sa.add ("file.txt");
    sa.add ("file.txt");

    sa.appendNumbersToDuplicates (false, true, CharPointer_UTF8 (" ("), CharPointer_UTF8 (")"));

    EXPECT_EQ (sa.size(), 4);
    EXPECT_EQ (sa[0], "file.txt (1)");
    EXPECT_EQ (sa[1], "document.doc");
    EXPECT_EQ (sa[2], "file.txt (2)");
    EXPECT_EQ (sa[3], "file.txt (3)");
}

TEST (StringArrayTests, AppendNumbersToDuplicatesIgnoreCase)
{
    StringArray sa;
    sa.add ("File.txt");
    sa.add ("file.TXT");
    sa.add ("FILE.txt");

    sa.appendNumbersToDuplicates (true, false, CharPointer_UTF8 (" ("), CharPointer_UTF8 (")"));

    EXPECT_EQ (sa.size(), 3);
    EXPECT_EQ (sa[0], "File.txt");
    EXPECT_EQ (sa[1], "file.TXT (2)");
    EXPECT_EQ (sa[2], "FILE.txt (3)");
}

TEST (StringArrayTests, AppendNumbersToDuplicatesWithDefaultSeparators)
{
    StringArray sa;
    sa.add ("test");
    sa.add ("test");
    sa.add ("test");

    sa.appendNumbersToDuplicates (false, false);

    EXPECT_EQ (sa.size(), 3);
    EXPECT_EQ (sa[0], "test");
    EXPECT_EQ (sa[1], "test (2)");
    EXPECT_EQ (sa[2], "test (3)");
}

TEST (StringArrayTests, BasicConstructorAndAdd)
{
    StringArray sa;
    sa.add ("first");
    sa.add ("second");

    EXPECT_EQ (sa.size(), 2);
    EXPECT_EQ (sa[0], "first");
    EXPECT_EQ (sa[1], "second");
}

TEST (StringArrayTests, CopyConstructor)
{
    StringArray sa1;
    sa1.add ("one");
    sa1.add ("two");

    StringArray sa2 (sa1);

    EXPECT_EQ (sa2.size(), 2);
    EXPECT_EQ (sa2[0], "one");
    EXPECT_EQ (sa2[1], "two");
}

TEST (StringArrayTests, Sort)
{
    StringArray sa;
    sa.add ("zebra");
    sa.add ("apple");
    sa.add ("Banana");

    sa.sort (false);
    EXPECT_EQ (sa[0], "Banana");
    EXPECT_EQ (sa[1], "apple");
    EXPECT_EQ (sa[2], "zebra");
}

TEST (StringArrayTests, SortIgnoreCase)
{
    StringArray sa;
    sa.add ("zebra");
    sa.add ("Apple");
    sa.add ("banana");

    sa.sort (true);
    EXPECT_EQ (sa[0], "Apple");
    EXPECT_EQ (sa[1], "banana");
    EXPECT_EQ (sa[2], "zebra");
}
