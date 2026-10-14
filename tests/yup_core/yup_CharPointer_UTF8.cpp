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

class CharPointerUTF8Tests : public ::testing::Test
{
protected:
    static constexpr yup_wchar replacement = 0xFFFD;
};

TEST_F (CharPointerUTF8Tests, DecodesWellFormedSequences)
{
    CharPointer_UTF8 p ("A\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80");

    EXPECT_EQ (p.getAndAdvance(), (yup_wchar) 'A');
    EXPECT_EQ (p.getAndAdvance(), (yup_wchar) 0xE9);
    EXPECT_EQ (p.getAndAdvance(), (yup_wchar) 0x20AC);
    EXPECT_EQ (p.getAndAdvance(), (yup_wchar) 0x1F600);
    EXPECT_TRUE (p.isEmpty());
}

TEST_F (CharPointerUTF8Tests, OverlongNullIsReplacementNotTerminator)
{
    CharPointer_UTF8 p ("\xC0\x80" "A");

    EXPECT_EQ (p.getAndAdvance(), replacement);
    EXPECT_EQ (p.getAndAdvance(), replacement);
    EXPECT_EQ (p.getAndAdvance(), (yup_wchar) 'A');
    EXPECT_TRUE (p.isEmpty());
}

TEST_F (CharPointerUTF8Tests, RejectsSurrogates)
{
    CharPointer_UTF8 p ("\xED\xA0\x80");

    EXPECT_EQ (p.getAndAdvance(), replacement);
    EXPECT_EQ (p.getAndAdvance(), replacement);
    EXPECT_EQ (p.getAndAdvance(), replacement);
    EXPECT_TRUE (p.isEmpty());
}

TEST_F (CharPointerUTF8Tests, RejectsOutOfRangeCodePoints)
{
    CharPointer_UTF8 p ("\xF4\x90\x80\x80");

    for (int i = 0; i < 4; ++i)
        EXPECT_EQ (p.getAndAdvance(), replacement);

    EXPECT_TRUE (p.isEmpty());
}

TEST_F (CharPointerUTF8Tests, TruncatedSequenceConsumesOnlyValidPrefix)
{
    CharPointer_UTF8 p ("\xE2\x82" "A");

    EXPECT_EQ (p.getAndAdvance(), replacement);
    EXPECT_EQ (p.getAndAdvance(), (yup_wchar) 'A');
    EXPECT_TRUE (p.isEmpty());
}

TEST_F (CharPointerUTF8Tests, StrayContinuationByteIsReplacement)
{
    CharPointer_UTF8 p ("\x80" "A");

    EXPECT_EQ (p.getAndAdvance(), replacement);
    EXPECT_EQ (p.getAndAdvance(), (yup_wchar) 'A');
}

TEST_F (CharPointerUTF8Tests, SkipOperatorsAgreeWithDecoder)
{
    CharPointer_UTF8 p ("\xC0\x80" "AB");

    p += 2;
    EXPECT_EQ (*p, (yup_wchar) 'A');
}

TEST_F (CharPointerUTF8Tests, MalformedStringCountsReplacementCharacters)
{
    const String str (CharPointer_UTF8 ("\xC0\x80" "ABCDEFGH"));

    EXPECT_EQ (str.length(), 10);
}

TEST_F (CharPointerUTF8Tests, DecodesBoundaryCodePoints)
{
    EXPECT_EQ (*CharPointer_UTF8 ("\xC2\x80"), (yup_wchar) 0x80);
    EXPECT_EQ (*CharPointer_UTF8 ("\xDF\xBF"), (yup_wchar) 0x7FF);
    EXPECT_EQ (*CharPointer_UTF8 ("\xE0\xA0\x80"), (yup_wchar) 0x800);
    EXPECT_EQ (*CharPointer_UTF8 ("\xED\x9F\xBF"), (yup_wchar) 0xD7FF);
    EXPECT_EQ (*CharPointer_UTF8 ("\xEE\x80\x80"), (yup_wchar) 0xE000);
    EXPECT_EQ (*CharPointer_UTF8 ("\xEF\xBF\xBF"), (yup_wchar) 0xFFFF);
    EXPECT_EQ (*CharPointer_UTF8 ("\xF0\x90\x80\x80"), (yup_wchar) 0x10000);
    EXPECT_EQ (*CharPointer_UTF8 ("\xF4\x8F\xBF\xBF"), (yup_wchar) 0x10FFFF);
}

TEST_F (CharPointerUTF8Tests, RejectsOverlongThreeByteForms)
{
    CharPointer_UTF8 p ("\xE0\x9F\xBF");

    EXPECT_EQ (p.getAndAdvance(), replacement);
    EXPECT_EQ (p.getAndAdvance(), replacement);
    EXPECT_EQ (p.getAndAdvance(), replacement);
    EXPECT_TRUE (p.isEmpty());
}

TEST_F (CharPointerUTF8Tests, RejectsInvalidLeadBytes)
{
    CharPointer_UTF8 p ("\xC0\xC1\xF5\xFE\xFF");

    for (int i = 0; i < 5; ++i)
        EXPECT_EQ (p.getAndAdvance(), replacement);

    EXPECT_TRUE (p.isEmpty());
}

TEST_F (CharPointerUTF8Tests, TruncatedSequencesAtTerminatorAreSingleReplacement)
{
    CharPointer_UTF8 four ("\xF0\x9F\x98");
    EXPECT_EQ (four.getAndAdvance(), replacement);
    EXPECT_TRUE (four.isEmpty());

    CharPointer_UTF8 two ("\xC2");
    EXPECT_EQ (two.getAndAdvance(), replacement);
    EXPECT_TRUE (two.isEmpty());
}

