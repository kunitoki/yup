/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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
namespace
{

/** Returns the axis-aligned bounding box of the corners of a rectangle mapped through @a mapPoint. */
template <class MapPoint>
Rectangle<float> getMappedBounds (const Rectangle<float>& bounds, MapPoint&& mapPoint)
{
    const auto p1 = mapPoint (bounds.getTopLeft());
    const auto p2 = mapPoint (bounds.getTopRight());
    const auto p3 = mapPoint (bounds.getBottomLeft());
    const auto p4 = mapPoint (bounds.getBottomRight());

    const auto minX = jmin (p1.getX(), p2.getX(), p3.getX(), p4.getX());
    const auto maxX = jmax (p1.getX(), p2.getX(), p3.getX(), p4.getX());
    const auto minY = jmin (p1.getY(), p2.getY(), p3.getY(), p4.getY());
    const auto maxY = jmax (p1.getY(), p2.getY(), p3.getY(), p4.getY());

    return { minX, minY, maxX - minX, maxY - minY };
}

/** Returns the axis-aligned bounding box of a rectangle mapped through a transform. */
Rectangle<float> getTransformedBounds (const Rectangle<float>& bounds, const AffineTransform& transform)
{
    return getMappedBounds (bounds, [&transform] (Point<float> p)
    {
        return p.transformed (transform);
    });
}

/** Returns the rectangles of @a region that lie inside @a localBounds mapped through @a toTopLevel,
    rounded to whole pixels.

    The @a region rectangles live in the coordinate space of the top level component, so @a localBounds
    are mapped through the accumulated transform of the component and its ancestors to obtain the area
    it really covers there. For untransformed components that mapping is a plain offset, so the
    intersection reduces to a bounds check.

    The returned list never contains overlapping rectangles, so it can be used directly as a
    clip region.
*/
RectangleList<float> intersectRepaintRegion (const RectangleList<float>& region, const Rectangle<float>& localBounds, const AffineTransform& toTopLevel)
{
    const auto clipBounds = getTransformedBounds (localBounds, toTopLevel);

    RectangleList<float> result;

    for (const auto& rect : region.getRectangles())
    {
        const auto clipped = clipBounds.intersection (rect).roundToInt().to<float>();

        if (! clipped.isEmpty())
            result.add (clipped);
    }

    return result;
}

/** Clips subsequent drawing to the union of the rectangles in @a region. */
void setClipRegion (Graphics& g, const RectangleList<float>& region)
{
    if (region.getNumRectangles() == 1)
    {
        g.setClipPath (region.getRectangles()[0]);
        return;
    }

    Path path;

    for (const auto& rect : region.getRectangles())
        path.addRectangle (rect);

    g.setClipPath (path);
}

/** Returns the device-pixel size of an offscreen canvas covering @a logicalSize at @a scale. */
Size<int> getCanvasPixelSize (const Size<float>& logicalSize, float scale)
{
    return { jmax (1, roundToInt (logicalSize.getWidth() * scale)),
             jmax (1, roundToInt (logicalSize.getHeight() * scale)) };
}

/** Returns true if @a canvas exists and has exactly @a pixelSize. */
bool hasPixelSize (const GpuCanvas::Ptr& canvas, const Size<int>& pixelSize)
{
    return canvas != nullptr
        && canvas->getWidth() == pixelSize.getWidth()
        && canvas->getHeight() == pixelSize.getHeight();
}

} // namespace

//==============================================================================

Component::Component()
    : optionsValue (0)
{
}

Component::Component (StringRef componentID)
    : componentID (componentID)
    , optionsValue (0)
{
}

Component::~Component()
{
    componentListeners.call (&ComponentListener::componentBeingDeleted, *this);

    if (options.onDesktop)
        removeFromDesktop();

    if (parentComponent != nullptr)
        parentComponent->removeChildComponent (this);

    for (auto component : children)
        component->parentComponent = nullptr;

    children.clear();

    masterReference.clear();
}

//==============================================================================

String Component::getComponentID() const
{
    return componentID;
}

//==============================================================================

bool Component::isEnabled() const
{
    return ! options.isDisabled && (parentComponent == nullptr || parentComponent->isEnabled());
}

void Component::setEnabled (bool shouldBeEnabled)
{
    if (options.isDisabled == ! shouldBeEnabled)
        return;

    options.isDisabled = ! shouldBeEnabled;

    //if (options.onDesktop && native != nullptr)
    //    native->setEnabled (shouldBeEnabled);

    enablementChanged();
}

void Component::enablementChanged() {}

//==============================================================================

bool Component::isVisible() const
{
    return options.isVisible;
}

void Component::setVisible (bool shouldBeVisible)
{
    if (options.isVisible == shouldBeVisible)
        return;

    const bool wasShowing = isShowing();

    options.isVisible = shouldBeVisible;

    auto bailOutChecker = BailOutChecker (this);

    if (options.onDesktop && native != nullptr)
        native->setVisible (shouldBeVisible);

    if (bailOutChecker.shouldBailOut())
        return;

    if (wasShowing != isShowing())
        internalVisibilityChanged();

    if (bailOutChecker.shouldBailOut())
        return;

    internalRepaint();
}

bool Component::isShowing() const
{
    if (! isVisible())
        return false;

    auto parent = getParentComponent();
    while (parent != nullptr)
    {
        if (! parent->isVisible())
            return false;

        parent = parent->getParentComponent();
    }

    return true;
}

void Component::visibilityChanged() {}

//==============================================================================

String Component::getTitle() const
{
    return componentTitle;
}

void Component::setTitle (const String& title)
{
    componentTitle = title;

    if (options.onDesktop && native != nullptr)
        native->setTitle (title);
}

//==============================================================================

Point<float> Component::getPosition() const
{
    return boundsInParent.getTopLeft();
}

void Component::setPosition (const Point<float>& newPosition)
{
    boundsInParent.setTopLeft (newPosition);

    if (options.onDesktop && native != nullptr)
        native->setPosition (newPosition.to<int>());

    sendMoved();
}

float Component::getX() const
{
    return boundsInParent.getX();
}

float Component::getY() const
{
    return boundsInParent.getY();
}

float Component::getLeft() const
{
    return boundsInParent.getX();
}

float Component::getTop() const
{
    return boundsInParent.getY();
}

float Component::getRight() const
{
    return boundsInParent.getX() + boundsInParent.getWidth();
}

float Component::getBottom() const
{
    return boundsInParent.getY() + boundsInParent.getHeight();
}

Point<float> Component::getTopLeft() const
{
    return boundsInParent.getTopLeft();
}

void Component::setTopLeft (const Point<float>& newTopLeft)
{
    boundsInParent.setTopLeft (newTopLeft);

    if (options.onDesktop && native != nullptr)
        native->setPosition (newTopLeft.to<int>());

    sendMoved();
}

Point<float> Component::getBottomLeft() const
{
    return boundsInParent.getBottomLeft();
}

void Component::setBottomLeft (const Point<float>& newBottomLeft)
{
    boundsInParent.setBottomLeft (newBottomLeft);

    if (options.onDesktop && native != nullptr)
        native->setPosition (newBottomLeft.translated (0.0f, -getHeight()).to<int>());

    sendMoved();
}

Point<float> Component::getTopRight() const
{
    return boundsInParent.getTopRight();
}

void Component::setTopRight (const Point<float>& newTopRight)
{
    boundsInParent.setTopRight (newTopRight);

    if (options.onDesktop && native != nullptr)
        native->setPosition (newTopRight.translated (-getWidth(), 0.0f).to<int>());

    sendMoved();
}

