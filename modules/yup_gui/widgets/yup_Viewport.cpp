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

namespace yup
{

namespace
{

struct ViewportIds
{
    static inline const Identifier scrollStep { "scrollStep" };
};

} // namespace
    
Viewport::Viewport()
    : Viewport (StringRef {})
{
}

Viewport::Viewport (StringRef ComponentID)
    : Component (ComponentID)
{
    setOpaque (false);
    setWantedGestures (GestureType::pan);

    verticalScrollBar.setVisibilityMode (ScrollBar::VisibilityMode::autoHide);
    horizontalScrollBar.setVisibilityMode (ScrollBar::VisibilityMode::autoHide);
    verticalScrollBar.onScrollPositionChanged = [this] (double position)
    {
        setViewPosition ({ viewPosition.getX(), static_cast<float> (position) });
    };
    horizontalScrollBar.onScrollPositionChanged = [this] (double position)
    {
        setViewPosition ({ static_cast<float> (position), viewPosition.getY() });
    };

    addAndMakeVisible (verticalScrollBar);
    addAndMakeVisible (horizontalScrollBar);
}

Viewport::~Viewport()
{
    if (viewedComponent != nullptr)
    {
        viewedComponent->removeComponentListener (this);
        removeChildComponent (viewedComponent);
    }

    viewedComponent = nullptr;
    ownedComponent.reset();
}

const ComponentTypeInfo& Viewport::getClassTypeInfo()
{
    static const auto info = ComponentTypeBuilder<Viewport> (Identifier ("Viewport"))
                                 .property (ViewportIds::scrollStep, &Viewport::getScrollStep, &Viewport::setScrollStep, 40.0, "How far one wheel or key step scrolls, in points.")
                                 .childAttacher ([] (Component& parent, Component& child)
                                 {
                                     static_cast<Viewport&> (parent).setViewedComponent (&child);
                                 })
                                 .build();

    return info;
}

//==============================================================================
void Viewport::setViewedComponent (Component* component, bool deleteWhenNoLongerNeeded)
{
    if (component == viewedComponent)
        return;

    if (viewedComponent != nullptr)
    {
        viewedComponent->removeComponentListener (this);
        removeChildComponent (viewedComponent);
    }

    viewedComponent = nullptr;
    ownedComponent.reset();
    viewPosition = {};

    if (component != nullptr)
    {
        if (auto* parent = component->getParentComponent())
            parent->removeChildComponent (component);
        else if (component->isOnDesktop())
            component->removeFromDesktop();

        viewedComponent = component;

        if (deleteWhenNoLongerNeeded)
            ownedComponent.reset (component);

        addAndMakeVisible (*component, 0);
        component->addComponentListener (this);
    }

    resized();
}

void Viewport::setViewPosition (Point<float> position)
{
    const auto contentWidth = viewedComponent != nullptr ? viewedComponent->getWidth() : 0.0f;
    const auto contentHeight = viewedComponent != nullptr ? viewedComponent->getHeight() : 0.0f;
    const auto maximumX = jmax (0.0f, contentWidth - visibleSize.getX());
    const auto maximumY = jmax (0.0f, contentHeight - visibleSize.getY());
    const auto clamped = Point<float> (jlimit (0.0f, maximumX, position.getX()),
                                       jlimit (0.0f, maximumY, position.getY()));

    if (clamped == viewPosition)
        return;

    viewPosition = clamped;
    horizontalScroller.setOffset (viewPosition.getX());
    verticalScroller.setOffset (viewPosition.getY());
    updateViewPosition();
    updateScrollBars();
    repaint();
}

void Viewport::scrollToShow (Rectangle<float> area)
{
    auto position = viewPosition;

    if (area.getX() < position.getX())
        position.setX (area.getX());
    else if (area.getRight() > position.getX() + visibleSize.getX())
        position.setX (area.getRight() - visibleSize.getX());

    if (area.getY() < position.getY())
        position.setY (area.getY());
    else if (area.getBottom() > position.getY() + visibleSize.getY())
        position.setY (area.getBottom() - visibleSize.getY());

    setViewPosition (position);
}

void Viewport::setScrollStep (float pixels)
{
    const auto clamped = jmax (0.0f, pixels);
    if (approximatelyEqual (scrollStep, clamped))
        return;

    scrollStep = clamped;
    notifyPropertyChanged (ViewportIds::scrollStep);
}

void Viewport::setScrollBarsShown (bool showVertical, bool showHorizontal)
{
    if (showVerticalScrollBar == showVertical && showHorizontalScrollBar == showHorizontal)
        return;

    showVerticalScrollBar = showVertical;
    showHorizontalScrollBar = showHorizontal;
    resized();
}

Rectangle<float> Viewport::getVisibleArea() const noexcept
{
    return { viewPosition.getX(), viewPosition.getY(), visibleSize.getX(), visibleSize.getY() };
}

void Viewport::resized()
{
    updateScrollBars();
    horizontalScroller.setOffset (viewPosition.getX());
    verticalScroller.setOffset (viewPosition.getY());
    updateViewPosition();
}

void Viewport::visibilityChanged()
{
    updateFrameAttachment();
}

void Viewport::parentHierarchyChanged()
{
    updateFrameAttachment();
}

void Viewport::refreshDisplay (double lastFrameTimeSeconds)
{
    const auto deltaSeconds = jlimit (0.0, 1.0 / 15.0, lastFrameTimeSeconds);
    gestureClock += deltaSeconds;

    const auto horizontalMoving = horizontalScroller.update (deltaSeconds);
    const auto verticalMoving = verticalScroller.update (deltaSeconds);

    if (horizontalMoving || verticalMoving)
        updateViewPositionFromScroller();

    updateFrameAttachment();
}

bool Viewport::mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData)
{
    ignoreUnused (event);

    const auto oldPosition = viewPosition;
    const auto deltaX = wheelData.getDeltaX();
    const auto deltaY = wheelData.getDeltaY();
    const auto canScrollX = viewedComponent != nullptr && viewedComponent->getWidth() > visibleSize.getX();
    const auto canScrollY = viewedComponent != nullptr && viewedComponent->getHeight() > visibleSize.getY();

    const auto delta = std::abs (deltaY) >= std::abs (deltaX) ? deltaY : deltaX;

    if (std::abs (deltaY) >= std::abs (deltaX) && canScrollY)
        setViewPosition ({ viewPosition.getX(), viewPosition.getY() - deltaY * scrollStep });
    else if (canScrollX)
        setViewPosition ({ viewPosition.getX() - delta * scrollStep, viewPosition.getY() });
    else if (canScrollY)
        setViewPosition ({ viewPosition.getX(), viewPosition.getY() - delta * scrollStep });

    return viewPosition != oldPosition;
}

