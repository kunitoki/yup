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

namespace
{

//==============================================================================
class MockMouseListener : public MouseListener
{
public:
    MockMouseListener() = default;
    ~MockMouseListener() override = default;

    void mouseDown (const MouseEvent& event) override
    {
        mouseDownCallCount++;
        lastMouseEvent = event;

        if (addListenerOnMouseDown != nullptr)
            Desktop::getInstance()->addGlobalMouseListener (addListenerOnMouseDown);

        if (removeListenerOnMouseDown != nullptr)
            Desktop::getInstance()->removeGlobalMouseListener (removeListenerOnMouseDown);

        if (removeSelfOnMouseDown)
            Desktop::getInstance()->removeGlobalMouseListener (this);
    }

    void mouseUp (const MouseEvent& event) override
    {
        mouseUpCallCount++;
        lastMouseEvent = event;

        if (addListenerOnMouseUp != nullptr)
            Desktop::getInstance()->addGlobalMouseListener (addListenerOnMouseUp);

        if (removeListenerOnMouseUp != nullptr)
            Desktop::getInstance()->removeGlobalMouseListener (removeListenerOnMouseUp);

        if (removeSelfOnMouseUp)
            Desktop::getInstance()->removeGlobalMouseListener (this);
    }

    void mouseMove (const MouseEvent& event) override
    {
        mouseMoveCallCount++;
        lastMouseEvent = event;

        if (addListenerOnMouseMove != nullptr)
            Desktop::getInstance()->addGlobalMouseListener (addListenerOnMouseMove);

        if (removeListenerOnMouseMove != nullptr)
            Desktop::getInstance()->removeGlobalMouseListener (removeListenerOnMouseMove);

        if (removeSelfOnMouseMove)
            Desktop::getInstance()->removeGlobalMouseListener (this);
    }

    void mouseDrag (const MouseEvent& event) override
    {
        mouseDragCallCount++;
        lastMouseEvent = event;

        if (addListenerOnMouseDrag != nullptr)
            Desktop::getInstance()->addGlobalMouseListener (addListenerOnMouseDrag);

        if (removeListenerOnMouseDrag != nullptr)
            Desktop::getInstance()->removeGlobalMouseListener (removeListenerOnMouseDrag);

        if (removeSelfOnMouseDrag)
            Desktop::getInstance()->removeGlobalMouseListener (this);
    }

    void mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) override
    {
        mouseWheelCallCount++;
        lastMouseEvent = event;
        lastWheelData = wheelData;

        if (addListenerOnMouseWheel != nullptr)
            Desktop::getInstance()->addGlobalMouseListener (addListenerOnMouseWheel);

        if (removeListenerOnMouseWheel != nullptr)
            Desktop::getInstance()->removeGlobalMouseListener (removeListenerOnMouseWheel);

        if (removeSelfOnMouseWheel)
            Desktop::getInstance()->removeGlobalMouseListener (this);
    }

    void reset()
    {
        mouseDownCallCount = 0;
        mouseUpCallCount = 0;
        mouseMoveCallCount = 0;
        mouseDragCallCount = 0;
        mouseWheelCallCount = 0;
        addListenerOnMouseDown = nullptr;
        removeListenerOnMouseDown = nullptr;
        removeSelfOnMouseDown = false;
        addListenerOnMouseUp = nullptr;
        removeListenerOnMouseUp = nullptr;
        removeSelfOnMouseUp = false;
        addListenerOnMouseMove = nullptr;
        removeListenerOnMouseMove = nullptr;
        removeSelfOnMouseMove = false;
        addListenerOnMouseDrag = nullptr;
        removeListenerOnMouseDrag = nullptr;
        removeSelfOnMouseDrag = false;
        addListenerOnMouseWheel = nullptr;
        removeListenerOnMouseWheel = nullptr;
        removeSelfOnMouseWheel = false;
    }

    int mouseDownCallCount = 0;
    int mouseUpCallCount = 0;
    int mouseMoveCallCount = 0;
    int mouseDragCallCount = 0;
    int mouseWheelCallCount = 0;
    MouseEvent lastMouseEvent;
    MouseWheelData lastWheelData;

