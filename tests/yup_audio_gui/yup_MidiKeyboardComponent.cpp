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

    void SetUp() override
    {
        state = std::make_unique<MidiKeyboardState>();
        listener = std::make_unique<TestListener>();
        keyboard = std::make_unique<MidiKeyboardComponent> (*state, MidiKeyboardComponent::horizontalKeyboard);

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
    keyboard->setOctaveForMiddleC (4);

    ASSERT_EQ (1, (int) listener->noteOffCalls.size());
    EXPECT_EQ (kDefaultOctaveOffset, listener->noteOffCalls[0].note);
    EXPECT_FALSE (keyboard->isNoteOn (kDefaultOctaveOffset));
}

TEST_F (MidiKeyboardComponentTests, KeysOutsideMidiRangeAreIgnored)
{
    keyboard->setOctaveForMiddleC (11);

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
    keyboard->setAvailableRange (12, 96);

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
