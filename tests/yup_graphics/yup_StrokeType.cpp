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

TEST (StrokeTypeTests, Default_Constructor)
{
    StrokeType stroke;
    EXPECT_FLOAT_EQ (stroke.getWidth(), 1.0f);
    EXPECT_EQ (stroke.getCap(), StrokeCap::Butt);
    EXPECT_EQ (stroke.getJoin(), StrokeJoin::Miter);
}

TEST (StrokeTypeTests, Width_Constructor)
{
    StrokeType stroke (5.0f);
    EXPECT_FLOAT_EQ (stroke.getWidth(), 5.0f);
    EXPECT_EQ (stroke.getCap(), StrokeCap::Butt);
    EXPECT_EQ (stroke.getJoin(), StrokeJoin::Miter);
}

TEST (StrokeTypeTests, Width_Join_Constructor)
{
    StrokeType stroke (3.5f, StrokeJoin::Round);
    EXPECT_FLOAT_EQ (stroke.getWidth(), 3.5f);
    EXPECT_EQ (stroke.getCap(), StrokeCap::Butt);
    EXPECT_EQ (stroke.getJoin(), StrokeJoin::Round);
}

TEST (StrokeTypeTests, Width_Cap_Constructor)
{
    StrokeType stroke (2.0f, StrokeCap::Round);
    EXPECT_FLOAT_EQ (stroke.getWidth(), 2.0f);
    EXPECT_EQ (stroke.getCap(), StrokeCap::Round);
    EXPECT_EQ (stroke.getJoin(), StrokeJoin::Miter);
}

TEST (StrokeTypeTests, Full_Constructor)
{
    StrokeType stroke (4.0f, StrokeJoin::Bevel, StrokeCap::Square);
    EXPECT_FLOAT_EQ (stroke.getWidth(), 4.0f);
    EXPECT_EQ (stroke.getCap(), StrokeCap::Square);
    EXPECT_EQ (stroke.getJoin(), StrokeJoin::Bevel);
}

TEST (StrokeTypeTests, Copy_Constructor)
{
    StrokeType original (6.0f, StrokeJoin::Round, StrokeCap::Round);
    StrokeType copied (original);

    EXPECT_FLOAT_EQ (copied.getWidth(), 6.0f);
    EXPECT_EQ (copied.getCap(), StrokeCap::Round);
    EXPECT_EQ (copied.getJoin(), StrokeJoin::Round);
}

TEST (StrokeTypeTests, Move_Constructor)
{
    StrokeType original (7.5f, StrokeJoin::Bevel, StrokeCap::Square);
    StrokeType moved (std::move (original));

    EXPECT_FLOAT_EQ (moved.getWidth(), 7.5f);
    EXPECT_EQ (moved.getCap(), StrokeCap::Square);
    EXPECT_EQ (moved.getJoin(), StrokeJoin::Bevel);
}

TEST (StrokeTypeTests, Copy_Assignment)
{
    StrokeType original (3.0f, StrokeJoin::Round, StrokeCap::Round);
    StrokeType assigned;

    assigned = original;

    EXPECT_FLOAT_EQ (assigned.getWidth(), 3.0f);
    EXPECT_EQ (assigned.getCap(), StrokeCap::Round);
    EXPECT_EQ (assigned.getJoin(), StrokeJoin::Round);
}

TEST (StrokeTypeTests, Move_Assignment)
{
    StrokeType original (8.0f, StrokeJoin::Bevel, StrokeCap::Square);
    StrokeType assigned;

    assigned = std::move (original);

    EXPECT_FLOAT_EQ (assigned.getWidth(), 8.0f);
    EXPECT_EQ (assigned.getCap(), StrokeCap::Square);
    EXPECT_EQ (assigned.getJoin(), StrokeJoin::Bevel);
}

TEST (StrokeTypeTests, With_Width)
{
    StrokeType original (2.0f, StrokeJoin::Round, StrokeCap::Round);
    StrokeType modified = original.withWidth (10.0f);

    // Original should be unchanged
    EXPECT_FLOAT_EQ (original.getWidth(), 2.0f);
    EXPECT_EQ (original.getCap(), StrokeCap::Round);
    EXPECT_EQ (original.getJoin(), StrokeJoin::Round);

    // Modified should have new width but same cap and join
    EXPECT_FLOAT_EQ (modified.getWidth(), 10.0f);
    EXPECT_EQ (modified.getCap(), StrokeCap::Round);
    EXPECT_EQ (modified.getJoin(), StrokeJoin::Round);
}