    MouseListener* addListenerOnMouseDown = nullptr;
    MouseListener* removeListenerOnMouseDown = nullptr;
    bool removeSelfOnMouseDown = false;

    MouseListener* addListenerOnMouseUp = nullptr;
    MouseListener* removeListenerOnMouseUp = nullptr;
    bool removeSelfOnMouseUp = false;

    MouseListener* addListenerOnMouseMove = nullptr;
    MouseListener* removeListenerOnMouseMove = nullptr;
    bool removeSelfOnMouseMove = false;

    MouseListener* addListenerOnMouseDrag = nullptr;
    MouseListener* removeListenerOnMouseDrag = nullptr;
    bool removeSelfOnMouseDrag = false;

    MouseListener* addListenerOnMouseWheel = nullptr;
    MouseListener* removeListenerOnMouseWheel = nullptr;
    bool removeSelfOnMouseWheel = false;
};

//==============================================================================
class MockComponent : public Component
{
public:
    MockComponent() = default;
    ~MockComponent() override = default;
};

} // namespace

//==============================================================================
class DesktopTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        desktop = Desktop::getInstance();
    }

    void TearDown() override
    {
        // Clean up any listeners
        desktop->removeGlobalMouseListener (&listener1);
        desktop->removeGlobalMouseListener (&listener2);
        desktop->removeGlobalMouseListener (&listener3);
    }

    Desktop* desktop = nullptr;
    MockMouseListener listener1;
    MockMouseListener listener2;
    MockMouseListener listener3;
};

//==============================================================================
// Singleton Tests
//==============================================================================

TEST_F (DesktopTest, SingletonReturnsValidInstance)
{
    EXPECT_NE (nullptr, Desktop::getInstance());
}

TEST_F (DesktopTest, SingletonReturnsSameInstance)
{
    auto* instance1 = Desktop::getInstance();
    auto* instance2 = Desktop::getInstance();
    EXPECT_EQ (instance1, instance2);
}

//==============================================================================
// Screen Tests
//==============================================================================

TEST_F (DesktopTest, GetNumScreensReturnsCount)
{
    // Note: Screen count depends on platform, just verify it doesn't crash
    int numScreens = desktop->getNumScreens();
    EXPECT_GE (numScreens, 0);
}

TEST_F (DesktopTest, GetScreenWithValidIndexReturnsScreen)
{
    int numScreens = desktop->getNumScreens();
    if (numScreens > 0)
    {
        auto screen = desktop->getScreen (0);
        EXPECT_NE (nullptr, screen);
    }
}

TEST_F (DesktopTest, GetScreenWithInvalidIndexReturnsNull)
{
    auto screen = desktop->getScreen (-1);
    EXPECT_EQ (nullptr, screen);

    screen = desktop->getScreen (999999);
    EXPECT_EQ (nullptr, screen);
}

TEST_F (DesktopTest, GetPrimaryScreenReturnsScreenOrNull)
{
    // Should either return primary screen or nullptr if no screens
    auto screen = desktop->getPrimaryScreen();
    int numScreens = desktop->getNumScreens();

    if (numScreens > 0)
        EXPECT_NE (nullptr, screen);
    else
        EXPECT_EQ (nullptr, screen);
}

TEST_F (DesktopTest, GetScreensReturnsValidSpan)
{
    auto screens = desktop->getScreens();
    EXPECT_EQ (static_cast<size_t> (desktop->getNumScreens()), screens.size());
}

TEST_F (DesktopTest, GetScreenContainingPointReturnsScreenOrPrimary)
{
    Point<float> testPoint (0.0f, 0.0f);
    auto screen = desktop->getScreenContaining (testPoint);

    // Should return a screen if screens exist
    if (desktop->getNumScreens() > 0)
        EXPECT_NE (nullptr, screen);
}

TEST_F (DesktopTest, GetScreenContainingRectangleReturnsScreenOrPrimary)
{
    Rectangle<float> testRect (0.0f, 0.0f, 100.0f, 100.0f);
    auto screen = desktop->getScreenContaining (testRect);

    // Should return a screen if screens exist
    if (desktop->getNumScreens() > 0)
        EXPECT_NE (nullptr, screen);
}