Point<float> Component::getBottomRight() const
{
    return boundsInParent.getBottomRight();
}

void Component::setBottomRight (const Point<float>& newBottomRight)
{
    boundsInParent.setBottomRight (newBottomRight);

    if (options.onDesktop && native != nullptr)
        native->setPosition (newBottomRight.translated (-getWidth(), -getHeight()).to<int>());

    sendMoved();
}

Point<float> Component::getCenter() const
{
    return boundsInParent.getCenter();
}

void Component::setCenter (const Point<float>& newCenter)
{
    boundsInParent.setCenter (newCenter);

    if (options.onDesktop && native != nullptr)
        native->setPosition (newCenter.translated (-getWidth() / 2.0f, -getHeight() / 2.0f).to<int>());

    sendMoved();
}

float Component::getCenterX() const
{
    return boundsInParent.getCenterX();
}

void Component::setCenterX (float newCenterX)
{
    boundsInParent.setCenterX (newCenterX);

    if (options.onDesktop && native != nullptr)
    {
        auto newCenter = boundsInParent.getCenter();
        native->setPosition (newCenter.translated (-getWidth() / 2.0f, 0.0f).to<int>());
    }

    sendMoved();
}

float Component::getCenterY() const
{
    return boundsInParent.getCenterY();
}

void Component::setCenterY (float newCenterY)
{
    boundsInParent.setCenterY (newCenterY);

    if (options.onDesktop && native != nullptr)
    {
        auto newCenter = boundsInParent.getCenter();
        native->setPosition (newCenter.translated (0.0f, -getHeight() / 2.0f).to<int>());
    }

    sendMoved();
}

void Component::moved() {}

void Component::sendMoved()
{
    moved();

    auto bailOutChecker = BailOutChecker (this);

    componentListeners.call ([this] (ComponentListener& listener)
    {
        listener.componentMoved (*this);
    });

    if (bailOutChecker.shouldBailOut())
        return;

    sendChildBoundsChangedToParent();
}

//==============================================================================

void Component::setSize (float width, float height)
{
    setSize ({ width, height });
}

void Component::setSize (const Size<float>& newSize)
{
    auto areaToRepaint = boundsInParent;
    boundsInParent = boundsInParent.withSize (newSize);
    areaToRepaint = areaToRepaint.unionWith (boundsInParent);

    if (options.onDesktop && native != nullptr)
        native->setSize (newSize.to<int>());

    sendResized();

    repaint (areaToRepaint);
}

Size<float> Component::getSize() const
{
    if (options.onDesktop && native != nullptr)
        return native->getSize().to<float>();

    return boundsInParent.getSize();
}

float Component::getWidth() const
{
    return boundsInParent.getWidth();
}

float Component::getHeight() const
{
    return boundsInParent.getHeight();
}

//==============================================================================

void Component::setBounds (float x, float y, float width, float height)
{
    setBounds ({ x, y, width, height });
}

void Component::setBounds (const Rectangle<float>& newBounds)
{
    repaint();

    boundsInParent = newBounds;

    repaint();

    if (options.onDesktop && native != nullptr)
        native->setBounds (newBounds.to<int>());

    auto bailOutChecker = BailOutChecker (this);

    suppressChildBoundsChanged = true;

    sendResized();

    if (bailOutChecker.shouldBailOut())
        return;

    sendMoved();

    if (bailOutChecker.shouldBailOut())
        return;

    suppressChildBoundsChanged = false;

    sendChildBoundsChangedToParent();
}

Rectangle<float> Component::getBounds() const
{
    return boundsInParent;
}

Rectangle<float> Component::getLocalBounds() const
{
    return boundsInParent.withZeroPosition();
}

Rectangle<float> Component::getBoundsRelativeToTopLevelComponent() const
{
    auto bounds = boundsInParent;
    if (options.onDesktop || options.paintAsOffscreenRoot)
        return bounds.withZeroPosition();

    auto parent = getParentComponent();
    while (parent != nullptr && ! parent->options.onDesktop && ! parent->options.paintAsOffscreenRoot)
    {
        bounds.translate (parent->getPosition());
        parent = parent->getParentComponent();
    }

    return bounds;
}

AffineTransform Component::getTransformToTopLevelComponent() const
{
    AffineTransform toTopLevel;

    for (auto comp = this; comp != nullptr && ! comp->options.onDesktop && ! comp->options.paintAsOffscreenRoot; comp = comp->getParentComponent())
    {
        if (comp->isTransformed())
            toTopLevel = toTopLevel.followedBy (comp->getTransform());

        toTopLevel = toTopLevel.translated (comp->getPosition());
    }

    return toTopLevel;
}

float Component::proportionOfWidth (float proportion) const
{
    return getWidth() * proportion;
}

float Component::proportionOfHeight (float proportion) const
{
    return getHeight() * proportion;
}

void Component::resized() {}

void Component::sendResized()
{
    resized();

    auto bailOutChecker = BailOutChecker (this);

    componentListeners.call ([this] (ComponentListener& listener)
    {
        listener.componentResized (*this);
    });

    if (bailOutChecker.shouldBailOut())
        return;

    for (int index = children.size(); --index >= 0;)
    {
        children.getUnchecked (index)->parentSizeChanged();

        if (bailOutChecker.shouldBailOut())
            return;

        index = jmin (index, children.size());
    }

    sendChildBoundsChangedToParent();
}

void Component::sendChildBoundsChangedToParent()
{
    if (suppressChildBoundsChanged || parentComponent == nullptr)
        return;

    parentComponent->childBoundsChanged (this);
}

void Component::parentSizeChanged() {}

void Component::childBoundsChanged ([[maybe_unused]] Component* child) {}

//==============================================================================

void Component::setTransform (const AffineTransform& newTransform)
{
    if (transform == newTransform)
        return;

    transform = newTransform;

    transformChanged();
}

AffineTransform Component::getTransform() const
{
    return transform;
}

bool Component::isTransformed() const
{
    return ! transform.isIdentity();
}

void Component::transformChanged()
{
}

//==============================================================================

bool Component::isFullScreen() const
{
    return options.isFullScreen;
}

void Component::setFullScreen (bool shouldBeFullScreen)
{
    if (options.isFullScreen == shouldBeFullScreen)
        return;

    options.isFullScreen = shouldBeFullScreen;

    if (options.onDesktop && native != nullptr)
        native->setFullScreen (shouldBeFullScreen);
}

//==============================================================================

void Component::displayChanged() {}

//==============================================================================

float Component::getScaleDpi() const
{
    if (options.onDesktop && native != nullptr)
        return native->getScaleDpi();

    if (parentComponent == nullptr)
        return 1.0f;

    return parentComponent->getScaleDpi();
}

void Component::contentScaleChanged ([[maybe_unused]] float dpiScale) {}

//==============================================================================

void Component::setOpacity (float newOpacity)
{
    auto clampedOpacity = static_cast<uint8> (jlimit (0.0f, 1.0f, newOpacity) * 255);
    if (opacity == clampedOpacity)
        return;

    opacity = clampedOpacity;

    if (options.onDesktop && native != nullptr)
        native->setOpacity (newOpacity);

    opacityChanged();
}

float Component::getOpacity() const
{
    return opacity / 255.0f;
}

void Component::opacityChanged() {}

//==============================================================================

