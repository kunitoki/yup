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

#pragma once

namespace yup
{

//==============================================================================
/**
    An interactive bar (sash) splitting an area into a leading and a trailing pane.

    The divider only tracks its own position along the split axis: the parent stays in charge of
    the layout, calling layoutComponents() in its resized(), or reading getEffectivePosition()
    and placing the divider and the two panes around it itself. The position is measured in the parent space, from the leading edge of
    the available extent to the leading edge of the bar, so it always lies in
    `[0, availableExtent - thickness]`.

    The divider supports pixel or percentage minimum and maximum positions, a policy deciding
    how the position follows changes of the available extent, snap points, collapsing either
    pane by dragging past its limit, resetting to a default position on double click, and
    keyboard nudging with the arrow, Home and End keys.

    @code
    divider.onPositionChanged = [this] (float) { resized(); };
    divider.setMinPosition (ResizableDivider::Length::pixels (120.0f));
    divider.setMinTrailingSize (ResizableDivider::Length::percent (20.0f));
    addAndMakeVisible (divider);

    void resized() override
    {
        divider.layoutComponents (getLocalBounds(), &leftPane, &rightPane);
    }
    @endcode

    @see ResizableCorner, ScrollBar

    @tags{UI}
*/
class YUP_API ResizableDivider : public Component
{
public:
    //==============================================================================
    /** The direction of the divider bar. */
    enum class Orientation
    {
        vertical,  /**< A vertical bar separating a left and a right pane, dragged along the X axis. */
        horizontal /**< A horizontal bar separating a top and a bottom pane, dragged along the Y axis. */
    };

    /** The unit of a Length. */
    enum class Unit
    {
        pixels, /**< An absolute distance, in points. */
        percent /**< A percentage of the usable extent, in `[0, 100]`. */
    };

    /** A distance along the split axis, in pixels or as a percentage of the usable extent. */
    struct Length
    {
        float value = 0.0f; /**< The distance, in the given unit. */
        Unit unit = Unit::pixels; /**< The unit of the value. */

        /** Creates a length in pixels. */
        static constexpr Length pixels (float pixelValue) noexcept { return { pixelValue, Unit::pixels }; }

        /** Creates a length as a percentage of the usable extent. */
        static constexpr Length percent (float percentValue) noexcept { return { percentValue, Unit::percent }; }

        /** Resolves the length to pixels against the usable extent (the available extent minus the thickness). */
        float resolve (float usableExtent) const noexcept
        {
            return unit == Unit::percent ? usableExtent * value / 100.0f : value;
        }

        bool operator== (const Length& other) const noexcept = default;
    };

    /** How the position follows changes of the available extent. */
    enum class ResizePolicy
    {
        proportional,    /**< Keeps the ratio between the position and the usable extent. */
        preserveLeading, /**< Keeps the size of the leading pane. */
        preserveTrailing /**< Keeps the size of the trailing pane. */
    };

    /** Which pane, if any, is collapsed. */
    enum class CollapseSide
    {
        none,    /**< No pane is collapsed. */
        leading, /**< The leading pane is collapsed, the divider sits at 0. */
        trailing /**< The trailing pane is collapsed, the divider sits at the end of the usable extent. */
    };

    /** The grip drawn in the middle of the bar. */
    enum class GripStyle
    {
        none, /**< Only the line is drawn. */
        pill, /**< A rounded capsule. */
        dots  /**< Three dots. */
    };

    //==============================================================================
    /** Creates a divider.

        @param orientation  The direction of the bar.
        @param componentID  An optional identifier for this component.
    */
    explicit ResizableDivider (Orientation orientation = Orientation::vertical, StringRef componentID = {});

    /** Destructor. */
    ~ResizableDivider() override;

    //==============================================================================
    /** Sets the direction of the bar, which also selects its mouse cursor. */
    void setOrientation (Orientation newOrientation);

    /** Returns the direction of the bar. */
    Orientation getOrientation() const noexcept { return orientation; }

    /** Sets the thickness of the interactive bar, in points. Defaults to 6. */
    void setThickness (float newThickness);

    /** Returns the thickness of the interactive bar, in points. */
    float getThickness() const noexcept { return thickness; }

    /** Sets the thickness of the line drawn when idle, in points. Defaults to 1. */
    void setLineThickness (float newThickness);

    /** Returns the thickness of the line drawn when idle, in points. */
    float getLineThickness() const noexcept { return lineThickness; }

    /** Sets the thickness of the line drawn while hovered, focused or dragged, in points. Defaults to 3. */
    void setActiveLineThickness (float newThickness);

    /** Returns the thickness of the line drawn while hovered, focused or dragged, in points. */
    float getActiveLineThickness() const noexcept { return activeLineThickness; }

    /** Sets the grip drawn in the middle of the bar. Defaults to GripStyle::pill. */
    void setGripStyle (GripStyle newGripStyle);

    /** Returns the grip drawn in the middle of the bar. */
    GripStyle getGripStyle() const noexcept { return gripStyle; }

