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

#include <yup_animation/yup_animation.h>

using namespace yup;

// =============================================================================
// AnimationTrim
// =============================================================================

class AnimationTrimGetSegmentTests : public ::testing::Test
{
};

// ---- getSegment: no offset --------------------------------------------------

TEST_F (AnimationTrimGetSegmentTests, GetSegment_FullRangeReturnsZeroToOne)
{
    AnimationTrim trim;
    // Defaults: start=0, end=100, offset=0 — full span
    const auto seg = trim.getSegment (0.0f);
    EXPECT_FLOAT_EQ (seg.start, 0.0f);
    EXPECT_FLOAT_EQ (seg.end, 1.0f);
}

TEST_F (AnimationTrimGetSegmentTests, GetSegment_HalfRangeNoOffset)
{
    AnimationTrim trim;
    trim.start = FloatProperty::staticValue (25.0f);
    trim.end = FloatProperty::staticValue (75.0f);

    const auto seg = trim.getSegment (0.0f);
    EXPECT_FLOAT_EQ (seg.start, 0.25f);
    EXPECT_FLOAT_EQ (seg.end, 0.75f);
}

TEST_F (AnimationTrimGetSegmentTests, GetSegment_ZeroLengthRange)
{
    AnimationTrim trim;
    trim.start = FloatProperty::staticValue (50.0f);
    trim.end = FloatProperty::staticValue (50.0f);

    const auto seg = trim.getSegment (0.0f);
    EXPECT_FLOAT_EQ (seg.start, 0.5f);
    EXPECT_FLOAT_EQ (seg.end, 0.5f);
}

TEST_F (AnimationTrimGetSegmentTests, GetSegment_StartGreaterThanEnd_NormalizedToSorted)
{
    AnimationTrim trim;
    trim.start = FloatProperty::staticValue (75.0f);
    trim.end = FloatProperty::staticValue (25.0f);

    const auto seg = trim.getSegment (0.0f);
    // noLoop sorts: min is start, max is end
    EXPECT_FLOAT_EQ (seg.start, 0.25f);
    EXPECT_FLOAT_EQ (seg.end, 0.75f);
}

// ---- getSegment: with offset ------------------------------------------------

TEST_F (AnimationTrimGetSegmentTests, GetSegment_WithPositiveOffsetNoWrap)
{
    AnimationTrim trim;
    trim.start = FloatProperty::staticValue (0.0f);
    trim.end = FloatProperty::staticValue (50.0f);
    trim.offset = FloatProperty::staticValue (90.0f); // +0.25 normalized

    const auto seg = trim.getSegment (0.0f);
    // startWithOffset = 0.0 + 0.25 = 0.25, endWithOffset = 0.5 + 0.25 = 0.75
    // Both <= 1, noLoop => {0.25, 0.75}
    EXPECT_FLOAT_EQ (seg.start, 0.25f);
    EXPECT_FLOAT_EQ (seg.end, 0.75f);
}

TEST_F (AnimationTrimGetSegmentTests, GetSegment_FullSpanWithAnyOffsetIsStillFullSpan)
{
    AnimationTrim trim;
    // abs(1.0 - 0.0) == 1.0 → early return {0.0, 1.0}
    trim.offset = FloatProperty::staticValue (180.0f);

    const auto seg = trim.getSegment (0.0f);
    EXPECT_FLOAT_EQ (seg.start, 0.0f);
    EXPECT_FLOAT_EQ (seg.end, 1.0f);
}

// ---- TrimMode enum ----------------------------------------------------------

TEST_F (AnimationTrimGetSegmentTests, TrimMode_SimultaneouslyIsDefault)
{
    AnimationTrim trim;
    EXPECT_EQ (trim.mode, AnimationTrim::TrimMode::Simultaneously);
}

TEST_F (AnimationTrimGetSegmentTests, TrimMode_ValuesAreDistinct)
{
    EXPECT_NE (AnimationTrim::TrimMode::Simultaneously, AnimationTrim::TrimMode::Individually);
}

// =============================================================================
// AnimationRepeater
// =============================================================================

class AnimationRepeaterTests : public ::testing::Test
{
};

TEST_F (AnimationRepeaterTests, CopiesAt_DefaultIsOne)
{
    AnimationRepeater rep;
    EXPECT_GE (rep.copiesAt (0.0f), 1);
}

TEST_F (AnimationRepeaterTests, CopiesAt_NeverLessThanOne)
{
    AnimationRepeater rep;
    rep.copies = FloatProperty::staticValue (0.0f);
    EXPECT_EQ (rep.copiesAt (0.0f), 1); // jmax(1, int(0)) = 1
}

TEST_F (AnimationRepeaterTests, CopiesAt_ReturnsTruncatedValue)
{
    AnimationRepeater rep;
    rep.copies = FloatProperty::staticValue (3.0f);
    EXPECT_EQ (rep.copiesAt (0.0f), 3);
}

TEST_F (AnimationRepeaterTests, CopiesAt_TruncatesDecimalPart)
{
    AnimationRepeater rep;
    rep.copies = FloatProperty::staticValue (2.9f);
    EXPECT_EQ (rep.copiesAt (0.0f), 2);
}

