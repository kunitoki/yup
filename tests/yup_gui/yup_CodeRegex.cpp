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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

class CodeRegexTests : public ::testing::Test
{
protected:
    static CodeRegex compile (StringRef pattern, bool caseSensitive = true)
    {
        auto regex = CodeRegex::compile (pattern, caseSensitive);
        EXPECT_TRUE (regex.wasOk()) << regex.getErrorMessage();
        return regex.valueOr (CodeRegex());
    }
};

TEST_F (CodeRegexTests, InvalidPatternFailsWithMessage)
{
    auto regex = CodeRegex::compile ("(unclosed");

    EXPECT_TRUE (regex.failed());
    EXPECT_TRUE (regex.getErrorMessage().contains ("(unclosed"));
}

TEST_F (CodeRegexTests, DefaultConstructedNeverMatches)
{
    CodeRegex regex;
    CodeRegex::Subject subject ("text");

    EXPECT_FALSE (regex.isValid());
    EXPECT_FALSE (regex.matchAt (subject, 0).has_value());
    EXPECT_FALSE (regex.findNext (subject, 0).has_value());
}

TEST_F (CodeRegexTests, MatchAtIsAnchoredAtPosition)
{
    const auto regex = compile ("[a-z]+");
    CodeRegex::Subject subject ("12 abc");

    EXPECT_FALSE (regex.matchAt (subject, 0).has_value());

    const auto match = regex.matchAt (subject, 3);
    ASSERT_TRUE (match.has_value());
    EXPECT_EQ (Range<int> (3, 6), match->groups[0]);
    EXPECT_EQ (String ("abc"), match->texts[0]);
}

TEST_F (CodeRegexTests, FindNextSearchesForward)
{
    const auto regex = compile ("\\d+");
    CodeRegex::Subject subject ("ab 12 cd 345");

    const auto first = regex.findNext (subject, 0);
    ASSERT_TRUE (first.has_value());
    EXPECT_EQ (Range<int> (3, 5), first->groups[0]);

    const auto second = regex.findNext (subject, first->groups[0].getEnd());
    ASSERT_TRUE (second.has_value());
    EXPECT_EQ (Range<int> (9, 12), second->groups[0]);

    EXPECT_FALSE (regex.findNext (subject, 12).has_value());
}

TEST_F (CodeRegexTests, ReportsCaptureGroupsAndUnmatchedGroups)
{
    const auto regex = compile ("(\\w+)(?:(:)|(=))");
    CodeRegex::Subject subject ("key=value");

    const auto match = regex.matchAt (subject, 0);
    ASSERT_TRUE (match.has_value());
    ASSERT_EQ (4u, match->groups.size());
    EXPECT_EQ (3, regex.getNumGroups());
    EXPECT_EQ (Range<int> (0, 3), match->groups[1]);
    EXPECT_LT (match->groups[2].getStart(), 0);
    EXPECT_EQ (Range<int> (3, 4), match->groups[3]);
    EXPECT_EQ (String ("key"), match->texts[1]);
}

TEST_F (CodeRegexTests, PositionsAreCharacterIndicesWithNonAsciiText)
{
    const auto regex = compile ("[a-z]+");
    CodeRegex::Subject subject (String::fromUTF8 ("\xc3\xa8\xe2\x82\xac abc"));

    EXPECT_EQ (6, subject.getLength());

    const auto match = regex.findNext (subject, 0);
    ASSERT_TRUE (match.has_value());
    EXPECT_EQ (Range<int> (3, 6), match->groups[0]);
    EXPECT_EQ (String ("abc"), match->texts[0]);
}

TEST_F (CodeRegexTests, LineStartAndWordBoundaryRespectPrecedingText)
{
    const auto lineStart = compile ("^b");
    const auto wordStart = compile ("\\bb");
    CodeRegex::Subject subject ("ab");

    EXPECT_FALSE (lineStart.matchAt (subject, 1).has_value());
    EXPECT_FALSE (wordStart.matchAt (subject, 1).has_value());
}

TEST_F (CodeRegexTests, CaretInsideCharacterClassStillMatchesPastLineStart)
{
    const auto regex = compile ("[^a]|\\^");
    CodeRegex::Subject subject ("ab^");

    EXPECT_TRUE (regex.matchAt (subject, 1).has_value());
    EXPECT_TRUE (regex.matchAt (subject, 2).has_value());
}

TEST_F (CodeRegexTests, CaseInsensitiveMatching)
{
    const auto regex = compile ("select", false);
    CodeRegex::Subject subject ("SELECT");

    EXPECT_TRUE (regex.matchAt (subject, 0).has_value());
}

TEST_F (CodeRegexTests, OutOfRangePositionDoesNotMatch)
{
    const auto regex = compile ("a*");
    CodeRegex::Subject subject ("aa");

    EXPECT_FALSE (regex.matchAt (subject, -1).has_value());
    EXPECT_FALSE (regex.matchAt (subject, 3).has_value());
    EXPECT_TRUE (regex.matchAt (subject, 2).has_value());
}

TEST_F (CodeRegexTests, EscapeMatchesTextLiterally)
{
    const String literal ("a+b(c)[d]*.$^|?{1}\\");
    const auto regex = compile (CodeRegex::escape (literal));
    CodeRegex::Subject subject (literal);

    const auto match = regex.matchAt (subject, 0);
    ASSERT_TRUE (match.has_value());
    EXPECT_EQ (literal.length(), match->groups[0].getEnd());
}