TEST (StrokeTypeTests, With_Cap)
{
    StrokeType original (5.0f, StrokeJoin::Miter, StrokeCap::Butt);
    StrokeType modified = original.withCap (StrokeCap::Square);

    // Original should be unchanged
    EXPECT_FLOAT_EQ (original.getWidth(), 5.0f);
    EXPECT_EQ (original.getCap(), StrokeCap::Butt);
    EXPECT_EQ (original.getJoin(), StrokeJoin::Miter);

    // Modified should have new cap but same width and join
    EXPECT_FLOAT_EQ (modified.getWidth(), 5.0f);
    EXPECT_EQ (modified.getCap(), StrokeCap::Square);
    EXPECT_EQ (modified.getJoin(), StrokeJoin::Miter);
}

TEST (StrokeTypeTests, With_Join)
{
    StrokeType original (3.5f, StrokeJoin::Miter, StrokeCap::Round);
    StrokeType modified = original.withJoin (StrokeJoin::Bevel);

    // Original should be unchanged
    EXPECT_FLOAT_EQ (original.getWidth(), 3.5f);
    EXPECT_EQ (original.getCap(), StrokeCap::Round);
    EXPECT_EQ (original.getJoin(), StrokeJoin::Miter);

    // Modified should have new join but same width and cap
    EXPECT_FLOAT_EQ (modified.getWidth(), 3.5f);
    EXPECT_EQ (modified.getCap(), StrokeCap::Round);
    EXPECT_EQ (modified.getJoin(), StrokeJoin::Bevel);
}

TEST (StrokeTypeTests, Equality_Operator)
{
    StrokeType stroke1 (4.0f, StrokeJoin::Round, StrokeCap::Round);
    StrokeType stroke2 (4.0f, StrokeJoin::Round, StrokeCap::Round);
    StrokeType stroke3 (5.0f, StrokeJoin::Round, StrokeCap::Round);  // Different width
    StrokeType stroke4 (4.0f, StrokeJoin::Bevel, StrokeCap::Round);  // Different join
    StrokeType stroke5 (4.0f, StrokeJoin::Round, StrokeCap::Square); // Different cap

    EXPECT_TRUE (stroke1 == stroke2);
    EXPECT_FALSE (stroke1 == stroke3);
    EXPECT_FALSE (stroke1 == stroke4);
    EXPECT_FALSE (stroke1 == stroke5);
}

TEST (StrokeTypeTests, Inequality_Operator)
{
    StrokeType stroke1 (4.0f, StrokeJoin::Round, StrokeCap::Round);
    StrokeType stroke2 (4.0f, StrokeJoin::Round, StrokeCap::Round);
    StrokeType stroke3 (5.0f, StrokeJoin::Round, StrokeCap::Round);  // Different width
    StrokeType stroke4 (4.0f, StrokeJoin::Bevel, StrokeCap::Round);  // Different join
    StrokeType stroke5 (4.0f, StrokeJoin::Round, StrokeCap::Square); // Different cap

    EXPECT_FALSE (stroke1 != stroke2);
    EXPECT_TRUE (stroke1 != stroke3);
    EXPECT_TRUE (stroke1 != stroke4);
    EXPECT_TRUE (stroke1 != stroke5);
}

TEST (StrokeTypeTests, All_Cap_Types)
{
    StrokeType buttCap (1.0f, StrokeCap::Butt);
    StrokeType roundCap (1.0f, StrokeCap::Round);
    StrokeType squareCap (1.0f, StrokeCap::Square);

    EXPECT_EQ (buttCap.getCap(), StrokeCap::Butt);
    EXPECT_EQ (roundCap.getCap(), StrokeCap::Round);
    EXPECT_EQ (squareCap.getCap(), StrokeCap::Square);

    // All should have different caps
    EXPECT_NE (buttCap, roundCap);
    EXPECT_NE (buttCap, squareCap);
    EXPECT_NE (roundCap, squareCap);
}

TEST (StrokeTypeTests, All_Join_Types)
{
    StrokeType miterJoin (1.0f, StrokeJoin::Miter);
    StrokeType roundJoin (1.0f, StrokeJoin::Round);
    StrokeType bevelJoin (1.0f, StrokeJoin::Bevel);

    EXPECT_EQ (miterJoin.getJoin(), StrokeJoin::Miter);
    EXPECT_EQ (roundJoin.getJoin(), StrokeJoin::Round);
    EXPECT_EQ (bevelJoin.getJoin(), StrokeJoin::Bevel);

    // All should have different joins
    EXPECT_NE (miterJoin, roundJoin);
    EXPECT_NE (miterJoin, bevelJoin);
    EXPECT_NE (roundJoin, bevelJoin);
}

TEST (StrokeTypeTests, Zero_Width)
{
    StrokeType zeroWidth (0.0f);
    EXPECT_FLOAT_EQ (zeroWidth.getWidth(), 0.0f);
    EXPECT_EQ (zeroWidth.getCap(), StrokeCap::Butt);
    EXPECT_EQ (zeroWidth.getJoin(), StrokeJoin::Miter);
}