bool Component::isOpaque() const
{
    return ! options.isTransparent;
}

void Component::setOpaque (bool shouldBeOpaque)
{
    options.isTransparent = ! shouldBeOpaque;
}

//==============================================================================

void Component::enableRenderingUnclipped (bool shouldBeEnabled)
{
    options.unclippedRendering = shouldBeEnabled;
}

bool Component::isRenderingUnclipped() const
{
    return options.unclippedRendering;
}

void Component::setPaintProfilingDisabled (bool shouldBeDisabled)
{
    options.paintProfilingDisabled = shouldBeDisabled;
}

bool Component::isPaintProfilingDisabled() const
{
    return options.paintProfilingDisabled;
}

void Component::repaint()
{
    repaint (getLocalBounds());
}

void Component::repaint (float x, float y, float width, float height)
{
    repaint ({ x, y, width, height });
}

void Component::repaint (const Rectangle<float>& rect)
{
    jassert (! isRepainting.load (std::memory_order_relaxed)); // You are likely repainting from paint !

    cachedTextureCanvas = nullptr;

    if (rect.isEmpty() || ! isShowing())
        return;

    internalRepaint (rect);
}

//==============================================================================

void* Component::getNativeHandle() const
{
    if (options.onDesktop && native != nullptr)
        return native->getNativeHandle();

    return nullptr;
}

//==============================================================================

ComponentNative* Component::getNativeComponent()
{
    if (native != nullptr)
        return native.get();

    if (parentComponent == nullptr)
        return nullptr;

    return parentComponent->getNativeComponent();
}

const ComponentNative* Component::getNativeComponent() const
{
    if (native != nullptr)
        return native.get();

    if (parentComponent == nullptr)
        return nullptr;

    return parentComponent->getNativeComponent();
}

void Component::attachedToNative() {}

void Component::detachedFromNative() {}

//==============================================================================

bool Component::isOnDesktop() const
{
    return options.onDesktop;
}

void Component::addToDesktop (const ComponentNative::Options& nativeOptions, void* parent)
{
    YUP_ASSERT_MESSAGE_MANAGER_IS_LOCKED

    if (options.onDesktop)
        removeFromDesktop();

    if (parentComponent != nullptr)
    {
        parentComponent->removeChildComponent (this);
        parentComponent = nullptr;
    }

    options.onDesktop = true;

    native = ComponentNative::createFor (*this, nativeOptions, parent);

    internalAttachedToNative();

    setBounds (getBounds()); // This is needed to update based on scaleDpi
}

void Component::removeFromDesktop()
{
    YUP_ASSERT_MESSAGE_MANAGER_IS_LOCKED

    if (! options.onDesktop)
        return;

    options.onDesktop = false;

    native.reset();

    internalDetachedFromNative();
}

//==============================================================================

void Component::toFront (bool shouldGainKeyboardFocus)
{
    if (options.onDesktop && native != nullptr)
        native->toFront();

    if (parentComponent == nullptr)
        return;

    parentComponent->addChildComponent (this, parentComponent->getNumChildComponents());

    if (shouldGainKeyboardFocus && options.wantsKeyboardFocus)
        takeKeyboardFocus();
}

void Component::toBack()
{
    if (parentComponent == nullptr)
        return;

    parentComponent->addChildComponent (this, 0);
}

void Component::raiseAbove (Component* component)
{
    if (parentComponent == nullptr)
        return;

    auto indexOfComponent = parentComponent->getIndexOfChildComponent (component);
    if (indexOfComponent < 0)
        return;

    indexOfComponent = jmin (indexOfComponent + 1, parentComponent->getNumChildComponents());

    parentComponent->addChildComponent (this, indexOfComponent);
}

void Component::lowerBelow (Component* component)
{
    if (parentComponent == nullptr)
        return;

    auto indexOfComponent = parentComponent->getIndexOfChildComponent (component);
    if (indexOfComponent < 0)
        return;

    indexOfComponent = jmax (indexOfComponent - 1, 0);

    parentComponent->addChildComponent (this, indexOfComponent);
}

void Component::raiseBy (int indexToRaise)
{
    if (parentComponent == nullptr)
        return;

    const int currentIndex = parentComponent->getIndexOfChildComponent (this);
    const int newIndex = jmin (currentIndex + indexToRaise, parentComponent->getNumChildComponents());

    if (currentIndex != newIndex)
        parentComponent->addChildComponent (this, newIndex);
}

void Component::lowerBy (int indexToLower)
{
    const int currentIndex = parentComponent->getIndexOfChildComponent (this);
    const int newIndex = jmax (currentIndex - indexToLower, 0);

    if (currentIndex != newIndex)
        parentComponent->addChildComponent (this, newIndex);
}

//==============================================================================

bool Component::hasParent() const
{
    return parentComponent != nullptr;
}

Component* Component::getParentComponent()
{
    return parentComponent;
}

const Component* Component::getParentComponent() const
{
    return parentComponent;
}

//==============================================================================

void Component::addChildComponent (Component& component, int index)
{
    addChildComponent (&component, index);
}

void Component::addChildComponent (Component* component, int index)
{
    jassert (component != nullptr);

    if (auto* previousParent = component->parentComponent; previousParent != nullptr && previousParent != this)
        previousParent->removeChildComponent (component);

    component->parentComponent = this;

    const int currentIndex = children.indexOf (component);

    if (isPositiveAndBelow (currentIndex, children.size()))
    {
        if (currentIndex != index)
        {
            children.move (currentIndex, index);

            auto bailOutChecker = BailOutChecker (this);

            if (const int newIndex = children.indexOf (component); newIndex != currentIndex)
            {
                component->indexInParentChildrenChanged (currentIndex, newIndex);

                if (bailOutChecker.shouldBailOut())
                    return;
            }

            component->internalHierarchyChanged();

            if (bailOutChecker.shouldBailOut())
                return;

            childrenChanged();
        }
    }
    else
    {
        children.insert (index, component);

        auto bailOutChecker = BailOutChecker (this);

        if (getNativeComponent() != nullptr)
        {
            component->internalAttachedToNative();

            if (bailOutChecker.shouldBailOut())
                return;
        }

        component->internalHierarchyChanged();

        if (bailOutChecker.shouldBailOut())
            return;

        childrenChanged();
    }
}

void Component::addAndMakeVisible (Component& component, int index)
{
    addAndMakeVisible (&component, index);
}

void Component::addAndMakeVisible (Component* component, int index)
{
    addChildComponent (component, index);

    component->setVisible (true);
}

void Component::removeChildComponent (Component& component)
{
    removeChildComponent (&component);
}

void Component::removeChildComponent (Component* component)
{
    jassert (component != nullptr);

    auto indexToRemove = children.indexOf (component);
    removeChildComponent (indexToRemove);
}

void Component::removeChildComponent (int index)
{
    if (! isPositiveAndBelow (index, children.size()))
        return;

    auto component = children.removeAndReturn (index);

    if (component->isShowing())
        repaint (component->getBounds());

    component->parentComponent = nullptr;

    auto bailOutChecker = BailOutChecker (this);

    if (getNativeComponent() != nullptr)
    {
        component->internalDetachedFromNative();

        if (bailOutChecker.shouldBailOut())
            return;
    }

    component->internalHierarchyChanged();

    if (bailOutChecker.shouldBailOut())
        return;

    childrenChanged();
}

void Component::removeAllChildren()
{
    while (! children.isEmpty())
        removeChildComponent (children.size() - 1);
}

