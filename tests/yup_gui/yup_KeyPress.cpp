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

TEST (KeyPressTests, DefaultConstruction)
{
    KeyPress press;

    EXPECT_EQ (0, press.getKey());
    EXPECT_EQ (KeyModifiers(), press.getModifiers());
    EXPECT_EQ (U'\0', press.getTextCharacter());
}

TEST (KeyPressTests, ConstructWithKeyCode)
{
    KeyPress press (KeyPress::spaceKey);

    EXPECT_EQ (KeyPress::spaceKey, press.getKey());
    EXPECT_EQ (KeyModifiers(), press.getModifiers());
    EXPECT_EQ (U'\0', press.getTextCharacter());
}

TEST (KeyPressTests, ConstructWithKeyCodeAndModifiers)
{
    KeyModifiers modifiers (KeyModifiers::shiftMask);
    KeyPress press (KeyPress::textAKey, modifiers);

    EXPECT_EQ (KeyPress::textAKey, press.getKey());
    EXPECT_TRUE (press.getModifiers().isShiftDown());
    EXPECT_EQ (U'\0', press.getTextCharacter());
}

TEST (KeyPressTests, ConstructWithKeyCodeModifiersAndScancode)
{
    KeyModifiers modifiers (KeyModifiers::shiftMask);
    KeyPress press (KeyPress::textAKey, modifiers, U'A');

    EXPECT_EQ (KeyPress::textAKey, press.getKey());
    EXPECT_TRUE (press.getModifiers().isShiftDown());
    EXPECT_EQ (U'A', press.getTextCharacter());
}

TEST (KeyPressTests, CopyConstructor)
{
    KeyModifiers modifiers (KeyModifiers::controlMask);
    KeyPress original (KeyPress::textBKey, modifiers, U'b');
    KeyPress copy (original);

    EXPECT_EQ (original.getKey(), copy.getKey());
    EXPECT_EQ (original.getModifiers(), copy.getModifiers());
    EXPECT_EQ (original.getTextCharacter(), copy.getTextCharacter());
}

TEST (KeyPressTests, CopyAssignment)
{
    KeyPress original (KeyPress::enterKey);
    KeyPress copy;
    copy = original;

    EXPECT_EQ (original.getKey(), copy.getKey());
    EXPECT_EQ (original.getModifiers(), copy.getModifiers());
}

TEST (KeyPressTests, MoveConstructor)
{
    KeyPress original (KeyPress::escapeKey);
    KeyPress moved (std::move (original));

    EXPECT_EQ (KeyPress::escapeKey, moved.getKey());
}

TEST (KeyPressTests, MoveAssignment)
{
    KeyPress original (KeyPress::tabKey);
    KeyPress moved;
    moved = std::move (original);

    EXPECT_EQ (KeyPress::tabKey, moved.getKey());
}

TEST (KeyPressTests, EqualitySame)
{
    EXPECT_TRUE (KeyPress (KeyPress::textAKey) == KeyPress (KeyPress::textAKey));
}

TEST (KeyPressTests, EqualityDifferentKey)
{
    EXPECT_FALSE (KeyPress (KeyPress::textAKey) == KeyPress (KeyPress::textBKey));
}

TEST (KeyPressTests, EqualityDifferentModifiers)
{
    EXPECT_FALSE (KeyPress (KeyPress::textAKey, KeyModifiers (KeyModifiers::shiftMask))
                  == KeyPress (KeyPress::textAKey));
}

TEST (KeyPressTests, EqualityDifferentScancode)
{
    EXPECT_FALSE (KeyPress (KeyPress::textAKey, KeyModifiers(), U'a')
                  == KeyPress (KeyPress::textAKey, KeyModifiers(), U'A'));
}

TEST (KeyPressTests, EqualityWithFullConstructor)
{
    KeyPress a (KeyPress::textAKey, KeyModifiers (KeyModifiers::shiftMask), U'A');
    KeyPress b (KeyPress::textAKey, KeyModifiers (KeyModifiers::shiftMask), U'A');

    EXPECT_TRUE (a == b);
}

TEST (KeyPressTests, Inequality)
{
    EXPECT_TRUE (KeyPress (KeyPress::textAKey) != KeyPress (KeyPress::textBKey));
    EXPECT_FALSE (KeyPress (KeyPress::textAKey) != KeyPress (KeyPress::textAKey));
}

TEST (KeyPressTests, EqualityWithSelf)
{
    KeyPress press (KeyPress::f1Key);
    EXPECT_TRUE (press == press);
    EXPECT_FALSE (press != press);
}

