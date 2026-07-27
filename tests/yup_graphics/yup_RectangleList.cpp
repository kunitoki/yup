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

#include <yup_graphics/yup_graphics.h>

using namespace yup;

TEST (RectangleListTests, DefaultConstructor)
{
    RectangleList<float> list;
    EXPECT_TRUE (list.isEmpty());
    EXPECT_EQ (list.getNumRectangles(), 0);
    EXPECT_TRUE (list.getBoundingBox().isEmpty());
}

TEST (RectangleListTests, InitializerListConstructor)
{
    RectangleList<float> list {
        Rectangle<float> (0.0f, 0.0f, 10.0f, 10.0f),
        Rectangle<float> (5.0f, 5.0f, 10.0f, 10.0f)
    };

    EXPECT_FALSE (list.isEmpty());
    EXPECT_EQ (list.getNumRectangles(), 2);
    EXPECT_EQ (list.getRectangle (0), Rectangle<float> (0.0f, 0.0f, 10.0f, 10.0f));
    EXPECT_EQ (list.getRectangle (1), Rectangle<float> (5.0f, 5.0f, 10.0f, 10.0f));
}

TEST (RectangleListTests, InitializerListConstructorWithTypeConversion)
{
    RectangleList<float> list {
        Rectangle<int> (0, 0, 10, 10),
        Rectangle<int> (5, 5, 10, 10)
    };

    EXPECT_FALSE (list.isEmpty());
    EXPECT_EQ (list.getNumRectangles(), 2);
    EXPECT_EQ (list.getRectangle (0), Rectangle<float> (0.0f, 0.0f, 10.0f, 10.0f));
    EXPECT_EQ (list.getRectangle (1), Rectangle<float> (5.0f, 5.0f, 10.0f, 10.0f));
}

TEST (RectangleListTests, CopyConstructor)
{
    RectangleList<float> original;
    original.add (Rectangle<float> (10.0f, 20.0f, 30.0f, 40.0f));

    RectangleList<float> copy (original);
    EXPECT_EQ (copy.getNumRectangles(), original.getNumRectangles());
    EXPECT_EQ (copy.getRectangle (0), original.getRectangle (0));
}

TEST (RectangleListTests, MoveConstructor)
{
    RectangleList<float> original;
    original.add (Rectangle<float> (10.0f, 20.0f, 30.0f, 40.0f));

    RectangleList<float> moved (std::move (original));
    EXPECT_EQ (moved.getNumRectangles(), 1);
    EXPECT_EQ (moved.getRectangle (0), Rectangle<float> (10.0f, 20.0f, 30.0f, 40.0f));
}

TEST (RectangleListTests, CopyAssignment)
{
    RectangleList<float> original;
    original.add (Rectangle<float> (10.0f, 20.0f, 30.0f, 40.0f));

    RectangleList<float> copy;
    copy = original;
    EXPECT_EQ (copy.getNumRectangles(), original.getNumRectangles());
    EXPECT_EQ (copy.getRectangle (0), original.getRectangle (0));
}

TEST (RectangleListTests, MoveAssignment)
{
    RectangleList<float> original;
    original.add (Rectangle<float> (10.0f, 20.0f, 30.0f, 40.0f));

    RectangleList<float> moved;
    moved = std::move (original);
    EXPECT_EQ (moved.getNumRectangles(), 1);
    EXPECT_EQ (moved.getRectangle (0), Rectangle<float> (10.0f, 20.0f, 30.0f, 40.0f));
}

TEST (RectangleListTests, AddRectangle)
{
    RectangleList<float> list;
    Rectangle<float> rect (10.0f, 20.0f, 30.0f, 40.0f);

    list.add (rect);
    EXPECT_EQ (list.getNumRectangles(), 1);
    EXPECT_EQ (list.getRectangle (0), rect);
    EXPECT_FALSE (list.isEmpty());
}

TEST (RectangleListTests, AddRectangleWithMerging)
{
    RectangleList<float> list;
    Rectangle<float> rect1 (0.0f, 0.0f, 10.0f, 10.0f);
    Rectangle<float> rect2 (5.0f, 5.0f, 10.0f, 10.0f); // Overlaps with rect1

    list.add (rect1);
    list.add (rect2);

    // Should merge into a single rectangle
    EXPECT_EQ (list.getNumRectangles(), 1);
    Rectangle<float> merged = list.getRectangle (0);
    EXPECT_EQ (merged, Rectangle<float> (0.0f, 0.0f, 15.0f, 15.0f));
}