void Viewport::gesturePan (const GestureEvent& event)
{
    switch (event.getPhase())
    {
        case GestureEvent::Phase::began:
            gesturePosition = event.getPosition();
            horizontalScroller.setOffset (viewPosition.getX());
            verticalScroller.setOffset (viewPosition.getY());
            horizontalScroller.beginDrag (gesturePosition.getX(), gestureClock);
            verticalScroller.beginDrag (gesturePosition.getY(), gestureClock);
            break;

        case GestureEvent::Phase::changed:
            gesturePosition = event.getPosition();
            horizontalScroller.dragTo (gesturePosition.getX(), gestureClock);
            verticalScroller.dragTo (gesturePosition.getY(), gestureClock);
            updateViewPositionFromScroller();
            break;

        case GestureEvent::Phase::ended:
            horizontalScroller.endDrag (gestureClock);
            verticalScroller.endDrag (gestureClock);
            break;

        case GestureEvent::Phase::canceled:
            horizontalScroller.stop();
            verticalScroller.stop();
            break;
    }

    updateFrameAttachment();
}

void Viewport::componentResized (Component& component)
{
    if (&component == viewedComponent)
        resized();
}

void Viewport::componentBeingDeleted (Component& component)
{
    if (&component != viewedComponent)
        return;

    viewedComponent = nullptr;
    ownedComponent.release();
    viewPosition = {};
    resized();
}

