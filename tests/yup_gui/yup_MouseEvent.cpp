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

TEST (MouseEventTests, DefaultConstruction)
{
    MouseEvent event;

    EXPECT_FALSE (event.isLeftButtonDown());
    EXPECT_FALSE (event.isMiddleButtonDown());
    EXPECT_FALSE (event.isRightButtonDown());
    EXPECT_FALSE (event.isAnyButtonDown());
    EXPECT_EQ (MouseEvent::noButtons, event.getButtons());
    EXPECT_EQ (KeyModifiers(), event.getModifiers());
    EXPECT_EQ (Point<float>(), event.getPosition());
    EXPECT_EQ (nullptr, event.getSourceComponent());
    EXPECT_EQ (Point<float>(), event.getLastMouseDownPosition());
}

TEST (MouseEventTests, ConstructWithButtonsModifiersAndPosition)
{
    KeyModifiers modifiers (KeyModifiers::shiftMask);
    Point<float> position (10.0f, 20.0f);
    MouseEvent event (MouseEvent::leftButton, modifiers, position);

    EXPECT_TRUE (event.isLeftButtonDown());
    EXPECT_FALSE (event.isMiddleButtonDown());
    EXPECT_FALSE (event.isRightButtonDown());
    EXPECT_TRUE (event.isAnyButtonDown());
    EXPECT_EQ (MouseEvent::leftButton, event.getButtons());
    EXPECT_TRUE (event.getModifiers().isShiftDown());
    EXPECT_EQ (position, event.getPosition());
    EXPECT_EQ (nullptr, event.getSourceComponent());
}

TEST (MouseEventTests, ConstructWithSourceComponent)
{
    Component comp;
    Point<float> position (10.0f, 20.0f);
    MouseEvent event (MouseEvent::rightButton, KeyModifiers(), position, &comp);

    EXPECT_TRUE (event.isRightButtonDown());
    EXPECT_EQ (position, event.getPosition());
    EXPECT_EQ (&comp, event.getSourceComponent());
}

TEST (MouseEventTests, CopyConstructor)
{
    KeyModifiers modifiers (KeyModifiers::controlMask);
    Point<float> position (5.0f, 15.0f);
    MouseEvent original (MouseEvent::middleButton, modifiers, position);
    MouseEvent copy (original);

    EXPECT_TRUE (copy.isMiddleButtonDown());
    EXPECT_TRUE (copy.getModifiers().isControlDown());
    EXPECT_EQ (position, copy.getPosition());
    EXPECT_EQ (nullptr, copy.getSourceComponent());
}

TEST (MouseEventTests, CopyAssignment)
{
    Point<float> position (42.0f, 99.0f);
    MouseEvent original (MouseEvent::leftButton, KeyModifiers(), position);
    MouseEvent copy;
    copy = original;

    EXPECT_TRUE (copy.isLeftButtonDown());
    EXPECT_EQ (position, copy.getPosition());
}

TEST (MouseEventTests, MoveConstructor)
{
    Point<float> position (17.0f, 23.0f);
    MouseEvent original (MouseEvent::rightButton, KeyModifiers(), position);
    MouseEvent moved (std::move (original));

    EXPECT_TRUE (moved.isRightButtonDown());
    EXPECT_EQ (position, moved.getPosition());
}

TEST (MouseEventTests, MoveAssignment)
{
    Point<float> position (88.0f, 77.0f);
    MouseEvent original (MouseEvent::middleButton, KeyModifiers(), position);
    MouseEvent moved;
    moved = std::move (original);

    EXPECT_TRUE (moved.isMiddleButtonDown());
    EXPECT_EQ (position, moved.getPosition());
}

TEST (MouseEventTests, IsLeftButtonDown)
{
    EXPECT_TRUE (MouseEvent (MouseEvent::leftButton, KeyModifiers(), Point<float>()).isLeftButtonDown());
    EXPECT_FALSE (MouseEvent (MouseEvent::rightButton, KeyModifiers(), Point<float>()).isLeftButtonDown());
    EXPECT_FALSE (MouseEvent().isLeftButtonDown());
}

