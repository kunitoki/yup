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

    if (options.isDisabled && hasKeyboardFocus())
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

    options.isVisible = shouldBeVisible;

    auto bailOutChecker = BailOutChecker (this);

    if (options.onDesktop && native != nullptr)
        native->setVisible (shouldBeVisible);

    if (bailOutChecker.shouldBailOut())
        return;

    visibilityChanged();

    if (bailOutChecker.shouldBailOut())
        return;

    repaint();
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

    moved();
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

    moved();
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

    moved();
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

    moved();
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

    moved();
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

    moved();
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

    moved();
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

    moved();
}

void Component::moved() {}

//==============================================================================

void Component::setSize (const Size<float>& newSize)
{
    boundsInParent = boundsInParent.withSize (newSize);

    if (options.onDesktop && native != nullptr)
        native->setSize (newSize.to<int>());

    resized();
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
    boundsInParent = newBounds;

    if (options.onDesktop && native != nullptr)
        native->setBounds (newBounds.to<int>());

    auto bailOutChecker = BailOutChecker (this);

    resized();

    if (bailOutChecker.shouldBailOut())
        return;

    moved();
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
    if (options.onDesktop)
        return bounds.withZeroPosition();

    auto parent = getParentComponent();
    while (parent != nullptr && ! parent->options.onDesktop)
    {
        bounds.translate (parent->getPosition());
        parent = parent->getParentComponent();
    }

    return bounds;
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
    newOpacity = jlimit (0.0f, 1.0f, newOpacity);

    opacity = static_cast<uint8> (newOpacity * 255);

    if (options.onDesktop && native != nullptr)
        native->setOpacity (newOpacity);
}

float Component::getOpacity() const
{
    return opacity / 255.0f;
}

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

void Component::repaint()
{
    jassert (! options.isRepainting); // You are likely repainting from paint !

    if (getBounds().isEmpty())
        return;

    if (auto nativeComponent = getNativeComponent())
        nativeComponent->repaint (getBoundsRelativeToTopLevelComponent());
}

void Component::repaint (const Rectangle<float>& rect)
{
    jassert (! options.isRepainting); // You are likely repainting from paint !

    if (rect.isEmpty())
        return;

    if (auto nativeComponent = getNativeComponent())
        nativeComponent->repaint (rect.translated (getBoundsRelativeToTopLevelComponent().getTopLeft()));
}