TEST (RectangleListTests, AddWithoutMerge)
{
    RectangleList<float> list;
    Rectangle<float> rect1 (0.0f, 0.0f, 10.0f, 10.0f);
    Rectangle<float> rect2 (5.0f, 5.0f, 10.0f, 10.0f); // Overlaps with rect1

    list.addWithoutMerge (rect1);
    list.addWithoutMerge (rect2);

    // Should keep both rectangles separate
    EXPECT_EQ (list.getNumRectangles(), 2);
    EXPECT_EQ (list.getRectangle (0), rect1);
    EXPECT_EQ (list.getRectangle (1), rect2);
}

TEST (RectangleListTests, AddWithoutMerge_DuplicateRectangle)
{
    RectangleList<float> list;
    Rectangle<float> rect (10.0f, 20.0f, 30.0f, 40.0f);

    list.addWithoutMerge (rect);
    list.addWithoutMerge (rect); // Same rectangle again

    // Should not add duplicate
    EXPECT_EQ (list.getNumRectangles(), 1);
    EXPECT_EQ (list.getRectangle (0), rect);
}

TEST (RectangleListTests, RemoveRectangle)
{
    RectangleList<float> list;
    Rectangle<float> rect1 (10.0f, 20.0f, 30.0f, 40.0f);
    Rectangle<float> rect2 (50.0f, 60.0f, 70.0f, 80.0f);

    list.add (rect1);
    list.add (rect2);
    EXPECT_EQ (list.getNumRectangles(), 2);

    list.remove (rect1);
    EXPECT_EQ (list.getNumRectangles(), 1);
    EXPECT_EQ (list.getRectangle (0), rect2);

    // Remove non-existent rectangle should not affect list
    Rectangle<float> nonExistent (100.0f, 100.0f, 10.0f, 10.0f);
    list.remove (nonExistent);
    EXPECT_EQ (list.getNumRectangles(), 1);
}

TEST (RectangleListTests, Clear)
{
    RectangleList<float> list;
    list.add (Rectangle<float> (10.0f, 20.0f, 30.0f, 40.0f));
    list.add (Rectangle<float> (50.0f, 60.0f, 70.0f, 80.0f));

    EXPECT_EQ (list.getNumRectangles(), 2);

    list.clear();
    EXPECT_EQ (list.getNumRectangles(), 0);
    EXPECT_TRUE (list.isEmpty());
}

TEST (RectangleListTests, ClearQuick)
{
    RectangleList<float> list;
    list.add (Rectangle<float> (10.0f, 20.0f, 30.0f, 40.0f));
    list.add (Rectangle<float> (50.0f, 60.0f, 70.0f, 80.0f));

    EXPECT_EQ (list.getNumRectangles(), 2);

    list.clearQuick();
    EXPECT_EQ (list.getNumRectangles(), 0);
    EXPECT_TRUE (list.isEmpty());
}

TEST (RectangleListTests, ContainsRectangle)
{
    RectangleList<float> list;
    Rectangle<float> rect (10.0f, 20.0f, 30.0f, 40.0f);

    list.add (rect);

    EXPECT_TRUE (list.contains (rect));
    EXPECT_TRUE (list.contains (10.0f, 20.0f, 30.0f, 40.0f));
    EXPECT_FALSE (list.contains (Rectangle<float> (50.0f, 60.0f, 70.0f, 80.0f)));
    EXPECT_FALSE (list.contains (50.0f, 60.0f, 70.0f, 80.0f));
}

TEST (RectangleListTests, ContainsPoint)
{
    RectangleList<float> list;
    Rectangle<float> rect (10.0f, 20.0f, 30.0f, 40.0f);

    list.add (rect);

    // Test point inside rectangle
    EXPECT_TRUE (list.contains (20.0f, 30.0f));
    EXPECT_TRUE (list.contains (Point<float> (20.0f, 30.0f)));

    // Test point outside rectangle
    EXPECT_FALSE (list.contains (50.0f, 60.0f));
    EXPECT_FALSE (list.contains (Point<float> (50.0f, 60.0f)));

    // Test point on edge
    EXPECT_TRUE (list.contains (10.0f, 20.0f));
    EXPECT_TRUE (list.contains (Point<float> (10.0f, 20.0f)));
}