TEST (MouseEventTests, IsMiddleButtonDown)
{
    EXPECT_TRUE (MouseEvent (MouseEvent::middleButton, KeyModifiers(), Point<float>()).isMiddleButtonDown());
    EXPECT_FALSE (MouseEvent (MouseEvent::leftButton, KeyModifiers(), Point<float>()).isMiddleButtonDown());
}

TEST (MouseEventTests, IsRightButtonDown)
{
    EXPECT_TRUE (MouseEvent (MouseEvent::rightButton, KeyModifiers(), Point<float>()).isRightButtonDown());
    EXPECT_FALSE (MouseEvent (MouseEvent::leftButton, KeyModifiers(), Point<float>()).isRightButtonDown());
}

TEST (MouseEventTests, IsAnyButtonDown)
{
    EXPECT_TRUE (MouseEvent (MouseEvent::leftButton, KeyModifiers(), Point<float>()).isAnyButtonDown());
    EXPECT_TRUE (MouseEvent (MouseEvent::middleButton, KeyModifiers(), Point<float>()).isAnyButtonDown());
    EXPECT_TRUE (MouseEvent (MouseEvent::rightButton, KeyModifiers(), Point<float>()).isAnyButtonDown());
    EXPECT_FALSE (MouseEvent().isAnyButtonDown());
}

TEST (MouseEventTests, MultipleButtonsDown)
{
    auto buttons = static_cast<MouseEvent::Buttons> (MouseEvent::leftButton | MouseEvent::rightButton);
    MouseEvent event (buttons, KeyModifiers(), Point<float>());

    EXPECT_TRUE (event.isLeftButtonDown());
    EXPECT_TRUE (event.isRightButtonDown());
    EXPECT_FALSE (event.isMiddleButtonDown());
    EXPECT_TRUE (event.isAnyButtonDown());
    EXPECT_EQ (buttons, event.getButtons());
}

TEST (MouseEventTests, AllButtonsDown)
{
    auto buttons = static_cast<MouseEvent::Buttons> (
        MouseEvent::leftButton | MouseEvent::middleButton | MouseEvent::rightButton);
    MouseEvent event (buttons, KeyModifiers(), Point<float>());

    EXPECT_TRUE (event.isLeftButtonDown());
    EXPECT_TRUE (event.isMiddleButtonDown());
    EXPECT_TRUE (event.isRightButtonDown());
    EXPECT_TRUE (event.isAnyButtonDown());
}

TEST (MouseEventTests, GetButtons)
{
    EXPECT_EQ (MouseEvent::leftButton,
               MouseEvent (MouseEvent::leftButton, KeyModifiers(), Point<float>()).getButtons());
}

TEST (MouseEventTests, WithButtonsAddsButton)
{
    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float>());
    MouseEvent modified = event.withButtons (MouseEvent::rightButton);

    EXPECT_TRUE (modified.isLeftButtonDown());
    EXPECT_TRUE (modified.isRightButtonDown());
    EXPECT_FALSE (modified.isMiddleButtonDown());
    EXPECT_TRUE (event.isLeftButtonDown());
    EXPECT_FALSE (event.isRightButtonDown());
}

TEST (MouseEventTests, WithButtonsFromNone)
{
    MouseEvent event;
    MouseEvent modified = event.withButtons (MouseEvent::leftButton);

    EXPECT_TRUE (modified.isLeftButtonDown());
    EXPECT_FALSE (event.isLeftButtonDown());
}

TEST (MouseEventTests, WithoutButtonsRemovesButton)
{
    auto buttons = static_cast<MouseEvent::Buttons> (MouseEvent::leftButton | MouseEvent::rightButton);
    MouseEvent event (buttons, KeyModifiers(), Point<float>());
    MouseEvent modified = event.withoutButtons (MouseEvent::leftButton);

    EXPECT_FALSE (modified.isLeftButtonDown());
    EXPECT_TRUE (modified.isRightButtonDown());
    EXPECT_TRUE (event.isLeftButtonDown());
}