TEST_F (CharPointerUTF8Tests, TerminatorIsNeverConsumed)
{
    CharPointer_UTF8 p ("");

    EXPECT_EQ (p.getAndAdvance(), (yup_wchar) 0);
    EXPECT_EQ (p.getAndAdvance(), (yup_wchar) 0);
    EXPECT_TRUE (p.isEmpty());
    EXPECT_EQ (*p, (yup_wchar) 0);
}

TEST_F (CharPointerUTF8Tests, LengthCountsDecodedCharacters)
{
    EXPECT_EQ (CharPointer_UTF8 ("").length(), (size_t) 0);
    EXPECT_EQ (CharPointer_UTF8 ("\xF0\x9F\x98\x80").length(), (size_t) 1);
    EXPECT_EQ (CharPointer_UTF8 ("\xFF\xFF\xFF").length(), (size_t) 3);
    EXPECT_EQ (CharPointer_UTF8 ("\xE2\x82\xAC\xE2\x82").length(), (size_t) 2);
}

TEST_F (CharPointerUTF8Tests, LongMalformedInputStaysLinear)
{
    const std::string input (4096, '\xFF');
    CharPointer_UTF8 p (input.c_str());

    EXPECT_EQ (p.length(), (size_t) 4096);

    const String str (CharPointer_UTF8 (input.c_str()));
    EXPECT_EQ (str.length(), 4096);
}

TEST_F (CharPointerUTF8Tests, ValidatorAcceptsWellFormedData)
{
    EXPECT_TRUE (CharPointer_UTF8::isValidString ("", 0));
    EXPECT_TRUE (CharPointer_UTF8::isValidString ("abc", 3));
    EXPECT_TRUE (CharPointer_UTF8::isValidString ("\xC2\x80\xDF\xBF\xE0\xA0\x80\xED\x9F\xBF\xEE\x80\x80\xEF\xBF\xBD", 16));
    EXPECT_TRUE (CharPointer_UTF8::isValidString ("\xF0\x90\x80\x80\xF4\x8F\xBF\xBF", 8));
}

TEST_F (CharPointerUTF8Tests, ValidatorRejectsOverlongAndSurrogates)
{
    EXPECT_FALSE (CharPointer_UTF8::isValidString ("\xC0\x80", 2));
    EXPECT_FALSE (CharPointer_UTF8::isValidString ("\xE0\x80\x80", 3));
    EXPECT_FALSE (CharPointer_UTF8::isValidString ("\xE0\x9F\xBF", 3));
    EXPECT_FALSE (CharPointer_UTF8::isValidString ("\xF0\x80\x80\x80", 4));
    EXPECT_FALSE (CharPointer_UTF8::isValidString ("\xED\xA0\x80", 3));
}

TEST_F (CharPointerUTF8Tests, ValidatorRejectsOutOfRangeAndStrayBytes)
{
    EXPECT_FALSE (CharPointer_UTF8::isValidString ("\xF4\x90\x80\x80", 4));
    EXPECT_FALSE (CharPointer_UTF8::isValidString ("\xF5\x80\x80\x80", 4));
    EXPECT_FALSE (CharPointer_UTF8::isValidString ("\xFF", 1));
    EXPECT_FALSE (CharPointer_UTF8::isValidString ("\x80", 1));
    EXPECT_FALSE (CharPointer_UTF8::isValidString ("A\xC3", 2));
}

TEST_F (CharPointerUTF8Tests, ValidatorRespectsLengthBound)
{
    EXPECT_TRUE (CharPointer_UTF8::isValidString ("abc", 2));
    EXPECT_FALSE (CharPointer_UTF8::isValidString ("\xC3\xA9", 1));
    EXPECT_FALSE (CharPointer_UTF8::isValidString ("\xE2\x82\xAC", 2));
    EXPECT_TRUE (CharPointer_UTF8::isValidString ("\xE2\x82\xAC", 3));
    EXPECT_TRUE (CharPointer_UTF8::isValidString ("\xFF", 0));
}

TEST_F (CharPointerUTF8Tests, ValidatorStopsAtTerminator)
{
    EXPECT_TRUE (CharPointer_UTF8::isValidString ("ok\0\xFF", 16));
}

TEST_F (CharPointerUTF8Tests, DecrementFindsPreviousCharacterOnInvalidBytes)
{
    CharPointer_UTF8 valid ("A\xC3\xA9");
    valid.getAndAdvance();
    valid.getAndAdvance();
    --valid;
    EXPECT_EQ (*valid, (yup_wchar) 0xE9);

    CharPointer_UTF8 stray ("A\x80" "B");
    stray.getAndAdvance();
    stray.getAndAdvance();
    --stray;
    EXPECT_EQ (*stray, replacement);
    EXPECT_EQ (stray.getAndAdvance(), replacement);
    EXPECT_EQ (stray.getAndAdvance(), (yup_wchar) 'B');
}

TEST_F (CharPointerUTF8Tests, BoundedDecodeNeverReadsPastBound)
{
    CharPointer_UTF8 p ("\xE2\x82\xAC");

    EXPECT_EQ (p.getAndAdvance (2), replacement);
    EXPECT_EQ (*p, replacement);
    EXPECT_EQ (p.getAndAdvance (1), replacement);
}

TEST_F (CharPointerUTF8Tests, CopiedMalformedStringGrowsSafely)
{
    const String original (CharPointer_UTF8 ("\xC0\x80"));
    String copy = original;

    copy += "z";
    copy += String (CharPointer_UTF8 ("\xF4\x90\x80\x80"));

    EXPECT_EQ (original.length(), 2);
    EXPECT_EQ (copy.length(), 2 + 1 + 4);
}