void Component::internalHierarchyChanged()
{
    parentHierarchyChanged();

    auto bailOutChecker = BailOutChecker (this);

    for (int index = children.size(); --index >= 0;)
    {
        auto child = children.getUnchecked (index);

        if (bailOutChecker.shouldBailOut())
        {
            jassertfalse; // Deleting a parent component when notifying its children!
            return;
        }

        child->internalHierarchyChanged();

        index = jmin (index, children.size());
    }
}

void Component::parentHierarchyChanged() {}

void Component::childrenChanged() {}

void Component::indexInParentChildrenChanged ([[maybe_unused]] int oldIndex, [[maybe_unused]] int newIndex) {}

//==============================================================================

int Component::getNumChildComponents() const
{
    return children.size();
}

Component* Component::getChildComponent (int index) const
{
    return children.getUnchecked (index);
}

int Component::getIndexOfChildComponent (Component* component) const
{
    return children.indexOf (component);
}

bool Component::hitTest (float x, float y)
{
    return getLocalBounds().contains (x, y);
}

Component* Component::findComponentAt (const Point<float>& p)
{
    if (! options.isVisible || ! hitTest (p.getX(), p.getY()))
        return nullptr;

    for (int index = children.size(); --index >= 0;)
    {
        auto child = children.getUnchecked (index);
        if (! child->isVisible())
            continue;

        const auto childPoint = child->getLocalPointFromParent (p);
        if (! childPoint || ! child->getLocalBounds().contains (*childPoint))
            continue;

        if (auto found = child->findComponentAt (*childPoint))
            return found;
    }

    return this;
}

Component* Component::findComponentAtForMouseEvent (const Point<float>& p)
{
    if (! options.isVisible || ! boundsInParent.withZeroPosition().contains (p) || ! hitTest (p.getX(), p.getY()))
        return nullptr;

    if (doesWantChildrenMouseEvents())
    {
        for (int index = children.size(); --index >= 0;)
        {
            auto child = children.getUnchecked (index);
            if (! child->isVisible() || ! child->boundsInParent.contains (p))
                continue;

            if (auto* hit = child->findComponentAtForMouseEvent (p - child->boundsInParent.getPosition()))
                return hit;
        }
    }

    return doesWantSelfMouseEvents() ? this : nullptr;
}

Component* Component::getTopLevelComponent()
{
    auto currentComponent = this;

    auto parent = getParentComponent();
    while (parent != nullptr)
    {
        currentComponent = parent;
        parent = currentComponent->getParentComponent();
    }

    return currentComponent;
}

Component* Component::getPopupParentComponent()
{
    auto* popupParent = this;

    for (auto* ancestor = parentComponent; ancestor != nullptr; ancestor = ancestor->parentComponent)
    {
        popupParent = ancestor;

        if (ancestor->isTransformed() || ancestor->options.manuallyComposited)
            break;
    }

    return popupParent;
}

//==============================================================================

void Component::setMouseCursor (const MouseCursor& cursorType)
{
    mouseCursor = cursorType;

    if (auto nativeComponent = getNativeComponent())
    {
        if (nativeComponent->getFocusedComponent() == this)
            updateMouseCursor();
    }
}

MouseCursor Component::getMouseCursor() const
{
    return mouseCursor;
}

//==============================================================================

void Component::setWantsKeyboardFocus (bool wantsFocus)
{
    options.wantsKeyboardFocus = wantsFocus;
}

bool Component::getWantsKeyboardFocus() const
{
    return options.wantsKeyboardFocus;
}

void Component::setClickingGrabFocus (bool shouldGrabFocus)
{
    options.clickingDoesNotGrabFocus = ! shouldGrabFocus;
}

bool Component::getClickingGrabFocus() const
{
    return ! options.clickingDoesNotGrabFocus;
}

void Component::takeKeyboardFocus()
{
    takeKeyboardFocus (FocusChangeType::focusChangedDirectly);
}

void Component::takeKeyboardFocus (FocusChangeType cause)
{
    if (! options.wantsKeyboardFocus || ! isEnabled())
        return;

    if (auto nativeComponent = getNativeComponent())
        nativeComponent->setFocusedComponent (this, cause);
}

void Component::leaveKeyboardFocus()
{
    if (auto nativeComponent = getNativeComponent())
    {
        if (nativeComponent->getFocusedComponent() == this)
            nativeComponent->setFocusedComponent (nullptr);
    }
}

bool Component::hasKeyboardFocus() const
{
    if (! options.wantsKeyboardFocus || ! isEnabled())
        return false;

    if (auto nativeComponent = getNativeComponent())
        return nativeComponent->getFocusedComponent() == this;

    return false;
}

void Component::focusGained() {}

void Component::focusLost() {}

void Component::focusOfChildComponentChanged ([[maybe_unused]] Component* child, [[maybe_unused]] FocusChangeType cause) {}

//==============================================================================

void Component::handleKeyboardFocusFromClick()
{
    for (auto* component = this; component != nullptr; component = component->parentComponent)
    {
        if (component->options.wantsKeyboardFocus && ! component->options.clickingDoesNotGrabFocus)
        {
            component->takeKeyboardFocus (FocusChangeType::focusChangedByMouseClick);
            return;
        }
    }
}

//==============================================================================

NamedValueSet& Component::getProperties()
{
    return properties;
}

const NamedValueSet& Component::getProperties() const
{
    return properties;
}

//==============================================================================

void Component::paint ([[maybe_unused]] Graphics& g)
{
    jassert (! isOpaque()); // If your component is opaque, you need to paint it !
}

void Component::paintOverChildren ([[maybe_unused]] Graphics& g) {}

void Component::refreshDisplay ([[maybe_unused]] double lastFrameTimeSeconds) {}

//==============================================================================

void Component::setWantsMouseEvents (bool allowSelfMouseEvents, bool allowChildrenMouseEvents)
{
    options.blockSelfMouseEvents = ! allowSelfMouseEvents;
    options.blockChildrenMouseEvents = ! allowChildrenMouseEvents;
}

bool Component::doesWantSelfMouseEvents() const
{
    return ! options.blockSelfMouseEvents;
}

bool Component::doesWantChildrenMouseEvents() const
{
    return ! options.blockChildrenMouseEvents;
}

//==============================================================================

void Component::mouseEnter ([[maybe_unused]] const MouseEvent& event) {}

void Component::mouseExit ([[maybe_unused]] const MouseEvent& event) {}

void Component::mouseDown ([[maybe_unused]] const MouseEvent& event) {}

void Component::mouseMove ([[maybe_unused]] const MouseEvent& event) {}

void Component::mouseDrag ([[maybe_unused]] const MouseEvent& event) {}

void Component::mouseUp ([[maybe_unused]] const MouseEvent& event) {}

void Component::mouseDoubleClick ([[maybe_unused]] const MouseEvent& event) {}

void Component::mouseWheel ([[maybe_unused]] const MouseEvent& event, [[maybe_unused]] const MouseWheelData& wheelData) {}

void Component::keyDown ([[maybe_unused]] const KeyPress& keys, [[maybe_unused]] const Point<float>& position) {}

void Component::keyUp ([[maybe_unused]] const KeyPress& keys, [[maybe_unused]] const Point<float>& position) {}

void Component::textInput ([[maybe_unused]] const String& text) {}

void Component::keyStateChanged ([[maybe_unused]] const KeyPress& key, [[maybe_unused]] bool isDown) {}

