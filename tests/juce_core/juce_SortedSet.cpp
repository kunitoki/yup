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

#include <juce_core/juce_core.h>

using namespace juce;

TEST (SortedSetTests, DefaultConstructor)
{
    SortedSet<int> set;
    EXPECT_TRUE (set.isEmpty());
}

TEST (SortedSetTests, CopyConstructor)
{
    SortedSet<int> set1;
    set1.add (1);
    set1.add (2);
    SortedSet<int> set2 (set1);
    EXPECT_EQ (set1, set2);
}

TEST (SortedSetTests, MoveConstructor)
{
    SortedSet<int> set1;
    set1.add (1);
    set1.add (2);
    SortedSet<int> set2 (std::move (set1));
    EXPECT_EQ (set2.size(), 2);
    EXPECT_TRUE (set1.isEmpty());
}

TEST (SortedSetTests, CopyAssignment)
{
    SortedSet<int> set1;
    set1.add (1);
    set1.add (2);
    SortedSet<int> set2;
    set2 = set1;
    EXPECT_EQ (set1, set2);
}

TEST (SortedSetTests, MoveAssignment)
{
    SortedSet<int> set1;
    set1.add (1);
    set1.add (2);
    SortedSet<int> set2;
    set2 = std::move (set1);
    EXPECT_EQ (set2.size(), 2);
    EXPECT_TRUE (set1.isEmpty());
}

TEST (SortedSetTests, AddElement)
{
    SortedSet<int> set;
    EXPECT_TRUE (set.add (1));
    EXPECT_FALSE (set.add (1)); // Duplicate element
    EXPECT_TRUE (set.add (2));
    EXPECT_EQ (set.size(), 2);
}

TEST (SortedSetTests, AddArray)
{
    SortedSet<int> set;
    int elements[] = { 1, 2, 3, 3, 4 };
    set.addArray (elements, 5);
    EXPECT_EQ (set.size(), 4); // 3 is duplicated
}

TEST (SortedSetTests, AddSet)
{
    SortedSet<int> set1;
    set1.add (1);
    set1.add (2);

    SortedSet<int> set2;
    set2.addSet (set1);
    EXPECT_EQ (set2.size(), 2);

    set2.add (3);
    set2.addSet (set1);
    EXPECT_EQ (set2.size(), 3); // No duplicates added
}

TEST (SortedSetTests, RemoveElement)
{
    SortedSet<int> set;
    set.add (1);
    set.add (2);
    EXPECT_EQ (set.remove (1), 2);
    EXPECT_EQ (set.size(), 1);
    EXPECT_EQ (set.remove (0), 1);
    EXPECT_TRUE (set.isEmpty());
}

TEST (SortedSetTests, RemoveValue)
{
    SortedSet<int> set;
    set.add (1);
    set.add (2);
    set.removeValue (1);
    EXPECT_EQ (set.size(), 1);
    set.removeValue (3); // Value not in set
    EXPECT_EQ (set.size(), 1);
}

TEST (SortedSetTests, RemoveValuesIn)
{
    SortedSet<int> set1;
    set1.add (1);
    set1.add (2);
    set1.add (3);

    SortedSet<int> set2;
    set2.add (2);
    set2.add (4);

    set1.removeValuesIn (set2);
    EXPECT_EQ (set1.size(), 2);
    EXPECT_FALSE (set1.contains (2));
}

TEST (SortedSetTests, RemoveValuesNotIn)
{
    SortedSet<int> set1;
    set1.add (1);
    set1.add (2);
    set1.add (3);

    SortedSet<int> set2;
    set2.add (2);
    set2.add (4);

    set1.removeValuesNotIn (set2);
    EXPECT_EQ (set1.size(), 1);
    EXPECT_TRUE (set1.contains (2));
}

TEST (SortedSetTests, Clear)
{
    SortedSet<int> set;
    set.add (1);
    set.add (2);
    set.clear();
    EXPECT_TRUE (set.isEmpty());
}

TEST (SortedSetTests, ClearQuick)
{
    SortedSet<int> set;
    set.add (1);
    set.add (2);
    set.clearQuick();
    EXPECT_TRUE (set.isEmpty());
}

TEST (SortedSetTests, IndexOf)
{
    SortedSet<int> set;
    set.add (1);
    set.add (2);
    EXPECT_EQ (set.indexOf (1), 0);
    EXPECT_EQ (set.indexOf (2), 1);
    EXPECT_EQ (set.indexOf (3), -1);
}

TEST (SortedSetTests, Contains)
{
    SortedSet<int> set;
    set.add (1);
    set.add (2);
    EXPECT_TRUE (set.contains (1));
    EXPECT_FALSE (set.contains (3));
}