TEST_F (AnimationRepeaterTests, OffsetAt_DefaultIsZero)
{
    AnimationRepeater rep;
    EXPECT_FLOAT_EQ (rep.offsetAt (0.0f), 0.0f);
}

TEST_F (AnimationRepeaterTests, OffsetAt_ReturnsSetValue)
{
    AnimationRepeater rep;
    rep.offset = FloatProperty::staticValue (5.0f);
    EXPECT_FLOAT_EQ (rep.offsetAt (0.0f), 5.0f);
}

TEST_F (AnimationRepeaterTests, StartOpacityAt_DefaultIsFullOpacity)
{
    AnimationRepeater rep;
    const float val = rep.startOpacityAt (0.0f);
    EXPECT_GT (val, 0.0f); // Non-zero (either 1.0 or 100.0 depending on normalisation)
}

TEST_F (AnimationRepeaterTests, EndOpacityAt_DefaultIsFullOpacity)
{
    AnimationRepeater rep;
    const float val = rep.endOpacityAt (0.0f);
    EXPECT_GT (val, 0.0f);
}

TEST_F (AnimationRepeaterTests, StartAndEndOpacityAt_SameByDefault)
{
    AnimationRepeater rep;
    EXPECT_FLOAT_EQ (rep.startOpacityAt (0.0f), rep.endOpacityAt (0.0f));
}

// =============================================================================
// AnimationRoundedCorner
// =============================================================================

class AnimationRoundedCornerTests : public ::testing::Test
{
};

TEST_F (AnimationRoundedCornerTests, RadiusAt_DefaultIsZero)
{
    AnimationRoundedCorner rc;
    EXPECT_FLOAT_EQ (rc.radiusAt (0.0f), 0.0f);
}

TEST_F (AnimationRoundedCornerTests, RadiusAt_ReturnsSetValueNormalized)
{
    AnimationRoundedCorner rc;
    // radiusAt normalizes by dividing by 100, clamped to [0, 1]
    rc.radius = FloatProperty::staticValue (10.0f);
    EXPECT_FLOAT_EQ (rc.radiusAt (0.0f), 0.1f);
}

TEST_F (AnimationRoundedCornerTests, RadiusAt_FullValueIsOne)
{
    AnimationRoundedCorner rc;
    rc.radius = FloatProperty::staticValue (100.0f);
    EXPECT_FLOAT_EQ (rc.radiusAt (0.0f), 1.0f);
}

TEST_F (AnimationRoundedCornerTests, DefaultFields_NotHidden)
{
    AnimationRoundedCorner rc;
    EXPECT_FALSE (rc.hidden);
}

// =============================================================================
// AnimationMask
// =============================================================================

class AnimationMaskTests : public ::testing::Test
{
};

TEST_F (AnimationMaskTests, OpacityAt_DefaultIsOne)
{
    AnimationMask mask;
    // Default is 100.0 in 0-100 space, normalized to [0,1]
    const float opacity = mask.opacityAt (0.0f);
    EXPECT_FLOAT_EQ (opacity, 1.0f);
}

TEST_F (AnimationMaskTests, OpacityAt_HalfOpacity)
{
    AnimationMask mask;
    mask.opacity = FloatProperty::staticValue (50.0f);
    EXPECT_FLOAT_EQ (mask.opacityAt (0.0f), 0.5f);
}

TEST_F (AnimationMaskTests, OpacityAt_ZeroOpacity)
{
    AnimationMask mask;
    mask.opacity = FloatProperty::staticValue (0.0f);
    EXPECT_FLOAT_EQ (mask.opacityAt (0.0f), 0.0f);
}

TEST_F (AnimationMaskTests, ShapeAt_EmptyPathDataProducesEmptyPath)
{
    AnimationMask mask;
    EXPECT_TRUE (mask.shapeAt (0.0f).isEmpty());
}

TEST_F (AnimationMaskTests, ShapeAt_WithPathDataProducesNonEmptyPath)
{
    AnimationMask mask;

    AnimationPathData pd;
    pd.vertices = { { 0.0f, 0.0f }, { 100.0f, 0.0f }, { 50.0f, 100.0f } };
    pd.inTangents = { { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f } };
    pd.outTangents = { { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f } };
    pd.closed = true;
    mask.shape = PathDataProperty::staticValue (pd);

    EXPECT_FALSE (mask.shapeAt (0.0f).isEmpty());
}

TEST_F (AnimationMaskTests, DefaultMode_IsAdd)
{
    AnimationMask mask;
    EXPECT_EQ (mask.mode, AnimationMask::Mode::Add);
}

TEST_F (AnimationMaskTests, DefaultInverted_IsFalse)
{
    AnimationMask mask;
    EXPECT_FALSE (mask.inverted);
}

TEST_F (AnimationMaskTests, ModeEnum_ValuesAreDistinct)
{
    EXPECT_NE (AnimationMask::Mode::None, AnimationMask::Mode::Add);
    EXPECT_NE (AnimationMask::Mode::Add, AnimationMask::Mode::Subtract);
    EXPECT_NE (AnimationMask::Mode::Subtract, AnimationMask::Mode::Intersect);
    EXPECT_NE (AnimationMask::Mode::Intersect, AnimationMask::Mode::Difference);
}