void Component::modifierKeysChanged ([[maybe_unused]] const KeyModifiers& modifiers) {}

//==============================================================================

void Component::addMouseListener (MouseListener* listener)
{
    mouseListeners.add (listener);
}

void Component::removeMouseListener (MouseListener* listener)
{
    mouseListeners.remove (listener);
}

//==============================================================================

void Component::addComponentListener (ComponentListener* listener)
{
    componentListeners.add (listener);
}

void Component::removeComponentListener (ComponentListener* listener)
{
    componentListeners.remove (listener);
}

//==============================================================================

void Component::setStyle (ComponentStyle::Ptr newStyle)
{
    if (style == newStyle)
        return;

    style = std::move (newStyle);

    auto bailOutChecker = BailOutChecker (this);

    styleChanged();

    if (bailOutChecker.shouldBailOut())
        return;

    repaint();
}

ComponentStyle::Ptr Component::getStyle() const
{
    return style;
}

void Component::styleChanged() {}

//==============================================================================

void Component::setColor (const Identifier& colorId, const std::optional<Color>& color)
{
    if (color)
        properties.set (colorId, static_cast<int64> (color->getARGB()));
    else
        properties.remove (colorId);

    styleChanged();
}

std::optional<Color> Component::getColor (const Identifier& colorId) const
{
    if (auto color = properties.getVarPointer (colorId); color != nullptr && color->isInt64())
        return Color (static_cast<uint32> (static_cast<int64> (*color)));

    return std::nullopt;
}

std::optional<Color> Component::findColor (const Identifier& colorId) const
{
    if (auto color = getColor (colorId))
        return color;

    if (parentComponent != nullptr)
        return parentComponent->findColor (colorId);

    return std::nullopt;
}

//==============================================================================

void Component::setMetric (const Identifier& metricId, const std::optional<float>& metric)
{
    if (metric)
        properties.set (metricId, static_cast<double> (*metric));
    else
        properties.remove (metricId);

    styleChanged();
}

std::optional<float> Component::getMetric (const Identifier& metricId) const
{
    if (auto value = properties.getVarPointer (metricId); value != nullptr && value->isDouble())
        return static_cast<float> (static_cast<double> (*value));

    return std::nullopt;
}

std::optional<float> Component::findMetric (const Identifier& metricId) const
{
    if (auto metric = getMetric (metricId))
        return metric;

    if (parentComponent != nullptr)
        return parentComponent->findMetric (metricId);

    return std::nullopt;
}

//==============================================================================

void Component::setComponentEffect (ComponentEffect::Ptr effect)
{
    componentEffect = std::move (effect);

    if (componentEffect == nullptr)
        effectOffscreenCanvas = nullptr;

    repaint();
}

ComponentEffect::Ptr Component::getComponentEffect() const
{
    return componentEffect;
}

void Component::setCachedToTexture (bool shouldCache)
{
    if (options.cachedToTexture == shouldCache)
        return;

    options.cachedToTexture = shouldCache;
    cachedTextureCanvas = nullptr;
    repaint();
}

bool Component::isCachedToTexture() const
{
    return options.cachedToTexture;
}

//==============================================================================

void Component::setManuallyComposited (bool shouldBeManuallyComposited)
{
    if (options.manuallyComposited == shouldBeManuallyComposited)
        return;

    options.manuallyComposited = shouldBeManuallyComposited;

    if (! shouldBeManuallyComposited)
        presentedCanvas = nullptr;

    repaint();
}

bool Component::isManuallyComposited() const
{
    return options.manuallyComposited;
}

GpuTexture::Ptr Component::renderToTexture (GraphicsContext& ctx, float scale)
{
    const bool sizeChanged = ! hasPixelSize (presentedCanvas, getCanvasPixelSize (getSize(), scale));

    if (! subtreeDirty.exchange (false) && ! sizeChanged)
        return presentedCanvas->asTexture();

    presentedCanvas = renderSnapshotOffscreen (ctx, true, scale, std::move (presentedCanvas));
    if (presentedCanvas == nullptr)
    {
        subtreeDirty = true;
        return nullptr;
    }

    return presentedCanvas->asTexture();
}

//==============================================================================

GpuCanvas::Ptr Component::renderSnapshotOffscreen (GraphicsContext& ctx, bool includeEffects, float scale, GpuCanvas::Ptr reuseCanvas)
{
    if (getWidth() <= 0.0f || getHeight() <= 0.0f)
        return nullptr;

    const auto renderSnapshot = [&] () -> GpuCanvas::Ptr
    {
        const bool applyEffect = includeEffects && componentEffect != nullptr;

        auto canvas = renderSubtreeOffscreen (ctx, getOpacity(), false, scale, applyEffect ? nullptr : std::move (reuseCanvas));
        if (canvas == nullptr)
            return nullptr;

        if (! applyEffect)
            return canvas;

        auto texture = canvas->asTexture();

        auto effectCanvas = std::move (reuseCanvas);
        if (effectCanvas == nullptr || effectCanvas->getWidth() != canvas->getWidth() || effectCanvas->getHeight() != canvas->getHeight())
            effectCanvas = GpuCanvas::create (ctx, canvas->getWidth(), canvas->getHeight());

        if (effectCanvas == nullptr)
            return canvas;

        auto& g = effectCanvas->beginDraw ({}, scale);
        auto localBounds = getLocalBounds();
        g.setDrawingArea (localBounds);
        componentEffect->apply (g, texture, localBounds);

        return effectCanvas;
    };

    if (auto* nativeComponent = getNativeComponent())
    {
        GpuCanvas::Ptr canvas;
        nativeComponent->runWithGraphicsContext ([&] { canvas = renderSnapshot(); });
        return canvas;
    }

    return renderSnapshot();
}

Image Component::snapshotToImage (GraphicsContext& ctx, bool includeEffects)
{
    Image result;
    const auto takeSnapshot = [&]
    {
        auto canvas = renderSnapshotOffscreen (ctx, includeEffects, 1.0f);
        if (canvas == nullptr)
            return;

        result = canvas->asImage();
    };

    if (auto* nativeComponent = getNativeComponent())
        nativeComponent->runWithGraphicsContext (takeSnapshot);
    else
        takeSnapshot();

    return result;
}

GpuTexture::Ptr Component::snapshotToTexture (GraphicsContext& ctx, bool includeEffects)
{
    GpuTexture::Ptr result;
    const auto takeSnapshot = [&]
    {
        auto canvas = renderSnapshotOffscreen (ctx, includeEffects, 1.0f);
        if (canvas == nullptr)
            return;

        result = canvas->asTexture();
    };

    if (auto* nativeComponent = getNativeComponent())
        nativeComponent->runWithGraphicsContext (takeSnapshot);
    else
        takeSnapshot();

    return result;
}

//==============================================================================

void Component::userTriedToCloseWindow() {}

//==============================================================================

bool Component::hasOpaqueChildCoveringArea (const Rectangle<float>& area)
{
    for (int childIndex = children.size(); --childIndex >= 0;)
    {
        auto child = children.getUnchecked (childIndex);
        if (! child->isVisible() || ! child->isOpaque() || child->options.unclippedRendering || child->options.manuallyComposited || child->isTransformed())
            continue;

        auto childBounds = child->getBoundsRelativeToTopLevelComponent();
        if (childBounds.contains (area))
            return true;
    }

    return false;
}

void Component::internalRefreshDisplay (double lastFrameTimeSeconds)
{
    refreshDisplay (lastFrameTimeSeconds);

    for (auto child : children)
        child->internalRefreshDisplay (lastFrameTimeSeconds);
}