TEST (RectangleListTests, ContainsPointMultipleRectangles)
{
    RectangleList<float> list;
    list.add (Rectangle<float> (0.0f, 0.0f, 10.0f, 10.0f));
    list.add (Rectangle<float> (20.0f, 20.0f, 10.0f, 10.0f));

    // Test points in different rectangles
    EXPECT_TRUE (list.contains (5.0f, 5.0f));
    EXPECT_TRUE (list.contains (25.0f, 25.0f));

    // Test point between rectangles
    EXPECT_FALSE (list.contains (15.0f, 15.0f));
}

TEST (RectangleListTests, IntersectsRectangle)
{
    RectangleList<float> list;
    list.add (Rectangle<float> (10.0f, 20.0f, 30.0f, 40.0f));

    // Test intersecting rectangle
    EXPECT_TRUE (list.intersects (Rectangle<float> (20.0f, 30.0f, 30.0f, 40.0f)));
    EXPECT_TRUE (list.intersects (20.0f, 30.0f, 30.0f, 40.0f));

    // Test non-intersecting rectangle
    EXPECT_FALSE (list.intersects (Rectangle<float> (100.0f, 100.0f, 10.0f, 10.0f)));
    EXPECT_FALSE (list.intersects (100.0f, 100.0f, 10.0f, 10.0f));
}

TEST (RectangleListTests, GetRectangles)
{
    RectangleList<float> list;
    Rectangle<float> rect1 (10.0f, 20.0f, 30.0f, 40.0f);
    Rectangle<float> rect2 (50.0f, 60.0f, 70.0f, 80.0f);

    list.add (rect1);
    list.add (rect2);

    auto rectangles = list.getRectangles();
    EXPECT_EQ (rectangles.size(), 2);
    EXPECT_EQ (rectangles[0], rect1);
    EXPECT_EQ (rectangles[1], rect2);
}

TEST (RectangleListTests, GetBoundingBox)
{
    RectangleList<float> list;

    // Empty list should have empty bounding box
    EXPECT_TRUE (list.getBoundingBox().isEmpty());

    // Single rectangle
    Rectangle<float> rect1 (10.0f, 20.0f, 30.0f, 40.0f);
    list.add (rect1);
    EXPECT_EQ (list.getBoundingBox(), rect1);

    // Multiple rectangles
    Rectangle<float> rect2 (50.0f, 60.0f, 70.0f, 80.0f);
    list.add (rect2);
    Rectangle<float> expectedBounds (10.0f, 20.0f, 110.0f, 120.0f);
    EXPECT_EQ (list.getBoundingBox(), expectedBounds);
}

TEST (RectangleListTests, GetBoundingBoxWithNegativeCoordinates)
{
    RectangleList<float> list;
    list.add (Rectangle<float> (-10.0f, -20.0f, 30.0f, 40.0f));
    list.add (Rectangle<float> (50.0f, 60.0f, 70.0f, 80.0f));

    Rectangle<float> expectedBounds (-10.0f, -20.0f, 130.0f, 160.0f);
    EXPECT_EQ (list.getBoundingBox(), expectedBounds);
}

TEST (RectangleListTests, OffsetByPoint)
{
    RectangleList<float> list;
    list.add (Rectangle<float> (10.0f, 20.0f, 30.0f, 40.0f));
    list.add (Rectangle<float> (50.0f, 60.0f, 70.0f, 80.0f));

    list.offset (Point<float> (5.0f, 10.0f));

    EXPECT_EQ (list.getRectangle (0), Rectangle<float> (15.0f, 30.0f, 30.0f, 40.0f));
    EXPECT_EQ (list.getRectangle (1), Rectangle<float> (55.0f, 70.0f, 70.0f, 80.0f));
}

TEST (RectangleListTests, OffsetByValues)
{
    RectangleList<float> list;
    list.add (Rectangle<float> (10.0f, 20.0f, 30.0f, 40.0f));
    list.add (Rectangle<float> (50.0f, 60.0f, 70.0f, 80.0f));

    list.offset (5.0f, 10.0f);

    EXPECT_EQ (list.getRectangle (0), Rectangle<float> (15.0f, 30.0f, 30.0f, 40.0f));
    EXPECT_EQ (list.getRectangle (1), Rectangle<float> (55.0f, 70.0f, 70.0f, 80.0f));
}

