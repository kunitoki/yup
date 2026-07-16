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

TEST (MouseCursorTests, DefaultConstruction)
{
    MouseCursor cursor;

    EXPECT_EQ (MouseCursor::Type::Arrow, cursor.getType());
    EXPECT_EQ (MouseCursor::Type::Default, cursor.getType());
}

TEST (MouseCursorTests, ConstructWithType)
{
    EXPECT_EQ (MouseCursor::Type::Hand, MouseCursor (MouseCursor::Type::Hand).getType());
    EXPECT_EQ (MouseCursor::Type::Crosshair, MouseCursor (MouseCursor::Type::Crosshair).getType());
    EXPECT_EQ (MouseCursor::Type::Text, MouseCursor (MouseCursor::Type::Text).getType());
    EXPECT_EQ (MouseCursor::Type::Wait, MouseCursor (MouseCursor::Type::Wait).getType());
    EXPECT_EQ (MouseCursor::Type::WaitArrow, MouseCursor (MouseCursor::Type::WaitArrow).getType());
    EXPECT_EQ (MouseCursor::Type::Crossbones, MouseCursor (MouseCursor::Type::Crossbones).getType());
    EXPECT_EQ (MouseCursor::Type::ResizeLeftRight, MouseCursor (MouseCursor::Type::ResizeLeftRight).getType());
    EXPECT_EQ (MouseCursor::Type::ResizeUpDown, MouseCursor (MouseCursor::Type::ResizeUpDown).getType());
    EXPECT_EQ (MouseCursor::Type::ResizeTopLeftRightBottom, MouseCursor (MouseCursor::Type::ResizeTopLeftRightBottom).getType());
    EXPECT_EQ (MouseCursor::Type::ResizeBottomLeftRightTop, MouseCursor (MouseCursor::Type::ResizeBottomLeftRightTop).getType());
    EXPECT_EQ (MouseCursor::Type::ResizeAll, MouseCursor (MouseCursor::Type::ResizeAll).getType());
}

TEST (MouseCursorTests, ConstructWithNone)
{
    MouseCursor cursor (MouseCursor::Type::None);

    EXPECT_EQ (MouseCursor::Type::None, cursor.getType());
    EXPECT_NE (MouseCursor::Type::Arrow, cursor.getType());
}

TEST (MouseCursorTests, CopyConstructor)
{
    MouseCursor original (MouseCursor::Type::Hand);
    MouseCursor copy (original);

    EXPECT_EQ (MouseCursor::Type::Hand, copy.getType());
}

TEST (MouseCursorTests, CopyAssignment)
{
    MouseCursor original (MouseCursor::Type::Wait);
    MouseCursor copy;
    copy = original;

    EXPECT_EQ (MouseCursor::Type::Wait, copy.getType());
}

TEST (MouseCursorTests, MoveConstructor)
{
    MouseCursor original (MouseCursor::Type::Crosshair);
    MouseCursor moved (std::move (original));

    EXPECT_EQ (MouseCursor::Type::Crosshair, moved.getType());
}

TEST (MouseCursorTests, MoveAssignment)
{
    MouseCursor original (MouseCursor::Type::Text);
    MouseCursor moved;
    moved = std::move (original);

    EXPECT_EQ (MouseCursor::Type::Text, moved.getType());
}

TEST (MouseCursorTests, EqualitySameType)
{
    EXPECT_TRUE (MouseCursor (MouseCursor::Type::Hand) == MouseCursor (MouseCursor::Type::Hand));
    EXPECT_TRUE (MouseCursor() == MouseCursor (MouseCursor::Type::Default));
}

TEST (MouseCursorTests, EqualityDifferentType)
{
    EXPECT_FALSE (MouseCursor (MouseCursor::Type::Hand) == MouseCursor (MouseCursor::Type::Arrow));
    EXPECT_FALSE (MouseCursor (MouseCursor::Type::Text) == MouseCursor (MouseCursor::Type::Wait));
}

TEST (MouseCursorTests, Inequality)
{
    EXPECT_TRUE (MouseCursor (MouseCursor::Type::Hand) != MouseCursor (MouseCursor::Type::Arrow));
    EXPECT_FALSE (MouseCursor (MouseCursor::Type::Hand) != MouseCursor (MouseCursor::Type::Hand));
}

TEST (MouseCursorTests, EqualityWithSelf)
{
    MouseCursor cursor (MouseCursor::Type::ResizeAll);
    EXPECT_TRUE (cursor == cursor);
    EXPECT_FALSE (cursor != cursor);
}

TEST (MouseCursorTests, ArrowIsDefault)
{
    EXPECT_EQ (MouseCursor::Type::Arrow, MouseCursor::Type::Default);
}

TEST (MouseCursorTests, AllTypesAreDistinct)
{
    const std::array<MouseCursor::Type, 13> types = {
        MouseCursor::Type::None,
        MouseCursor::Type::Default,
        MouseCursor::Type::Text,
        MouseCursor::Type::Wait,
        MouseCursor::Type::WaitArrow,
        MouseCursor::Type::Hand,
        MouseCursor::Type::Crosshair,
        MouseCursor::Type::Crossbones,
        MouseCursor::Type::ResizeLeftRight,
        MouseCursor::Type::ResizeUpDown,
        MouseCursor::Type::ResizeTopLeftRightBottom,
        MouseCursor::Type::ResizeBottomLeftRightTop,
        MouseCursor::Type::ResizeAll
    };

    for (size_t i = 0; i < types.size(); ++i)
    {
        for (size_t j = i + 1; j < types.size(); ++j)
        {
            if (types[i] != MouseCursor::Type::Default || types[j] != MouseCursor::Type::Arrow)
                EXPECT_NE (MouseCursor (types[i]), MouseCursor (types[j]))
                    << "Types " << static_cast<int> (types[i]) << " and " << static_cast<int> (types[j]) << " should differ";
        }
    }
}

TEST (MouseCursorTests, ConstructWithDefaultExplicitly)
{
    MouseCursor cursor (MouseCursor::Type::Default);

    EXPECT_EQ (MouseCursor::Type::Default, cursor.getType());
}

TEST (MouseCursorTests, NoneNotEqualToDefault)
{
    EXPECT_NE (MouseCursor (MouseCursor::Type::None), MouseCursor());
}