void Viewport::updateScrollBars()
{
    auto bounds = getLocalBounds();
    const auto contentWidth = viewedComponent != nullptr ? viewedComponent->getWidth() : 0.0f;
    const auto contentHeight = viewedComponent != nullptr ? viewedComponent->getHeight() : 0.0f;
    const auto verticalSize = verticalScrollBar.getScrollBarWidth();
    const auto horizontalSize = horizontalScrollBar.getScrollBarWidth();
    auto needsHorizontal = showHorizontalScrollBar && contentWidth > bounds.getWidth();
    auto needsVertical = showVerticalScrollBar && contentHeight > bounds.getHeight();

    for (int pass = 0; pass < 2; ++pass)
    {
        const auto width = bounds.getWidth() - (needsVertical ? verticalSize : 0.0f);
        const auto height = bounds.getHeight() - (needsHorizontal ? horizontalSize : 0.0f);
        needsHorizontal = showHorizontalScrollBar && contentWidth > width;
        needsVertical = showVerticalScrollBar && contentHeight > height;
    }

    visibleSize = { jmax (0.0f, bounds.getWidth() - (needsVertical ? verticalSize : 0.0f)),
                    jmax (0.0f, bounds.getHeight() - (needsHorizontal ? horizontalSize : 0.0f)) };

    if (needsVertical)
        verticalScrollBar.setBounds (visibleSize.getX(), 0.0f, verticalSize, visibleSize.getY());

    if (needsHorizontal)
        horizontalScrollBar.setBounds (0.0f, visibleSize.getY(), visibleSize.getX(), horizontalSize);

    verticalScrollBar.setRangeLimits (0.0, contentHeight);
    horizontalScrollBar.setRangeLimits (0.0, contentWidth);

    const auto maximumX = jmax (0.0f, contentWidth - visibleSize.getX());
    const auto maximumY = jmax (0.0f, contentHeight - visibleSize.getY());
    viewPosition = { jlimit (0.0f, maximumX, viewPosition.getX()),
                     jlimit (0.0f, maximumY, viewPosition.getY()) };

    verticalScrollBar.setCurrentRange (viewPosition.getY(), viewPosition.getY() + jmin (visibleSize.getY(), contentHeight));
    horizontalScrollBar.setCurrentRange (viewPosition.getX(), viewPosition.getX() + jmin (visibleSize.getX(), contentWidth));
    verticalScrollBar.setVisible (needsVertical);
    horizontalScrollBar.setVisible (needsHorizontal);
    horizontalScroller.setLimits (0.0f, maximumX, visibleSize.getX());
    verticalScroller.setLimits (0.0f, maximumY, visibleSize.getY());
    updateViewPosition();
}

void Viewport::updateViewPositionFromScroller()
{
    const auto contentWidth = viewedComponent != nullptr ? viewedComponent->getWidth() : 0.0f;
    const auto contentHeight = viewedComponent != nullptr ? viewedComponent->getHeight() : 0.0f;
    viewPosition = { jlimit (0.0f, jmax (0.0f, contentWidth - visibleSize.getX()), horizontalScroller.getOffset()),
                     jlimit (0.0f, jmax (0.0f, contentHeight - visibleSize.getY()), verticalScroller.getOffset()) };
    updateViewPosition();
    updateScrollBars();
    repaint();
}

void Viewport::updateViewPosition()
{
    if (viewedComponent != nullptr)
        viewedComponent->setTopLeft ({ -viewPosition.getX(), -viewPosition.getY() });
}

void Viewport::updateFrameAttachment()
{
    // A pan is timed by the frames, for the speed it lets go with
    const auto needsFrames = [] (const KineticScroller& scroller)
    {
        return scroller.isDragging() || scroller.isAnimating();
    };

    frameAttachment.setActive ((needsFrames (horizontalScroller) || needsFrames (verticalScroller)) && isShowing());
}

} // namespace yup