    /** Returns true while the mouse is over the bar. */
    bool isHovered() const noexcept { return hovered; }

    /** Returns true while the bar is being dragged. */
    bool isDragging() const noexcept { return dragging; }

    //==============================================================================
    /** Sets the extent the divider splits, along its axis.

        When unset (the default) the parent width is used for a vertical divider and the parent
        height for a horizontal one.
    */
    void setAvailableExtent (std::optional<float> newExtent);

    /** Returns the extent the divider splits: the explicit one, or the parent's, or 0 when unparented. */
    float getAvailableExtent() const noexcept;

    /** Returns the extent the position can move in: the available extent minus the thickness. */
    float getUsableExtent() const noexcept;

    /** Moves the divider to a position in pixels, clamped to the constraints, and expands a collapsed pane. */
    void setPosition (float newPosition, NotificationType notification = sendNotification);

    /** Moves the divider to a position in pixels or percent, clamped to the constraints, and expands a collapsed pane.

        A position set before the extent is known is kept as is and resolved once it is.
    */
    void setPosition (Length newPosition, NotificationType notification = sendNotification);

    /** Returns the position ignoring the collapsed state, resolved against the current extent and clamped. */
    float getPosition() const noexcept;

    /** Moves the divider to a position expressed as a ratio of the usable extent, in `[0, 1]`. */
    void setNormalizedPosition (float newRatio, NotificationType notification = sendNotification);

    /** Returns the position as a ratio of the usable extent, in `[0, 1]`. */
    float getNormalizedPosition() const noexcept;

    /** Returns the position the parent should lay out: 0 when the leading pane is collapsed, the usable
        extent when the trailing pane is collapsed, getPosition() otherwise.
    */
    float getEffectivePosition() const noexcept;

    /** Lays out the divider and the two panes it splits inside an area of the parent.

        The available extent becomes the width (vertical) or height (horizontal) of the area, and
        the position is measured from its leading edge. The leading pane takes the space before
        the bar and the trailing pane the space after it; a collapsed pane gets an empty bounds.

        Call it from the parent's resized(), and from onPositionChanged to follow the bar.

        @param area     The area to split, in the parent space.
        @param leading  The left (vertical) or top (horizontal) pane, or nullptr.
        @param trailing The right (vertical) or bottom (horizontal) pane, or nullptr.
    */
    void layoutComponents (Rectangle<float> area, Component* leading, Component* trailing);

    //==============================================================================
    /** Sets the minimum position, or none to allow 0. */
    void setMinPosition (std::optional<Length> newMinPosition);

    /** Returns the minimum position. */
    std::optional<Length> getMinPosition() const noexcept { return minPosition; }

    /** Sets the maximum position, or none to allow the whole usable extent. */
    void setMaxPosition (std::optional<Length> newMaxPosition);

    /** Returns the maximum position. */
    std::optional<Length> getMaxPosition() const noexcept { return maxPosition; }

    /** Sets the minimum size of the trailing pane, combined with the maximum position. */
    void setMinTrailingSize (std::optional<Length> newMinTrailingSize);

    /** Returns the minimum size of the trailing pane. */
    std::optional<Length> getMinTrailingSize() const noexcept { return minTrailingSize; }

    /** Returns the minimum position resolved against the current extent. */
    float getResolvedMinPosition() const noexcept;

    /** Returns the maximum position resolved against the current extent, never below the minimum. */
    float getResolvedMaxPosition() const noexcept;

    /** Sets how the position follows changes of the available extent. Defaults to ResizePolicy::proportional. */
    void setResizePolicy (ResizePolicy newPolicy);

    /** Returns how the position follows changes of the available extent. */
    ResizePolicy getResizePolicy() const noexcept { return resizePolicy; }

    /** Sets the position restored by a double click, or none to disable the reset. */
    void setDefaultPosition (std::optional<Length> newDefaultPosition);

    /** Returns the position restored by a double click. */
    std::optional<Length> getDefaultPosition() const noexcept { return defaultPosition; }

    //==============================================================================
    /** Replaces the positions a drag snaps to. */
    void setSnapPositions (Span<const Length> newSnapPositions);

    /** Adds a position a drag snaps to. */
    void addSnapPosition (Length snapPosition);

    /** Removes every snap position. */
    void clearSnapPositions();

    /** Returns the positions a drag snaps to. */
    const std::vector<Length>& getSnapPositions() const noexcept { return snapPositions; }

    /** Sets how close a drag must get to a snap position to snap to it, in points. Defaults to 6. */
    void setSnapThreshold (float newThreshold);

    /** Returns how close a drag must get to a snap position to snap to it, in points. */
    float getSnapThreshold() const noexcept { return snapThreshold; }

    //==============================================================================
    /** Allows collapsing the leading pane by dragging past the minimum position. */
    void setCanCollapseLeading (bool shouldBeAllowed);

    /** Returns true when dragging can collapse the leading pane. */
    bool canCollapseLeading() const noexcept { return collapseLeadingAllowed; }

    /** Allows collapsing the trailing pane by dragging past the maximum position. */
    void setCanCollapseTrailing (bool shouldBeAllowed);

