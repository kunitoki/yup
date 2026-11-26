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

#include <yup_graphics/yup_graphics.h>

#include <gtest/gtest.h>

using namespace yup;

//==============================================================================
class PolylineReductionTests : public ::testing::Test
{
protected:
    using Points = std::vector<Point<float>>;

    /** Reduces a copy of the points and returns the ones that are kept. */
    static Points reduce (Points points, float pixelsPerUnit)
    {
        points.resize (reducePolylineToPixelColumns (points, pixelsPerUnit));
        return points;
    }

    /** A line of many points going up and down, each a step further along x than the one before. */
    static Points makeSignal (int numPoints, float step)
    {
        Points points;

        for (int i = 0; i < numPoints; ++i)
        {
            const auto phase = static_cast<float> (i);
            points.emplace_back (phase * step, 50.0f + 30.0f * std::sin (phase * 0.37f) + 15.0f * std::sin (phase * 2.3f));
        }

        return points;
    }

    /** The lowest and the highest y of the points of each column, by the number of the column. */
    static std::map<int, std::pair<float, float>> getColumnRanges (const Points& points, float pixelsPerUnit)
    {
        std::map<int, std::pair<float, float>> ranges;

        for (const auto& point : points)
        {
            const auto column = static_cast<int> (std::floor (point.getX() * pixelsPerUnit));
            const auto found = ranges.try_emplace (column, point.getY(), point.getY()).first;

            found->second.first = jmin (found->second.first, point.getY());
            found->second.second = jmax (found->second.second, point.getY());
        }

        return ranges;
    }
};

//==============================================================================
TEST_F (PolylineReductionTests, KeepsTheFirstTheExtremesAndTheLastOfEachColumn)
{
    const Points points {
        // Up, down, then somewhere in between
        { 0.0f, 5.0f },
        { 0.1f, 9.0f },
        { 0.2f, 1.0f },
        { 0.3f, 4.0f },
        { 0.4f, 6.0f },
        { 0.9f, 3.0f },
        // Level
        { 1.0f, 2.0f },
        { 1.2f, 2.0f },
        { 1.5f, 2.0f },
        { 1.6f, 2.0f },
        { 1.7f, 2.0f },
        { 1.9f, 2.0f },
        // Rising: the first is the lowest and the last the highest
        { 2.0f, 0.0f },
        { 2.1f, 1.0f },
        { 2.2f, 2.0f },
        { 2.3f, 3.0f },
        { 2.4f, 4.0f },
        { 2.5f, 5.0f }
    };

    const Points expected {
        { 0.0f, 5.0f },
        { 0.1f, 9.0f },
        { 0.2f, 1.0f },
        { 0.9f, 3.0f },
        { 1.0f, 2.0f },
        { 1.9f, 2.0f },
        { 2.0f, 0.0f },
        { 2.5f, 5.0f }
    };

    EXPECT_EQ (reduce (points, 1.0f), expected);
}

TEST_F (PolylineReductionTests, KeepsTheExtremesInTheOrderTheyCome)
{
    // The lowest comes before the highest here, and both share the x of the others
    const Points points { { 3.0f, 5.0f }, { 3.0f, 1.0f }, { 3.0f, 4.0f }, { 3.0f, 9.0f }, { 3.0f, 2.0f }, { 3.0f, 6.0f } };
    const Points expected { { 3.0f, 5.0f }, { 3.0f, 1.0f }, { 3.0f, 9.0f }, { 3.0f, 6.0f } };

    EXPECT_EQ (reduce (points, 1.0f), expected);
}

TEST_F (PolylineReductionTests, ADenseLineKeepsItsEndsAndTheRangeOfEveryColumn)
{
    // 4096 points across 256 units at 2 pixels a unit: 8 points for each of the 512 columns
    const auto points = makeSignal (4096, 256.0f / 4095.0f);
    const auto reduced = reduce (points, 2.0f);

    ASSERT_LT (reduced.size(), points.size());
    EXPECT_LE (reduced.size(), static_cast<size_t> (4 * 513));
    EXPECT_EQ (reduced.front(), points.front());
    EXPECT_EQ (reduced.back(), points.back());

    EXPECT_EQ (getColumnRanges (reduced, 2.0f), getColumnRanges (points, 2.0f));

    // What is kept is some of the points, in their order
    auto next = points.begin();

    for (const auto& point : reduced)
    {
        next = std::find (next, points.end(), point);
        ASSERT_NE (next, points.end());
        ++next;
    }
}

TEST_F (PolylineReductionTests, LeavesALineWithFewPointsForItsColumns)
{
    // Two points for each column
    const auto sparse = makeSignal (2048, 512.0f / 2047.0f);
    EXPECT_EQ (reduce (sparse, 2.0f), sparse);

    // Four points at most are what a column keeps anyway
    const Points few { { 0.0f, 1.0f }, { 0.1f, 3.0f }, { 0.2f, 2.0f }, { 0.3f, 0.0f } };
    EXPECT_EQ (reduce (few, 1.0f), few);

    EXPECT_EQ (reducePolylineToPixelColumns (Span<Point<float>>(), 1.0f), 0u);
}

TEST_F (PolylineReductionTests, LeavesALineWhoseXGoesBack)
{
    auto points = makeSignal (4096, 256.0f / 4095.0f);
    std::swap (points[2000], points[2001]);

    EXPECT_EQ (reduce (points, 2.0f), points);

    // A closed shape comes back to where it started
    Points circle;

    for (int i = 0; i <= 64; ++i)
    {
        const auto angle = MathConstants<float>::twoPi * static_cast<float> (i) / 64.0f;
        circle.emplace_back (2.0f + std::cos (angle), 2.0f + std::sin (angle));
    }

    EXPECT_EQ (reduce (circle, 1.0f), circle);
}

TEST_F (PolylineReductionTests, ReducesForTheScaleItIsGiven)
{
    const auto points = makeSignal (4096, 256.0f / 4095.0f);

    const auto small = reduce (points, 1.0f);
    const auto large = reduce (points, 2.0f);

    // Half the columns keep half the points at most, and enough columns keep them all
    EXPECT_LE (small.size(), static_cast<size_t> (4 * 257));
    EXPECT_GT (large.size(), small.size());
    EXPECT_EQ (reduce (points, 4.0f), points);

    // No scale, no columns
    EXPECT_EQ (reduce (points, 0.0f), points);
    EXPECT_EQ (reduce (points, -2.0f), points);
    EXPECT_EQ (reduce (points, std::numeric_limits<float>::quiet_NaN()), points);
}