TEST (MouseEventTests, WithoutButtonsRemoveAll)
{
    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float>());
    MouseEvent modified = event.withoutButtons (MouseEvent::leftButton);

    EXPECT_FALSE (modified.isAnyButtonDown());
    EXPECT_TRUE (event.isAnyButtonDown());
}

TEST (MouseEventTests, GetModifiers)
{
    KeyModifiers modifiers (KeyModifiers::shiftMask | KeyModifiers::altMask);
    MouseEvent event (MouseEvent::noButtons, modifiers, Point<float>());

    EXPECT_TRUE (event.getModifiers().isShiftDown());
    EXPECT_TRUE (event.getModifiers().isAltDown());
    EXPECT_FALSE (event.getModifiers().isControlDown());
}

TEST (MouseEventTests, WithModifiersReturnsNewObject)
{
    MouseEvent event (MouseEvent::noButtons, KeyModifiers(), Point<float>());
    KeyModifiers newModifiers (KeyModifiers::commandMask);
    MouseEvent modified = event.withModifiers (newModifiers);

    EXPECT_TRUE (modified.getModifiers().isCommandDown());
    EXPECT_FALSE (event.getModifiers().isCommandDown());
}

TEST (MouseEventTests, WithModifiersPreservesOtherState)
{
    Point<float> position (1.0f, 2.0f);
    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), position);
    KeyModifiers newModifiers (KeyModifiers::shiftMask);
    MouseEvent modified = event.withModifiers (newModifiers);

    EXPECT_TRUE (modified.isLeftButtonDown());
    EXPECT_EQ (position, modified.getPosition());
    EXPECT_TRUE (modified.getModifiers().isShiftDown());
}

TEST (MouseEventTests, GetPosition)
{
    Point<float> position (30.0f, 40.0f);
    MouseEvent event (MouseEvent::noButtons, KeyModifiers(), position);

    EXPECT_FLOAT_EQ (30.0f, event.getPosition().getX());
    EXPECT_FLOAT_EQ (40.0f, event.getPosition().getY());
}

TEST (MouseEventTests, GetScreenPositionWithoutSourceComponent)
{
    Point<float> position (50.0f, 60.0f);
    MouseEvent event (MouseEvent::noButtons, KeyModifiers(), position);

    EXPECT_EQ (position, event.getScreenPosition());
}

TEST (MouseEventTests, WithPositionReturnsNewObject)
{
    MouseEvent event (MouseEvent::noButtons, KeyModifiers(), Point<float> (10.0f, 20.0f));
    Point<float> newPosition (100.0f, 200.0f);
    MouseEvent modified = event.withPosition (newPosition);

    EXPECT_EQ (newPosition, modified.getPosition());
    EXPECT_FLOAT_EQ (10.0f, event.getPosition().getX());
    EXPECT_FLOAT_EQ (20.0f, event.getPosition().getY());
}

TEST (MouseEventTests, WithTranslatedPosition)
{
    MouseEvent event (MouseEvent::noButtons, KeyModifiers(), Point<float> (10.0f, 20.0f));
    MouseEvent modified = event.withTranslatedPosition (Point<float> (5.0f, -3.0f));

    EXPECT_FLOAT_EQ (15.0f, modified.getPosition().getX());
    EXPECT_FLOAT_EQ (17.0f, modified.getPosition().getY());
}

TEST (MouseEventTests, WithRelativePositionToNullTarget)
{
    MouseEvent event (MouseEvent::noButtons, KeyModifiers(), Point<float> (10.0f, 20.0f));
    MouseEvent result = event.withRelativePositionTo (nullptr);

    EXPECT_EQ (event.getPosition(), result.getPosition());
    EXPECT_EQ (event, result);
}

