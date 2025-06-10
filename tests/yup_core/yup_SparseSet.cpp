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

#include <yup_core/yup_core.h>

using namespace yup;

TEST (SparseSetTests, BasicOperations)
{
    SparseSet<int> set;

    EXPECT_TRUE (set.isEmpty());
    EXPECT_EQ (set.size(), 0);
    EXPECT_EQ (set.getNumRanges(), 0);
    EXPECT_TRUE (set.getTotalRange().isEmpty());

    set.addRange ({ 0, 10 });
    EXPECT_TRUE (! set.isEmpty());
    EXPECT_EQ (set.size(), 10);
    EXPECT_EQ (set.getNumRanges(), 1);
    EXPECT_TRUE (! set.getTotalRange().isEmpty());
    EXPECT_TRUE (set.getRange (0) == Range<int> (0, 10));

    EXPECT_EQ (set[0], 0);
    EXPECT_EQ (set[5], 5);
    EXPECT_EQ (set[9], 9);
    // Index out of range yields a default value for a type
    EXPECT_EQ (set[10], 0);
    EXPECT_TRUE (set.contains (0));
    EXPECT_TRUE (set.contains (9));
    EXPECT_TRUE (! set.contains (10));
}

TEST (SparseSetTests, AddingRanges)
{
    SparseSet<int> set;

    // Adding same range twice should yield just a single range
    set.addRange ({ 0, 10 });
    set.addRange ({ 0, 10 });
    EXPECT_EQ (set.getNumRanges(), 1);
    EXPECT_TRUE (set.getRange (0) == Range<int> (0, 10));

    // Adding already included range does not increase num ranges
    set.addRange ({ 0, 2 });
    EXPECT_EQ (set.getNumRanges(), 1);
    set.addRange ({ 8, 10 });
    EXPECT_EQ (set.getNumRanges(), 1);
    set.addRange ({ 2, 5 });
    EXPECT_EQ (set.getNumRanges(), 1);

    // Adding non adjacent range includes total number of ranges
    set.addRange ({ -10, -5 });
    EXPECT_EQ (set.getNumRanges(), 2);
    EXPECT_TRUE (set.getRange (0) == Range<int> (-10, -5));
    EXPECT_TRUE (set.getRange (1) == Range<int> (0, 10));
    EXPECT_TRUE (set.getTotalRange() == Range<int> (-10, 10));

    set.addRange ({ 15, 20 });
    EXPECT_EQ (set.getNumRanges(), 3);
    EXPECT_TRUE (set.getRange (0) == Range<int> (-10, -5));
    EXPECT_TRUE (set.getRange (1) == Range<int> (0, 10));
    EXPECT_TRUE (set.getRange (2) == Range<int> (15, 20));
    EXPECT_TRUE (set.getTotalRange() == Range<int> (-10, 20));

    // Adding adjacent ranges merges them.
    set.addRange ({ -5, -3 });
    EXPECT_EQ (set.getNumRanges(), 3);
    EXPECT_TRUE (set.getRange (0) == Range<int> (-10, -3));
    EXPECT_TRUE (set.getRange (1) == Range<int> (0, 10));
    EXPECT_TRUE (set.getRange (2) == Range<int> (15, 20));
    EXPECT_TRUE (set.getTotalRange() == Range<int> (-10, 20));

    set.addRange ({ 20, 25 });
    EXPECT_EQ (set.getNumRanges(), 3);
    EXPECT_TRUE (set.getRange (0) == Range<int> (-10, -3));
    EXPECT_TRUE (set.getRange (1) == Range<int> (0, 10));
    EXPECT_TRUE (set.getRange (2) == Range<int> (15, 25));
    EXPECT_TRUE (set.getTotalRange() == Range<int> (-10, 25));

    // Adding range containing other ranges merges them
    set.addRange ({ -50, 50 });
    EXPECT_EQ (set.getNumRanges(), 1);
    EXPECT_TRUE (set.getRange (0) == Range<int> (-50, 50));
    EXPECT_TRUE (set.getTotalRange() == Range<int> (-50, 50));
}

