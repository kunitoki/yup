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

TEST (KeyModifiersTests, DefaultConstruction)
{
    KeyModifiers modifiers;

    EXPECT_EQ (0, modifiers.getFlags());
    EXPECT_FALSE (modifiers.isShiftDown());
    EXPECT_FALSE (modifiers.isControlDown());
    EXPECT_FALSE (modifiers.isCommandDown());
    EXPECT_FALSE (modifiers.isAltDown());
    EXPECT_FALSE (modifiers.isSuperDown());
    EXPECT_FALSE (modifiers.isCapsLockDown());
    EXPECT_FALSE (modifiers.isNumLockDown());
}

TEST (KeyModifiersTests, ConstructWithFlags)
{
    KeyModifiers modifiers (0x0001);

    EXPECT_EQ (0x0001, modifiers.getFlags());
    EXPECT_TRUE (modifiers.isShiftDown());
    EXPECT_FALSE (modifiers.isControlDown());
}

TEST (KeyModifiersTests, ConstructWithMultipleFlags)
{
    KeyModifiers modifiers (KeyModifiers::shiftMask | KeyModifiers::controlMask);

    EXPECT_TRUE (modifiers.isShiftDown());
    EXPECT_TRUE (modifiers.isControlDown());
    EXPECT_FALSE (modifiers.isCommandDown());
}

TEST (KeyModifiersTests, ShiftDown)
{
    EXPECT_TRUE (KeyModifiers (KeyModifiers::shiftMask).isShiftDown());
    EXPECT_FALSE (KeyModifiers (0).isShiftDown());
}

TEST (KeyModifiersTests, ControlDown)
{
    EXPECT_TRUE (KeyModifiers (KeyModifiers::controlMask).isControlDown());
    EXPECT_FALSE (KeyModifiers (0).isControlDown());
}

TEST (KeyModifiersTests, CommandDown)
{
    EXPECT_TRUE (KeyModifiers (KeyModifiers::commandMask).isCommandDown());
    EXPECT_FALSE (KeyModifiers (0).isCommandDown());
}

TEST (KeyModifiersTests, AltDown)
{
    EXPECT_TRUE (KeyModifiers (KeyModifiers::altMask).isAltDown());
    EXPECT_FALSE (KeyModifiers (0).isAltDown());
}

TEST (KeyModifiersTests, SuperDown)
{
    EXPECT_TRUE (KeyModifiers (KeyModifiers::superMask).isSuperDown());
    EXPECT_FALSE (KeyModifiers (0).isSuperDown());
}

TEST (KeyModifiersTests, CapsLockDown)
{
    EXPECT_TRUE (KeyModifiers (KeyModifiers::capsLockMask).isCapsLockDown());
    EXPECT_FALSE (KeyModifiers (0).isCapsLockDown());
}

TEST (KeyModifiersTests, NumLockDown)
{
    EXPECT_TRUE (KeyModifiers (KeyModifiers::numLockMask).isNumLockDown());
    EXPECT_FALSE (KeyModifiers (0).isNumLockDown());
}

TEST (KeyModifiersTests, AllModifiers)
{
    KeyModifiers all (KeyModifiers::shiftMask | KeyModifiers::controlMask | KeyModifiers::commandMask | KeyModifiers::altMask | KeyModifiers::superMask | KeyModifiers::capsLockMask | KeyModifiers::numLockMask);

    EXPECT_TRUE (all.isShiftDown());
    EXPECT_TRUE (all.isControlDown());
    EXPECT_TRUE (all.isCommandDown());
    EXPECT_TRUE (all.isAltDown());
    EXPECT_TRUE (all.isSuperDown());
    EXPECT_TRUE (all.isCapsLockDown());
    EXPECT_TRUE (all.isNumLockDown());
}

TEST (KeyModifiersTests, GetFlags)
{
    EXPECT_EQ (0, KeyModifiers().getFlags());
    EXPECT_EQ (0x0002, KeyModifiers (0x0002).getFlags());
    EXPECT_EQ (0x000F, KeyModifiers (0x000F).getFlags());
}

TEST (KeyModifiersTests, WithFlags)
{
    KeyModifiers modifiers;

    KeyModifiers withShift = modifiers.withFlags (KeyModifiers::shiftMask);
    EXPECT_TRUE (withShift.isShiftDown());
    EXPECT_FALSE (modifiers.isShiftDown());
}

TEST (KeyModifiersTests, WithFlagsChaining)
{
    KeyModifiers modifiers;

    KeyModifiers result = modifiers.withFlags (KeyModifiers::shiftMask)
                              .withFlags (KeyModifiers::controlMask);

    EXPECT_TRUE (result.isShiftDown());
    EXPECT_TRUE (result.isControlDown());
    EXPECT_FALSE (modifiers.isShiftDown());
    EXPECT_FALSE (modifiers.isControlDown());
}

