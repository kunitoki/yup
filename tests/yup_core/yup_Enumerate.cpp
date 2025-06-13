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

TEST (EnumerateTests, Works_With_Bidirectional_Iterators)
{
    const std::list<int> elements { 10, 20, 30, 40, 50 };
    std::vector<int> counts;

    for (const auto pair : yup::enumerate (elements))
        counts.push_back ((int) pair.index);

    EXPECT_EQ (counts, (std::vector<int> { 0, 1, 2, 3, 4 }));
}

TEST (EnumerateTests, Works_With_Random_Access_Iterators)
{
    const std::vector<std::string> strings { "a", "bb", "ccc", "dddd", "eeeee" };

    std::vector<int> counts;

    for (const auto [count, element] : yup::enumerate (strings))
        counts.push_back ((int) ((size_t) count + element.size()));

    EXPECT_EQ (counts, (std::vector<int> { 1, 3, 5, 7, 9 }));
}

TEST (EnumerateTests, Works_With_Mutable_Ranges)
{
    std::vector<std::string> strings { "", "", "", "", "" };

    for (const auto [count, element] : yup::enumerate (strings))
        element = std::to_string (count);

    EXPECT_EQ (strings, (std::vector<std::string> { "0", "1", "2", "3", "4" }));
}

TEST (EnumerateTests, Iterator_Can_Be_Incremented_By_More_Than_One)
{
    std::vector<int> ints (6);

    const auto enumerated = yup::enumerate (ints);

    std::vector<int> counts;

    for (auto b = enumerated.begin(), e = enumerated.end(); b != e; b += 2)
        counts.push_back ((int) (*b).index);

    EXPECT_EQ (counts, (std::vector<int> { 0, 2, 4 }));
}

TEST (EnumerateTests, Iterator_Can_Be_Started_At_Non_Zero_Value)
{
    const std::vector<int> ints (6);

    std::vector<int> counts;

    for (const auto enumerated : yup::enumerate (ints, 5))
        counts.push_back ((int) enumerated.index);

    EXPECT_EQ (counts, (std::vector<int> { 5, 6, 7, 8, 9, 10 }));
}

TEST (EnumerateTests, Subtracting_Two_Iterators_Returns_The_Difference_Between_The_Base_Iterators)
{
    const std::vector<int> ints (6);
    const auto enumerated = yup::enumerate (ints);
    EXPECT_EQ ((int) (enumerated.end() - enumerated.begin()), (int) ints.size());
}

TEST (EnumerateTests, EnumerateIterator_Can_Be_Decremented)
{
    const std::vector<int> ints (5);
    std::vector<int> counts;

    const auto enumerated = yup::enumerate (std::as_const (ints));

    for (auto i = enumerated.end(), b = enumerated.begin(); i != b; --i)
        counts.push_back ((int) (*(i - 1)).index);

    EXPECT_EQ (counts, (std::vector<int> { -1, -2, -3, -4, -5 }));
}

TEST (EnumerateTests, EnumerateIterator_Can_Be_Compared)
{
    const std::vector<int> ints (6);
    const auto enumerated = yup::enumerate (ints);

    EXPECT_LT (enumerated.begin(), enumerated.end());
    EXPECT_LE (enumerated.begin(), enumerated.end());
    EXPECT_GT (enumerated.end(), enumerated.begin());
    EXPECT_GE (enumerated.end(), enumerated.begin());
}