void Component::repaint (float x, float y, float width, float height)
{
    repaint ({ x, y, width, height });
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

    component->parentComponent = this;

    const int currentIndex = children.indexOf (component);

    if (isPositiveAndBelow (currentIndex, children.size()))
    {
        if (currentIndex != index)
        {
            children.move (currentIndex, index);

            childrenChanged();
        }
    }
    else
    {
        children.insert (index, component);

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
    component->parentComponent = nullptr;

    auto bailOutChecker = BailOutChecker (this);

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

Component* Component::findComponentAt (const Point<float>& p)
{
    if (! options.isVisible || ! boundsInParent.withZeroPosition().contains (p))
        return nullptr;

    for (int index = children.size(); --index >= 0;)
    {
        auto child = children.getUnchecked (index);
        if (! child->isVisible() || ! child->boundsInParent.contains (p))
            continue;

        child = child->findComponentAt (p - child->boundsInParent.getPosition());
        if (child != nullptr)
            return child;
    }

    return this;
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

void Component::takeKeyboardFocus()
{
    if (! options.wantsKeyboardFocus)
        return;

    if (auto nativeComponent = getNativeComponent())
        nativeComponent->setFocusedComponent (this);
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
    if (! options.wantsKeyboardFocus)
        return false;

    if (auto nativeComponent = getNativeComponent())
        return nativeComponent->getFocusedComponent() == this;

    return false;
}

void Component::focusGained() {}

void Component::focusLost() {}

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

void Component::paint (Graphics& g)
{
    jassert (! isOpaque()); // If your component is opaque, you need to paint it !
}

void Component::paintOverChildren (Graphics& g) {}

void Component::refreshDisplay (double lastFrameTimeSeconds) {}

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

void Component::mouseEnter (const MouseEvent& event) {}

void Component::mouseExit (const MouseEvent& event) {}

void Component::mouseDown (const MouseEvent& event) {}

void Component::mouseMove (const MouseEvent& event) {}

void Component::mouseDrag (const MouseEvent& event) {}

void Component::mouseUp (const MouseEvent& event) {}

void Component::mouseDoubleClick (const MouseEvent& event) {}

void Component::mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) {}

void Component::keyDown (const KeyPress& keys, const Point<float>& position) {}

void Component::keyUp (const KeyPress& keys, const Point<float>& position) {}

void Component::textInput (const String& text) {}

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

void Component::setStyleProperty (const Identifier& propertyId, const std::optional<var>& property)
{
    if (property)
        properties.set (propertyId, *property);
    else
        properties.remove (propertyId);

    styleChanged();
}

std::optional<var> Component::getStyleProperty (const Identifier& propertyId) const
{
    if (auto property = properties.getVarPointer (propertyId); property != nullptr && ! property->isVoid())
        return *property;

    return std::nullopt;
}

std::optional<var> Component::findStyleProperty (const Identifier& propertyId) const
{
    if (auto property = getStyleProperty (propertyId))
        return property;

    if (parentComponent != nullptr)
        return parentComponent->findStyleProperty (propertyId);

    return std::nullopt;
}

//==============================================================================

void Component::userTriedToCloseWindow() {}

//==============================================================================

void Component::internalRefreshDisplay (double lastFrameTimeSeconds)
{
    refreshDisplay (lastFrameTimeSeconds);

    for (auto child : children)
        child->internalRefreshDisplay (lastFrameTimeSeconds);
}

//==============================================================================

void Component::internalPaint (Graphics& g, const Rectangle<float>& repaintArea, bool renderContinuous)
{
    if (! isVisible() || (getWidth() == 0 || getHeight() == 0))
        return;

    auto bounds = getBoundsRelativeToTopLevelComponent();

    auto boundsToRedraw = bounds
                              .intersection (repaintArea)
                              .roundToInt()
                              .to<float>();

    if (! renderContinuous && boundsToRedraw.isEmpty())
        return;

    const auto opacity = g.getOpacity() * ((! options.onDesktop && native == nullptr) ? getOpacity() : 1.0f);
    if (opacity <= 0.0f)
        return;

    options.isRepainting = true;

    {
        const auto globalState = g.saveState();

        g.setOpacity (opacity);
        g.setDrawingArea (bounds);
        if (! options.unclippedRendering)
            g.setClipPath (boundsToRedraw);

        g.setTransform (transform);

        if (auto opaqueChild = findTopmostOpaqueChild (boundsToRedraw))
        {
            if (auto parentOfOpaque = opaqueChild->getParentComponent())
            {
                auto opaqueIndex = parentOfOpaque->getIndexOfChildComponent (opaqueChild);
                for (int i = opaqueIndex; i < parentOfOpaque->children.size(); ++i)
                    parentOfOpaque->children.getUnchecked(i)->internalPaint (g, boundsToRedraw, renderContinuous);
            }
        }
        else
        {
            {
                const auto paintState = g.saveState();

                paint (g);
            }

            for (auto child : children)
                child->internalPaint (g, boundsToRedraw, renderContinuous);
        }

        paintOverChildren (g);
    }

    options.isRepainting = false;

#if YUP_ENABLE_COMPONENT_REPAINT_DEBUGGING
    g.setFillColor (debugColor);
    g.setOpacity (0.2f);
    g.fillAll();

    if (--counter == 0)
    {
        counter = 2;
        debugColor = Color::opaqueRandom();
    }
#endif
}

Component* Component::findTopmostOpaqueChild (const Rectangle<float>& area)
{
    // Search from back to front (topmost to bottommost in z-order) for an opaque descendant
    // that fully covers the repaint area
    for (int i = children.size() - 1; i >= 0; --i)
    {
        auto child = children[i];
        if (! child->isVisible())
            continue;

        auto childBounds = child->getBoundsRelativeToTopLevelComponent();

        // First recursively check if any descendant of this child is opaque and covers the area
        auto opaqueDescendant = child->findTopmostOpaqueChild (area);
        if (opaqueDescendant != nullptr)
        {
            // Found an opaque descendant - return it (deepest first)
            return opaqueDescendant;
        }

        // Check if this child itself is opaque and covers the area
        if (child->isOpaque() &&
            child->getOpacity() >= 1.0f &&
            ! child->isTransformed() &&
            childBounds.contains (area))
        {
            // This child covers the area
            return child;
        }
    }

    // No opaque descendant found that covers the area
    return nullptr;
}

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
    if (! isVisible())
        return;

    keyDown (keys, position);
}

//==============================================================================

void Component::internalKeyUp (const KeyPress& keys, const Point<float>& position)
{
    if (! isVisible())
        return;

    keyUp (keys, position);
}

//==============================================================================

void Component::internalTextInput (const String& text)
{
    if (! options.wantsKeyboardFocus || ! isVisible())
        return;

    textInput (text);
}

//==============================================================================

void Component::internalResized (int width, int height)
{
    boundsInParent = boundsInParent.withSize (Size<int> (width, height).to<float>());

    resized();
}

//==============================================================================

void Component::internalMoved (int xpos, int ypos)
{
    boundsInParent = boundsInParent.withPosition (Point<int> (xpos, ypos).to<float>());

    moved();
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

void Component::internalDisplayChanged() {}

//==============================================================================

void Component::internalContentScaleChanged (float dpiScale)
{
    contentScaleChanged (dpiScale);
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

void Component::updateMouseCursor()
{
    Desktop::getInstance()->setMouseCursor (mouseCursor);
}

//==============================================================================

Point<float> Component::getScreenPosition() const
{
    return localToScreen (getPosition());
}

//==============================================================================

Rectangle<float> Component::getScreenBounds() const
{
    return localToScreen (getLocalBounds());
}

//==============================================================================

Point<float> Component::localToScreen (const Point<float>& localPoint) const
{
    if (options.onDesktop && native != nullptr)
        return native->getPosition().to<float>() + localPoint;

    auto screenPos = localPoint + getPosition();
    auto parent = getParentComponent();

    while (parent != nullptr)
    {
        if (parent->options.onDesktop && parent->native != nullptr)
        {
            screenPos += parent->native->getPosition().to<float>();
            break;
        }
        else
        {
            screenPos += parent->getPosition();
        }

        parent = parent->getParentComponent();
    }

    return screenPos;
}

Point<float> Component::screenToLocal (const Point<float>& screenPoint) const
{
    return screenPoint - localToScreen (Point<float> (0.0f, 0.0f));
}

Rectangle<float> Component::localToScreen (const Rectangle<float>& localRectangle) const
{
    return Rectangle<float> (localToScreen (localRectangle.getPosition()), localRectangle.getSize());
}

Rectangle<float> Component::screenToLocal (const Rectangle<float>& screenRectangle) const
{
    return Rectangle<float> (screenToLocal (screenRectangle.getPosition()), screenRectangle.getSize());
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
    const Component* comp = this;

    while (comp != nullptr)
    {
        if (comp->isTransformed())
            transform = transform.followedBy (comp->getTransform());

        transform = transform.translated (comp->getPosition());

        if (comp->options.onDesktop)
        {
            if (comp->native != nullptr)
            {
                auto nativePos = comp->native->getPosition().to<float>();
                transform = transform.translated (nativePos);
            }

            break;
        }

        comp = comp->getParentComponent();
    }

    return transform;
}

} // namespace yup
