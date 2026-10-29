/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, distribute, and/or sell copies of this software for any
   purpose with or without fee is hereby granted provided that the above
   copyright notice and this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS, ARE DISCLAIMED.

  ==============================================================================
*/

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

namespace yup
{

template <>
class ComponentTestHelper<Viewport>
{
public:
    static void triggerMouseWheel (Component& component, const MouseEvent& event, const MouseWheelData& wheelData)
    {
        component.internalMouseWheel (event, wheelData);
    }
};

} // namespace yup

//==============================================================================
TEST (ViewportTests, ShowsScrollBarsWhenContentExceedsVisibleArea)
{
    Viewport viewport;
    viewport.setBounds (0.0f, 0.0f, 100.0f, 100.0f);

    auto content = std::make_unique<Component>();
    content->setSize (300.0f, 400.0f);
    viewport.setViewedComponent (content.release(), true);

    EXPECT_TRUE (viewport.getVerticalScrollBar().isVisible());
    EXPECT_TRUE (viewport.getHorizontalScrollBar().isVisible());
    EXPECT_EQ (0.0f, viewport.getViewPosition().getX());
    EXPECT_EQ (0.0f, viewport.getViewPosition().getY());
    EXPECT_LT (viewport.getVisibleArea().getWidth(), 100.0f);
    EXPECT_LT (viewport.getVisibleArea().getHeight(), 100.0f);
}

TEST (ViewportTests, ClampsViewPositionToContentBounds)
{
    Viewport viewport;
    viewport.setBounds (0.0f, 0.0f, 100.0f, 100.0f);

    auto content = std::make_unique<Component>();
    content->setSize (300.0f, 400.0f);
    viewport.setViewedComponent (content.release(), true);
    viewport.setViewPosition ({ 1000.0f, 1000.0f });

    EXPECT_FLOAT_EQ (300.0f - viewport.getVisibleArea().getWidth(), viewport.getViewPosition().getX());
    EXPECT_FLOAT_EQ (400.0f - viewport.getVisibleArea().getHeight(), viewport.getViewPosition().getY());
}

TEST (ViewportTests, ScrollsTheSmallestAmountToShowAnArea)
{
    Viewport viewport;
    viewport.setBounds (0.0f, 0.0f, 100.0f, 100.0f);

    auto content = std::make_unique<Component>();
    content->setSize (300.0f, 400.0f);
    viewport.setViewedComponent (content.release(), true);
    viewport.scrollToShow ({ 250.0f, 350.0f, 20.0f, 20.0f });

    EXPECT_FLOAT_EQ (250.0f + 20.0f - viewport.getVisibleArea().getWidth(), viewport.getViewPosition().getX());
    EXPECT_FLOAT_EQ (350.0f + 20.0f - viewport.getVisibleArea().getHeight(), viewport.getViewPosition().getY());
}

TEST (ViewportTests, MouseWheelScrollsTheVisibleContent)
{
    Viewport viewport;
    viewport.setBounds (0.0f, 0.0f, 100.0f, 100.0f);

    auto content = std::make_unique<Component>();
    content->setSize (300.0f, 400.0f);
    viewport.setViewedComponent (content.release(), true);

    const MouseEvent event (MouseEvent::noButtons, KeyModifiers(), Point<float>());
    EXPECT_TRUE (viewport.mouseWheel (event, MouseWheelData (0.0f, -1.0f)));

    EXPECT_FLOAT_EQ (40.0f, viewport.getViewPosition().getY());
}

TEST (ViewportTests, PanGestureMovesTheVisibleContent)
{
    Viewport viewport;
    viewport.setBounds (0.0f, 0.0f, 100.0f, 100.0f);

    auto content = std::make_unique<Component>();
    content->setSize (300.0f, 400.0f);
    viewport.setViewedComponent (content.release(), true);

    viewport.gesturePan (GestureEvent (GestureType::pan, GestureEvent::Phase::began)
                             .withPosition ({ 50.0f, 50.0f }));
    viewport.gesturePan (GestureEvent (GestureType::pan, GestureEvent::Phase::changed)
                             .withPosition ({ 50.0f, 30.0f })
                             .withTranslation ({ 0.0f, -20.0f }, { 0.0f, -20.0f }));

    EXPECT_FLOAT_EQ (20.0f, viewport.getViewPosition().getY());
}

