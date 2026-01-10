/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

namespace yup
{

//==============================================================================
ScrollBar::ScrollBar (Orientation orientation)
    : orientation (orientation)
{
    setOpaque (false);
}

ScrollBar::~ScrollBar()
{
}

//==============================================================================
void ScrollBar::setOrientation (Orientation newOrientation)
{
    if (orientation != newOrientation)
    {
        orientation = newOrientation;
        updateThumbBounds();
        repaint();
    }
}

ScrollBar::Orientation ScrollBar::getOrientation() const noexcept
{
    return orientation;
}

//==============================================================================
void ScrollBar::setVisibilityMode (VisibilityMode mode)
{
    if (visibilityMode != mode)
    {
        visibilityMode = mode;
        updateVisibility();
    }
}

ScrollBar::VisibilityMode ScrollBar::getVisibilityMode() const noexcept
{
    return visibilityMode;
}

//==============================================================================
void ScrollBar::setRangeLimits (double minimum, double maximum)
{
    if (rangeMinimum != minimum || rangeMaximum != maximum)
    {
        rangeMinimum = minimum;
        rangeMaximum = jmax (minimum, maximum);

        // Clamp current range
        currentRangeStart = jlimit (rangeMinimum, rangeMaximum, currentRangeStart);
        currentRangeEnd = jlimit (currentRangeStart, rangeMaximum, currentRangeEnd);

        updateThumbBounds();
        updateVisibility();
        repaint();
    }
}

double ScrollBar::getRangeMinimum() const noexcept
{
    return rangeMinimum;
}

double ScrollBar::getRangeMaximum() const noexcept
{
    return rangeMaximum;
}

//==============================================================================
void ScrollBar::setCurrentRange (double start, double end)
{
    start = jlimit (rangeMinimum, rangeMaximum, start);
    end = jlimit (start, rangeMaximum, end);

    if (currentRangeStart != start || currentRangeEnd != end)
    {
        currentRangeStart = start;
        currentRangeEnd = end;

        updateThumbBounds();
        updateVisibility();
        repaint();
    }
}

double ScrollBar::getCurrentRangeStart() const noexcept
{
    return currentRangeStart;
}

double ScrollBar::getCurrentRangeEnd() const noexcept
{
    return currentRangeEnd;
}

double ScrollBar::getCurrentRangeSize() const noexcept
{
    return currentRangeEnd - currentRangeStart;
}

//==============================================================================
void ScrollBar::setCurrentRangeStart (double newPosition, NotificationType notification)
{
    auto rangeSize = getCurrentRangeSize();
    auto newStart = jlimit (rangeMinimum, rangeMaximum - rangeSize, newPosition);
    auto newEnd = newStart + rangeSize;

    if (currentRangeStart != newStart)
    {
        currentRangeStart = newStart;
        currentRangeEnd = newEnd;

        updateThumbBounds();
        repaint();

        if (notification != dontSendNotification && onScrollPositionChanged)
            onScrollPositionChanged (currentRangeStart);
    }
}

void ScrollBar::scrollBy (double delta, NotificationType notification)
{
    setCurrentRangeStart (currentRangeStart + delta, notification);
}

//==============================================================================
void ScrollBar::setAutoHide (bool shouldAutoHide)
{
    setVisibilityMode (shouldAutoHide ? VisibilityMode::autoHide : VisibilityMode::alwaysVisible);
}

bool ScrollBar::isAutoHide() const noexcept
{
    return visibilityMode == VisibilityMode::autoHide;
}

bool ScrollBar::isScrollingNeeded() const noexcept
{
    return getCurrentRangeSize() < (rangeMaximum - rangeMinimum);
}

//==============================================================================
void ScrollBar::setScrollBarWidth (float newSize)
{
    if (scrollBarWidth != newSize)
    {
        scrollBarWidth = jmax (1.0f, newSize);
        updateThumbBounds();
        repaint();
    }
}

float ScrollBar::getScrollBarWidth() const noexcept
{
    return scrollBarWidth;
}

//==============================================================================
void ScrollBar::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

void ScrollBar::resized()
{
    updateThumbBounds();
    updateVisibility();
}

//==============================================================================
void ScrollBar::mouseDown (const MouseEvent& event)
{
    auto clickPos = event.getPosition();
    auto thumbBounds = getThumbBounds();

    if (thumbBounds.contains (clickPos))
    {
        // Start dragging thumb
        isDraggingThumb = true;
        dragStartPosition = clickPos;
        dragStartRangeStart = currentRangeStart;

        repaint();
    }
    else
    {
        // Click on track - jump to position
        auto newPosition = pixelToPosition (orientation == Orientation::vertical
                                                ? clickPos.getY()
                                                : clickPos.getX());

        // Center the visible range at the clicked position
        auto halfRangeSize = getCurrentRangeSize() * 0.5;
        setCurrentRangeStart (newPosition - halfRangeSize, sendNotification);
    }
}

void ScrollBar::mouseUp (const MouseEvent& event)
{
    ignoreUnused (event);

    if (isDraggingThumb)
    {
        isDraggingThumb = false;

        repaint();
    }
}

void ScrollBar::mouseDrag (const MouseEvent& event)
{
    if (! isDraggingThumb)
        return;

    auto currentPos = event.getPosition();

    auto dragDelta = (orientation == Orientation::vertical)
                       ? (currentPos.getY() - dragStartPosition.getY())
                       : (currentPos.getX() - dragStartPosition.getX());

    auto trackSize = (orientation == Orientation::vertical)
                       ? getHeight()
                       : getWidth();

    auto totalRange = rangeMaximum - rangeMinimum;
    auto positionDelta = (dragDelta / trackSize) * totalRange;

    setCurrentRangeStart (dragStartRangeStart + positionDelta, sendNotification);
}

void ScrollBar::mouseEnter (const MouseEvent& event)
{
    ignoreUnused (event);
    isThumbHover = true;
    repaint();
}

void ScrollBar::mouseExit (const MouseEvent& event)
{
    ignoreUnused (event);
    isThumbHover = false;
    repaint();
}

void ScrollBar::mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData)
{
    ignoreUnused (event);

    auto delta = (orientation == Orientation::vertical)
                   ? wheelData.getDeltaY()
                   : wheelData.getDeltaX();

    // Scroll proportionally based on visible range size
    auto scrollAmount = delta * getCurrentRangeSize();
    scrollBy (-scrollAmount, sendNotification);
}