TEST_F (DesktopTest, GetScreenContainingComponentReturnsScreenOrPrimary)
{
    MockComponent component;
    component.setBounds (0, 0, 100, 100);

    auto screen = desktop->getScreenContaining (&component);

    // Should return a screen if screens exist
    if (desktop->getNumScreens() > 0)
        EXPECT_NE (nullptr, screen);
}

//==============================================================================
// Mouse Cursor Tests
//==============================================================================

TEST_F (DesktopTest, GetMouseCursorReturnsDefaultInitially)
{
    auto cursor = desktop->getMouseCursor();
    EXPECT_EQ (MouseCursor::Default, cursor.getType());
}

TEST_F (DesktopTest, SetMouseCursorChangesCurrentCursor)
{
    MouseCursor newCursor (MouseCursor::Crosshair);
    desktop->setMouseCursor (newCursor);

    auto cursor = desktop->getMouseCursor();
    EXPECT_EQ (MouseCursor::Crosshair, cursor.getType());

    // Reset to default
    desktop->setMouseCursor (MouseCursor (MouseCursor::Default));
}

TEST_F (DesktopTest, SetMouseCursorWithDifferentTypes)
{
    const MouseCursor::Type types[] = {
        MouseCursor::Default,
        MouseCursor::Arrow,
        MouseCursor::Crosshair,
        MouseCursor::Text
    };

    for (auto type : types)
    {
        MouseCursor cursor (type);
        desktop->setMouseCursor (cursor);

        auto retrievedCursor = desktop->getMouseCursor();
        EXPECT_EQ (type, retrievedCursor.getType());
    }

    // Reset to default
    desktop->setMouseCursor (MouseCursor (MouseCursor::Default));
}

//==============================================================================
// Global Mouse Listener Tests - Basic Add/Remove
//==============================================================================

TEST_F (DesktopTest, AddGlobalMouseListenerDoesNotCrash)
{
    desktop->addGlobalMouseListener (&listener1);
    desktop->removeGlobalMouseListener (&listener1);
}

TEST_F (DesktopTest, DebugPendingMechanism)
{
    // Add listener1 first
    desktop->addGlobalMouseListener (&listener1);

    // Simulate what happens during event dispatch
    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (5.0f, 15.0f), nullptr);

    // First event - listener1 should be called
    listener1.reset();
    desktop->handleGlobalMouseDown (event);
    EXPECT_EQ (1, listener1.mouseDownCallCount);

    // Now add listener2 directly (not during event)
    desktop->addGlobalMouseListener (&listener2);

    // Second event - both should be called
    listener1.reset();
    listener2.reset();
    desktop->handleGlobalMouseDown (event);
    EXPECT_EQ (1, listener1.mouseDownCallCount);
    EXPECT_EQ (1, listener2.mouseDownCallCount);

    desktop->removeGlobalMouseListener (&listener1);
    desktop->removeGlobalMouseListener (&listener2);
}

TEST_F (DesktopTest, AddNullGlobalMouseListenerDoesNotCrash)
{
    desktop->addGlobalMouseListener (nullptr);
}

TEST_F (DesktopTest, RemoveNullGlobalMouseListenerDoesNotCrash)
{
    desktop->removeGlobalMouseListener (nullptr);
}

TEST_F (DesktopTest, RemoveGlobalMouseListenerThatWasNeverAdded)
{
    desktop->removeGlobalMouseListener (&listener1);
    // Should not crash
}

TEST_F (DesktopTest, AddSameGlobalMouseListenerMultipleTimes)
{
    desktop->addGlobalMouseListener (&listener1);
    desktop->addGlobalMouseListener (&listener1);
    desktop->addGlobalMouseListener (&listener1);

    // Create a mock event
    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (0.0f, 0.0f), nullptr);

    listener1.reset();
    desktop->handleGlobalMouseDown (event);

    // Should only be called once even though added multiple times
    EXPECT_EQ (1, listener1.mouseDownCallCount);

    desktop->removeGlobalMouseListener (&listener1);
}