TEST (MouseEventTests, GetLastMouseDownPosition)
{
    Point<float> lastPos (15.0f, 25.0f);
    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (10.0f, 20.0f), lastPos, Time(), nullptr);

    EXPECT_EQ (lastPos, event.getLastMouseDownPosition());
}

TEST (MouseEventTests, DefaultLastMouseDownPosition)
{
    MouseEvent event;

    EXPECT_EQ (Point<float>(), event.getLastMouseDownPosition());
}

TEST (MouseEventTests, WithLastMouseDownPosition)
{
    MouseEvent event (MouseEvent::noButtons, KeyModifiers(), Point<float>());
    Point<float> newLastPos (42.0f, 99.0f);
    MouseEvent modified = event.withLastMouseDownPosition (newLastPos);

    EXPECT_EQ (newLastPos, modified.getLastMouseDownPosition());
    EXPECT_EQ (Point<float>(), event.getLastMouseDownPosition());
}

TEST (MouseEventTests, GetLastMouseDownTime)
{
    Time t = Time::getCurrentTime();
    MouseEvent event (MouseEvent::noButtons, KeyModifiers(), Point<float>(), Point<float>(), t, nullptr);

    EXPECT_EQ (t.toMilliseconds(), event.getLastMouseDownTime().toMilliseconds());
}

TEST (MouseEventTests, WithLastMouseDownTime)
{
    MouseEvent event (MouseEvent::noButtons, KeyModifiers(), Point<float>());
    Time newTime = Time::getCurrentTime();
    MouseEvent modified = event.withLastMouseDownTime (newTime);

    EXPECT_EQ (newTime.toMilliseconds(), modified.getLastMouseDownTime().toMilliseconds());
}

TEST (MouseEventTests, GetSourceComponent)
{
    MouseEvent event;
    EXPECT_EQ (nullptr, event.getSourceComponent());
}

TEST (MouseEventTests, ConstructWithSourceComponentSetsIt)
{
    Component comp;
    MouseEvent event (MouseEvent::noButtons, KeyModifiers(), Point<float>(), &comp);

    EXPECT_EQ (&comp, event.getSourceComponent());
}

TEST (MouseEventTests, WithSourceComponent)
{
    MouseEvent event;
    Component comp;
    MouseEvent modified = event.withSourceComponent (&comp);

    EXPECT_EQ (&comp, modified.getSourceComponent());
    EXPECT_EQ (nullptr, event.getSourceComponent());
}

TEST (MouseEventTests, WithSourceComponentSetToNull)
{
    Component comp;
    MouseEvent event (MouseEvent::noButtons, KeyModifiers(), Point<float>(), &comp);
    MouseEvent modified = event.withSourceComponent (nullptr);

    EXPECT_EQ (nullptr, modified.getSourceComponent());
    EXPECT_EQ (&comp, event.getSourceComponent());
}

TEST (MouseEventTests, EqualitySame)
{
    Point<float> position (10.0f, 20.0f);
    MouseEvent a (MouseEvent::leftButton, KeyModifiers(), position);
    MouseEvent b (MouseEvent::leftButton, KeyModifiers(), position);

    EXPECT_TRUE (a == b);
    EXPECT_FALSE (a != b);
}

TEST (MouseEventTests, EqualityDifferentButtons)
{
    Point<float> position (10.0f, 20.0f);
    MouseEvent a (MouseEvent::leftButton, KeyModifiers(), position);
    MouseEvent b (MouseEvent::rightButton, KeyModifiers(), position);

    EXPECT_FALSE (a == b);
    EXPECT_TRUE (a != b);
}

TEST (MouseEventTests, EqualityDifferentModifiers)
{
    Point<float> position (10.0f, 20.0f);
    MouseEvent a (MouseEvent::leftButton, KeyModifiers(), position);
    MouseEvent b (MouseEvent::leftButton, KeyModifiers (KeyModifiers::shiftMask), position);

    EXPECT_FALSE (a == b);
}

