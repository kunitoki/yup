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

#include <yup_audio_gui/yup_audio_gui.h>

#include <gtest/gtest.h>

#include <thread>

using namespace yup;

namespace yup
{
extern std::unique_ptr<yup::GraphicsContext> yup_constructHeadlessGraphicsContext (yup::GpuDevice::Options, yup::GpuDevice::Ptr);
} // namespace yup

class PitchWheelComponentTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        state = std::make_unique<MidiKeyboardState>();
        wheel = std::make_unique<PitchWheelComponent> (*state, "testPitchWheel");
        wheel->setBounds (0, 0, 30, 120);
    }

    std::unique_ptr<MidiKeyboardState> state;
    std::unique_ptr<PitchWheelComponent> wheel;
};

//==============================================================================
TEST_F (PitchWheelComponentTest, DefaultInitialization)
{
    EXPECT_DOUBLE_EQ (0.0, wheel->getValue());
    EXPECT_DOUBLE_EQ (0.0, wheel->getDefaultValue());
    EXPECT_TRUE (wheel->getResetOnRelease());
    EXPECT_DOUBLE_EQ (1.0, wheel->getMouseDragSensitivity());
    EXPECT_EQ (1, wheel->getMidiChannel());
    EXPECT_FALSE (wheel->isMouseOver());
    EXPECT_FALSE (wheel->isCurrentlyBeingDragged());
}

TEST_F (PitchWheelComponentTest, ConstructWithComponentId)
{
    EXPECT_EQ (String ("testPitchWheel"), wheel->getComponentID());
}

//==============================================================================
TEST_F (PitchWheelComponentTest, SetValueClampsToRange)
{
    wheel->setValue (0.5);
    EXPECT_DOUBLE_EQ (0.5, wheel->getValue());

    wheel->setValue (5.0);
    EXPECT_DOUBLE_EQ (1.0, wheel->getValue());

    wheel->setValue (-5.0);
    EXPECT_DOUBLE_EQ (-1.0, wheel->getValue());
}

TEST_F (PitchWheelComponentTest, SetValueWithNotificationCallback)
{
    int callCount = 0;
    double lastValue = 0.0;
    wheel->onValueChanged = [&] (double v)
    {
        ++callCount;
        lastValue = v;
    };

    wheel->setValue (0.5, sendNotification);
    EXPECT_EQ (1, callCount);
    EXPECT_DOUBLE_EQ (0.5, lastValue);
}

TEST_F (PitchWheelComponentTest, SetValueWithoutNotification)
{
    int callCount = 0;
    wheel->onValueChanged = [&callCount] (double)
    {
        ++callCount;
    };

    wheel->setValue (0.5, dontSendNotification);
    EXPECT_EQ (0, callCount);
}

//==============================================================================
TEST_F (PitchWheelComponentTest, DefaultValueOperations)
{
    wheel->setDefaultValue (0.25);
    EXPECT_DOUBLE_EQ (0.25, wheel->getDefaultValue());

    wheel->setDefaultValue (5.0);
    EXPECT_DOUBLE_EQ (1.0, wheel->getDefaultValue());
}

//==============================================================================
TEST_F (PitchWheelComponentTest, ResetOnReleaseRoundTrip)
{
    wheel->setResetOnRelease (false);
    EXPECT_FALSE (wheel->getResetOnRelease());

    wheel->setResetOnRelease (true);
    EXPECT_TRUE (wheel->getResetOnRelease());
}

//==============================================================================
TEST_F (PitchWheelComponentTest, MouseDragSensitivity)
{
    wheel->setMouseDragSensitivity (2.5);
    EXPECT_DOUBLE_EQ (2.5, wheel->getMouseDragSensitivity());

    wheel->setMouseDragSensitivity (0.1);
    EXPECT_DOUBLE_EQ (0.1, wheel->getMouseDragSensitivity());
}

//==============================================================================
TEST_F (PitchWheelComponentTest, IsMouseOverDefaultFalse)
{
    EXPECT_FALSE (wheel->isMouseOver());
}

TEST_F (PitchWheelComponentTest, IsCurrentlyBeingDraggedDefaultFalse)
{
    EXPECT_FALSE (wheel->isCurrentlyBeingDragged());
}

//==============================================================================
TEST_F (PitchWheelComponentTest, DragStartCallback)
{
    bool called = false;
    wheel->onDragStart = [&] (const MouseEvent&)
    {
        called = true;
    };

    EXPECT_FALSE (called);
}

TEST_F (PitchWheelComponentTest, DragEndCallback)
{
    bool called = false;
    wheel->onDragEnd = [&] (const MouseEvent&)
    {
        called = true;
    };

    EXPECT_FALSE (called);
}

//==============================================================================
TEST_F (PitchWheelComponentTest, PaintWithThemeDoesNotCrash)
{
    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (30, 120);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({ wheel->paint (g); });
}

TEST_F (PitchWheelComponentTest, PaintAtTinyBoundsDoesNotCrash)
{
    // The grip-band and gradient math must degrade gracefully at a height too
    // small for the band's travel margins to make sense.
    wheel->setBounds (0, 0, 30, 10);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (30, 10);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({ wheel->paint (g); });
}

