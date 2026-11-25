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

namespace yup
{

namespace
{

struct ResizableDividerIds
{
    static inline const Identifier orientation { "orientation" };
    static inline const Identifier gripStyle { "gripStyle" };
    static inline const Identifier resizePolicy { "resizePolicy" };
    static inline const Identifier thickness { "thickness" };
    static inline const Identifier lineThickness { "lineThickness" };
    static inline const Identifier activeLineThickness { "activeLineThickness" };
    static inline const Identifier position { "position" };
    static inline const Identifier normalized { "normalized" };
    static inline const Identifier dragging { "dragging" };
    static inline const Identifier collapsed { "collapsed" };
};

} // namespace

//==============================================================================
const Identifier ResizableDivider::Style::backgroundColorId { "resizableDividerBackground" };
const Identifier ResizableDivider::Style::lineColorId { "resizableDividerLine" };
const Identifier ResizableDivider::Style::hoverColorId { "resizableDividerHover" };
const Identifier ResizableDivider::Style::draggingColorId { "resizableDividerDragging" };
const Identifier ResizableDivider::Style::gripColorId { "resizableDividerGrip" };

//==============================================================================
const ComponentTypeInfo& ResizableDivider::getClassTypeInfo()
{
    static const auto info = ComponentTypeBuilder<ResizableDivider> (Identifier ("ResizableDivider"))
                                 .color ("backgroundColor", Style::backgroundColorId)
                                 .color ("lineColor", Style::lineColorId)
                                 .color ("hoverColor", Style::hoverColorId)
                                 .color ("draggingColor", Style::draggingColorId)
                                 .color ("gripColor", Style::gripColorId)
                                 .enumeration (ResizableDividerIds::orientation, { "vertical", "horizontal" }, &ResizableDivider::getOrientation, &ResizableDivider::setOrientation, "The direction of the bar.")
                                 .enumeration (ResizableDividerIds::gripStyle, { "none", "pill", "dots" }, &ResizableDivider::getGripStyle, &ResizableDivider::setGripStyle, "The grip drawn in the middle of the bar.")
                                 .enumeration (ResizableDividerIds::resizePolicy, { "proportional", "preserveLeading", "preserveTrailing" }, &ResizableDivider::getResizePolicy, &ResizableDivider::setResizePolicy, "How the position follows changes of the available extent.")
                                 .property (ResizableDividerIds::thickness, &ResizableDivider::getThickness, &ResizableDivider::setThickness, 6.0f, "The thickness of the interactive bar, in points.")
                                 .property (ResizableDividerIds::lineThickness, &ResizableDivider::getLineThickness, &ResizableDivider::setLineThickness, 1.0f, "The thickness of the idle line, in points.")
                                 .property (ResizableDividerIds::activeLineThickness, &ResizableDivider::getActiveLineThickness, &ResizableDivider::setActiveLineThickness, 3.0f, "The thickness of the hovered, focused or dragged line, in points.")
                                 .addProperty (ComponentProperty {
                                     .name = ResizableDividerIds::position,
                                     .type = ComponentProperty::Type::number,
                                     .getter = [] (const Component& c)
                                     {
                                         // A divider that is nowhere, as a new one, has no position in points to tell
                                         const auto& divider = static_cast<const ResizableDivider&> (c);
                                         const auto hasExtent = divider.availableExtent.has_value() || divider.getParentComponent() != nullptr;

                                         return hasExtent ? var (static_cast<double> (divider.getPosition())) : var();
                                     },
                                     .setter = [] (Component& c, const var& v)
                                     {
                                         // No position puts the divider back where a new one is, in the middle
                                         static_cast<ResizableDivider&> (c).setPosition (v.isVoid() ? Length::percent (50.0f) : Length::pixels (static_cast<float> (static_cast<double> (v))));
                                     },
                                     .isStyleable = false,
                                     .description = "The position of the divider in the parent, in points." })
                                 .property (ResizableDividerIds::normalized, &ResizableDivider::getNormalizedPosition, [] (ResizableDivider& d, float v) { d.setNormalizedPosition (v); }, 0.5f, "The position as a ratio of the usable extent.")
                                 .state (ResizableDividerIds::dragging, &ResizableDivider::isDragging)
                                 .state (ResizableDividerIds::collapsed, &ResizableDivider::isCollapsed)
                                 .signal ("onPositionChanged", &ResizableDivider::onPositionChanged, "Emitted with the effective position when it changes.", { { Identifier ("position"), ComponentProperty::Type::number } })
                                 .signal ("onDragStarted", &ResizableDivider::onDragStarted, "Emitted when a drag starts.")
                                 .signal ("onDragEnded", &ResizableDivider::onDragEnded, "Emitted when a drag ends.")
                                 .signal ("onDoubleClick", &ResizableDivider::onDoubleClick, "Emitted when the bar is double clicked.")
                                 .build();

    return info;
}

//==============================================================================
ResizableDivider::ResizableDivider (Orientation orientation, StringRef componentID)
    : Component (componentID)
    , orientation (orientation)
{
    setOpaque (false);
    setWantsKeyboardFocus (true);
    setMouseCursor (orientation == Orientation::vertical ? MouseCursor::ResizeLeftRight : MouseCursor::ResizeUpDown);
}

ResizableDivider::~ResizableDivider()
{
}

//==============================================================================
void ResizableDivider::setOrientation (Orientation newOrientation)
{
    if (orientation == newOrientation)
        return;

    orientation = newOrientation;
    setMouseCursor (orientation == Orientation::vertical ? MouseCursor::ResizeLeftRight : MouseCursor::ResizeUpDown);
    notifyPropertyChanged (ResizableDividerIds::orientation);
    repaint();
    constraintsChanged();
}

void ResizableDivider::setThickness (float newThickness)
{
    newThickness = jmax (1.0f, newThickness);
    if (approximatelyEqual (thickness, newThickness))
        return;

    thickness = newThickness;
    notifyPropertyChanged (ResizableDividerIds::thickness);
    repaint();
    constraintsChanged();
}

void ResizableDivider::setLineThickness (float newThickness)
{
    newThickness = jmax (0.0f, newThickness);
    if (approximatelyEqual (lineThickness, newThickness))
        return;

    lineThickness = newThickness;
    notifyPropertyChanged (ResizableDividerIds::lineThickness);
    repaint();
}

void ResizableDivider::setActiveLineThickness (float newThickness)
{
    newThickness = jmax (0.0f, newThickness);
    if (approximatelyEqual (activeLineThickness, newThickness))
        return;

    activeLineThickness = newThickness;
    notifyPropertyChanged (ResizableDividerIds::activeLineThickness);
    repaint();
}

void ResizableDivider::setGripStyle (GripStyle newGripStyle)
{
    if (gripStyle == newGripStyle)
        return;

    gripStyle = newGripStyle;
    notifyPropertyChanged (ResizableDividerIds::gripStyle);
    repaint();
}

//==============================================================================
void ResizableDivider::setAvailableExtent (std::optional<float> newExtent)
{
    if (availableExtent == newExtent)
        return;

    availableExtent = newExtent;
    commitPendingPosition();
    constraintsChanged();
}

float ResizableDivider::getAvailableExtent() const noexcept
{
    if (availableExtent.has_value())
        return jmax (0.0f, *availableExtent);

    const auto* parent = getParentComponent();
    if (parent == nullptr)
        return 0.0f;

    return orientation == Orientation::vertical ? parent->getWidth() : parent->getHeight();
}

float ResizableDivider::getUsableExtent() const noexcept
{
    return jmax (0.0f, getAvailableExtent() - thickness);
}

void ResizableDivider::setPosition (float newPosition, NotificationType notification)
{
    setPosition (Length::pixels (newPosition), notification);
}

void ResizableDivider::setPosition (Length newPosition, NotificationType notification)
{
    storePosition (newPosition);
    setCollapsedSideInternal (CollapseSide::none, notification);
    updateEffectivePosition (notification);
}

float ResizableDivider::getPosition() const noexcept
{
    const auto usableExtent = getUsableExtent();
    return clampPosition (getUnclampedPosition (usableExtent), usableExtent);
}

void ResizableDivider::setNormalizedPosition (float newRatio, NotificationType notification)
{
    setPosition (Length::percent (newRatio * 100.0f), notification);
}

float ResizableDivider::getNormalizedPosition() const noexcept
{
    if (const auto usableExtent = getUsableExtent(); usableExtent > 0.0f)
        return getPosition() / usableExtent;

    if (pendingPosition.has_value() && pendingPosition->unit == Unit::percent)
        return jlimit (0.0f, 1.0f, pendingPosition->value / 100.0f);

    return storedRatio;
}

float ResizableDivider::getEffectivePosition() const noexcept
{
    switch (collapsedSide)
    {
        case CollapseSide::leading:
            return 0.0f;

        case CollapseSide::trailing:
            return getUsableExtent();

        case CollapseSide::none:
            break;
    }

    return getPosition();
}

void ResizableDivider::layoutComponents (Rectangle<float> area, Component* leading, Component* trailing)
{
    const auto isVertical = orientation == Orientation::vertical;
    setAvailableExtent (isVertical ? area.getWidth() : area.getHeight());

    const auto position = getEffectivePosition();
    const auto barStart = isVertical ? area.withTrimmedLeft (position) : area.withTrimmedTop (position);
    const auto trailingArea = isVertical ? barStart.withTrimmedLeft (thickness) : barStart.withTrimmedTop (thickness);

    setBounds (isVertical ? barStart.withWidth (jmin (thickness, barStart.getWidth()))
                          : barStart.withHeight (jmin (thickness, barStart.getHeight())));

    if (leading != nullptr)
        leading->setBounds (isVertical ? area.withWidth (barStart.getX() - area.getX())
                                       : area.withHeight (barStart.getY() - area.getY()));

    if (trailing != nullptr)
        trailing->setBounds (trailingArea);
}

//==============================================================================
void ResizableDivider::setMinPosition (std::optional<Length> newMinPosition)
{
    minPosition = newMinPosition;
    constraintsChanged();
}

void ResizableDivider::setMaxPosition (std::optional<Length> newMaxPosition)
{
    maxPosition = newMaxPosition;
    constraintsChanged();
}

void ResizableDivider::setMinTrailingSize (std::optional<Length> newMinTrailingSize)
{
    minTrailingSize = newMinTrailingSize;
    constraintsChanged();
}

float ResizableDivider::getResolvedMinPosition() const noexcept
{
    return resolveMinPosition (getUsableExtent());
}

float ResizableDivider::getResolvedMaxPosition() const noexcept
{
    return resolveMaxPosition (getUsableExtent());
}

void ResizableDivider::setResizePolicy (ResizePolicy newPolicy)
{
    if (resizePolicy == newPolicy)
        return;

    // Re-anchor at the current position so switching policy never makes the divider jump.
    if (! pendingPosition.has_value() && getUsableExtent() > 0.0f)
        storePosition (Length::pixels (getPosition()));

    resizePolicy = newPolicy;
    notifyPropertyChanged (ResizableDividerIds::resizePolicy);
}

void ResizableDivider::setDefaultPosition (std::optional<Length> newDefaultPosition)
{
    defaultPosition = newDefaultPosition;
}

//==============================================================================
void ResizableDivider::setSnapPositions (Span<const Length> newSnapPositions)
{
    snapPositions.assign (newSnapPositions.begin(), newSnapPositions.end());
}

void ResizableDivider::addSnapPosition (Length snapPosition)
{
    snapPositions.push_back (snapPosition);
}

void ResizableDivider::clearSnapPositions()
{
    snapPositions.clear();
}

void ResizableDivider::setSnapThreshold (float newThreshold)
{
    snapThreshold = jmax (0.0f, newThreshold);
}

//==============================================================================
void ResizableDivider::setCanCollapseLeading (bool shouldBeAllowed)
{
    collapseLeadingAllowed = shouldBeAllowed;
}

void ResizableDivider::setCanCollapseTrailing (bool shouldBeAllowed)
{
    collapseTrailingAllowed = shouldBeAllowed;
}

void ResizableDivider::setCollapseThreshold (float newThreshold)
{
    collapseThreshold = jmax (0.0f, newThreshold);
}

void ResizableDivider::setCollapsedSide (CollapseSide newSide, NotificationType notification)
{
    setCollapsedSideInternal (newSide, notification);
    updateEffectivePosition (notification);
}

//==============================================================================
void ResizableDivider::setKeyboardStep (float newStep)
{
    keyboardStep = jmax (0.0f, newStep);
}

//==============================================================================
void ResizableDivider::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

void ResizableDivider::mouseEnter (const MouseEvent& event)
{
    ignoreUnused (event);

    hovered = true;
    repaint();
}

void ResizableDivider::mouseExit (const MouseEvent& event)
{
    ignoreUnused (event);

    hovered = false;
    repaint();
}

void ResizableDivider::mouseDown (const MouseEvent& event)
{
    dragStartPointer = getPointerAlongAxis (event);
    dragStartPosition = getEffectivePosition();
    setDragging (true);

    if (onDragStarted)
        onDragStarted();
}

void ResizableDivider::mouseDrag (const MouseEvent& event)
{
    if (! dragging)
        return;

    const auto usableExtent = getUsableExtent();
    const auto requested = dragStartPosition + getPointerAlongAxis (event) - dragStartPointer;
    const auto previous = getEffectivePosition();

    if (collapseLeadingAllowed && requested < resolveMinPosition (usableExtent) - collapseThreshold)
    {
        setCollapsedSideInternal (CollapseSide::leading, sendNotification);
    }
    else if (collapseTrailingAllowed && requested > resolveMaxPosition (usableExtent) + collapseThreshold)
    {
        setCollapsedSideInternal (CollapseSide::trailing, sendNotification);
    }
    else
    {
        auto target = clampPosition (requested, usableExtent);
        auto bestDistance = snapThreshold;

        for (const auto& snapPosition : snapPositions)
        {
            const auto snapped = clampPosition (snapPosition.resolve (usableExtent), usableExtent);
            if (const auto distance = std::abs (snapped - requested); distance <= bestDistance)
            {
                bestDistance = distance;
                target = snapped;
            }
        }

        storePosition (Length::pixels (target));
        setCollapsedSideInternal (CollapseSide::none, sendNotification);
    }

    const BailOutChecker bailOutChecker (this);
    updateEffectivePosition (sendNotification);

    if (bailOutChecker.shouldBailOut())
        return;

    if (const auto delta = getEffectivePosition() - previous; delta != 0.0f && onDragDelta)
        onDragDelta (delta);
}

void ResizableDivider::mouseUp (const MouseEvent& event)
{
    ignoreUnused (event);

    if (! dragging)
        return;

    setDragging (false);

    if (onDragEnded)
        onDragEnded();
}

void ResizableDivider::mouseDoubleClick (const MouseEvent& event)
{
    ignoreUnused (event);

    const BailOutChecker bailOutChecker (this);

    if (defaultPosition.has_value())
        setPosition (*defaultPosition);

    if (! bailOutChecker.shouldBailOut() && onDoubleClick)
        onDoubleClick();
}

bool ResizableDivider::keyDown (const KeyPress& key, const Point<float>& position)
{
    ignoreUnused (position);

    const auto keyCode = key.getKey();
    const auto previousKey = orientation == Orientation::vertical ? KeyPress::leftKey : KeyPress::upKey;
    const auto nextKey = orientation == Orientation::vertical ? KeyPress::rightKey : KeyPress::downKey;
    const auto step = keyboardStep * (key.getModifiers().isShiftDown() ? 5.0f : 1.0f);

    float target = 0.0f;

    if (keyCode == previousKey)
        target = getEffectivePosition() - step;
    else if (keyCode == nextKey)
        target = getEffectivePosition() + step;
    else if (keyCode == KeyPress::homeKey)
        target = getResolvedMinPosition();
    else if (keyCode == KeyPress::endKey)
        target = getResolvedMaxPosition();
    else
        return false;

    setPosition (target);
    return true;
}

void ResizableDivider::focusGained()
{
    repaint();
}

void ResizableDivider::focusLost()
{
    repaint();
}

void ResizableDivider::parentSizeChanged()
{
    commitPendingPosition();
    updateEffectivePosition (sendNotification);
}

void ResizableDivider::parentHierarchyChanged()
{
    commitPendingPosition();
    updateEffectivePosition (sendNotification);
}

//==============================================================================
float ResizableDivider::getPointerAlongAxis (const MouseEvent& event) const
{
    // Measuring in the parent space keeps the pointer stable while the parent moves the divider.
    const auto* parent = getParentComponent();
    const auto point = parent != nullptr ? parent->getLocalPoint (this, event.getPosition()) : event.getScreenPosition();

    return orientation == Orientation::vertical ? point.getX() : point.getY();
}

float ResizableDivider::getUnclampedPosition (float usableExtent) const noexcept
{
    if (pendingPosition.has_value())
        return pendingPosition->resolve (usableExtent);

    switch (resizePolicy)
    {
        case ResizePolicy::preserveLeading:
            return storedLeadingPixels;

        case ResizePolicy::preserveTrailing:
            return usableExtent - storedTrailingPixels;

        case ResizePolicy::proportional:
            break;
    }

    return storedRatio * usableExtent;
}

float ResizableDivider::clampPosition (float position, float usableExtent) const noexcept
{
    return jlimit (resolveMinPosition (usableExtent), resolveMaxPosition (usableExtent), position);
}

float ResizableDivider::resolveMinPosition (float usableExtent) const noexcept
{
    if (! minPosition.has_value())
        return 0.0f;

    return jlimit (0.0f, usableExtent, minPosition->resolve (usableExtent));
}

float ResizableDivider::resolveMaxPosition (float usableExtent) const noexcept
{
    auto result = usableExtent;

    if (maxPosition.has_value())
        result = jmin (result, maxPosition->resolve (usableExtent));

    if (minTrailingSize.has_value())
        result = jmin (result, usableExtent - minTrailingSize->resolve (usableExtent));

    return jlimit (resolveMinPosition (usableExtent), usableExtent, result);
}

void ResizableDivider::storePosition (Length newPosition)
{
    const auto usableExtent = getUsableExtent();
    if (usableExtent <= 0.0f)
    {
        pendingPosition = newPosition;
        return;
    }

    const auto position = clampPosition (newPosition.resolve (usableExtent), usableExtent);

    pendingPosition.reset();
    storedLeadingPixels = position;
    storedTrailingPixels = usableExtent - position;
    storedRatio = position / usableExtent;
}

void ResizableDivider::commitPendingPosition()
{
    if (pendingPosition.has_value() && getUsableExtent() > 0.0f)
        storePosition (*pendingPosition);
}

void ResizableDivider::setCollapsedSideInternal (CollapseSide newSide, NotificationType notification)
{
    if (collapsedSide == newSide)
        return;

    collapsedSide = newSide;
    repaint();

    const BailOutChecker bailOutChecker (this);
    notifyPropertyChanged (ResizableDividerIds::collapsed);

    if (bailOutChecker.shouldBailOut())
        return;

    sendChangeNotification (notification, [this, newSide]
    {
        if (onCollapsedChanged)
            onCollapsedChanged (newSide);
    });
}

void ResizableDivider::updateEffectivePosition (NotificationType notification)
{
    const auto effectivePosition = getEffectivePosition();
    if (approximatelyEqual (lastEffectivePosition, effectivePosition))
        return;

    lastEffectivePosition = effectivePosition;

    const BailOutChecker bailOutChecker (this);
    notifyPropertyChanged (ResizableDividerIds::position);
    if (bailOutChecker.shouldBailOut())
        return;

    notifyPropertyChanged (ResizableDividerIds::normalized);
    if (bailOutChecker.shouldBailOut())
        return;

    sendChangeNotification (notification, [this]
    {
        if (onPositionChanged)
            onPositionChanged (getEffectivePosition());
    });
}

void ResizableDivider::setDragging (bool shouldBeDragging)
{
    if (dragging == shouldBeDragging)
        return;

    dragging = shouldBeDragging;
    repaint();
    notifyPropertyChanged (ResizableDividerIds::dragging);
}

void ResizableDivider::constraintsChanged()
{
    repaint();
    updateEffectivePosition (sendNotification);
}

} // namespace yup