    /** Returns true when dragging can collapse the trailing pane. */
    bool canCollapseTrailing() const noexcept { return collapseTrailingAllowed; }

    /** Sets how far past a limit a drag must go to collapse the pane, in points. Defaults to 32. */
    void setCollapseThreshold (float newThreshold);

    /** Returns how far past a limit a drag must go to collapse the pane, in points. */
    float getCollapseThreshold() const noexcept { return collapseThreshold; }

    /** Collapses a pane, or expands it back to the previous position with CollapseSide::none.

        This works regardless of canCollapseLeading() and canCollapseTrailing(), which only
        govern dragging.
    */
    void setCollapsedSide (CollapseSide newSide, NotificationType notification = sendNotification);

    /** Returns the collapsed pane, if any. */
    CollapseSide getCollapsedSide() const noexcept { return collapsedSide; }

    /** Returns true when a pane is collapsed. */
    bool isCollapsed() const noexcept { return collapsedSide != CollapseSide::none; }

    //==============================================================================
    /** Sets how far an arrow key moves the divider, in points. Defaults to 10, Shift moves 5 times further. */
    void setKeyboardStep (float newStep);

    /** Returns how far an arrow key moves the divider, in points. */
    float getKeyboardStep() const noexcept { return keyboardStep; }

    //==============================================================================
    /** Called with the effective position whenever it changes. */
    std::function<void (float)> onPositionChanged;

    /** Called during a drag with the change of the effective position, in points. */
    std::function<void (float)> onDragDelta;

    /** Called when a drag starts. */
    std::function<void()> onDragStarted;

    /** Called when a drag ends. */
    std::function<void()> onDragEnded;

    /** Called with the collapsed pane whenever it changes. */
    std::function<void (CollapseSide)> onCollapsedChanged;

    /** Called when the bar is double clicked, after resetting to the default position if any. */
    std::function<void()> onDoubleClick;

    //==============================================================================
    /** Style identifiers for theme customization. */
    struct Style
    {
        static const Identifier backgroundColorId; /**< The fill of the whole bar. */
        static const Identifier lineColorId;       /**< The line drawn when idle. */
        static const Identifier hoverColorId;      /**< The line drawn while hovered or focused. */
        static const Identifier draggingColorId;   /**< The line drawn while dragged. */
        static const Identifier gripColorId;       /**< The grip in the middle of the bar. */
    };

    //==============================================================================
    /** Returns the static type info describing the ResizableDivider type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

    //==============================================================================
    /** @internal */
    void mouseEnter (const MouseEvent& event) override;
    /** @internal */
    void mouseExit (const MouseEvent& event) override;
    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void mouseDrag (const MouseEvent& event) override;
    /** @internal */
    void mouseUp (const MouseEvent& event) override;
    /** @internal */
    void mouseDoubleClick (const MouseEvent& event) override;
    /** @internal */
    bool keyDown (const KeyPress& key, const Point<float>& position) override;
    /** @internal */
    void focusGained() override;
    /** @internal */
    void focusLost() override;
    /** @internal */
    void parentSizeChanged() override;
    /** @internal */
    void parentHierarchyChanged() override;

private:
    //==============================================================================
    float getPointerAlongAxis (const MouseEvent& event) const;
    float getUnclampedPosition (float usableExtent) const noexcept;
    float clampPosition (float position, float usableExtent) const noexcept;
    float resolveMinPosition (float usableExtent) const noexcept;
    float resolveMaxPosition (float usableExtent) const noexcept;
    void storePosition (Length newPosition);
    void commitPendingPosition();
    void setCollapsedSideInternal (CollapseSide newSide, NotificationType notification);
    void updateEffectivePosition (NotificationType notification);
    void setDragging (bool shouldBeDragging);
    void constraintsChanged();

    //==============================================================================
    Orientation orientation;
    float thickness = 6.0f;
    float lineThickness = 1.0f;
    float activeLineThickness = 3.0f;
    GripStyle gripStyle = GripStyle::pill;

    std::optional<float> availableExtent;
    std::optional<Length> pendingPosition = Length::percent (50.0f);
    float storedRatio = 0.5f;
    float storedLeadingPixels = 0.0f;
    float storedTrailingPixels = 0.0f;
    float lastEffectivePosition = 0.0f;

    std::optional<Length> minPosition;
    std::optional<Length> maxPosition;
    std::optional<Length> minTrailingSize;
    std::optional<Length> defaultPosition;
    ResizePolicy resizePolicy = ResizePolicy::proportional;

    std::vector<Length> snapPositions;
    float snapThreshold = 6.0f;

    bool collapseLeadingAllowed = false;
    bool collapseTrailingAllowed = false;
    float collapseThreshold = 32.0f;
    CollapseSide collapsedSide = CollapseSide::none;

    float keyboardStep = 10.0f;

    bool hovered = false;
    bool dragging = false;
    float dragStartPointer = 0.0f;
    float dragStartPosition = 0.0f;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResizableDivider)
};

} // namespace yup