//==============================================================================

void Component::internalRepaint()
{
    internalRepaint (getLocalBounds());
}

void Component::internalRepaint (const Rectangle<float>& rect)
{
    if (rect.isEmpty())
        return;

    auto dirtyArea = rect;

    for (auto* component = this;; component = component->parentComponent)
    {
        component->subtreeDirty = true;

        // A warp can move pixels anywhere inside the component
        if (component->componentEffect != nullptr)
            dirtyArea = component->getLocalBounds();

        auto* parent = component->parentComponent;

        if (parent != nullptr && component->options.manuallyComposited)
            dirtyArea = parent->getLocalBounds();
        else if (! component->options.onDesktop)
            dirtyArea = getTransformedBounds (dirtyArea, component->transform.translated (component->getPosition()));

        if (parent == nullptr)
        {
            if (component->native != nullptr)
                component->native->repaint (dirtyArea);

            return;
        }
    }
}

//==============================================================================

void Component::paintChildrenAndOverChildren (Graphics& g, const RectangleList<float>& clipRegion, bool renderContinuous)
{
    for (auto child : children)
        child->internalPaint (g, clipRegion, renderContinuous);

    paintOverChildren (g);
}

GpuCanvas::Ptr Component::renderSubtreeOffscreen (GraphicsContext& ctx, float opacity, bool renderContinuous, float scale, GpuCanvas::Ptr reuseCanvas)
{
    if (getWidth() <= 0.0f || getHeight() <= 0.0f)
        return nullptr;

    const auto renderOffscreen = [&] () -> GpuCanvas::Ptr
    {
        const auto pixelSize = getCanvasPixelSize (getSize(), scale);

        GpuCanvas::Ptr canvas;
        if (hasPixelSize (reuseCanvas, pixelSize))
        {
            canvas = std::move (reuseCanvas);
        }
        else
        {
            reuseCanvas = nullptr;
            canvas = GpuCanvas::create (ctx, pixelSize.getWidth(), pixelSize.getHeight());
        }

        if (canvas == nullptr)
            return nullptr;

        auto& offscreenG = canvas->beginDraw ({}, scale);

        options.paintAsOffscreenRoot = true;

        paintSubtree (offscreenG, RectangleList<float> { getLocalBounds() }, opacity, renderContinuous);

        options.paintAsOffscreenRoot = false;

        canvas->commit();
        return canvas;
    };

    if (auto* nativeComponent = getNativeComponent())
    {
        GpuCanvas::Ptr canvas;
        nativeComponent->runWithGraphicsContext ([&] { canvas = renderOffscreen(); });
        return canvas;
    }

    return renderOffscreen();
}

//==============================================================================

void Component::applyPaintState (Graphics& g, const RectangleList<float>& clipRegion) const
{
    const auto toTopLevel = getTransformToTopLevelComponent();

    g.setTransform (AffineTransform::identity());

    if (! options.unclippedRendering)
        setClipRegion (g, clipRegion);

    // The translation goes in the drawing area and only the linear part in the transform, so an
    // untransformed hierarchy keeps painting in exactly the same state it always had
    g.setDrawingArea (getLocalBounds().withPosition (toTopLevel.getTranslation()));
    g.setTransform (toTopLevel.withAbsoluteTranslation (0.0f, 0.0f));
}

void Component::paintSubtree (Graphics& g, const RectangleList<float>& clipRegion, float opacity, bool renderContinuous)
{
    isRepainting.store (true, std::memory_order_relaxed);

    const ErasedScopeGuard clearRepaintingFlag ([this]
    {
        isRepainting.store (false, std::memory_order_relaxed);
    });

    {
        const bool shouldMeasurePaint = ! options.paintProfilingDisabled && ! componentListeners.isEmpty();
        const auto toTopLevel = getTransformToTopLevelComponent();

        ComponentPaintMetrics metrics;
        int64 totalStartTicks = 0;
        int64 selfStartTicks = 0;

        if (shouldMeasurePaint)
        {
            const auto topLevelBounds = getTransformedBounds (getLocalBounds(), toTopLevel);

            totalStartTicks = Time::getHighResolutionTicks();
            metrics.repaintArea = topLevelBounds;
            metrics.componentBounds = topLevelBounds;
            metrics.renderContinuous = renderContinuous;
        }

        const auto globalState = g.saveState();

        g.setOpacity (opacity);
        applyPaintState (g, clipRegion);

        bool canSkipPaint = false;
        if (! options.unclippedRendering && toTopLevel.isOnlyTranslation() && clipRegion.getNumRectangles() == 1)
            canSkipPaint = hasOpaqueChildCoveringArea (clipRegion.getRectangles()[0]);

        if (! canSkipPaint)
        {
            const auto paintState = g.saveState();

            if (shouldMeasurePaint)
            {
                selfStartTicks = Time::getHighResolutionTicks();
                paint (g);
                metrics.selfTicks += Time::getHighResolutionTicks() - selfStartTicks;
            }
            else
            {
                paint (g);
            }
        }
        else
        {
            if (shouldMeasurePaint)
                metrics.selfPaintSkipped = true;
        }

        if (shouldMeasurePaint)
        {
            const int64 childrenStartTicks = Time::getHighResolutionTicks();

            for (auto child : children)
                child->internalPaint (g, clipRegion, renderContinuous);

            metrics.childrenTicks += Time::getHighResolutionTicks() - childrenStartTicks;

            selfStartTicks = Time::getHighResolutionTicks();

            paintOverChildren (g);

            const int64 selfEndTicks = Time::getHighResolutionTicks();

            metrics.selfTicks += selfEndTicks - selfStartTicks;
            metrics.totalTicks = selfEndTicks - totalStartTicks;

            componentListeners.call (&ComponentListener::componentPaintCompleted, *this, metrics);
        }
        else
        {
            paintChildrenAndOverChildren (g, clipRegion, renderContinuous);
        }
    }
}

//==============================================================================