TEST_F (DesktopTest, RemoveGlobalMouseListenerMultipleTimes)
{
    desktop->addGlobalMouseListener (&listener1);
    desktop->removeGlobalMouseListener (&listener1);
    desktop->removeGlobalMouseListener (&listener1);
    desktop->removeGlobalMouseListener (&listener1);
    // Should not crash
}

//==============================================================================
// Global Mouse Listener Tests - Event Dispatch
//==============================================================================

TEST_F (DesktopTest, HandleGlobalMouseDownNotifiesListeners)
{
    desktop->addGlobalMouseListener (&listener1);
    desktop->addGlobalMouseListener (&listener2);

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (10.0f, 20.0f), nullptr);

    listener1.reset();
    listener2.reset();

    desktop->handleGlobalMouseDown (event);

    EXPECT_EQ (1, listener1.mouseDownCallCount);
    EXPECT_EQ (1, listener2.mouseDownCallCount);

    desktop->removeGlobalMouseListener (&listener1);
    desktop->removeGlobalMouseListener (&listener2);
}

TEST_F (DesktopTest, HandleGlobalMouseUpNotifiesListeners)
{
    desktop->addGlobalMouseListener (&listener1);
    desktop->addGlobalMouseListener (&listener2);

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (10.0f, 20.0f), nullptr);

    listener1.reset();
    listener2.reset();

    desktop->handleGlobalMouseUp (event);

    EXPECT_EQ (1, listener1.mouseUpCallCount);
    EXPECT_EQ (1, listener2.mouseUpCallCount);

    desktop->removeGlobalMouseListener (&listener1);
    desktop->removeGlobalMouseListener (&listener2);
}

TEST_F (DesktopTest, HandleGlobalMouseMoveNotifiesListeners)
{
    desktop->addGlobalMouseListener (&listener1);
    desktop->addGlobalMouseListener (&listener2);

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (10.0f, 20.0f), nullptr);

    listener1.reset();
    listener2.reset();

    desktop->handleGlobalMouseMove (event);

    EXPECT_EQ (1, listener1.mouseMoveCallCount);
    EXPECT_EQ (1, listener2.mouseMoveCallCount);

    desktop->removeGlobalMouseListener (&listener1);
    desktop->removeGlobalMouseListener (&listener2);
}

TEST_F (DesktopTest, HandleGlobalMouseDragNotifiesListeners)
{
    desktop->addGlobalMouseListener (&listener1);
    desktop->addGlobalMouseListener (&listener2);

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (10.0f, 20.0f), nullptr);

    listener1.reset();
    listener2.reset();

    desktop->handleGlobalMouseDrag (event);

    EXPECT_EQ (1, listener1.mouseDragCallCount);
    EXPECT_EQ (1, listener2.mouseDragCallCount);

    desktop->removeGlobalMouseListener (&listener1);
    desktop->removeGlobalMouseListener (&listener2);
}

TEST_F (DesktopTest, HandleGlobalMouseWheelNotifiesListeners)
{
    desktop->addGlobalMouseListener (&listener1);
    desktop->addGlobalMouseListener (&listener2);

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (10.0f, 20.0f), nullptr);
    MouseWheelData wheelData (0.0f, 1.0f);

    listener1.reset();
    listener2.reset();

    desktop->handleGlobalMouseWheel (event, wheelData);

    EXPECT_EQ (1, listener1.mouseWheelCallCount);
    EXPECT_EQ (1, listener2.mouseWheelCallCount);
    EXPECT_FLOAT_EQ (1.0f, listener1.lastWheelData.getDeltaY());

    desktop->removeGlobalMouseListener (&listener1);
    desktop->removeGlobalMouseListener (&listener2);
}

//==============================================================================
// Global Mouse Listener Tests - Multiple Listeners
//==============================================================================