TEST (StrokeTypeTests, Negative_Width)
{
    StrokeType negativeWidth (-5.0f);
    EXPECT_FLOAT_EQ (negativeWidth.getWidth(), -5.0f);
    // Note: StrokeType doesn't clamp negative values - that's up to the user or Graphics class
}

TEST (StrokeTypeTests, Large_Width)
{
    StrokeType largeWidth (1000.0f);
    EXPECT_FLOAT_EQ (largeWidth.getWidth(), 1000.0f);
}

TEST (StrokeTypeTests, Very_Small_Width)
{
    StrokeType smallWidth (0.001f);
    EXPECT_FLOAT_EQ (smallWidth.getWidth(), 0.001f);
}

TEST (StrokeTypeTests, Chaining_With_Methods)
{
    StrokeType original;
    StrokeType modified = original.withWidth (10.0f).withCap (StrokeCap::Round).withJoin (StrokeJoin::Bevel);

    // Original should be unchanged
    EXPECT_FLOAT_EQ (original.getWidth(), 1.0f);
    EXPECT_EQ (original.getCap(), StrokeCap::Butt);
    EXPECT_EQ (original.getJoin(), StrokeJoin::Miter);

    // Modified should have all new values
    EXPECT_FLOAT_EQ (modified.getWidth(), 10.0f);
    EXPECT_EQ (modified.getCap(), StrokeCap::Round);
    EXPECT_EQ (modified.getJoin(), StrokeJoin::Bevel);
}

TEST (StrokeTypeTests, Self_Assignment)
{
    StrokeType stroke (5.0f, StrokeJoin::Round, StrokeCap::Square);
    stroke = stroke; // Self assignment

    EXPECT_FLOAT_EQ (stroke.getWidth(), 5.0f);
    EXPECT_EQ (stroke.getCap(), StrokeCap::Square);
    EXPECT_EQ (stroke.getJoin(), StrokeJoin::Round);
}

TEST (StrokeTypeTests, Multiple_Copies)
{
    StrokeType original (7.0f, StrokeJoin::Bevel, StrokeCap::Round);
    StrokeType copy1 (original);
    StrokeType copy2 = copy1;
    StrokeType copy3;
    copy3 = copy2;

    // All copies should be equal
    EXPECT_EQ (original, copy1);
    EXPECT_EQ (copy1, copy2);
    EXPECT_EQ (copy2, copy3);
    EXPECT_EQ (original, copy3);

    // All should have same values
    EXPECT_FLOAT_EQ (copy3.getWidth(), 7.0f);
    EXPECT_EQ (copy3.getCap(), StrokeCap::Round);
    EXPECT_EQ (copy3.getJoin(), StrokeJoin::Bevel);
}

TEST (StrokeTypeTests, Floating_Point_Precision)
{
    StrokeType stroke1 (1.0f);
    StrokeType stroke2 (1.0000001f); // Very close but different

    // Should be different due to exact floating point comparison
    EXPECT_NE (stroke1, stroke2);

    // But widths should be very close
    EXPECT_NEAR (stroke1.getWidth(), stroke2.getWidth(), 1e-6f);
}

TEST (StrokeTypeTests, Consistency_After_Operations)
{
    StrokeType original (3.0f, StrokeJoin::Round, StrokeCap::Round);

    // Create variations
    StrokeType widthVariation = original.withWidth (6.0f);
    StrokeType capVariation = original.withCap (StrokeCap::Square);
    StrokeType joinVariation = original.withJoin (StrokeJoin::Bevel);

    // Each variation should differ from original in exactly one property
    EXPECT_NE (original, widthVariation);
    EXPECT_NE (original, capVariation);
    EXPECT_NE (original, joinVariation);

    // But should be equal if we revert the changed property
    EXPECT_EQ (original, widthVariation.withWidth (3.0f));
    EXPECT_EQ (original, capVariation.withCap (StrokeCap::Round));
    EXPECT_EQ (original, joinVariation.withJoin (StrokeJoin::Round));
}

TEST (StrokeTypeTests, All_Combinations)
{
    std::vector<float> widths = { 0.0f, 1.0f, 2.5f, 10.0f };
    std::vector<StrokeCap> caps = { StrokeCap::Butt, StrokeCap::Round, StrokeCap::Square };
    std::vector<StrokeJoin> joins = { StrokeJoin::Miter, StrokeJoin::Round, StrokeJoin::Bevel };

    // Test that we can create all combinations
    for (float width : widths)
    {
        for (StrokeCap cap : caps)
        {
            for (StrokeJoin join : joins)
            {
                StrokeType stroke (width, join, cap);
                EXPECT_FLOAT_EQ (stroke.getWidth(), width);
                EXPECT_EQ (stroke.getCap(), cap);
                EXPECT_EQ (stroke.getJoin(), join);

                // Test equality with identical stroke
                StrokeType identical (width, join, cap);
                EXPECT_EQ (stroke, identical);
            }
        }
    }
}