void Component::internalPaint (Graphics& g, const RectangleList<float>& repaintRegions, bool renderContinuous)
{
    if (! isVisible() || options.manuallyComposited || getWidth() <= 0.0f || getHeight() <= 0.0f)
        return;

    const auto toTopLevel = getTransformToTopLevelComponent();

    auto boundsToRedraw = intersectRepaintRegion (repaintRegions, getLocalBounds(), toTopLevel);

    if (boundsToRedraw.isEmpty())
    {
        if (! renderContinuous)
            return;

        boundsToRedraw.addWithoutMerge (getTransformedBounds (getLocalBounds(), toTopLevel));
    }

    const auto selfOpacity = (! options.onDesktop && native == nullptr) ? getOpacity() : 1.0f;
    const auto opacity = g.getOpacity() * selfOpacity;
    if (opacity <= 0.0f)
        return;

    if (componentEffect != nullptr)
    {
        auto canvas = renderSubtreeOffscreen (g.getGraphicsContext(), 1.0f, renderContinuous, g.getContextScale(), std::move (effectOffscreenCanvas));
        if (canvas == nullptr)
            return;

        auto texture = canvas->asTexture();

        {
            const auto saved = g.saveState();
            g.setOpacity (opacity);
            applyPaintState (g, boundsToRedraw);

            componentEffect->apply (g, texture, getLocalBounds());
        }

        effectOffscreenCanvas = canvas;

        if (options.cachedToTexture)
            cachedTextureCanvas = canvas;

        return;
    }

    if (options.cachedToTexture)
    {
        const auto scale = g.getContextScale();
        const auto pixelSize = getCanvasPixelSize (getSize(), scale);

        if (! hasPixelSize (cachedTextureCanvas, pixelSize))
        {
            cachedTextureCanvas = nullptr;

            auto canvas = GpuCanvas::create (g.getGraphicsContext(), pixelSize.getWidth(), pixelSize.getHeight());
            if (canvas != nullptr)
            {
                auto& offscreenG = canvas->beginDraw ({}, scale);
                offscreenG.setDrawingArea (getLocalBounds());

                options.paintAsOffscreenRoot = true;
                paint (offscreenG);
                options.paintAsOffscreenRoot = false;

                canvas->commit();
                cachedTextureCanvas = canvas;
            }
        }

        isRepainting.store (true, std::memory_order_relaxed);

        {
            const auto saved = g.saveState();
            g.setOpacity (opacity);
            applyPaintState (g, boundsToRedraw);

            if (cachedTextureCanvas != nullptr)
                g.drawTexture (cachedTextureCanvas->asTexture(), getLocalBounds());
            else
                paint (g);
        }

        isRepainting.store (false, std::memory_order_relaxed);

        paintChildrenAndOverChildren (g, boundsToRedraw, renderContinuous);
        return;
    }

    paintSubtree (g, boundsToRedraw, opacity, renderContinuous);

#if YUP_ENABLE_COMPONENT_PAINT_DEBUGGING
    paintDebugOverlay (g, boundsToRedraw);
#endif
}

#if YUP_ENABLE_COMPONENT_PAINT_DEBUGGING
void Component::paintDebugOverlay (Graphics& g, const RectangleList<float>& boundsToRedraw)
{
    const auto saved = g.saveState();

    applyPaintState (g, boundsToRedraw);
    g.setFillColor (debugColor.withMultipliedAlpha (0.2f));
    g.fillRect (getLocalBounds());

    if (--counter == 0)
    {
        counter = 2;
        debugColor = Color::opaqueRandom();
    }
}
#endif

//==============================================================================

void Component::internalMouseEnter (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    auto bailOutChecker = BailOutChecker (this);

    mouseEnter (event);

    if (bailOutChecker.shouldBailOut())
        return;

    mouseListeners.callChecked (bailOutChecker, &MouseListener::mouseEnter, event);
}

//==============================================================================

void Component::internalMouseExit (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    auto bailOutChecker = BailOutChecker (this);

    mouseExit (event);

    if (bailOutChecker.shouldBailOut())
        return;

    mouseListeners.callChecked (bailOutChecker, &MouseListener::mouseExit, event);
}

//==============================================================================

void Component::internalMouseDown (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    auto bailOutChecker = BailOutChecker (this);

    handleKeyboardFocusFromClick();

    if (bailOutChecker.shouldBailOut())
        return;

    mouseDown (event);

    if (bailOutChecker.shouldBailOut())
        return;

    mouseListeners.callChecked (bailOutChecker, &MouseListener::mouseDown, event);
}

//==============================================================================

void Component::internalMouseMove (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    auto bailOutChecker = BailOutChecker (this);

    mouseMove (event);

    if (bailOutChecker.shouldBailOut())
        return;

    mouseListeners.callChecked (bailOutChecker, &MouseListener::mouseMove, event);
}

//==============================================================================

void Component::internalMouseDrag (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    auto bailOutChecker = BailOutChecker (this);

    mouseDrag (event);

    if (bailOutChecker.shouldBailOut())
        return;

    mouseListeners.callChecked (bailOutChecker, &MouseListener::mouseDrag, event);
}

//==============================================================================

void Component::internalMouseUp (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    auto bailOutChecker = BailOutChecker (this);

    mouseUp (event);

    if (bailOutChecker.shouldBailOut())
        return;

    mouseListeners.callChecked (bailOutChecker, &MouseListener::mouseUp, event);
}

//==============================================================================

void Component::internalMouseDoubleClick (const MouseEvent& event)
{
    if (! isVisible())
        return;

    auto bailOutChecker = BailOutChecker (this);

    mouseDoubleClick (event);

    if (bailOutChecker.shouldBailOut())
        return;

    mouseListeners.callChecked (bailOutChecker, &MouseListener::mouseDoubleClick, event);
}

//==============================================================================

void Component::internalMouseWheel (const MouseEvent& event, const MouseWheelData& wheelData)
{
    if (! isVisible())
        return;

    auto bailOutChecker = BailOutChecker (this);

    mouseWheel (event, wheelData);

    if (bailOutChecker.shouldBailOut())
        return;

    mouseListeners.callChecked (bailOutChecker, &MouseListener::mouseWheel, event, wheelData);
}

//==============================================================================

void Component::internalKeyDown (const KeyPress& keys, const Point<float>& position)
{
    if (! isVisible() || ! isEnabled())
        return;

    keyDown (keys, position);
}

//==============================================================================

void Component::internalKeyUp (const KeyPress& keys, const Point<float>& position)
{
    if (! isVisible() || ! isEnabled())
        return;

    keyUp (keys, position);
}

//==============================================================================

void Component::internalTextInput (const String& text)
{
    if (! options.wantsKeyboardFocus || ! isVisible() || ! isEnabled())
        return;

    textInput (text);
}

//==============================================================================

void Component::internalKeyStateChanged (const KeyPress& keys, bool isDown)
{
    if (! isVisible() || ! isEnabled())
        return;

    keyStateChanged (keys, isDown);
}

//==============================================================================

void Component::internalModifierKeysChanged (const KeyModifiers& modifiers)
{
    if (! isVisible() || ! isEnabled())
        return;

    modifierKeysChanged (modifiers);
}

//==============================================================================

void Component::internalResized (int width, int height)
{
    const auto newBounds = boundsInParent.withSize (Size<int> (width, height).to<float>());

    if (newBounds != boundsInParent)
    {
        boundsInParent = newBounds;

        sendResized();
    }
}

//==============================================================================

void Component::internalMoved (int xpos, int ypos)
{
    const auto newBounds = boundsInParent.withPosition (Point<int> (xpos, ypos).to<float>());

    if (newBounds != boundsInParent)
    {
        boundsInParent = newBounds;

        sendMoved();
    }
}

//==============================================================================

void Component::internalFocusChanged (bool gotFocus)
{
    if (gotFocus)
        focusGained();
    else
        focusLost();
}

//==============================================================================

void Component::internalFocusOfComponentChanged (FocusChangeType cause)
{
    auto bailOutChecker = BailOutChecker (this);

    for (auto* ancestor = parentComponent; ancestor != nullptr;)
    {
        auto ancestorBailOutChecker = BailOutChecker (ancestor);

        ancestor->focusOfChildComponentChanged (this, cause);

        if (bailOutChecker.shouldBailOut() || ancestorBailOutChecker.shouldBailOut())
            return;

        ancestor = ancestor->parentComponent;
    }
}

//==============================================================================

void Component::internalDisplayChanged() {}

//==============================================================================

void Component::internalContentScaleChanged (float dpiScale)
{
    if (contentScale != dpiScale)
    {
        contentScale = dpiScale;

        contentScaleChanged (dpiScale);
    }
}

//==============================================================================