TEST (SparseSetTests, RemovingRanges)
{
    SparseSet<int> set;

    set.addRange ({ -20, -10 });
    set.addRange ({ 0, 10 });
    set.addRange ({ 20, 30 });
    EXPECT_EQ (set.getNumRanges(), 3);

    // Removing ranges not included in the set has no effect
    set.removeRange ({ -5, 5 });
    EXPECT_EQ (set.getNumRanges(), 3);

    // Removing partially overlapping range
    set.removeRange ({ -15, 5 });
    EXPECT_EQ (set.getNumRanges(), 3);
    EXPECT_TRUE (set.getRange (0) == Range<int> (-20, -15));
    EXPECT_TRUE (set.getRange (1) == Range<int> (5, 10));
    EXPECT_TRUE (set.getRange (2) == Range<int> (20, 30));

    // Removing subrange of existing range
    set.removeRange ({ 20, 22 });
    EXPECT_EQ (set.getNumRanges(), 3);
    EXPECT_TRUE (set.getRange (2) == Range<int> (22, 30));

    set.removeRange ({ 28, 30 });
    EXPECT_EQ (set.getNumRanges(), 3);
    EXPECT_TRUE (set.getRange (2) == Range<int> (22, 28));

    set.removeRange ({ 24, 26 });
    EXPECT_EQ (set.getNumRanges(), 4);
    EXPECT_TRUE (set.getRange (0) == Range<int> (-20, -15));
    EXPECT_TRUE (set.getRange (1) == Range<int> (5, 10));
    EXPECT_TRUE (set.getRange (2) == Range<int> (22, 24));
    EXPECT_TRUE (set.getRange (3) == Range<int> (26, 28));
}

TEST (SparseSetTests, XORingRanges)
{
    SparseSet<int> set;
    set.addRange ({ 0, 10 });

    set.invertRange ({ 0, 10 });
    EXPECT_EQ (set.getNumRanges(), 0);
    set.invertRange ({ 0, 10 });
    EXPECT_EQ (set.getNumRanges(), 1);

    set.invertRange ({ 4, 6 });
    EXPECT_EQ (set.getNumRanges(), 2);
    EXPECT_TRUE (set.getRange (0) == Range<int> (0, 4));
    EXPECT_TRUE (set.getRange (1) == Range<int> (6, 10));

    set.invertRange ({ -2, 2 });
    EXPECT_EQ (set.getNumRanges(), 3);
    EXPECT_TRUE (set.getRange (0) == Range<int> (-2, 0));
    EXPECT_TRUE (set.getRange (1) == Range<int> (2, 4));
    EXPECT_TRUE (set.getRange (2) == Range<int> (6, 10));
}

TEST (SparseSetTests, RangeContainsAndOverlapsChecks)
{
    SparseSet<int> set;
    set.addRange ({ 0, 10 });

    EXPECT_TRUE (set.containsRange (Range<int> (0, 2)));
    EXPECT_TRUE (set.containsRange (Range<int> (8, 10)));
    EXPECT_TRUE (set.containsRange (Range<int> (0, 10)));

    EXPECT_TRUE (! set.containsRange (Range<int> (-2, 0)));
    EXPECT_TRUE (! set.containsRange (Range<int> (-2, 10)));
    EXPECT_TRUE (! set.containsRange (Range<int> (10, 12)));
    EXPECT_TRUE (! set.containsRange (Range<int> (0, 12)));

    EXPECT_TRUE (set.overlapsRange (Range<int> (0, 2)));
    EXPECT_TRUE (set.overlapsRange (Range<int> (8, 10)));
    EXPECT_TRUE (set.overlapsRange (Range<int> (0, 10)));

    EXPECT_TRUE (! set.overlapsRange (Range<int> (-2, 0)));
    EXPECT_TRUE (set.overlapsRange (Range<int> (-2, 10)));
    EXPECT_TRUE (! set.overlapsRange (Range<int> (10, 12)));
    EXPECT_TRUE (set.overlapsRange (Range<int> (0, 12)));
}