//==============================================================================
void ScrollBar::updateThumbBounds()
{
    thumbBounds = getThumbBounds();
}

void ScrollBar::updateVisibility()
{
    if (visibilityMode == VisibilityMode::alwaysHidden)
    {
        setVisible (false);
        return;
    }

    if (visibilityMode == VisibilityMode::alwaysVisible)
    {
        setVisible (true);
        return;
    }

    // Auto-hide mode
    setVisible (isScrollingNeeded());
}

Rectangle<float> ScrollBar::getThumbBounds() const
{
    auto trackBounds = getTrackBounds();

    if (rangeMaximum <= rangeMinimum)
        return trackBounds;

    auto totalRange = rangeMaximum - rangeMinimum;
    auto visibleSize = getCurrentRangeSize();
    auto visibleRatio = visibleSize / totalRange;

    // Calculate position ratio based on scrollable range (not total range)
    auto scrollableRange = totalRange - visibleSize;
    auto positionRatio = scrollableRange > 0.0 ? (currentRangeStart - rangeMinimum) / scrollableRange : 0.0;

    // Ensure minimum thumb size for usability
    auto minThumbSize = scrollBarWidth * 2.0f;

    if (orientation == Orientation::vertical)
    {
        auto trackHeight = trackBounds.getHeight();
        auto thumbHeight = jmax ((float) minThumbSize, (float) (trackHeight * visibleRatio));
        auto availableTrackHeight = trackHeight - thumbHeight;
        auto thumbY = trackBounds.getY() + availableTrackHeight * positionRatio;

        return Rectangle<float> (trackBounds.getX(), thumbY, trackBounds.getWidth(), thumbHeight);
    }
    else
    {
        auto trackWidth = trackBounds.getWidth();
        auto thumbWidth = jmax ((float) minThumbSize, (float) (trackWidth * visibleRatio));
        auto availableTrackWidth = trackWidth - thumbWidth;
        auto thumbX = trackBounds.getX() + availableTrackWidth * positionRatio;

        return Rectangle<float> (thumbX, trackBounds.getY(), thumbWidth, trackBounds.getHeight());
    }
}

Rectangle<float> ScrollBar::getTrackBounds() const
{
    auto bounds = getLocalBounds();

    if (orientation == Orientation::vertical)
    {
        // Center the track vertically, use scrollBarWidth for width
        auto x = (bounds.getWidth() - scrollBarWidth) * 0.5f;
        return Rectangle<float> (x, 0.0f, scrollBarWidth, bounds.getHeight());
    }
    else
    {
        // Center the track horizontally, use scrollBarWidth for height
        auto y = (bounds.getHeight() - scrollBarWidth) * 0.5f;
        return Rectangle<float> (0.0f, y, bounds.getWidth(), scrollBarWidth);
    }
}

bool ScrollBar::isThumbHovered (Point<float> position) const
{
    return thumbBounds.contains (position);
}

double ScrollBar::pixelToPosition (float pixel) const
{
    auto trackBounds = getTrackBounds();
    auto trackSize = (orientation == Orientation::vertical)
                       ? trackBounds.getHeight()
                       : trackBounds.getWidth();

    auto trackStart = (orientation == Orientation::vertical)
                        ? trackBounds.getY()
                        : trackBounds.getX();

    if (trackSize <= 0.0f)
        return rangeMinimum;

    auto ratio = (pixel - trackStart) / trackSize;
    auto totalRange = rangeMaximum - rangeMinimum;

    return rangeMinimum + ratio * totalRange;
}

float ScrollBar::positionToPixel (double position) const
{
    auto trackBounds = getTrackBounds();
    auto trackSize = (orientation == Orientation::vertical)
                       ? trackBounds.getHeight()
                       : trackBounds.getWidth();

    auto trackStart = (orientation == Orientation::vertical)
                        ? trackBounds.getY()
                        : trackBounds.getX();

    auto totalRange = rangeMaximum - rangeMinimum;

    if (totalRange <= 0.0)
        return trackStart;

    auto ratio = (position - rangeMinimum) / totalRange;

    return trackStart + ratio * trackSize;
}

} // namespace yup