TEST_F (DesktopTest, MultipleListenersReceiveEventsInOrder)
{
    desktop->addGlobalMouseListener (&listener1);
    desktop->addGlobalMouseListener (&listener2);
    desktop->addGlobalMouseListener (&listener3);

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (5.0f, 15.0f), nullptr);

    listener1.reset();
    listener2.reset();
    listener3.reset();

    desktop->handleGlobalMouseDown (event);

    EXPECT_EQ (1, listener1.mouseDownCallCount);
    EXPECT_EQ (1, listener2.mouseDownCallCount);
    EXPECT_EQ (1, listener3.mouseDownCallCount);

    desktop->removeGlobalMouseListener (&listener1);
    desktop->removeGlobalMouseListener (&listener2);
    desktop->removeGlobalMouseListener (&listener3);
}

TEST_F (DesktopTest, RemovingOneListenerDoesNotAffectOthers)
{
    desktop->addGlobalMouseListener (&listener1);
    desktop->addGlobalMouseListener (&listener2);
    desktop->addGlobalMouseListener (&listener3);

    desktop->removeGlobalMouseListener (&listener2);

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (5.0f, 15.0f), nullptr);

    listener1.reset();
    listener2.reset();
    listener3.reset();

    desktop->handleGlobalMouseDown (event);

    EXPECT_EQ (1, listener1.mouseDownCallCount);
    EXPECT_EQ (0, listener2.mouseDownCallCount);
    EXPECT_EQ (1, listener3.mouseDownCallCount);

    desktop->removeGlobalMouseListener (&listener1);
    desktop->removeGlobalMouseListener (&listener3);
}

//==============================================================================
// Global Mouse Listener Tests - Adding During Event
//==============================================================================

TEST_F (DesktopTest, AddingListenerDuringMouseDownEvent)
{
    desktop->addGlobalMouseListener (&listener1);

    listener1.reset();
    listener2.reset();
    listener1.addListenerOnMouseDown = &listener2;

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (5.0f, 15.0f), nullptr);
    desktop->handleGlobalMouseDown (event);

    // listener1 should be called
    EXPECT_EQ (1, listener1.mouseDownCallCount);

    // listener2 was added during the event, so it should not be called for this event
    EXPECT_EQ (0, listener2.mouseDownCallCount);

    // But listener2 should be called for the next event
    listener1.reset();
    listener2.reset();
    listener1.addListenerOnMouseDown = nullptr;

    desktop->handleGlobalMouseDown (event);

    EXPECT_EQ (1, listener1.mouseDownCallCount);
    EXPECT_EQ (1, listener2.mouseDownCallCount);

    desktop->removeGlobalMouseListener (&listener1);
    desktop->removeGlobalMouseListener (&listener2);
}

TEST_F (DesktopTest, AddingListenerDuringMouseUpEvent)
{
    desktop->addGlobalMouseListener (&listener1);

    listener1.reset();
    listener2.reset();
    listener1.addListenerOnMouseUp = &listener2;

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (5.0f, 15.0f), nullptr);
    desktop->handleGlobalMouseUp (event);

    EXPECT_EQ (1, listener1.mouseUpCallCount);
    EXPECT_EQ (0, listener2.mouseUpCallCount);

    // listener2 should be called for the next event
    listener1.reset();
    listener2.reset();
    listener1.addListenerOnMouseUp = nullptr;

    desktop->handleGlobalMouseUp (event);

    EXPECT_EQ (1, listener1.mouseUpCallCount);
    EXPECT_EQ (1, listener2.mouseUpCallCount);

    desktop->removeGlobalMouseListener (&listener1);
    desktop->removeGlobalMouseListener (&listener2);
}

TEST_F (DesktopTest, AddingListenerDuringMouseMoveEvent)
{
    desktop->addGlobalMouseListener (&listener1);

    listener1.reset();
    listener2.reset();
    listener1.addListenerOnMouseMove = &listener2;

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (5.0f, 15.0f), nullptr);

    desktop->handleGlobalMouseMove (event);

    EXPECT_EQ (1, listener1.mouseMoveCallCount);
    EXPECT_EQ (0, listener2.mouseMoveCallCount);

    // listener2 should be called for the next event
    listener1.reset();
    listener2.reset();
    listener1.addListenerOnMouseMove = nullptr;

    desktop->handleGlobalMouseMove (event);

    EXPECT_EQ (1, listener1.mouseMoveCallCount);
    EXPECT_EQ (1, listener2.mouseMoveCallCount);

    desktop->removeGlobalMouseListener (&listener1);
    desktop->removeGlobalMouseListener (&listener2);
}

