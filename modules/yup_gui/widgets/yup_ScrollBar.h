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
/**
    A scrollbar component with macOS-style appearance and behavior.

    The ScrollBar provides a draggable thumb to scroll through content. It supports
    both vertical and horizontal orientations, auto-hide behavior, and proportional
    sizing based on the visible content ratio.

    The scrollbar has a rounded appearance similar to modern macOS scrollbars.

    @code
    ScrollBar scrollBar (ScrollBar::Orientation::vertical);
    scrollBar.setRangeLimits (0.0, 1000.0);
    scrollBar.setCurrentRange (0.0, 100.0);
    scrollBar.onScrollPositionChanged = [](double newPosition)
    {
        // Handle scroll
    };
    @endcode

    @see ListBox
*/
class YUP_API ScrollBar : public Component
{
public:
    //==============================================================================
    /** Defines the orientation of the scrollbar. */
    enum class Orientation
    {
        vertical,  /**< Vertical scrollbar (scrolls up/down). */
        horizontal /**< Horizontal scrollbar (scrolls left/right). */
    };

    /** Defines when the scrollbar should be visible. */
    enum class VisibilityMode
    {
        alwaysVisible, /**< Always show the scrollbar. */
        autoHide,      /**< Show only when content exceeds viewport. */
        alwaysHidden   /**< Never show the scrollbar. */
    };

    //==============================================================================
    /** Creates a ScrollBar.

        @param orientation  The orientation of the scrollbar
    */
    explicit ScrollBar (Orientation orientation = Orientation::vertical);

    /** Destructor. */
    ~ScrollBar() override;

    //==============================================================================
    /** Sets the orientation of the scrollbar.

        @param newOrientation  The orientation to use
    */
    void setOrientation (Orientation newOrientation);

    /** Returns the current orientation.

        @return The orientation
    */
    Orientation getOrientation() const noexcept;

    //==============================================================================
    /** Sets the visibility mode.

        @param mode  The visibility mode to use
    */
    void setVisibilityMode (VisibilityMode mode);

    /** Returns the current visibility mode.

        @return The visibility mode
    */
    VisibilityMode getVisibilityMode() const noexcept;

    //==============================================================================
    /** Sets the total range of scrollable content.

        @param minimum  The minimum scroll position
        @param maximum  The maximum scroll position
    */
    void setRangeLimits (double minimum, double maximum);

    /** Returns the minimum scroll position.

        @return The minimum position
    */
    double getRangeMinimum() const noexcept;

    /** Returns the maximum scroll position.

        @return The maximum position
    */
    double getRangeMaximum() const noexcept;

    //==============================================================================
    /** Sets the current visible range.

        This represents the portion of content that is currently visible.
        The thumb size is calculated proportionally based on this range.

        @param start  The start of the visible range
        @param end    The end of the visible range
    */
    void setCurrentRange (double start, double end);

    /** Returns the start of the current visible range.

        @return The range start position
    */
    double getCurrentRangeStart() const noexcept;

    /** Returns the end of the current visible range.

        @return The range end position
    */
    double getCurrentRangeEnd() const noexcept;

    /** Returns the size of the current visible range.

        @return The range size
    */
    double getCurrentRangeSize() const noexcept;

    //==============================================================================
    /** Scrolls to a specific position.

        @param newPosition       The new scroll position
        @param notification      Whether to send change notifications
    */
    void setCurrentRangeStart (double newPosition, NotificationType notification = sendNotification);

    /** Scrolls by a delta amount.

        @param delta         The amount to scroll (positive = forward, negative = backward)
        @param notification  Whether to send change notifications
    */
    void scrollBy (double delta, NotificationType notification = sendNotification);

    //==============================================================================
    /** Sets whether the scrollbar should be automatically hidden when not needed.

        This is a convenience method that sets the visibility mode.

        @param shouldAutoHide  True to enable auto-hide behavior
    */
    void setAutoHide (bool shouldAutoHide);

    /** Returns whether auto-hide is enabled.

        @return True if auto-hide is enabled
    */
    bool isAutoHide() const noexcept;

    /** Returns whether the scrollbar is currently needed (content exceeds viewport).

        @return True if scrolling is needed
    */
    bool isScrollingNeeded() const noexcept;

    //==============================================================================
    /** Sets the width of the scrollbar (for vertical) or height (for horizontal).

        @param newSize  The scrollbar width/height in pixels
    */
    void setScrollBarWidth (float newSize);

    /** Returns the scrollbar width/height.

        @return The scrollbar width/height in pixels
    */
    float getScrollBarWidth() const noexcept;

    //==============================================================================
    /** Callback invoked when the scroll position changes.

        @param newPosition  The new scroll position
    */
    std::function<void (double newPosition)> onScrollPositionChanged;

    //==============================================================================
    /** Style identifiers for theming. */
    struct Style
    {
        static inline const Identifier trackColorId { "scrollBarTrack" };
        static inline const Identifier thumbColorId { "scrollBarThumb" };
        static inline const Identifier thumbHoverColorId { "scrollBarThumbHover" };
        static inline const Identifier thumbDraggingColorId { "scrollBarThumbDragging" };
    };

    //==============================================================================
    /** Returns whether the thumb is currently being dragged.

        This is used by the theme for rendering.

        @return True if dragging
    */
    bool isDragging() const noexcept { return isDraggingThumb; }

    /** Returns whether the thumb is currently hovered.

        This is used by the theme for rendering.

        @return True if hovered
    */
    bool isThumbHovered() const noexcept { return isThumbHover; }

    /** Returns the bounds of the thumb for rendering.

        This is used by the theme for rendering.

        @return The thumb bounds
    */
    Rectangle<float> getThumbBoundsForRendering() const { return thumbBounds; }

    /** Returns the bounds of the track for rendering.

        This is used by the theme for rendering.

        @return The track bounds
    */
    Rectangle<float> getTrackBoundsForRendering() const { return getTrackBounds(); }

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void mouseUp (const MouseEvent& event) override;
    /** @internal */
    void mouseDrag (const MouseEvent& event) override;
    /** @internal */
    void mouseEnter (const MouseEvent& event) override;
    /** @internal */
    void mouseExit (const MouseEvent& event) override;
    /** @internal */
    void mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) override;

private:
    //==============================================================================
    void updateThumbBounds();
    void updateVisibility();
    Rectangle<float> getThumbBounds() const;
    Rectangle<float> getTrackBounds() const;
    bool isThumbHovered (Point<float> position) const;
    double pixelToPosition (float pixel) const;
    float positionToPixel (double position) const;

    //==============================================================================
    Orientation orientation = Orientation::vertical;
    VisibilityMode visibilityMode = VisibilityMode::autoHide;

    double rangeMinimum = 0.0;
    double rangeMaximum = 1.0;
    double currentRangeStart = 0.0;
    double currentRangeEnd = 1.0;

    Rectangle<float> thumbBounds;
    bool isDraggingThumb = false;
    bool isThumbHover = false;
    Point<float> dragStartPosition;
    double dragStartRangeStart = 0.0;

    float scrollBarWidth = 12.0f;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScrollBar)
};

} // namespace yup