TEST (MouseEventTests, EqualityDifferentPosition)
{
    MouseEvent a (MouseEvent::leftButton, KeyModifiers(), Point<float> (10.0f, 20.0f));
    MouseEvent b (MouseEvent::leftButton, KeyModifiers(), Point<float> (30.0f, 40.0f));

    EXPECT_FALSE (a == b);
}

TEST (MouseEventTests, EqualityDifferentLastMouseDownPosition)
{
    Point<float> position (10.0f, 20.0f);
    MouseEvent a (MouseEvent::leftButton, KeyModifiers(), position, Point<float> (1.0f, 1.0f), Time(), nullptr);
    MouseEvent b (MouseEvent::leftButton, KeyModifiers(), position, Point<float> (2.0f, 2.0f), Time(), nullptr);

    EXPECT_FALSE (a == b);
}

TEST (MouseEventTests, EqualityDifferentLastMouseDownTime)
{
    Point<float> position (10.0f, 20.0f);
    MouseEvent a (MouseEvent::leftButton, KeyModifiers(), position, Point<float>(), Time (100), nullptr);
    MouseEvent b (MouseEvent::leftButton, KeyModifiers(), position, Point<float>(), Time (200), nullptr);

    EXPECT_FALSE (a == b);
}

TEST (MouseEventTests, EqualityDifferentSourceComponent)
{
    Component comp1, comp2;
    Point<float> position (10.0f, 20.0f);
    MouseEvent a (MouseEvent::leftButton, KeyModifiers(), position, &comp1);
    MouseEvent b (MouseEvent::leftButton, KeyModifiers(), position, &comp2);

    EXPECT_FALSE (a == b);
}

TEST (MouseEventTests, EqualityWithSelf)
{
    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (5.0f, 5.0f));
    EXPECT_TRUE (event == event);
}

TEST (MouseEventTests, EqualityDefaultConstructed)
{
    EXPECT_TRUE (MouseEvent() == MouseEvent());
}

TEST (MouseEventTests, ButtonsEnumValues)
{
    EXPECT_EQ (0x0000, MouseEvent::noButtons);
    EXPECT_EQ (0x0001, MouseEvent::leftButton);
    EXPECT_EQ (0x0002, MouseEvent::middleButton);
    EXPECT_EQ (0x0004, MouseEvent::rightButton);
    EXPECT_EQ (0x0007, MouseEvent::allButtons);
}

TEST (MouseEventTests, ButtonValuesAreBitmaskDistinct)
{
    EXPECT_FALSE (MouseEvent::leftButton & MouseEvent::rightButton);
    EXPECT_FALSE (MouseEvent::leftButton & MouseEvent::middleButton);
    EXPECT_FALSE (MouseEvent::rightButton & MouseEvent::middleButton);
}

TEST (MouseEventTests, WithButtonsPreservesOtherFields)
{
    Point<float> position (3.0f, 7.0f);
    KeyModifiers modifiers (KeyModifiers::altMask);
    MouseEvent event (MouseEvent::noButtons, modifiers, position);
    MouseEvent modified = event.withButtons (MouseEvent::leftButton);

    EXPECT_EQ (position, modified.getPosition());
    EXPECT_TRUE (modified.getModifiers().isAltDown());
    EXPECT_TRUE (modified.isLeftButtonDown());
}

TEST (MouseEventTests, WithoutButtonsPreservesOtherFields)
{
    Point<float> position (3.0f, 7.0f);
    KeyModifiers modifiers (KeyModifiers::altMask);
    auto buttons = static_cast<MouseEvent::Buttons> (MouseEvent::leftButton | MouseEvent::rightButton);
    MouseEvent event (buttons, modifiers, position);
    MouseEvent modified = event.withoutButtons (MouseEvent::rightButton);

    EXPECT_EQ (position, modified.getPosition());
    EXPECT_TRUE (modified.getModifiers().isAltDown());
    EXPECT_TRUE (modified.isLeftButtonDown());
    EXPECT_FALSE (modified.isRightButtonDown());
}