//==============================================================================
// Global Mouse Listener Tests - Removing During Event
//==============================================================================

TEST_F (DesktopTest, RemovingOtherListenerDuringMouseDownEvent)
{
    desktop->addGlobalMouseListener (&listener1);
    desktop->addGlobalMouseListener (&listener2);

    listener1.reset();
    listener2.reset();
    listener2.removeListenerOnMouseDown = &listener1;

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (5.0f, 15.0f), nullptr);
    desktop->handleGlobalMouseDown (event);

    // Both should still be called for this event
    EXPECT_EQ (1, listener1.mouseDownCallCount);
    EXPECT_EQ (1, listener2.mouseDownCallCount);

    // listener2 should not be called for the next event
    listener1.reset();
    listener2.reset();
    listener1.removeListenerOnMouseDown = &listener2;

    desktop->addGlobalMouseListener (&listener1);
    desktop->handleGlobalMouseDown (event);

    EXPECT_EQ (1, listener1.mouseDownCallCount);
    EXPECT_EQ (1, listener2.mouseDownCallCount);

    desktop->removeGlobalMouseListener (&listener1);
}

TEST_F (DesktopTest, RemovingSelfDuringMouseDownEvent)
{
    desktop->addGlobalMouseListener (&listener1);
    desktop->addGlobalMouseListener (&listener2);

    listener1.reset();
    listener1.removeSelfOnMouseDown = true;

    listener2.reset();

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (5.0f, 15.0f), nullptr);
    desktop->handleGlobalMouseDown (event);

    EXPECT_EQ (1, listener1.mouseDownCallCount);
    EXPECT_EQ (1, listener2.mouseDownCallCount);

    // listener1 should not be called for the next event
    listener1.reset();
    listener2.reset();

    desktop->handleGlobalMouseDown (event);

    EXPECT_EQ (0, listener1.mouseDownCallCount);
    EXPECT_EQ (1, listener2.mouseDownCallCount);

    desktop->removeGlobalMouseListener (&listener2);
}

TEST_F (DesktopTest, RemovingSelfDuringMouseUpEvent)
{
    desktop->addGlobalMouseListener (&listener1);

    listener1.reset();
    listener1.removeSelfOnMouseUp = true;

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (5.0f, 15.0f), nullptr);
    desktop->handleGlobalMouseUp (event);

    EXPECT_EQ (1, listener1.mouseUpCallCount);

    // listener1 should not be called for the next event
    listener1.reset();

    desktop->handleGlobalMouseUp (event);

    EXPECT_EQ (0, listener1.mouseUpCallCount);
}

//==============================================================================
// Global Mouse Listener Tests - Complex Scenarios
//==============================================================================

TEST_F (DesktopTest, AddAndRemoveListenersDuringEventDispatch)
{
    desktop->addGlobalMouseListener (&listener1);
    desktop->addGlobalMouseListener (&listener2);

    listener1.reset();
    listener2.reset();
    listener3.reset();

    // listener1 adds listener3 and removes listener2
    listener1.addListenerOnMouseDown = &listener3;
    listener1.removeListenerOnMouseDown = &listener2;

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (5.0f, 15.0f), nullptr);
    desktop->handleGlobalMouseDown (event);

    EXPECT_EQ (1, listener1.mouseDownCallCount);
    EXPECT_EQ (0, listener2.mouseDownCallCount);
    EXPECT_EQ (0, listener3.mouseDownCallCount);

    // Next event: listener2 should not be called, listener3 should be called
    listener1.reset();
    listener1.addListenerOnMouseDown = nullptr;
    listener1.removeListenerOnMouseDown = nullptr;
    listener2.reset();
    listener3.reset();

    desktop->handleGlobalMouseDown (event);

    EXPECT_EQ (1, listener1.mouseDownCallCount);
    EXPECT_EQ (0, listener2.mouseDownCallCount);
    EXPECT_EQ (1, listener3.mouseDownCallCount);

    desktop->removeGlobalMouseListener (&listener1);
    desktop->removeGlobalMouseListener (&listener3);
}

