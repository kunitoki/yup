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

#pragma once

namespace yup
{

//==============================================================================
/**
    Displays a component through a clipped, scrollable area.

    The viewed component keeps its own size and is moved inside the viewport. Vertical and
    horizontal scrollbars appear when its size exceeds the visible area. The mouse wheel and
    trackpad scroll the viewport, and a multi-finger pan gesture can move it on touch devices
    with kinetic scrolling.

    @code
    Viewport viewport;
    auto content = std::make_unique<Component>();
    content->setSize (800.0f, 1200.0f);
    viewport.setViewedComponent (content.release(), true);
    @endcode

    @see ScrollBar
*/
class YUP_API Viewport : public Component,
                         private ComponentListener
{
public:
    /** Creates an empty viewport with automatically hidden scrollbars. */
    Viewport();

    /** Removes the viewed component and any registered listeners. */
    ~Viewport() override;

    //==============================================================================
    /** Sets the component shown inside the viewport.

        Its current size defines the scrollable content size. The component is removed from its
        current parent, if any. When deleteWhenNoLongerNeeded is true, the viewport owns and
        deletes it when replaced or destroyed.

        @param component                 The component to display, or nullptr to clear the view.
        @param deleteWhenNoLongerNeeded  Whether the viewport takes ownership of the component.
    */
    void setViewedComponent (Component* component, bool deleteWhenNoLongerNeeded = false);

    /** Returns the component shown inside the viewport, or nullptr. */
    Component* getViewedComponent() const noexcept { return viewedComponent; }

    /** Returns the top-left position of the visible area in viewed-component coordinates. */
    Point<float> getViewPosition() const noexcept { return viewPosition; }

    /** Sets the top-left position of the visible area, clamped to the content bounds.

        @param position  The desired position in viewed-component coordinates.
    */
    void setViewPosition (Point<float> position);

    /** Scrolls the viewport by the smallest amount that makes the given content area visible.

        @param area  The area in viewed-component coordinates.
    */
    void scrollToShow (Rectangle<float> area);

    /** Sets the wheel distance, in pixels, for each unit of wheel movement.

        @param pixels  The non-negative distance per wheel unit.
    */
    void setScrollStep (float pixels);

    /** Returns the bounds of the visible area in viewed-component coordinates. */
    Rectangle<float> getVisibleArea() const noexcept;

    /** Returns the vertical scrollbar, including its appearance and range settings. */
    ScrollBar& getVerticalScrollBar() noexcept { return verticalScrollBar; }

    /** Returns the horizontal scrollbar, including its appearance and range settings. */
    ScrollBar& getHorizontalScrollBar() noexcept { return horizontalScrollBar; }

    //==============================================================================
    void resized() override;
    void refreshDisplay (double lastFrameTimeSeconds) override;
    bool mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) override;
    void gesturePan (const GestureEvent& event) override;

private:
    void componentResized (Component& component) override;
    void componentBeingDeleted (Component& component) override;
    void updateScrollBars();
    void updateViewPosition();
    void updateViewPositionFromScroller();

    Component* viewedComponent = nullptr;
    std::unique_ptr<Component> ownedComponent;
    ScrollBar verticalScrollBar { ScrollBar::Orientation::vertical };
    ScrollBar horizontalScrollBar { ScrollBar::Orientation::horizontal };
    Point<float> viewPosition;
    Point<float> visibleSize;
    Point<float> gesturePosition;
    KineticScroller horizontalScroller;
    KineticScroller verticalScroller;
    double gestureClock = 0.0;
    float scrollStep = 40.0f;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Viewport)
};

} // namespace yup
