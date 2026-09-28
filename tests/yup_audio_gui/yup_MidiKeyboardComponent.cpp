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

#include <yup_audio_gui/yup_audio_gui.h>

#include <thread>
#include <utility>
#include <vector>

using namespace yup;

//==============================================================================
namespace
{

constexpr int kDefaultOctave = 3;
constexpr int kDefaultOctaveOffset = 12 * kDefaultOctave;

} // namespace

//==============================================================================
class MidiKeyboardComponentTests : public ::testing::Test
{
protected:
    struct NoteEvent
    {
        int channel;
        int note;
        float velocity;
    };

    class TestListener : public MidiKeyboardState::Listener
    {
    public:
        void handleNoteOn (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
        {
            noteOnCalls.push_back ({ midiChannel, midiNoteNumber, velocity });
        }

        void handleNoteOff (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
        {
            noteOffCalls.push_back ({ midiChannel, midiNoteNumber, velocity });
        }

        std::vector<NoteEvent> noteOnCalls;
        std::vector<NoteEvent> noteOffCalls;
    };

    class KeyHookKeyboard : public MidiKeyboardComponent
    {
    public:
        using MidiKeyboardComponent::MidiKeyboardComponent;

        bool mouseDownOnKey (int, const MouseEvent&) override { return allowPress; }

        bool mouseDraggedToKey (int, const MouseEvent&) override { return allowPress; }

        void mouseUpOnKey (int midiNoteNumber, const MouseEvent&) override { releasedKeys.push_back (midiNoteNumber); }

        bool allowPress = true;
        std::vector<int> releasedKeys;
    };

    void SetUp() override
    {
        state = std::make_unique<MidiKeyboardState>();
        listener = std::make_unique<TestListener>();
        keyboard = std::make_unique<MidiKeyboardComponent> (*state, MidiKeyboardComponent::horizontalKeyboard);
        keyboard->setBounds (0.0f, 0.0f, 1000.0f, 200.0f);

        state->addListener (listener.get());
    }

    void TearDown() override
    {
        state->removeListener (listener.get());
        keyboard.reset();
        listener.reset();
        state.reset();
    }

    MouseEvent makeWheelEvent (KeyModifiers modifiers = {}) const
    {
        return MouseEvent (MouseEvent::noButtons, modifiers, Point<float> (0.0f, 0.0f));
    }

    void wheel (const MouseWheelData& data, KeyModifiers modifiers = {})
    {
        keyboard->mouseWheel (makeWheelEvent (modifiers), data);
    }

    static MouseEvent makePointerEvent (Point<float> position, int touchIndex, bool isDown)
    {
        return MouseEvent (isDown ? MouseEvent::leftButton : MouseEvent::noButtons, {}, position).withTouchIndex (touchIndex);
    }

    /** Returns a point on the key, at the given fraction of its length from the back edge. */
    Point<float> pointOnKey (int note, float depth = 0.5f) const
    {
        const auto key = keyboard->getRectangleForKey (note);

        switch (keyboard->getOrientation())
        {
            case MidiKeyboardComponent::verticalKeyboardFacingLeft:
                return { key.getRight() - key.getWidth() * depth, key.getCenterY() };

            case MidiKeyboardComponent::verticalKeyboardFacingRight:
                return { key.getX() + key.getWidth() * depth, key.getCenterY() };

            default:
                return { key.getCenterX(), key.getY() + key.getHeight() * depth };
        }
    }

    void press (int note, int touchIndex = -1, float depth = 0.5f)
    {
        keyboard->mouseDown (makePointerEvent (pointOnKey (note, depth), touchIndex, true));
    }

    void drag (int note, int touchIndex = -1)
    {
        keyboard->mouseDrag (makePointerEvent (pointOnKey (note), touchIndex, true));
    }

    void release (int note, int touchIndex = -1)
    {
        keyboard->mouseUp (makePointerEvent (pointOnKey (note), touchIndex, false));
    }

    MidiKeyboardComponent::ScrollButton* findScrollButton (bool up) const
    {
        for (int i = 0; i < keyboard->getNumChildComponents(); ++i)
        {
            if (auto* button = dynamic_cast<MidiKeyboardComponent::ScrollButton*> (keyboard->getChildComponent (i)))
            {
                if (button->isScrollingUp() == up)
                    return button;
            }
        }

        return nullptr;
    }

    void clickScrollButton (bool up, int touchIndex = -1)
    {
        auto* button = findScrollButton (up);
        ASSERT_NE (nullptr, button);

        const auto center = button->getLocalBounds().getCenter();
        button->mouseDown (makePointerEvent (center, touchIndex, true));
        button->mouseUp (makePointerEvent (center, touchIndex, false));
    }

    std::unique_ptr<MidiKeyboardState> state;
    std::unique_ptr<TestListener> listener;
    std::unique_ptr<MidiKeyboardComponent> keyboard;
};

//==============================================================================
TEST_F (MidiKeyboardComponentTests, KeyDownSendsNoteOnForMappedKey)
{
    keyboard->keyDown (KeyPress ('z'), {});

    ASSERT_EQ (1, (int) listener->noteOnCalls.size());
    EXPECT_EQ (1, listener->noteOnCalls[0].channel);
    EXPECT_EQ (kDefaultOctaveOffset, listener->noteOnCalls[0].note);
    EXPECT_FLOAT_EQ (1.0f, listener->noteOnCalls[0].velocity);
    EXPECT_TRUE (keyboard->isNoteOn (kDefaultOctaveOffset));
}

TEST_F (MidiKeyboardComponentTests, KeyUpSendsNoteOffForTrackedNote)
{
    keyboard->keyDown (KeyPress ('z'), {});
    keyboard->keyUp (KeyPress ('z'), {});

    ASSERT_EQ (1, (int) listener->noteOnCalls.size());
    ASSERT_EQ (1, (int) listener->noteOffCalls.size());
    EXPECT_EQ (kDefaultOctaveOffset, listener->noteOffCalls[0].note);
    EXPECT_FALSE (keyboard->isNoteOn (kDefaultOctaveOffset));
}

TEST_F (MidiKeyboardComponentTests, KeyUpWithoutPriorKeyDownSendsNoNoteOff)
{
    keyboard->keyUp (KeyPress ('z'), {});

    EXPECT_TRUE (listener->noteOnCalls.empty());
    EXPECT_TRUE (listener->noteOffCalls.empty());
}

TEST_F (MidiKeyboardComponentTests, RepeatedKeyDownDoesNotRetriggerNote)
{
    keyboard->keyDown (KeyPress ('z'), {});
    keyboard->keyDown (KeyPress ('z'), {});

    ASSERT_EQ (1, (int) listener->noteOnCalls.size());
    EXPECT_EQ (kDefaultOctaveOffset, listener->noteOnCalls[0].note);
}

TEST_F (MidiKeyboardComponentTests, FocusLostReleasesKeyboardNotes)
{
    keyboard->keyDown (KeyPress ('z'), {});
    keyboard->keyDown (KeyPress ('x'), {});

    ASSERT_EQ (2, (int) listener->noteOnCalls.size());
    EXPECT_TRUE (keyboard->isNoteOn (kDefaultOctaveOffset));
    EXPECT_TRUE (keyboard->isNoteOn (kDefaultOctaveOffset + 2));

    keyboard->focusLost();

    ASSERT_EQ (2, (int) listener->noteOffCalls.size());
    EXPECT_FALSE (keyboard->isNoteOn (kDefaultOctaveOffset));
    EXPECT_FALSE (keyboard->isNoteOn (kDefaultOctaveOffset + 2));
}

TEST_F (MidiKeyboardComponentTests, OctaveChangeReleasesHeldKeyboardNotes)
{
    keyboard->keyDown (KeyPress ('z'), {});

    EXPECT_TRUE (keyboard->isNoteOn (kDefaultOctaveOffset));

    // A key held across an octave change can no longer be resolved by keyUp,
    // so the held note must be released when the octave changes.
    keyboard->setKeyPressBaseOctave (4);

    ASSERT_EQ (1, (int) listener->noteOffCalls.size());
    EXPECT_EQ (kDefaultOctaveOffset, listener->noteOffCalls[0].note);
    EXPECT_FALSE (keyboard->isNoteOn (kDefaultOctaveOffset));
}

TEST_F (MidiKeyboardComponentTests, KeysOutsideMidiRangeAreIgnored)
{
    keyboard->setKeyPressBaseOctave (11);

    keyboard->keyDown (KeyPress ('z'), {});

    EXPECT_TRUE (listener->noteOnCalls.empty());
    EXPECT_TRUE (listener->noteOffCalls.empty());
}

TEST_F (MidiKeyboardComponentTests, KeyMappingPlaysExpectedNotes)
{
    const std::pair<char, int> mappings[] = {
        { 'z', 0 }, { 's', 1 }, { 'x', 2 }, { 'd', 3 }, { 'c', 4 }, { 'v', 5 }, { 'g', 6 }, { 'b', 7 }, { 'h', 8 }, { 'n', 9 }, { 'j', 10 }, { 'm', 11 }, { 'q', 12 }, { '2', 13 }, { 'w', 14 }, { '3', 15 }, { 'e', 16 }, { 'r', 17 }, { '5', 18 }, { 't', 19 }, { '6', 20 }, { 'y', 21 }, { '7', 22 }, { 'u', 23 }, { 'i', 24 }, { '9', 25 }, { 'o', 26 }, { '0', 27 }, { 'p', 28 }
    };

    for (const auto& [key, relativeNote] : mappings)
    {
        listener->noteOnCalls.clear();
        listener->noteOffCalls.clear();

        keyboard->keyDown (KeyPress (key), {});
        keyboard->keyUp (KeyPress (key), {});

        ASSERT_EQ (1, (int) listener->noteOnCalls.size()) << "key: " << key;
        ASSERT_EQ (1, (int) listener->noteOffCalls.size()) << "key: " << key;
        EXPECT_EQ (kDefaultOctaveOffset + relativeNote, listener->noteOnCalls[0].note) << "key: " << key;
        EXPECT_EQ (kDefaultOctaveOffset + relativeNote, listener->noteOffCalls[0].note) << "key: " << key;
        EXPECT_FALSE (keyboard->isNoteOn (kDefaultOctaveOffset + relativeNote));
    }
}

TEST_F (MidiKeyboardComponentTests, KeyMappingIsCaseInsensitive)
{
    keyboard->keyDown (KeyPress ('Z'), {});

    ASSERT_EQ (1, (int) listener->noteOnCalls.size());
    EXPECT_EQ (kDefaultOctaveOffset, listener->noteOnCalls[0].note);
}

TEST_F (MidiKeyboardComponentTests, UnmappedKeyIsIgnored)
{
    keyboard->keyDown (KeyPress ('k'), {});

    EXPECT_TRUE (listener->noteOnCalls.empty());
}

TEST_F (MidiKeyboardComponentTests, DefaultKeyboardKeysMatchesDocumentedLayout)
{
    EXPECT_EQ (String ("zsxdcvgbhnjmq2w3er5t6y7ui9o0p"), keyboard->getKeyboardKeys());
}

TEST_F (MidiKeyboardComponentTests, SetKeyboardKeysChangesMapping)
{
    keyboard->setKeyboardKeys ("ab");

    keyboard->keyDown (KeyPress ('a'), {});
    keyboard->keyDown (KeyPress ('b'), {});

    ASSERT_EQ (2, (int) listener->noteOnCalls.size());
    EXPECT_EQ (kDefaultOctaveOffset, listener->noteOnCalls[0].note);
    EXPECT_EQ (kDefaultOctaveOffset + 1, listener->noteOnCalls[1].note);

    // The old default mapping no longer plays anything.
    keyboard->keyDown (KeyPress ('z'), {});
    EXPECT_EQ (2, (int) listener->noteOnCalls.size());
}

TEST_F (MidiKeyboardComponentTests, SetKeyboardKeysIsCaseInsensitive)
{
    keyboard->setKeyboardKeys ("AB");

    EXPECT_EQ (String ("ab"), keyboard->getKeyboardKeys());
}

TEST_F (MidiKeyboardComponentTests, SetKeyboardKeysReleasesHeldKeyboardNotes)
{
    keyboard->keyDown (KeyPress ('z'), {});
    EXPECT_TRUE (keyboard->isNoteOn (kDefaultOctaveOffset));

    // A key held across a mapping change can no longer be resolved by keyUp,
    // so the held note must be released when the mapping changes.
    keyboard->setKeyboardKeys ("ab");

    ASSERT_EQ (1, (int) listener->noteOffCalls.size());
    EXPECT_EQ (kDefaultOctaveOffset, listener->noteOffCalls[0].note);
    EXPECT_FALSE (keyboard->isNoteOn (kDefaultOctaveOffset));
}

//==============================================================================
TEST_F (MidiKeyboardComponentTests, WheelScrollsRightBySingleWhiteKeys)
{
    wheel (MouseWheelData (1.0f, 0.0f));

    EXPECT_EQ (14, keyboard->getLowestVisibleKey());
    EXPECT_EQ (98, keyboard->getHighestVisibleKey());
}

TEST_F (MidiKeyboardComponentTests, WheelScrollsLeftBySingleWhiteKeys)
{
    wheel (MouseWheelData (-1.0f, 0.0f));

    EXPECT_EQ (11, keyboard->getLowestVisibleKey());
    EXPECT_EQ (95, keyboard->getHighestVisibleKey());
}

TEST_F (MidiKeyboardComponentTests, WheelScrollSkipsBlackKeys)
{
    wheel (MouseWheelData (1.0f, 0.0f));

    // Scrolling by one white key moves the range start from C (12) to D (14),
    // skipping the C# black key in between.
    EXPECT_EQ (14, keyboard->getLowestVisibleKey());
}

TEST_F (MidiKeyboardComponentTests, VerticalWheelScrollsHorizontalKeyboard)
{
    wheel (MouseWheelData (0.0f, 1.0f));

    EXPECT_EQ (14, keyboard->getLowestVisibleKey());
    EXPECT_EQ (98, keyboard->getHighestVisibleKey());
}

TEST_F (MidiKeyboardComponentTests, WheelScrollsVerticalKeyboardAlongItsAxis)
{
    MidiKeyboardComponent vertical (*state, MidiKeyboardComponent::verticalKeyboardFacingLeft);
    vertical.mouseWheel (makeWheelEvent(), MouseWheelData (0.0f, 1.0f));

    EXPECT_EQ (14, vertical.getLowestVisibleKey());
    EXPECT_EQ (98, vertical.getHighestVisibleKey());
}

TEST_F (MidiKeyboardComponentTests, WheelScrollClampsAtRangeEdges)
{
    keyboard->setAvailableRange (0, 12);
    wheel (MouseWheelData (-1.0f, 0.0f));

    EXPECT_EQ (0, keyboard->getLowestVisibleKey());
    EXPECT_EQ (12, keyboard->getHighestVisibleKey());

    keyboard->setAvailableRange (0, 127);
    wheel (MouseWheelData (1.0f, 0.0f));

    EXPECT_EQ (0, keyboard->getLowestVisibleKey());
    EXPECT_EQ (127, keyboard->getHighestVisibleKey());
}

TEST_F (MidiKeyboardComponentTests, WheelScrollKeepsRangeSpan)
{
    keyboard->setAvailableRange (12, 96);
    wheel (MouseWheelData (1.0f, 0.0f));

    EXPECT_EQ (84, keyboard->getHighestVisibleKey() - keyboard->getLowestVisibleKey());
}

TEST_F (MidiKeyboardComponentTests, WheelScrollAtTopEdgeKeepsSpanWithinRange)
{
    keyboard->setAvailableRange (115, 127);
    wheel (MouseWheelData (1.0f, 0.0f));

    EXPECT_EQ (115, keyboard->getLowestVisibleKey());
    EXPECT_EQ (127, keyboard->getHighestVisibleKey());
}

//==============================================================================
TEST_F (MidiKeyboardComponentTests, CtrlWheelZoomsInAroundAnchorNote)
{
    keyboard->setBounds (0.0f, 0.0f, 1000.0f, 200.0f);
    keyboard->setAvailableRange (0, 127);

    // Anchor the zoom on the C4 key under the mouse position.
    const auto anchorPoint = keyboard->getRectangleForKey (60).getCenter();

    keyboard->mouseWheel (
        MouseEvent (MouseEvent::noButtons, KeyModifiers (KeyModifiers::controlMask), anchorPoint),
        MouseWheelData (0.0f, 1.0f));

    // 127 / 1.25 rounded to 102, anchored so that note 60 keeps its position.
    EXPECT_EQ (12, keyboard->getLowestVisibleKey());
    EXPECT_EQ (114, keyboard->getHighestVisibleKey());
}

TEST_F (MidiKeyboardComponentTests, CtrlWheelZoomInClampedToOneOctave)
{
    keyboard->setAvailableRange (0, 127);

    const auto modifiers = KeyModifiers (KeyModifiers::controlMask);

    for (int i = 0; i < 30 && keyboard->getHighestVisibleKey() - keyboard->getLowestVisibleKey() > 12; ++i)
        wheel (MouseWheelData (0.0f, 1.0f), modifiers);

    EXPECT_EQ (12, keyboard->getHighestVisibleKey() - keyboard->getLowestVisibleKey());

    // Further zooming in is clamped and must not shrink the range further.
    wheel (MouseWheelData (0.0f, 1.0f), modifiers);
    EXPECT_EQ (12, keyboard->getHighestVisibleKey() - keyboard->getLowestVisibleKey());
    EXPECT_GE (keyboard->getLowestVisibleKey(), 0);
    EXPECT_LE (keyboard->getHighestVisibleKey(), 127);
}

TEST_F (MidiKeyboardComponentTests, CtrlWheelZoomOutExpandsToFullRange)
{
    keyboard->setVisibleRange (36, 72);

    const auto modifiers = KeyModifiers (KeyModifiers::controlMask);

    for (int i = 0; i < 30 && keyboard->getHighestVisibleKey() - keyboard->getLowestVisibleKey() < 127; ++i)
        wheel (MouseWheelData (0.0f, -1.0f), modifiers);

    EXPECT_EQ (0, keyboard->getLowestVisibleKey());
    EXPECT_EQ (127, keyboard->getHighestVisibleKey());

    // Further zooming out is a no-op once the full range is reached.
    wheel (MouseWheelData (0.0f, -1.0f), modifiers);
    EXPECT_EQ (0, keyboard->getLowestVisibleKey());
    EXPECT_EQ (127, keyboard->getHighestVisibleKey());
}

TEST_F (MidiKeyboardComponentTests, CtrlWheelZoomOutClampedToAvailableRange)
{
    keyboard->setAvailableRange (12, 96);

    const auto modifiers = KeyModifiers (KeyModifiers::controlMask);

    for (int i = 0; i < 30; ++i)
        wheel (MouseWheelData (0.0f, -1.0f), modifiers);

    EXPECT_EQ (12, keyboard->getLowestVisibleKey());
    EXPECT_EQ (96, keyboard->getHighestVisibleKey());
}

TEST_F (MidiKeyboardComponentTests, CtrlWheelZoomInOnNarrowAvailableRangeKeepsItsSpan)
{
    keyboard->setAvailableRange (60, 67);

    wheel (MouseWheelData (0.0f, 1.0f), KeyModifiers (KeyModifiers::controlMask));

    EXPECT_EQ (60, keyboard->getLowestVisibleKey());
    EXPECT_EQ (67, keyboard->getHighestVisibleKey());
}

TEST_F (MidiKeyboardComponentTests, WheelScrollClampedToAvailableRange)
{
    keyboard->setAvailableRange (36, 84);
    keyboard->setVisibleRange (48, 72);

    for (int i = 0; i < 20; ++i)
        wheel (MouseWheelData (1.0f, 0.0f));

    EXPECT_EQ (60, keyboard->getLowestVisibleKey());
    EXPECT_EQ (84, keyboard->getHighestVisibleKey());

    for (int i = 0; i < 20; ++i)
        wheel (MouseWheelData (-1.0f, 0.0f));

    EXPECT_EQ (36, keyboard->getLowestVisibleKey());
    EXPECT_EQ (60, keyboard->getHighestVisibleKey());
}

TEST_F (MidiKeyboardComponentTests, CtrlWheelZoomKeepsRangeValid)
{
    const auto modifiers = KeyModifiers (KeyModifiers::controlMask);

    for (int i = 0; i < 30; ++i)
    {
        wheel (MouseWheelData (0.0f, 1.0f), modifiers);
        wheel (MouseWheelData (0.0f, -1.0f), modifiers);

        const auto start = keyboard->getLowestVisibleKey();
        const auto end = keyboard->getHighestVisibleKey();

        EXPECT_GE (start, 0);
        EXPECT_LE (end, 127);
        EXPECT_GE (end - start, 12);
        EXPECT_LE (end - start, 127);
    }
}

TEST_F (MidiKeyboardComponentTests, PlainWheelDoesNotZoom)
{
    keyboard->setAvailableRange (0, 127);

    wheel (MouseWheelData (0.0f, 1.0f));

    EXPECT_EQ (0, keyboard->getLowestVisibleKey());
    EXPECT_EQ (127, keyboard->getHighestVisibleKey());
}

//==============================================================================
TEST_F (MidiKeyboardComponentTests, UsesHandCursor)
{
    EXPECT_EQ (MouseCursor::Hand, keyboard->getMouseCursor().getType());
}

TEST_F (MidiKeyboardComponentTests, TwoTouchesHoldTwoNotes)
{
    press (60, 0);
    press (64, 1);

    ASSERT_EQ (2, (int) listener->noteOnCalls.size());
    EXPECT_EQ (60, listener->noteOnCalls[0].note);
    EXPECT_EQ (64, listener->noteOnCalls[1].note);
    EXPECT_TRUE (listener->noteOffCalls.empty());
    EXPECT_TRUE (keyboard->isNoteOn (60));
    EXPECT_TRUE (keyboard->isNoteOn (64));
}

TEST_F (MidiKeyboardComponentTests, LiftingOneTouchReleasesOnlyItsNote)
{
    press (60, 0);
    press (64, 1);
    release (60, 0);

    ASSERT_EQ (1, (int) listener->noteOffCalls.size());
    EXPECT_EQ (60, listener->noteOffCalls[0].note);
    EXPECT_FALSE (keyboard->isNoteOn (60));
    EXPECT_TRUE (keyboard->isNoteOn (64));
}

TEST_F (MidiKeyboardComponentTests, DraggingOneTouchMovesOnlyItsNote)
{
    press (60, 0);
    press (64, 1);
    drag (62, 0);

    EXPECT_FALSE (keyboard->isNoteOn (60));
    EXPECT_TRUE (keyboard->isNoteOn (62));
    EXPECT_TRUE (keyboard->isNoteOn (64));
}

TEST_F (MidiKeyboardComponentTests, DraggingOffTheKeysReleasesOnlyThatTouch)
{
    press (60, 0);
    press (64, 1);

    keyboard->mouseDrag (makePointerEvent ({ 4.0f, 100.0f }, 0, true));

    EXPECT_FALSE (keyboard->isNoteOn (60));
    EXPECT_TRUE (keyboard->isNoteOn (64));
}

TEST_F (MidiKeyboardComponentTests, SameNoteHeldByTwoTouchesIsReleasedAfterLastLift)
{
    press (60, 0);
    press (60, 1);

    EXPECT_EQ (1, (int) listener->noteOnCalls.size());

    release (60, 0);

    EXPECT_TRUE (listener->noteOffCalls.empty());
    EXPECT_TRUE (keyboard->isNoteOn (60));

    release (60, 1);

    EXPECT_EQ (1, (int) listener->noteOffCalls.size());
    EXPECT_FALSE (keyboard->isNoteOn (60));
}

TEST_F (MidiKeyboardComponentTests, MouseAndTouchAreIndependent)
{
    press (60);
    press (64, 0);
    release (60);

    EXPECT_FALSE (keyboard->isNoteOn (60));
    EXPECT_TRUE (keyboard->isNoteOn (64));

    release (64, 0);

    EXPECT_FALSE (keyboard->isNoteOn (64));
}

TEST_F (MidiKeyboardComponentTests, FocusLostKeepsPointerNotesButReleasesKeyPressNotes)
{
    press (72, 0);
    keyboard->keyDown (KeyPress ('z'), {});

    keyboard->focusLost();

    EXPECT_TRUE (keyboard->isNoteOn (72));
    EXPECT_FALSE (keyboard->isNoteOn (kDefaultOctaveOffset));
}

TEST_F (MidiKeyboardComponentTests, MouseHoverTracksNoteUnderPointer)
{
    keyboard->mouseMove (makePointerEvent (pointOnKey (60), -1, false));

    EXPECT_TRUE (keyboard->isMouseOverNote (60));
}

TEST_F (MidiKeyboardComponentTests, TouchLeavesNoHover)
{
    press (60, 0);

    EXPECT_FALSE (keyboard->isMouseOverNote (60));

    release (60, 0);
    keyboard->mouseEnter (makePointerEvent (pointOnKey (60), 0, false));

    EXPECT_FALSE (keyboard->isMouseOverNote (60));
}

TEST_F (MidiKeyboardComponentTests, TappingScrollButtonKeepsHeldNotes)
{
    auto* upButton = findScrollButton (true);
    ASSERT_NE (nullptr, upButton);

    // Buttons that refuse focus let the click hand it to the keyboard instead.
    EXPECT_FALSE (upButton->getWantsKeyboardFocus());

    press (60, 0);
    clickScrollButton (true, 1);

    EXPECT_EQ (24, keyboard->getLowestVisibleKey());
    EXPECT_TRUE (keyboard->isNoteOn (60));
    EXPECT_TRUE (listener->noteOffCalls.empty());
}

//==============================================================================
TEST_F (MidiKeyboardComponentTests, PressNearFrontEdgeIsLouderThanNearBackEdge)
{
    press (60, -1, 0.9f);
    release (60);
    press (60, -1, 0.2f);

    ASSERT_EQ (2, (int) listener->noteOnCalls.size());
    EXPECT_NEAR (0.9f, listener->noteOnCalls[0].velocity, 0.01f);
    EXPECT_NEAR (0.2f, listener->noteOnCalls[1].velocity, 0.01f);
}

TEST_F (MidiKeyboardComponentTests, VelocityInsensitiveKeyboardUsesFixedVelocity)
{
    keyboard->setMidiVelocitySensitive (false);
    keyboard->setVelocity (0.7f);

    EXPECT_FALSE (keyboard->isMidiVelocitySensitive());

    press (60, -1, 0.2f);

    ASSERT_EQ (1, (int) listener->noteOnCalls.size());
    EXPECT_FLOAT_EQ (0.7f, listener->noteOnCalls[0].velocity);
}

TEST_F (MidiKeyboardComponentTests, PositionVelocityIsScaledBySetVelocity)
{
    keyboard->setVelocity (0.5f);

    press (60, -1, 0.8f);

    ASSERT_EQ (1, (int) listener->noteOnCalls.size());
    EXPECT_NEAR (0.4f, listener->noteOnCalls[0].velocity, 0.01f);
}

TEST_F (MidiKeyboardComponentTests, VerticalVelocityGrowsTowardFrontEdge)
{
    for (auto orientation : { MidiKeyboardComponent::verticalKeyboardFacingLeft, MidiKeyboardComponent::verticalKeyboardFacingRight })
    {
        listener->noteOnCalls.clear();

        keyboard->setOrientation (orientation);
        keyboard->setBounds (0.0f, 0.0f, 200.0f, 1000.0f);

        press (60, -1, 0.9f);
        release (60);
        press (60, -1, 0.2f);
        release (60);

        ASSERT_EQ (2, (int) listener->noteOnCalls.size());
        EXPECT_NEAR (0.9f, listener->noteOnCalls[0].velocity, 0.01f);
        EXPECT_NEAR (0.2f, listener->noteOnCalls[1].velocity, 0.01f);
    }
}

TEST_F (MidiKeyboardComponentTests, NoteAndVelocityAtPositionReportsRawPositionVelocity)
{
    keyboard->setVelocity (0.5f);

    const auto noteInfo = keyboard->getNoteAndVelocityAtPosition (pointOnKey (60, 0.8f));

    EXPECT_EQ (60, noteInfo.note);
    EXPECT_NEAR (0.8f, noteInfo.velocity, 0.01f);
    EXPECT_EQ (-1, keyboard->getNoteAndVelocityAtPosition ({ -10.0f, -10.0f }).note);
}

//==============================================================================
TEST_F (MidiKeyboardComponentTests, SetHighestVisibleKeyMovesOnlyTheUpperEdge)
{
    keyboard->setHighestVisibleKey (72);

    EXPECT_EQ (12, keyboard->getLowestVisibleKey());
    EXPECT_EQ (72, keyboard->getHighestVisibleKey());

    keyboard->setHighestVisibleKey (200);
    EXPECT_EQ (127, keyboard->getHighestVisibleKey());

    keyboard->setHighestVisibleKey (0);
    EXPECT_EQ (12, keyboard->getLowestVisibleKey());
    EXPECT_EQ (12, keyboard->getHighestVisibleKey());
}

TEST_F (MidiKeyboardComponentTests, SetAvailableRangeResetsVisibleRange)
{
    keyboard->setAvailableRange (36, 84);

    EXPECT_EQ (36, keyboard->getRangeStart());
    EXPECT_EQ (84, keyboard->getRangeEnd());
    EXPECT_EQ (36, keyboard->getLowestVisibleKey());
    EXPECT_EQ (84, keyboard->getHighestVisibleKey());

    keyboard->setLowestVisibleKey (48);
    keyboard->setAvailableRange (36, 84);

    EXPECT_EQ (36, keyboard->getLowestVisibleKey());
}

TEST_F (MidiKeyboardComponentTests, SetLowestVisibleKeyIsClampedToAvailableRange)
{
    keyboard->setAvailableRange (36, 84);

    keyboard->setLowestVisibleKey (20);
    EXPECT_EQ (36, keyboard->getLowestVisibleKey());

    keyboard->setLowestVisibleKey (48);
    EXPECT_EQ (48, keyboard->getLowestVisibleKey());
    EXPECT_EQ (84, keyboard->getHighestVisibleKey());

    keyboard->setLowestVisibleKey (100);
    EXPECT_EQ (84, keyboard->getLowestVisibleKey());
    EXPECT_EQ (84, keyboard->getHighestVisibleKey());
}

TEST_F (MidiKeyboardComponentTests, VisibleRangeCallbackFiresOnlyOnRealChange)
{
    int numCalls = 0;
    keyboard->onVisibleRangeChanged = [&numCalls]
    {
        ++numCalls;
    };

    keyboard->setVisibleRange (12, 96);
    EXPECT_EQ (0, numCalls);

    keyboard->setVisibleRange (24, 96);
    EXPECT_EQ (1, numCalls);

    keyboard->setLowestVisibleKey (24);
    EXPECT_EQ (1, numCalls);

    wheel (MouseWheelData (1.0f, 0.0f));
    EXPECT_EQ (2, numCalls);

    clickScrollButton (false);
    EXPECT_EQ (3, numCalls);

    keyboard->setAvailableRange (0, 127);
    EXPECT_EQ (4, numCalls);

    keyboard->setAvailableRange (0, 127);
    EXPECT_EQ (4, numCalls);
}

//==============================================================================
TEST_F (MidiKeyboardComponentTests, ScrollButtonsShiftByAnOctaveKeepingTheSpan)
{
    keyboard->setAvailableRange (24, 96);
    keyboard->setVisibleRange (36, 60);

    clickScrollButton (true);
    EXPECT_EQ (48, keyboard->getLowestVisibleKey());
    EXPECT_EQ (72, keyboard->getHighestVisibleKey());

    clickScrollButton (true);
    clickScrollButton (true);
    EXPECT_EQ (72, keyboard->getLowestVisibleKey());
    EXPECT_EQ (96, keyboard->getHighestVisibleKey());

    clickScrollButton (true);
    EXPECT_EQ (72, keyboard->getLowestVisibleKey());
    EXPECT_EQ (96, keyboard->getHighestVisibleKey());

    keyboard->setVisibleRange (30, 54);
    clickScrollButton (false);
    EXPECT_EQ (24, keyboard->getLowestVisibleKey());
    EXPECT_EQ (48, keyboard->getHighestVisibleKey());
}

TEST_F (MidiKeyboardComponentTests, ScrollButtonsHiddenWhenWholeRangeIsVisible)
{
    auto* upButton = findScrollButton (true);
    auto* downButton = findScrollButton (false);
    ASSERT_NE (nullptr, upButton);
    ASSERT_NE (nullptr, downButton);

    EXPECT_TRUE (upButton->isVisible());
    EXPECT_TRUE (downButton->isVisible());

    keyboard->setAvailableRange (0, 127);

    EXPECT_FALSE (upButton->isVisible());
    EXPECT_FALSE (downButton->isVisible());
    EXPECT_FLOAT_EQ (0.0f, keyboard->getKeyStartRange().getStart());
    EXPECT_FLOAT_EQ (1000.0f, keyboard->getKeyStartRange().getEnd());
}

TEST_F (MidiKeyboardComponentTests, ScrollButtonsCanBeTurnedOff)
{
    keyboard->setScrollButtonsVisible (false);

    EXPECT_FALSE (keyboard->areScrollButtonsVisible());
    EXPECT_FALSE (findScrollButton (true)->isVisible());
    EXPECT_FALSE (findScrollButton (false)->isVisible());
    EXPECT_FLOAT_EQ (0.0f, keyboard->getKeyStartRange().getStart());
    EXPECT_FLOAT_EQ (1000.0f, keyboard->getKeyStartRange().getEnd());
}

TEST_F (MidiKeyboardComponentTests, ScrollButtonsAreDisabledAtTheirEdge)
{
    keyboard->setAvailableRange (36, 84);
    keyboard->setLowestVisibleKey (48);

    EXPECT_TRUE (findScrollButton (false)->isEnabled());
    EXPECT_FALSE (findScrollButton (true)->isEnabled());

    keyboard->setVisibleRange (36, 60);

    EXPECT_FALSE (findScrollButton (false)->isEnabled());
    EXPECT_TRUE (findScrollButton (true)->isEnabled());
}

TEST_F (MidiKeyboardComponentTests, KeysAreLaidOutBetweenScrollButtons)
{
    const auto buttonWidth = keyboard->getScrollButtonWidth();

    EXPECT_FLOAT_EQ (buttonWidth, keyboard->getKeyStartRange().getStart());
    EXPECT_FLOAT_EQ (1000.0f - buttonWidth, keyboard->getKeyStartRange().getEnd());
    EXPECT_FLOAT_EQ (buttonWidth, keyboard->getRectangleForKey (12).getX());
    EXPECT_FLOAT_EQ (buttonWidth, findScrollButton (false)->getBounds().getRight());
    EXPECT_FLOAT_EQ (1000.0f - buttonWidth, findScrollButton (true)->getX());

    EXPECT_EQ (-1, keyboard->getNoteAtPosition ({ buttonWidth * 0.5f, 20.0f }));
    EXPECT_EQ (-1, keyboard->getNoteAtPosition ({ 1000.0f - buttonWidth * 0.5f, 20.0f }));

    keyboard->setScrollButtonWidth (40.0f);

    EXPECT_FLOAT_EQ (40.0f, keyboard->getScrollButtonWidth());
    EXPECT_FLOAT_EQ (40.0f, keyboard->getKeyStartRange().getStart());
}

TEST_F (MidiKeyboardComponentTests, BlackKeyAtVisibleEdgeIsNotHitOverScrollButton)
{
    keyboard->setVisibleRange (13, 96);

    EXPECT_EQ (-1, keyboard->getNoteAtPosition ({ keyboard->getScrollButtonWidth() - 1.0f, 20.0f }));
}

//==============================================================================
TEST_F (MidiKeyboardComponentTests, ChannelsToDisplayFiltersNoteState)
{
    state->noteOn (2, 60, 1.0f);
    EXPECT_TRUE (keyboard->isNoteOn (60));

    keyboard->setMidiChannelsToDisplay (1 << 0);

    EXPECT_EQ (1 << 0, keyboard->getMidiChannelsToDisplay());
    EXPECT_FALSE (keyboard->isNoteOn (60));

    state->noteOn (1, 62, 1.0f);
    EXPECT_TRUE (keyboard->isNoteOn (62));
}

TEST_F (MidiKeyboardComponentTests, BlackNoteProportionsChangeBlackKeyRect)
{
    const auto whiteKey = keyboard->getRectangleForKey (60);

    EXPECT_FLOAT_EQ (0.6f, keyboard->getBlackNoteLengthProportion());
    EXPECT_FLOAT_EQ (0.7f, keyboard->getBlackNoteWidthProportion());
    EXPECT_FLOAT_EQ (whiteKey.getHeight() * 0.6f, keyboard->getRectangleForKey (61).getHeight());
    EXPECT_FLOAT_EQ (whiteKey.getWidth() * 0.7f, keyboard->getRectangleForKey (61).getWidth());

    keyboard->setBlackNoteLengthProportion (0.5f);
    keyboard->setBlackNoteWidthProportion (0.4f);

    const auto blackKey = keyboard->getRectangleForKey (61);

    EXPECT_FLOAT_EQ (0.0f, blackKey.getY());
    EXPECT_FLOAT_EQ (whiteKey.getHeight() * 0.5f, blackKey.getHeight());
    EXPECT_FLOAT_EQ (whiteKey.getWidth() * 0.4f, blackKey.getWidth());
    EXPECT_NEAR (whiteKey.getRight(), blackKey.getCenterX(), 0.001f);
}

TEST_F (MidiKeyboardComponentTests, SetOrientationLaysOutKeysAlongTheNewAxis)
{
    press (60, 0);

    keyboard->setOrientation (MidiKeyboardComponent::verticalKeyboardFacingLeft);
    keyboard->setBounds (0.0f, 0.0f, 200.0f, 1000.0f);

    EXPECT_EQ (MidiKeyboardComponent::verticalKeyboardFacingLeft, keyboard->getOrientation());
    EXPECT_FALSE (keyboard->isNoteOn (60));

    const auto buttonWidth = keyboard->getScrollButtonWidth();

    // Low notes at the top, black keys anchored at the back (right) edge.
    EXPECT_FLOAT_EQ (buttonWidth, keyboard->getRectangleForKey (12).getY());
    EXPECT_FLOAT_EQ (200.0f, keyboard->getRectangleForKey (12).getWidth());
    EXPECT_LT (keyboard->getRectangleForKey (12).getY(), keyboard->getRectangleForKey (14).getY());
    EXPECT_FLOAT_EQ (200.0f, keyboard->getRectangleForKey (13).getRight());
    EXPECT_FLOAT_EQ (0.0f, findScrollButton (false)->getY());
    EXPECT_FLOAT_EQ (1000.0f, findScrollButton (true)->getBounds().getBottom());

    keyboard->setOrientation (MidiKeyboardComponent::verticalKeyboardFacingRight);

    // Low notes at the bottom, black keys anchored at the back (left) edge.
    EXPECT_FLOAT_EQ (1000.0f - buttonWidth, keyboard->getRectangleForKey (12).getBottom());
    EXPECT_GT (keyboard->getRectangleForKey (12).getY(), keyboard->getRectangleForKey (14).getY());
    EXPECT_FLOAT_EQ (0.0f, keyboard->getRectangleForKey (13).getX());
    EXPECT_FLOAT_EQ (1000.0f, findScrollButton (false)->getBounds().getBottom());
    EXPECT_FLOAT_EQ (0.0f, findScrollButton (true)->getY());
}

TEST_F (MidiKeyboardComponentTests, KeyPressBaseOctaveShiftsTypedNotes)
{
    keyboard->setKeyPressBaseOctave (5);

    EXPECT_EQ (5, keyboard->getKeyPressBaseOctave());

    keyboard->keyDown (KeyPress ('z'), {});
    keyboard->keyUp (KeyPress ('z'), {});

    // Naming octaves has no effect on the typed notes.
    keyboard->setOctaveForMiddleC (5);
    keyboard->keyDown (KeyPress ('x'), {});

    ASSERT_EQ (2, (int) listener->noteOnCalls.size());
    EXPECT_EQ (60, listener->noteOnCalls[0].note);
    EXPECT_EQ (62, listener->noteOnCalls[1].note);
    EXPECT_TRUE (keyboard->isNoteOn (62));
}

TEST_F (MidiKeyboardComponentTests, WhiteNoteTextNamesCKeysWithTheirOctave)
{
    EXPECT_EQ (String ("C3"), keyboard->getWhiteNoteText (60));
    EXPECT_EQ (String ("D"), keyboard->getWhiteNoteText (62));
    EXPECT_EQ (String(), keyboard->getWhiteNoteText (61));

    keyboard->setOctaveForMiddleC (4);

    EXPECT_EQ (4, keyboard->getOctaveForMiddleC());
    EXPECT_EQ (String ("C4"), keyboard->getWhiteNoteText (60));
}

TEST_F (MidiKeyboardComponentTests, KeyHooksCanSuppressNotes)
{
    KeyHookKeyboard hooked (*state, MidiKeyboardComponent::horizontalKeyboard);
    hooked.setBounds (0.0f, 0.0f, 1000.0f, 200.0f);
    hooked.allowPress = false;

    const auto keyCenter = hooked.getRectangleForKey (60).getCenter();

    hooked.mouseDown (makePointerEvent (keyCenter, -1, true));
    hooked.mouseDrag (makePointerEvent (hooked.getRectangleForKey (62).getCenter(), -1, true));

    EXPECT_TRUE (listener->noteOnCalls.empty());

    hooked.mouseUp (makePointerEvent (hooked.getRectangleForKey (62).getCenter(), -1, false));

    ASSERT_EQ (1, (int) hooked.releasedKeys.size());
    EXPECT_EQ (62, hooked.releasedKeys[0]);
}

//==============================================================================
class MidiKeyboardComponentAsyncTests : public ::testing::Test
{
protected:
    class ObservableKeyboard : public MidiKeyboardComponent
    {
    public:
        using MidiKeyboardComponent::MidiKeyboardComponent;