TEST (ViewportTests, ResizingContentClampsTheCurrentPosition)
{
    Viewport viewport;
    viewport.setBounds (0.0f, 0.0f, 100.0f, 100.0f);

    auto content = std::make_unique<Component>();
    content->setSize (300.0f, 400.0f);
    auto* contentPointer = content.get();
    viewport.setViewedComponent (content.release(), true);
    viewport.setViewPosition ({ 200.0f, 300.0f });

    contentPointer->setSize (80.0f, 70.0f);

    EXPECT_EQ (Point<float>(), viewport.getViewPosition());
    EXPECT_FALSE (viewport.getVerticalScrollBar().isVisible());
    EXPECT_FALSE (viewport.getHorizontalScrollBar().isVisible());
}

TEST (ViewportTests, WheelOverTheContentScrollsTheViewport)
{
    Viewport viewport;
    viewport.setBounds (0.0f, 0.0f, 100.0f, 100.0f);

    auto content = std::make_unique<Component>();
    content->setSize (300.0f, 400.0f);
    auto* contentPointer = content.get();
    viewport.setViewedComponent (content.release(), true);

    const MouseEvent event (MouseEvent::noButtons, KeyModifiers(), Point<float> (10.0f, 10.0f));
    ComponentTestHelper<Viewport>::triggerMouseWheel (*contentPointer, event.withSourceComponent (contentPointer), MouseWheelData (0.0f, -1.0f));

    EXPECT_FLOAT_EQ (40.0f, viewport.getViewPosition().getY());
}

TEST (ViewportTests, WheelWithNothingToScrollPassesToTheParent)
{
    struct WheelRecorder : Component
    {
        bool mouseWheel (const MouseEvent&, const MouseWheelData&) override { ++wheelCount; return true; }

        int wheelCount = 0;
    };

    WheelRecorder parent;
    Viewport viewport;
    parent.setVisible (true);
    parent.addAndMakeVisible (viewport);
    viewport.setBounds (0.0f, 0.0f, 100.0f, 100.0f);

    auto content = std::make_unique<Component>();
    content->setSize (50.0f, 50.0f);
    auto* contentPointer = content.get();
    viewport.setViewedComponent (content.release(), true);

    const MouseEvent event (MouseEvent::noButtons, KeyModifiers(), Point<float> (10.0f, 10.0f));
    ComponentTestHelper<Viewport>::triggerMouseWheel (*contentPointer, event.withSourceComponent (contentPointer), MouseWheelData (0.0f, -1.0f));

    EXPECT_EQ (1, parent.wheelCount);
    EXPECT_EQ (Point<float>(), viewport.getViewPosition());
}

TEST (ViewportTests, WheelAtTheInnerViewportLimitScrollsTheOuterViewport)
{
    Viewport outerViewport;
    Viewport innerViewport;
    outerViewport.setVisible (true);
    innerViewport.setVisible (true);
    outerViewport.setBounds (0.0f, 0.0f, 100.0f, 100.0f);
    innerViewport.setBounds (0.0f, 0.0f, 100.0f, 100.0f);

    auto outerContent = std::make_unique<Component>();
    outerContent->setSize (100.0f, 300.0f);
    outerContent->addAndMakeVisible (innerViewport);
    outerViewport.setViewedComponent (outerContent.release(), true);

    auto innerContent = std::make_unique<Component>();
    innerContent->setSize (100.0f, 200.0f);
    auto* innerContentPointer = innerContent.get();
    innerViewport.setViewedComponent (innerContent.release(), true);
    innerViewport.setViewPosition ({ 0.0f, 100.0f });

    const MouseEvent event (MouseEvent::noButtons, KeyModifiers(), Point<float> (10.0f, 10.0f));
    ComponentTestHelper<Viewport>::triggerMouseWheel (*innerContentPointer, event.withSourceComponent (innerContentPointer), MouseWheelData (0.0f, -1.0f));

    EXPECT_FLOAT_EQ (100.0f, innerViewport.getViewPosition().getY());
    EXPECT_FLOAT_EQ (40.0f, outerViewport.getViewPosition().getY());
}