void Component::internalSafeAreaChanged()
{
    auto bailOutChecker = BailOutChecker (this);

    safeAreaChanged();

    if (bailOutChecker.shouldBailOut())
        return;

    for (auto child : children)
    {
        child->internalSafeAreaChanged();

        if (bailOutChecker.shouldBailOut())
            return;
    }
}

//==============================================================================

void Component::internalUserTriedToCloseWindow()
{
    userTriedToCloseWindow();
}

//==============================================================================

void Component::internalAttachedToNative()
{
    auto bailOutChecker = BailOutChecker (this);

    attachedToNative();

    if (bailOutChecker.shouldBailOut())
        return;

    for (auto child : children)
    {
        child->internalAttachedToNative();

        if (bailOutChecker.shouldBailOut())
            return;
    }
}

void Component::internalDetachedFromNative()
{
    auto bailOutChecker = BailOutChecker (this);

    detachedFromNative();

    if (bailOutChecker.shouldBailOut())
        return;

    for (auto child : children)
    {
        child->internalDetachedFromNative();

        if (bailOutChecker.shouldBailOut())
            return;
    }
}

//==============================================================================

void Component::internalVisibilityChanged()
{
    visibilityChanged();

    auto bailOutChecker = BailOutChecker (this);

    for (int index = children.size(); --index >= 0;)
    {
        auto child = children.getUnchecked (index);

        if (bailOutChecker.shouldBailOut())
            return;

        if (child->isVisible())
            child->internalVisibilityChanged();

        index = jmin (index, children.size());
    }
}

//==============================================================================

void Component::updateMouseCursor()
{
    Desktop::getInstance()->setMouseCursor (mouseCursor);
}

//==============================================================================

Point<float> Component::getScreenPosition() const
{
    return localToScreen (Point<float>());
}

//==============================================================================

Rectangle<float> Component::getScreenBounds() const
{
    return localToScreen (getLocalBounds());
}

//==============================================================================

Rectangle<float> Component::getSafeAreaBounds() const
{
    if (options.onDesktop && native != nullptr)
        return native->getSafeAreaBounds().to<float>();

    if (parentComponent == nullptr)
        return getLocalBounds();

    return parentComponent->getSafeAreaBounds()
        .translated (-getPosition())
        .intersection (getLocalBounds());
}

void Component::safeAreaChanged() {}

//==============================================================================

std::optional<Point<float>> Component::getChildPointFromLocal (const Component& child, Point<float> localPoint) const
{
    jassert (child.parentComponent == this);

    const auto childPoint = localPoint - child.getPosition();
    if (! child.isTransformed())
        return childPoint;

    if (approximatelyEqual (child.transform.getDeterminant(), 0.0f))
        return std::nullopt;

    return childPoint.transformed (child.transform.inverted());
}

std::optional<Point<float>> Component::getLocalPointFromChild (const Component& child, Point<float> childPoint) const
{
    jassert (child.parentComponent == this);

    return childPoint.transformed (child.transform) + child.getPosition();
}

std::optional<Point<float>> Component::getLocalPointFromParent (Point<float> parentPoint) const
{
    if (parentComponent == nullptr)
        return std::nullopt;

    auto localPoint = parentComponent->getChildPointFromLocal (*this, parentPoint);
    if (localPoint && componentEffect != nullptr)
        localPoint = componentEffect->displayToContent (*localPoint, getLocalBounds());

    return localPoint;
}

std::optional<Point<float>> Component::getParentPointFromLocal (Point<float> localPoint) const
{
    if (parentComponent == nullptr)
        return std::nullopt;

    std::optional<Point<float>> displayPoint = localPoint;
    if (componentEffect != nullptr)
        displayPoint = componentEffect->contentToDisplay (localPoint, getLocalBounds());

    if (! displayPoint)
        return std::nullopt;

    return parentComponent->getLocalPointFromChild (*this, *displayPoint);
}

Point<float> Component::getLocalPointFromTopLevel (Point<float> topLevelPoint) const
{
    if (parentComponent == nullptr)
        return topLevelPoint;

    const auto parentPoint = parentComponent->getLocalPointFromTopLevel (topLevelPoint);

    return getLocalPointFromParent (parentPoint).value_or (parentPoint - getPosition());
}

Point<float> Component::getTopLevelScreenOrigin() const
{
    auto topLevel = this;
    while (topLevel->parentComponent != nullptr)
        topLevel = topLevel->parentComponent;

    if (topLevel->options.onDesktop && topLevel->native != nullptr)
        return topLevel->native->getPosition().to<float>();

    return topLevel->getPosition();
}

Point<float> Component::localToScreen (const Point<float>& localPoint) const
{
    auto point = localPoint;

    for (auto component = this; component->parentComponent != nullptr; component = component->parentComponent)
        point = component->getParentPointFromLocal (point).value_or (point + component->getPosition());

    return point + getTopLevelScreenOrigin();
}

Point<float> Component::screenToLocal (const Point<float>& screenPoint) const
{
    return getLocalPointFromTopLevel (screenPoint - getTopLevelScreenOrigin());
}

Rectangle<float> Component::localToScreen (const Rectangle<float>& localRectangle) const
{
    return getMappedBounds (localRectangle, [this] (Point<float> p)
    {
        return localToScreen (p);
    });
}

Rectangle<float> Component::screenToLocal (const Rectangle<float>& screenRectangle) const
{
    return getMappedBounds (screenRectangle, [this] (Point<float> p)
    {
        return screenToLocal (p);
    });
}

//==============================================================================

Point<float> Component::getLocalPoint (const Component* sourceComponent, Point<float> pointInSource) const
{
    if (sourceComponent == nullptr || sourceComponent == this)
        return pointInSource;

    return screenToLocal (sourceComponent->localToScreen (pointInSource));
}

Rectangle<float> Component::getLocalArea (const Component* sourceComponent, Rectangle<float> rectangleInSource) const
{
    if (sourceComponent == nullptr || sourceComponent == this)
        return rectangleInSource;

    return screenToLocal (sourceComponent->localToScreen (rectangleInSource));
}

//==============================================================================

Point<float> Component::getRelativePoint (const Component* targetComponent, Point<float> localPoint) const
{
    if (targetComponent == nullptr || targetComponent == this)
        return localPoint;

    return targetComponent->screenToLocal (localToScreen (localPoint));
}

Rectangle<float> Component::getRelativeArea (const Component* targetComponent, Rectangle<float> localRectangle) const
{
    if (targetComponent == nullptr || targetComponent == this)
        return localRectangle;

    return targetComponent->screenToLocal (localToScreen (localRectangle));
}

//==============================================================================

AffineTransform Component::getTransformToComponent (const Component* targetComponent) const
{
    if (targetComponent == nullptr || targetComponent == this)
        return AffineTransform();

    AffineTransform transform;

    auto thisToScreen = getTransformToScreen();
    auto targetToScreen = targetComponent->getTransformToScreen();

    transform = thisToScreen.followedBy (targetToScreen.inverted());

    return transform;
}

AffineTransform Component::getTransformFromComponent (const Component* sourceComponent) const
{
    if (sourceComponent == nullptr)
        return AffineTransform();

    return sourceComponent->getTransformToComponent (this);
}

AffineTransform Component::getTransformToScreen() const
{
    AffineTransform transform;

    for (auto comp = this; comp->parentComponent != nullptr; comp = comp->parentComponent)
    {
        if (comp->isTransformed())
            transform = transform.followedBy (comp->getTransform());

        transform = transform.translated (comp->getPosition());
    }

    return transform.translated (getTopLevelScreenOrigin());
}

} // namespace yup