        int repaintCount = 0;

    protected:
        void handleAsyncUpdate() override
        {
            ++repaintCount;
            MidiKeyboardComponent::handleAsyncUpdate();
        }
    };

    void SetUp() override
    {
        messageManager = MessageManager::getInstance();
        state = std::make_unique<MidiKeyboardState>();
        keyboard = std::make_unique<ObservableKeyboard> (*state, MidiKeyboardComponent::horizontalKeyboard);
    }

    void runDispatchLoopUntil (int millisecondsToRunFor = 100)
    {
        messageManager->runDispatchLoopUntil (millisecondsToRunFor);
    }

    MessageManager* messageManager = nullptr;
    std::unique_ptr<MidiKeyboardState> state;
    std::unique_ptr<ObservableKeyboard> keyboard;
};

TEST_F (MidiKeyboardComponentAsyncTests, NotesFedFromAnotherThreadRepaintOnTheMessageThread)
{
    std::thread feeder ([this]
    {
        state->processNextMidiEvent (MidiMessage::noteOn (1, 60, 0.5f));
    });
    feeder.join();

    EXPECT_TRUE (keyboard->isNoteOn (60));
    EXPECT_EQ (0, keyboard->repaintCount); // deferred - nothing repaints on the feeding thread

    runDispatchLoopUntil (100);

    EXPECT_GE (keyboard->repaintCount, 1);

    const auto repaintCountAfterNoteOn = keyboard->repaintCount;

    std::thread releaser ([this]
    {
        state->processNextMidiEvent (MidiMessage::noteOff (1, 60));
    });
    releaser.join();

    EXPECT_FALSE (keyboard->isNoteOn (60));

    runDispatchLoopUntil (100);

    EXPECT_GT (keyboard->repaintCount, repaintCountAfterNoteOn);
}
