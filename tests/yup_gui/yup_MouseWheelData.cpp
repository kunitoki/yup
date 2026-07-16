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

TEST (MouseWheelDataTests, DefaultConstruction)
{
    MouseWheelData data;

    EXPECT_FLOAT_EQ (0.0f, data.getDeltaX());
    EXPECT_FLOAT_EQ (0.0f, data.getDeltaY());
}

TEST (MouseWheelDataTests, ConstructorStoresValues)
{
    MouseWheelData data (1.5f, 2.5f);

    EXPECT_FLOAT_EQ (1.5f, data.getDeltaX());
    EXPECT_FLOAT_EQ (2.5f, data.getDeltaY());
}

TEST (MouseWheelDataTests, ConstructorHandlesNegativeValues)
{
    MouseWheelData data (-3.0f, -4.0f);

    EXPECT_FLOAT_EQ (-3.0f, data.getDeltaX());
    EXPECT_FLOAT_EQ (-4.0f, data.getDeltaY());
}

TEST (MouseWheelDataTests, ConstructorHandlesZeroValues)
{
    MouseWheelData data (0.0f, 0.0f);

    EXPECT_FLOAT_EQ (0.0f, data.getDeltaX());
    EXPECT_FLOAT_EQ (0.0f, data.getDeltaY());
}

TEST (MouseWheelDataTests, CopyConstructor)
{
    MouseWheelData original (10.0f, 20.0f);
    MouseWheelData copy (original);

    EXPECT_FLOAT_EQ (10.0f, copy.getDeltaX());
    EXPECT_FLOAT_EQ (20.0f, copy.getDeltaY());
}

TEST (MouseWheelDataTests, CopyAssignment)
{
    MouseWheelData original (10.0f, 20.0f);
    MouseWheelData copy;
    copy = original;

    EXPECT_FLOAT_EQ (10.0f, copy.getDeltaX());
    EXPECT_FLOAT_EQ (20.0f, copy.getDeltaY());
}

TEST (MouseWheelDataTests, MoveConstructor)
{
    MouseWheelData original (10.0f, 20.0f);
    MouseWheelData moved (std::move (original));

    EXPECT_FLOAT_EQ (10.0f, moved.getDeltaX());
    EXPECT_FLOAT_EQ (20.0f, moved.getDeltaY());
}

TEST (MouseWheelDataTests, MoveAssignment)
{
    MouseWheelData original (10.0f, 20.0f);
    MouseWheelData moved;
    moved = std::move (original);

    EXPECT_FLOAT_EQ (10.0f, moved.getDeltaX());
    EXPECT_FLOAT_EQ (20.0f, moved.getDeltaY());
}

TEST (MouseWheelDataTests, SetDeltaX)
{
    MouseWheelData data;
    auto& result = data.setDeltaX (5.0f);

    EXPECT_FLOAT_EQ (5.0f, data.getDeltaX());
    EXPECT_FLOAT_EQ (0.0f, data.getDeltaY());
    EXPECT_EQ (&data, &result);
}

TEST (MouseWheelDataTests, SetDeltaY)
{
    MouseWheelData data;
    auto& result = data.setDeltaY (5.0f);

    EXPECT_FLOAT_EQ (0.0f, data.getDeltaX());
    EXPECT_FLOAT_EQ (5.0f, data.getDeltaY());
    EXPECT_EQ (&data, &result);
}

TEST (MouseWheelDataTests, SetDeltaXAndYChainable)
{
    MouseWheelData data;
    data.setDeltaX (1.0f).setDeltaY (2.0f);

    EXPECT_FLOAT_EQ (1.0f, data.getDeltaX());
    EXPECT_FLOAT_EQ (2.0f, data.getDeltaY());
}

TEST (MouseWheelDataTests, WithDeltaXReturnsNewObject)
{
    MouseWheelData data (1.0f, 2.0f);
    MouseWheelData modified = data.withDeltaX (10.0f);

    EXPECT_FLOAT_EQ (1.0f, data.getDeltaX());
    EXPECT_FLOAT_EQ (2.0f, data.getDeltaY());
    EXPECT_FLOAT_EQ (10.0f, modified.getDeltaX());
    EXPECT_FLOAT_EQ (2.0f, modified.getDeltaY());
}