TEST (RectangleListTests, ScaleUniform)
{
    RectangleList<float> list;
    list.add (Rectangle<float> (10.0f, 20.0f, 30.0f, 40.0f));
    list.add (Rectangle<float> (50.0f, 60.0f, 70.0f, 80.0f));

    list.scale (2.0f);

    EXPECT_EQ (list.getRectangle (0), Rectangle<float> (20.0f, 40.0f, 60.0f, 80.0f));
    EXPECT_EQ (list.getRectangle (1), Rectangle<float> (100.0f, 120.0f, 140.0f, 160.0f));
}

TEST (RectangleListTests, ScaleNonUniform)
{
    RectangleList<float> list;
    list.add (Rectangle<float> (10.0f, 20.0f, 30.0f, 40.0f));
    list.add (Rectangle<float> (50.0f, 60.0f, 70.0f, 80.0f));

    list.scale (2.0f, 0.5f);

    EXPECT_EQ (list.getRectangle (0), Rectangle<float> (20.0f, 10.0f, 60.0f, 20.0f));
    EXPECT_EQ (list.getRectangle (1), Rectangle<float> (100.0f, 30.0f, 140.0f, 40.0f));
}

TEST (RectangleListTests, BeginEndIterators)
{
    RectangleList<float> list;
    Rectangle<float> rect1 (10.0f, 20.0f, 30.0f, 40.0f);
    Rectangle<float> rect2 (50.0f, 60.0f, 70.0f, 80.0f);

    list.add (rect1);
    list.add (rect2);

    // Test const iterators
    const RectangleList<float>& constList = list;
    auto constBegin = constList.begin();
    auto constEnd = constList.end();

    EXPECT_EQ (*constBegin, rect1);
    EXPECT_EQ (*(constBegin + 1), rect2);
    EXPECT_EQ (constEnd - constBegin, 2);

    // Test non-const iterators
    auto begin = list.begin();
    auto end = list.end();

    EXPECT_EQ (*begin, rect1);
    EXPECT_EQ (*(begin + 1), rect2);
    EXPECT_EQ (end - begin, 2);
}

TEST (RectangleListTests, RangeBasedForLoop)
{
    RectangleList<float> list;
    Rectangle<float> rect1 (10.0f, 20.0f, 30.0f, 40.0f);
    Rectangle<float> rect2 (50.0f, 60.0f, 70.0f, 80.0f);

    list.add (rect1);
    list.add (rect2);

    std::vector<Rectangle<float>> collected;
    for (const auto& rect : list)
    {
        collected.push_back (rect);
    }

    EXPECT_EQ (collected.size(), 2);
    EXPECT_EQ (collected[0], rect1);
    EXPECT_EQ (collected[1], rect2);
}

TEST (RectangleListTests, EmptyListOperations)
{
    RectangleList<float> list;

    // Operations on empty list should be safe
    EXPECT_TRUE (list.isEmpty());
    EXPECT_EQ (list.getNumRectangles(), 0);
    EXPECT_TRUE (list.getBoundingBox().isEmpty());

    // Contains operations
    EXPECT_FALSE (list.contains (Point<float> (10.0f, 20.0f)));
    EXPECT_FALSE (list.contains (10.0f, 20.0f));
    EXPECT_FALSE (list.contains (Rectangle<float> (10.0f, 20.0f, 30.0f, 40.0f)));

    // Intersects operations
    EXPECT_FALSE (list.intersects (Rectangle<float> (10.0f, 20.0f, 30.0f, 40.0f)));
    EXPECT_FALSE (list.intersects (10.0f, 20.0f, 30.0f, 40.0f));

    // Transformations should be safe
    list.offset (10.0f, 20.0f);
    list.scale (2.0f);
    EXPECT_TRUE (list.isEmpty());
}

TEST (RectangleListTests, ComplexMergingScenario)
{
    RectangleList<float> list;

    // Add rectangles that will merge in complex ways
    list.add (Rectangle<float> (0.0f, 0.0f, 10.0f, 10.0f));
    list.add (Rectangle<float> (5.0f, 5.0f, 10.0f, 10.0f));
    list.add (Rectangle<float> (10.0f, 10.0f, 10.0f, 10.0f));

    // Should merge into fewer rectangles
    EXPECT_LT (list.getNumRectangles(), 3);

    // Bounding box should contain all original rectangles
    Rectangle<float> bounds = list.getBoundingBox();
    EXPECT_LE (bounds.getX(), 0.0f);
    EXPECT_LE (bounds.getY(), 0.0f);
    EXPECT_GE (bounds.getRight(), 20.0f);
    EXPECT_GE (bounds.getBottom(), 20.0f);
}