TEST (KeyModifiersTests, WithFlagsPreservesExisting)
{
    KeyModifiers modifiers (KeyModifiers::shiftMask);
    KeyModifiers result = modifiers.withFlags (KeyModifiers::controlMask);

    EXPECT_TRUE (result.isShiftDown());
    EXPECT_TRUE (result.isControlDown());
}

TEST (KeyModifiersTests, WithoutFlags)
{
    KeyModifiers modifiers (KeyModifiers::shiftMask | KeyModifiers::controlMask);
    KeyModifiers result = modifiers.withoutFlags (KeyModifiers::shiftMask);

    EXPECT_FALSE (result.isShiftDown());
    EXPECT_TRUE (result.isControlDown());
    EXPECT_TRUE (modifiers.isShiftDown());
}

TEST (KeyModifiersTests, WithoutFlagsAllModifiers)
{
    KeyModifiers modifiers (KeyModifiers::shiftMask | KeyModifiers::altMask);
    KeyModifiers result = modifiers.withoutFlags (KeyModifiers::shiftMask | KeyModifiers::altMask);

    EXPECT_EQ (0, result.getFlags());
    EXPECT_FALSE (result.isShiftDown());
    EXPECT_FALSE (result.isAltDown());
}

TEST (KeyModifiersTests, WithoutFlagsNonExisting)
{
    KeyModifiers modifiers (KeyModifiers::shiftMask);
    KeyModifiers result = modifiers.withoutFlags (KeyModifiers::controlMask);

    EXPECT_TRUE (result.isShiftDown());
    EXPECT_EQ (modifiers.getFlags(), result.getFlags());
}

TEST (KeyModifiersTests, TestFlagsSingle)
{
    KeyModifiers modifiers (KeyModifiers::shiftMask);
    EXPECT_TRUE (modifiers.testFlags (KeyModifiers::shiftMask));
    EXPECT_FALSE (modifiers.testFlags (KeyModifiers::controlMask));
}

TEST (KeyModifiersTests, TestFlagsMultiple)
{
    KeyModifiers modifiers (KeyModifiers::shiftMask | KeyModifiers::controlMask);
    EXPECT_TRUE (modifiers.testFlags (KeyModifiers::shiftMask));
    EXPECT_TRUE (modifiers.testFlags (KeyModifiers::controlMask));
    EXPECT_TRUE (modifiers.testFlags (KeyModifiers::shiftMask | KeyModifiers::controlMask));
}

TEST (KeyModifiersTests, Equality)
{
    EXPECT_TRUE (KeyModifiers (0x0001) == KeyModifiers (0x0001));
    EXPECT_TRUE (KeyModifiers() == KeyModifiers (0));
    EXPECT_FALSE (KeyModifiers (0x0001) == KeyModifiers (0x0002));
}

TEST (KeyModifiersTests, Inequality)
{
    EXPECT_TRUE (KeyModifiers (0x0001) != KeyModifiers (0x0002));
    EXPECT_FALSE (KeyModifiers (0x0001) != KeyModifiers (0x0001));
}

TEST (KeyModifiersTests, MaskValuesAreDistinct)
{
    const std::array<int, 7> masks = {
        KeyModifiers::shiftMask,
        KeyModifiers::controlMask,
        KeyModifiers::commandMask,
        KeyModifiers::altMask,
        KeyModifiers::superMask,
        KeyModifiers::capsLockMask,
        KeyModifiers::numLockMask
    };

    for (size_t i = 0; i < masks.size(); ++i)
    {
        for (size_t j = 0; j < masks.size(); ++j)
        {
            if (i != j)
                EXPECT_FALSE (masks[i] & masks[j]);
        }
    }
}

TEST (KeyModifiersTests, MaskValuesAreCorrect)
{
    EXPECT_EQ (0x0001, KeyModifiers::shiftMask);
    EXPECT_EQ (0x0002, KeyModifiers::controlMask);
    EXPECT_EQ (0x0004, KeyModifiers::commandMask);
    EXPECT_EQ (0x0008, KeyModifiers::altMask);
    EXPECT_EQ (0x0010, KeyModifiers::superMask);
    EXPECT_EQ (0x0020, KeyModifiers::capsLockMask);
    EXPECT_EQ (0x0040, KeyModifiers::numLockMask);
}

TEST (KeyModifiersTests, CopyConstructor)
{
    KeyModifiers original (KeyModifiers::shiftMask | KeyModifiers::altMask);
    KeyModifiers copy (original);

    EXPECT_EQ (original.getFlags(), copy.getFlags());
    EXPECT_TRUE (copy.isShiftDown());
    EXPECT_TRUE (copy.isAltDown());
    EXPECT_FALSE (copy.isControlDown());
}

TEST (KeyModifiersTests, CopyAssignment)
{
    KeyModifiers original (KeyModifiers::superMask);
    KeyModifiers copy;
    copy = original;

    EXPECT_EQ (original.getFlags(), copy.getFlags());
    EXPECT_TRUE (copy.isSuperDown());
}
