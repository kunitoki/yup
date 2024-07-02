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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <gtest/gtest.h>

#include <juce_core/juce_core.h>

#include <random>

using namespace juce;

class TextDiffTests : public ::testing::Test
{
protected:
    String createString (Random& r)
    {
        juce_wchar buffer[500] = { 0 };

        for (int i = r.nextInt (numElementsInArray (buffer) - 1); --i >= 0;)
        {
            if (r.nextInt (10) == 0)
            {
                do
                {
                    buffer[i] = (juce_wchar) (1 + r.nextInt (0x10ffff - 1));
                } while (! CharPointer_UTF16::canRepresent (buffer[i]));
            }
            else
                buffer[i] = (juce_wchar) ('a' + r.nextInt (3));
        }

        return CharPointer_UTF32 (buffer);
    }

    auto& getRandom()
    {
        auto& random = Random::getSystemRandom();
        random.setSeedRandomly();
        return random;
    }
};

TEST_F (TextDiffTests, AppliesSingleInsertionCorrectly)
{
    String original = "hello";
    String target = "hello world";
    TextDiff diff (original, target);

    EXPECT_EQ (diff.appliedTo (original), target);
}

TEST_F (TextDiffTests, AppliesSingleDeletionCorrectly)
{
    String original = "hello world";
    String target = "hello";
    TextDiff diff (original, target);

    EXPECT_EQ (diff.appliedTo (original), target);
}

TEST_F (TextDiffTests, AppliesMultipleChangesCorrectly)
{
    String original = "hello world";
    String target = "hi universe";
    TextDiff diff (original, target);

    EXPECT_EQ (diff.appliedTo (original), target);
}

TEST_F (TextDiffTests, NoChangeWhenOriginalAndTargetAreSame)
{
    String original = "hello";
    String target = "hello";
    TextDiff diff (original, target);

    EXPECT_EQ (diff.appliedTo (original), target);
    EXPECT_TRUE (diff.changes.isEmpty());
}

TEST_F (TextDiffTests, ChangeDetectionInsertAndDelete)
{
    String original = "hello world";
    String target = "hi world";
    TextDiff diff (original, target);

    // Expecting two changes: one deletion and one insertion
    EXPECT_EQ (diff.changes.size(), 2);
    EXPECT_TRUE (diff.changes[0].isDeletion());
    EXPECT_FALSE (diff.changes[1].isDeletion());
    EXPECT_EQ (diff.appliedTo (original), target);
}

TEST_F (TextDiffTests, HandlesEmptyStrings)
{
    String original = "";
    String target = "hello";
    TextDiff diff (original, target);

    EXPECT_EQ (diff.appliedTo (original), target);
}

TEST_F (TextDiffTests, HandlesMoreEmptyStrings)
{
    TextDiff diff { String(), String() };
    EXPECT_EQ (diff.appliedTo (String()), String());
}

TEST_F (TextDiffTests, AppliesChangesToCorrectPosition)
{
    String original = "12345";
    String target = "12abc345";
    TextDiff diff (original, target);

    String result = diff.appliedTo (original);
    EXPECT_EQ (result, target);
}

TEST_F (TextDiffTests, HandlesComplexChanges)
{
    String original = "The quick brown fox";
    String target = "A quick red fox jumps";
    TextDiff diff (original, target);

    EXPECT_EQ (diff.appliedTo (original), target);
}

TEST_F (TextDiffTests, SingleCharacterChanges)
{
    TextDiff diff1 ("x", String());
    EXPECT_EQ (diff1.appliedTo ("x"), String());

    TextDiff diff2 (String(), "x");
    EXPECT_EQ (diff2.appliedTo (String()), "x");

    TextDiff diff3 ("x", "x");
    EXPECT_EQ (diff3.appliedTo ("x"), "x");

    TextDiff diff4 ("x", "y");
    EXPECT_EQ (diff4.appliedTo ("x"), "y");

    TextDiff diff5 ("xxx", "x");
    EXPECT_EQ (diff5.appliedTo ("xxx"), "x");

    TextDiff diff6 ("x", "xxx");
    EXPECT_EQ (diff6.appliedTo ("x"), "xxx");
}

TEST_F (TextDiffTests, RandomStringDiffs)
{
    auto& r = getRandom();

    for (int i = 0; i < 500; ++i)
    {
        auto s = createString (r);
        auto t1 = createString (r);
        auto t2 = createString (r);

        TextDiff diff1 (s, t1);
        EXPECT_EQ (diff1.appliedTo (s), t1);

        TextDiff diff2 (s + t1, s + t2);
        EXPECT_EQ (diff2.appliedTo (s + t1), s + t2);
    }
}