TEST (RectangleListTests, NonIntersectingRectangles)
{
    RectangleList<float> list;

    // Add rectangles that don't intersect
    list.add (Rectangle<float> (0.0f, 0.0f, 10.0f, 10.0f));
    list.add (Rectangle<float> (20.0f, 20.0f, 10.0f, 10.0f));
    list.add (Rectangle<float> (40.0f, 40.0f, 10.0f, 10.0f));

    // Should keep all rectangles separate
    EXPECT_EQ (list.getNumRectangles(), 3);

    // Test containment
    EXPECT_TRUE (list.contains (5.0f, 5.0f));
    EXPECT_TRUE (list.contains (25.0f, 25.0f));
    EXPECT_TRUE (list.contains (45.0f, 45.0f));
    EXPECT_FALSE (list.contains (15.0f, 15.0f));
}

TEST (RectangleListTests, EdgeCasesWithZeroSizeRectangles)
{
    RectangleList<float> list;

    // Add zero-width rectangle
    list.add (Rectangle<float> (10.0f, 10.0f, 0.0f, 20.0f));

    // Add zero-height rectangle
    list.add (Rectangle<float> (20.0f, 20.0f, 30.0f, 0.0f));

    // Add zero-size rectangle
    list.add (Rectangle<float> (30.0f, 30.0f, 0.0f, 0.0f));

    // Test operations with zero-size rectangles
    EXPECT_FALSE (list.isEmpty());
    EXPECT_EQ (list.getNumRectangles(), 3);

    // Bounding box should still be computed correctly
    Rectangle<float> bounds = list.getBoundingBox();
    EXPECT_FALSE (bounds.isEmpty());
}

TEST (RectangleListTests, StressTestWithManyRectangles)
{
    RectangleList<float> list;

    // Add many rectangles
    for (int i = 0; i < 100; ++i)
    {
        list.add (Rectangle<float> (i * 5.0f, i * 5.0f, 10.0f, 10.0f));
    }

    // Test that all operations still work
    EXPECT_FALSE (list.isEmpty());
    EXPECT_GT (list.getNumRectangles(), 0);

    Rectangle<float> bounds = list.getBoundingBox();
    EXPECT_FALSE (bounds.isEmpty());

    // Test containment
    EXPECT_TRUE (list.contains (50.0f, 50.0f));
    EXPECT_FALSE (list.contains (1000.0f, 1000.0f));
}

TEST (RectangleListTests, TypeConversions)
{
    RectangleList<int> intList;
    intList.add (Rectangle<int> (10, 20, 30, 40));

    // Test that we can work with different types
    EXPECT_EQ (intList.getNumRectangles(), 1);
    EXPECT_EQ (intList.getRectangle (0), Rectangle<int> (10, 20, 30, 40));

    EXPECT_TRUE (intList.contains (Point<int> (20, 30)));
    EXPECT_FALSE (intList.contains (Point<int> (50, 60)));
}

TEST (RectangleListTests, GetRectangleOutOfBounds)
{
    RectangleList<float> list;
    list.add (Rectangle<float> (10.0f, 20.0f, 30.0f, 40.0f));

    // Test valid index
    EXPECT_EQ (list.getRectangle (0), Rectangle<float> (10.0f, 20.0f, 30.0f, 40.0f));

    // Test invalid indices (should trigger assertion in debug builds)
    // In release builds, behavior is undefined but shouldn't crash
    // We can't easily test this without triggering assertions
}

TEST (RectangleListTests, MergeRecursiveScenario)
{
    RectangleList<float> list;

    // Create a scenario where adding one rectangle causes multiple merges
    list.addWithoutMerge (Rectangle<float> (0.0f, 0.0f, 10.0f, 10.0f));
    list.addWithoutMerge (Rectangle<float> (20.0f, 20.0f, 10.0f, 10.0f));
    list.addWithoutMerge (Rectangle<float> (40.0f, 40.0f, 10.0f, 10.0f));

    EXPECT_EQ (list.getNumRectangles(), 3);

    // Add a rectangle that connects all three
    list.add (Rectangle<float> (5.0f, 5.0f, 40.0f, 40.0f));

    // Should merge into fewer rectangles
    EXPECT_LT (list.getNumRectangles(), 3);
}