TEST (SortedSetTests, SwapWith)
{
    SortedSet<int> set1;
    set1.add (1);
    set1.add (2);

    SortedSet<int> set2;
    set2.add (3);

    set1.swapWith (set2);
    EXPECT_EQ (set1.size(), 1);
    EXPECT_EQ (set2.size(), 2);
    EXPECT_TRUE (set1.contains (3));
    EXPECT_TRUE (set2.contains (1));
    EXPECT_TRUE (set2.contains (2));
}

TEST (SortedSetTests, MinimiseStorageOverheads)
{
    SortedSet<int> set;
    set.add (1);
    set.add (2);
    set.minimiseStorageOverheads();
    EXPECT_EQ (set.size(), 2); // Ensure function does not affect elements
}

TEST (SortedSetTests, EnsureStorageAllocated)
{
    SortedSet<int> set;
    set.ensureStorageAllocated (100);
    EXPECT_TRUE (set.isEmpty()); // Ensure function does not add elements
}

TEST (SortedSetTests, Accessors)
{
    SortedSet<int> set;
    set.add (1);
    set.add (2);
    set.add (3);

    EXPECT_EQ (set[0], 1);
    EXPECT_EQ (set[1], 2);
    EXPECT_EQ (set[2], 3);
    EXPECT_EQ (set.getUnchecked (1), 2);
    EXPECT_EQ (set.getReference (1), 2);
    EXPECT_EQ (set.getFirst(), 1);
    EXPECT_EQ (set.getLast(), 3);
}

TEST (SortedSetTests, Iterators)
{
    SortedSet<int> set;
    set.add (1);
    set.add (2);
    set.add (3);

    auto it = set.begin();
    EXPECT_EQ (*it, 1);
    ++it;
    EXPECT_EQ (*it, 2);
    ++it;
    EXPECT_EQ (*it, 3);
    ++it;
    EXPECT_EQ (it, set.end());
}

TEST (SortedSetTests, DuplicateElementHandling)
{
    SortedSet<int> set;
    EXPECT_TRUE (set.add (1));
    EXPECT_TRUE (set.add (2));

    // Attempt to add a duplicate element
    EXPECT_FALSE (set.add (1));

    // Check the set size is correct
    EXPECT_EQ (set.size(), 2);

    // Check the elements in the set
    EXPECT_EQ (set[0], 1);
    EXPECT_EQ (set[1], 2);

    // Attempt to remove an element
    set.removeValue (1);

    // Check the set size is correct after removal
    EXPECT_EQ (set.size(), 1);

    // Check the remaining element in the set
    EXPECT_EQ (set[0], 2);

    // Attempt to remove a non-existent element
    set.removeValue (3);

    // Check the set size remains unchanged
    EXPECT_EQ (set.size(), 1);

    // Add another element
    EXPECT_TRUE (set.add (3));

    // Check the elements in the set
    EXPECT_EQ (set[0], 2);
    EXPECT_EQ (set[1], 3);

    // Attempt to remove the element that is not in order
    set.removeValue (2);

    // Check the elements in the set
    EXPECT_EQ (set.size(), 1);
    EXPECT_EQ (set[0], 3);

    // Attempt to add an element that is already in the set
    EXPECT_FALSE (set.add (3));

    // Check the set size remains unchanged
    EXPECT_EQ (set.size(), 1);
}

TEST (SortedSetTests, InconsistentStateHandling)
{
    SortedSet<int> set;

    // Add a series of elements
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_TRUE (set.add (i));
    }

    // Verify the size
    EXPECT_EQ (set.size(), 100);

    // Remove every second element
    for (int i = 0; i < 100; i += 2)
    {
        set.removeValue (i);
    }

    // Verify the size after removal
    EXPECT_EQ (set.size(), 50);

    // Add elements back in reverse order
    for (int i = 98; i >= 0; i -= 2)
    {
        set.add (i);
    }

    // Verify the size after adding elements back
    EXPECT_EQ (set.size(), 100);

    // Verify that elements are in sorted order
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ (set[i], i);
    }

    // Clear the set
    set.clear();
    EXPECT_TRUE (set.isEmpty());
    set.minimiseStorageOverheads();

    // Re-add elements to test reuse of internal structures
    for (int i = 0; i < 100; ++i)
    {
        set.add (i);
    }

    // Attempt to remove elements in a random order
    juce::Random random;
    for (int i = 0; i < 100; ++i)
    {
        int index = random.nextInt ({ 0, set.size() });
        set.remove (index);
        set.minimiseStorageOverheads();
    }

    // Verify that the set is empty after all removals
    EXPECT_TRUE (set.isEmpty());

    // Test adding duplicate elements in a complex sequence
    for (int i = 0; i < 50; ++i)
    {
        EXPECT_TRUE (set.add (i));
        EXPECT_FALSE (set.add (i)); // Duplicate add
        EXPECT_TRUE (set.add (i + 50));
        EXPECT_FALSE (set.add (i + 50)); // Duplicate add
    }

    // Verify that duplicates are not added
    EXPECT_EQ (set.size(), 100);

    // Verify that elements are in sorted order
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ (set[i], i);
    }
}