TEST (MouseWheelDataTests, WithDeltaYReturnsNewObject)
{
    MouseWheelData data (1.0f, 2.0f);
    MouseWheelData modified = data.withDeltaY (20.0f);

    EXPECT_FLOAT_EQ (1.0f, data.getDeltaX());
    EXPECT_FLOAT_EQ (2.0f, data.getDeltaY());
    EXPECT_FLOAT_EQ (1.0f, modified.getDeltaX());
    EXPECT_FLOAT_EQ (20.0f, modified.getDeltaY());
}

TEST (MouseWheelDataTests, WithDeltaXAndYChainTogether)
{
    MouseWheelData data;
    MouseWheelData modified = data.withDeltaX (5.0f).withDeltaY (10.0f);

    EXPECT_FLOAT_EQ (0.0f, data.getDeltaX());
    EXPECT_FLOAT_EQ (0.0f, data.getDeltaY());
    EXPECT_FLOAT_EQ (5.0f, modified.getDeltaX());
    EXPECT_FLOAT_EQ (10.0f, modified.getDeltaY());
}

TEST (MouseWheelDataTests, EqualitySameValues)
{
    EXPECT_TRUE (MouseWheelData (1.0f, 2.0f) == MouseWheelData (1.0f, 2.0f));
}

TEST (MouseWheelDataTests, EqualityDifferentDeltaX)
{
    EXPECT_FALSE (MouseWheelData (1.0f, 2.0f) == MouseWheelData (3.0f, 2.0f));
}

TEST (MouseWheelDataTests, EqualityDifferentDeltaY)
{
    EXPECT_FALSE (MouseWheelData (1.0f, 2.0f) == MouseWheelData (1.0f, 4.0f));
}

TEST (MouseWheelDataTests, EqualityBothDifferent)
{
    EXPECT_FALSE (MouseWheelData (1.0f, 2.0f) == MouseWheelData (5.0f, 6.0f));
}

TEST (MouseWheelDataTests, Inequality)
{
    EXPECT_TRUE (MouseWheelData (1.0f, 2.0f) != MouseWheelData (3.0f, 4.0f));
    EXPECT_FALSE (MouseWheelData (1.0f, 2.0f) != MouseWheelData (1.0f, 2.0f));
}

TEST (MouseWheelDataTests, EqualityWithSelf)
{
    MouseWheelData data (1.0f, 2.0f);
    EXPECT_TRUE (data == data);
    EXPECT_FALSE (data != data);
}

TEST (MouseWheelDataTests, EqualityDefaultConstructed)
{
    EXPECT_TRUE (MouseWheelData() == MouseWheelData());
    EXPECT_TRUE (MouseWheelData() == MouseWheelData (0.0f, 0.0f));
}

TEST (MouseWheelDataTests, LargeValues)
{
    MouseWheelData data (1e6f, -1e6f);

    EXPECT_FLOAT_EQ (1e6f, data.getDeltaX());
    EXPECT_FLOAT_EQ (-1e6f, data.getDeltaY());
}

TEST (MouseWheelDataTests, FractionalValues)
{
    MouseWheelData data (0.001f, 0.0001f);

    EXPECT_FLOAT_EQ (0.001f, data.getDeltaX());
    EXPECT_FLOAT_EQ (0.0001f, data.getDeltaY());
}

TEST (MouseWheelDataTests, SetThenVerifyWithDifferentValue)
{
    MouseWheelData data (1.0f, 1.0f);
    data.setDeltaX (100.0f);

    EXPECT_FLOAT_EQ (100.0f, data.getDeltaX());
    EXPECT_FLOAT_EQ (1.0f, data.getDeltaY());
}

TEST (MouseWheelDataTests, SetNegativeValues)
{
    MouseWheelData data;
    data.setDeltaX (-42.0f);
    data.setDeltaY (-99.0f);

    EXPECT_FLOAT_EQ (-42.0f, data.getDeltaX());
    EXPECT_FLOAT_EQ (-99.0f, data.getDeltaY());
}
