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

#include <vector>
#include <algorithm>

using namespace yup;

struct IntComparator
{
    int compareElements (int first, int second) const
    {
        return (first < second) ? -1 : ((second < first) ? 1 : 0);
    }
};

TEST (SortFunctionConverterTests, SortFunctionConverterWorks)
{
    std::vector<int> vec { 5, 2, 9, 1, 5, 6 };

    IntComparator comparator;
    SortFunctionConverter<IntComparator> converter (comparator);

    std::sort (vec.begin(), vec.end(), converter);

    std::vector<int> expected { 1, 2, 5, 5, 6, 9 };
    EXPECT_EQ (vec, expected);
}

TEST (SortArrayTest, SortsCorrectly)
{
    int array[] = { 5, 2, 9, 1, 5, 6 };
    IntComparator comparator;

    sortArray (comparator, array, 0, 5, false);

    int expected[] = { 1, 2, 5, 5, 6, 9 };
    EXPECT_TRUE (std::equal (std::begin (array), std::end (array), std::begin (expected)));
}

TEST (SortArrayTest, SortsCorrectlyWithOrderRetained)
{
    int array[] = { 5, 2, 9, 1, 5, 6 };
    IntComparator comparator;

    sortArray (comparator, array, 0, 5, true);

    int expected[] = { 1, 2, 5, 5, 6, 9 };
    EXPECT_TRUE (std::equal (std::begin (array), std::end (array), std::begin (expected)));
}

TEST (FindInsertIndexInSortedArrayTest, FindsCorrectIndex)
{
    int array[] = { 1, 2, 4, 5, 6 };
    IntComparator comparator;

    int index = findInsertIndexInSortedArray (comparator, array, 3, 0, 5);
    EXPECT_EQ (index, 2);
}

TEST (DefaultElementComparatorTest, ComparesCorrectly)
{
    DefaultElementComparator<int> comparator;
    EXPECT_EQ (comparator.compareElements (1, 2), -1);
    EXPECT_EQ (comparator.compareElements (2, 1), 1);
    EXPECT_EQ (comparator.compareElements (1, 1), 0);
}