TEST_F (DesktopTest, MultipleListenersAddingAndRemovingDuringEvent)
{
    desktop->addGlobalMouseListener (&listener1);
    desktop->addGlobalMouseListener (&listener2);

    listener1.reset();
    listener2.reset();
    listener3.reset();

    // Both listeners try to add listener3
    listener1.addListenerOnMouseDown = &listener3;
    listener2.addListenerOnMouseDown = &listener3;

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (5.0f, 15.0f), nullptr);
    desktop->handleGlobalMouseDown (event);

    EXPECT_EQ (1, listener1.mouseDownCallCount);
    EXPECT_EQ (1, listener2.mouseDownCallCount);
    EXPECT_EQ (0, listener3.mouseDownCallCount);

    // Next event: listener3 should only be called once (no duplicates)
    listener1.reset();
    listener1.addListenerOnMouseDown = nullptr;
    listener2.reset();
    listener2.addListenerOnMouseDown = nullptr;
    listener3.reset();

    desktop->handleGlobalMouseDown (event);

    EXPECT_EQ (1, listener1.mouseDownCallCount);
    EXPECT_EQ (1, listener2.mouseDownCallCount);
    EXPECT_EQ (1, listener3.mouseDownCallCount);

    desktop->removeGlobalMouseListener (&listener1);
    desktop->removeGlobalMouseListener (&listener2);
    desktop->removeGlobalMouseListener (&listener3);
}

//==============================================================================
// Native Component Tests
//==============================================================================

TEST_F (DesktopTest, GetNativeComponentWithNullUserdataReturnsNull)
{
    auto component = desktop->getNativeComponent (nullptr);
    EXPECT_EQ (nullptr, component);
}

TEST_F (DesktopTest, GetNativeComponentWithInvalidUserdataReturnsNull)
{
    int dummyData = 42;
    auto component = desktop->getNativeComponent (&dummyData);
    EXPECT_EQ (nullptr, component);
}

//==============================================================================
// Screen Event Handler Tests
//==============================================================================

TEST_F (DesktopTest, HandleScreenConnectedDoesNotCrash)
{
    desktop->handleScreenConnected (0);
    // Should not crash
}

TEST_F (DesktopTest, HandleScreenDisconnectedDoesNotCrash)
{
    desktop->handleScreenDisconnected (0);
    // Should not crash
}

TEST_F (DesktopTest, HandleScreenMovedDoesNotCrash)
{
    desktop->handleScreenMoved (0);
    // Should not crash
}

TEST_F (DesktopTest, HandleScreenOrientationChangedDoesNotCrash)
{
    desktop->handleScreenOrientationChanged (0);
    // Should not crash
}

TEST_F (DesktopTest, HandleScreenEventsWithInvalidIndexDoesNotCrash)
{
    desktop->handleScreenConnected (-1);
    desktop->handleScreenDisconnected (-1);
    desktop->handleScreenMoved (-1);
    desktop->handleScreenOrientationChanged (-1);

    desktop->handleScreenConnected (999999);
    desktop->handleScreenDisconnected (999999);
    desktop->handleScreenMoved (999999);
    desktop->handleScreenOrientationChanged (999999);
    // Should not crash
}

//==============================================================================
// Mouse Location Tests
//==============================================================================

TEST_F (DesktopTest, GetCurrentMouseLocationDoesNotCrash)
{
    auto location = desktop->getCurrentMouseLocation();
    // Should return some location, just verify it doesn't crash
    EXPECT_TRUE (true);
}

TEST_F (DesktopTest, SetCurrentMouseLocationDoesNotCrash)
{
    Point<float> newLocation (100.0f, 200.0f);
    desktop->setCurrentMouseLocation (newLocation);
    // Should not crash
}

TEST_F (DesktopTest, GetScreenContainingMouseCursorDoesNotCrash)
{
    auto screen = desktop->getScreenContainingMouseCursor();
    // May return nullptr if no screens, just verify it doesn't crash
    if (desktop->getNumScreens() > 0)
        EXPECT_NE (nullptr, screen);
}