//==============================================================================
class PitchWheelComponentAsyncTests : public ::testing::Test
{
protected:
    class ObservablePitchWheel : public PitchWheelComponent
    {
    public:
        using PitchWheelComponent::PitchWheelComponent;

        int asyncUpdateCount = 0;

    protected:
        void handleAsyncUpdate() override
        {
            ++asyncUpdateCount;
            PitchWheelComponent::handleAsyncUpdate();
        }
    };

    void SetUp() override
    {
        messageManager = MessageManager::getInstance();
        state = std::make_unique<MidiKeyboardState>();
        wheel = std::make_unique<ObservablePitchWheel> (*state, "asyncPitchWheel");
    }

    void runDispatchLoopUntil (int millisecondsToRunFor = 100)
    {
        messageManager->runDispatchLoopUntil (millisecondsToRunFor);
    }

    MouseEvent makeButtonEvent (float y) const
    {
        return MouseEvent (MouseEvent::Buttons::leftButton, {}, Point<float> (15.0f, y));
    }

    MessageManager* messageManager = nullptr;
    std::unique_ptr<MidiKeyboardState> state;
    std::unique_ptr<ObservablePitchWheel> wheel;
};

TEST_F (PitchWheelComponentAsyncTests, PitchWheelFedFromAnotherThreadAppliesOnTheMessageThread)
{
    std::thread feeder ([this]
    {
        state->processNextMidiEvent (MidiMessage::pitchWheel (1, 4096));
    });
    feeder.join();

    EXPECT_DOUBLE_EQ (0.0, wheel->getValue()); // deferred - nothing changes on the feeding thread
    EXPECT_EQ (0, wheel->asyncUpdateCount);

    runDispatchLoopUntil (100);

    EXPECT_GE (wheel->asyncUpdateCount, 1);
    EXPECT_NEAR ((4096.0 - 8192.0) / 8191.0, wheel->getValue(), 1e-9);
}

TEST_F (PitchWheelComponentAsyncTests, OnlyTheLatestPitchWheelPositionIsApplied)
{
    std::thread feeder ([this]
    {
        state->processNextMidiEvent (MidiMessage::pitchWheel (1, 0));
        state->processNextMidiEvent (MidiMessage::pitchWheel (1, 4096));
        state->processNextMidiEvent (MidiMessage::pitchWheel (1, 12288));
    });
    feeder.join();

    runDispatchLoopUntil (100);

    EXPECT_NEAR ((12288.0 - 8192.0) / 8191.0, wheel->getValue(), 1e-9);
}

TEST_F (PitchWheelComponentAsyncTests, PitchWheelOnOtherChannelsIsIgnored)
{
    std::thread feeder ([this]
    {
        state->processNextMidiEvent (MidiMessage::pitchWheel (2, 12288));
    });
    feeder.join();

    runDispatchLoopUntil (100);

    EXPECT_DOUBLE_EQ (0.0, wheel->getValue());
    EXPECT_EQ (0, wheel->asyncUpdateCount);
}

TEST_F (PitchWheelComponentAsyncTests, NoteEventsDoNotTriggerAsyncUpdate)
{
    std::thread feeder ([this]
    {
        state->processNextMidiEvent (MidiMessage::noteOn (1, 60, 0.5f));
        state->processNextMidiEvent (MidiMessage::noteOff (1, 60));
    });
    feeder.join();

    runDispatchLoopUntil (100);

    EXPECT_DOUBLE_EQ (0.0, wheel->getValue());
    EXPECT_EQ (0, wheel->asyncUpdateCount);
}

TEST_F (PitchWheelComponentAsyncTests, PitchWheelIsNotAppliedWhileDragging)
{
    std::thread feeder ([this]
    {
        state->processNextMidiEvent (MidiMessage::pitchWheel (1, 8192));
    });
    feeder.join();

    runDispatchLoopUntil (100);

    EXPECT_NEAR (0.0, wheel->getValue(), 1e-9);

    wheel->mouseDown (makeButtonEvent (60.0f));
    EXPECT_TRUE (wheel->isCurrentlyBeingDragged());

    std::thread dragger ([this]
    {
        state->processNextMidiEvent (MidiMessage::pitchWheel (1, 12288));
    });
    dragger.join();

    runDispatchLoopUntil (100);

    // The MIDI position must not move the wheel while the user is dragging it.
    EXPECT_NEAR (0.0, wheel->getValue(), 1e-9);

    wheel->mouseUp (makeButtonEvent (60.0f));
    EXPECT_FALSE (wheel->isCurrentlyBeingDragged());
}

TEST_F (PitchWheelComponentAsyncTests, MidiChannelSelection)
{
    wheel->setMidiChannel (2);

    std::thread feeder ([this]
    {
        state->processNextMidiEvent (MidiMessage::pitchWheel (2, 12288));
    });
    feeder.join();

    runDispatchLoopUntil (100);

    EXPECT_NEAR ((12288.0 - 8192.0) / 8191.0, wheel->getValue(), 1e-9);
    EXPECT_EQ (2, wheel->getMidiChannel());
}