TEST (KeyPressTests, AllKeyConstantsAreUnique)
{
    std::set<int> keys;
    keys.insert (KeyPress::spaceKey);
    keys.insert (KeyPress::apostropheKey);
    keys.insert (KeyPress::commaKey);
    keys.insert (KeyPress::minusKey);
    keys.insert (KeyPress::periodKey);
    keys.insert (KeyPress::slashKey);
    keys.insert (KeyPress::number0Key);
    keys.insert (KeyPress::number1Key);
    keys.insert (KeyPress::number2Key);
    keys.insert (KeyPress::number3Key);
    keys.insert (KeyPress::number4Key);
    keys.insert (KeyPress::number5Key);
    keys.insert (KeyPress::number6Key);
    keys.insert (KeyPress::number7Key);
    keys.insert (KeyPress::number8Key);
    keys.insert (KeyPress::number9Key);
    keys.insert (KeyPress::semicolonKey);
    keys.insert (KeyPress::equalKey);
    keys.insert (KeyPress::textAKey);
    keys.insert (KeyPress::textBKey);
    keys.insert (KeyPress::textCKey);
    keys.insert (KeyPress::textDKey);
    keys.insert (KeyPress::textEKey);
    keys.insert (KeyPress::textFKey);
    keys.insert (KeyPress::textGKey);
    keys.insert (KeyPress::textHKey);
    keys.insert (KeyPress::textIKey);
    keys.insert (KeyPress::textJKey);
    keys.insert (KeyPress::textKKey);
    keys.insert (KeyPress::textLKey);
    keys.insert (KeyPress::textMKey);
    keys.insert (KeyPress::textNKey);
    keys.insert (KeyPress::textOKey);
    keys.insert (KeyPress::textPKey);
    keys.insert (KeyPress::textQKey);
    keys.insert (KeyPress::textRKey);
    keys.insert (KeyPress::textSKey);
    keys.insert (KeyPress::textTKey);
    keys.insert (KeyPress::textUKey);
    keys.insert (KeyPress::textVKey);
    keys.insert (KeyPress::textWKey);
    keys.insert (KeyPress::textXKey);
    keys.insert (KeyPress::textYKey);
    keys.insert (KeyPress::textZKey);
    keys.insert (KeyPress::leftBracketKey);
    keys.insert (KeyPress::backslashKey);
    keys.insert (KeyPress::rightBracketKey);
    keys.insert (KeyPress::graveAccentKey);
    keys.insert (KeyPress::world1Key);
    keys.insert (KeyPress::world2Key);
    keys.insert (KeyPress::escapeKey);
    keys.insert (KeyPress::enterKey);
    keys.insert (KeyPress::tabKey);
    keys.insert (KeyPress::backspaceKey);
    keys.insert (KeyPress::insertKey);
    keys.insert (KeyPress::deleteKey);
    keys.insert (KeyPress::rightKey);
    keys.insert (KeyPress::leftKey);
    keys.insert (KeyPress::downKey);
    keys.insert (KeyPress::upKey);
    keys.insert (KeyPress::pageUpKey);
    keys.insert (KeyPress::pageDownKey);
    keys.insert (KeyPress::homeKey);
    keys.insert (KeyPress::endKey);
    keys.insert (KeyPress::capsLockKey);
    keys.insert (KeyPress::scrollLockKey);
    keys.insert (KeyPress::numLockKey);
    keys.insert (KeyPress::printScreenKey);
    keys.insert (KeyPress::pauseKey);
    keys.insert (KeyPress::f1Key);
    keys.insert (KeyPress::f2Key);
    keys.insert (KeyPress::f3Key);
    keys.insert (KeyPress::f4Key);
    keys.insert (KeyPress::f5Key);
    keys.insert (KeyPress::f6Key);
    keys.insert (KeyPress::f7Key);
    keys.insert (KeyPress::f8Key);
    keys.insert (KeyPress::f9Key);
    keys.insert (KeyPress::f10Key);
    keys.insert (KeyPress::f11Key);
    keys.insert (KeyPress::f12Key);
    keys.insert (KeyPress::f13Key);
    keys.insert (KeyPress::f14Key);
    keys.insert (KeyPress::f15Key);
    keys.insert (KeyPress::f16Key);
    keys.insert (KeyPress::f17Key);
    keys.insert (KeyPress::f18Key);
    keys.insert (KeyPress::f19Key);
    keys.insert (KeyPress::f20Key);
    keys.insert (KeyPress::f21Key);
    keys.insert (KeyPress::f22Key);
    keys.insert (KeyPress::f23Key);
    keys.insert (KeyPress::f24Key);
    keys.insert (KeyPress::f25Key);
    keys.insert (KeyPress::kp0Key);
    keys.insert (KeyPress::kp1Key);
    keys.insert (KeyPress::kp2Key);
    keys.insert (KeyPress::kp3Key);
    keys.insert (KeyPress::kp4Key);
    keys.insert (KeyPress::kp5Key);
    keys.insert (KeyPress::kp6Key);
    keys.insert (KeyPress::kp7Key);
    keys.insert (KeyPress::kp8Key);
    keys.insert (KeyPress::kp9Key);
    keys.insert (KeyPress::kpDecimalKey);
    keys.insert (KeyPress::kpDivideKey);
    keys.insert (KeyPress::kpMultiplyKey);
    keys.insert (KeyPress::kpSubtractKey);
    keys.insert (KeyPress::kpAddKey);
    keys.insert (KeyPress::kpEnterKey);
    keys.insert (KeyPress::kpEqualKey);
    keys.insert (KeyPress::leftShiftKey);
    keys.insert (KeyPress::leftControlKey);
    keys.insert (KeyPress::leftAltKey);
    keys.insert (KeyPress::leftSuperKey);
    keys.insert (KeyPress::rightShiftKey);
    keys.insert (KeyPress::rightControlKey);
    keys.insert (KeyPress::rightAltKey);
    keys.insert (KeyPress::rightSuperKey);
    keys.insert (KeyPress::menuKey);

    EXPECT_GE (keys.size(), 100u);
}

TEST (KeyPressTests, TextCharacterForNonPrintableKeys)
{
    KeyPress press (KeyPress::escapeKey);

    EXPECT_EQ (U'\0', press.getTextCharacter());
}

TEST (KeyPressTests, TextCharacterUnicode)
{
    KeyPress press (KeyPress::textAKey, KeyModifiers(), U'ع');

    EXPECT_EQ (U'ع', press.getTextCharacter());
}

TEST (KeyPressTests, ModifiersCombinations)
{
    KeyModifiers mods (KeyModifiers::shiftMask | KeyModifiers::controlMask);
    KeyPress press (KeyPress::textZKey, mods, U'Z');

    EXPECT_TRUE (press.getModifiers().isShiftDown());
    EXPECT_TRUE (press.getModifiers().isControlDown());
    EXPECT_FALSE (press.getModifiers().isAltDown());
}
