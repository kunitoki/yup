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

void Component::enablementChanged()
{
}

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

    if (options.onDesktop && native != nullptr)
        native->setVisible (shouldBeVisible);

    visibilityChanged();

    repaint();
}

void Component::visibilityChanged()
{
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

//==============================================================================

String Component::getTitle() const
{
    return componentTitle;
}

void Component::setTitle (const String& title)
{
    componentTitle = title;

    if (options.onDesktop)
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
        native->setPosition (newBottomLeft.to<int>().translated (0, -getHeight()));

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
        native->setPosition (newTopRight.to<int>().translated (-getWidth(), 0));

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
        native->setPosition (newBottomRight.to<int>().translated (-getWidth(), -getHeight()));

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
        native->setPosition (newCenter.to<int>().translated (-getWidth() / 2, -getHeight() / 2));

    moved();
}

void Component::moved()
{
}

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

void Component::setBounds (const Rectangle<float>& newBounds)
{
    boundsInParent = newBounds;

    if (options.onDesktop && native != nullptr)
        native->setBounds (newBounds.to<int>());

    resized();
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

void Component::resized()
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

void Component::displayChanged()
{
}

//==============================================================================

float Component::getScaleDpi() const
{
    if (native != nullptr)
        return native->getScaleDpi();

    if (parentComponent == nullptr)
        return 1.0f;

    return parentComponent->getScaleDpi();
}

void Component::contentScaleChanged (float dpiScale)
{
}

//==============================================================================

void Component::setOpacity (float newOpacity)
{
    newOpacity = jlimit (0.0f, 1.0f, newOpacity);

    opacity = static_cast<uint8> (newOpacity * 255);

    if (native != nullptr)
        native->setOpacity (newOpacity);
}

float Component::getOpacity() const
{
    return opacity / 255.0f;
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
    if (getBounds().isEmpty())
        return;

    if (auto nativeComponent = getNativeComponent())
        nativeComponent->repaint (getBoundsRelativeToTopLevelComponent());
}

void Component::repaint (const Rectangle<float>& rect)
{
    if (rect.isEmpty())
        return;

    if (auto nativeComponent = getNativeComponent())
        nativeComponent->repaint (rect.translated (getBoundsRelativeToTopLevelComponent().getTopLeft()));
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

void Component::attachedToNative()
{
}

void Component::detachedFromNative()
{
}

//==============================================================================

bool Component::isOnDesktop() const
{
    return options.onDesktop;
}

void Component::addToDesktop (const ComponentNative::Options& nativeOptions, void* parent)
{
    JUCE_ASSERT_MESSAGE_MANAGER_IS_LOCKED

    if (options.onDesktop)
        removeFromDesktop();

    if (parentComponent != nullptr)
    {
        parentComponent->removeChildComponent (this);
        parentComponent = nullptr;
    }

    options.onDesktop = true;

    native = ComponentNative::createFor (*this, nativeOptions, parent);

    attachedToNative();

    setBounds (getBounds()); // This is needed to update based on scaleDpi
}

void Component::removeFromDesktop()
{
    JUCE_ASSERT_MESSAGE_MANAGER_IS_LOCKED

    if (! options.onDesktop)
        return;

    options.onDesktop = false;

    native.reset();

    detachedFromNative();
}

//==============================================================================

void Component::toFront (bool shouldGainKeyboardFocus)
{
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

    component->internalHierarchyChanged();

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

    auto checker = BailOutChecker (this);

    for (int index = children.size(); --index >= 0;)
    {
        auto child = children.getUnchecked (index);

        if (checker.shouldBailOut())
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

Component* Component::getComponentAt (int index) const
{
    return children.getUnchecked (index);
}

int Component::getIndexOfChildComponent (Component* component) const
{
    return children.indexOf (component);
}

Component* Component::findComponentAt (const Point<float>& p)
{
    if (options.isVisible && boundsInParent.withZeroPosition().contains (p))
    {
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

    return nullptr;
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
    if (options.wantsKeyboardFocus)
    {
        if (auto nativeComponent = getNativeComponent())
            nativeComponent->setFocusedComponent (this);
    }
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

void Component::setColor (const Identifier& colorId, const std::optional<Color>& color)
{
    if (color)
        properties.set (colorId, static_cast<int64> (color->getARGB()));
    else
        properties.remove (colorId);
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

NamedValueSet& Component::getProperties()
{
    return properties;
}

const NamedValueSet& Component::getProperties() const
{
    return properties;
}

//==============================================================================

void Component::paint (Graphics& g) {}

void Component::paintOverChildren (Graphics& g) {}

void Component::refreshDisplay (double lastFrameTimeSeconds) {}

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

    auto dirtyBounds = repaintArea;
    auto boundsToRedraw = bounds.intersection (dirtyBounds);
    if (! renderContinuous && boundsToRedraw.isEmpty())
        return;

    const auto opacity = g.getOpacity() * ((! options.onDesktop && native == nullptr) ? getOpacity() : 1.0f);
    if (opacity <= 0.0f)
        return;

    const auto globalState = g.saveState();

    g.setOpacity (opacity);
    g.setDrawingArea (bounds);
    if (! options.unclippedRendering)
        g.setClipPath (boundsToRedraw);

    {
        const auto paintState = g.saveState();

        paint (g);
    }

    for (auto child : children)
        child->internalPaint (g, boundsToRedraw, renderContinuous);

    paintOverChildren (g);

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

//==============================================================================

void Component::internalMouseEnter (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    mouseEnter (event);

    mouseListeners.callChecked (BailOutChecker (this), &MouseListener::mouseEnter, event);
}

//==============================================================================

void Component::internalMouseExit (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    mouseExit (event);

    mouseListeners.callChecked (BailOutChecker (this), &MouseListener::mouseExit, event);
}

//==============================================================================

void Component::internalMouseDown (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    mouseDown (event);

    mouseListeners.callChecked (BailOutChecker (this), &MouseListener::mouseDown, event);
}

//==============================================================================

void Component::internalMouseMove (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    mouseMove (event);

    mouseListeners.callChecked (BailOutChecker (this), &MouseListener::mouseMove, event);
}

//==============================================================================

void Component::internalMouseDrag (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    mouseDrag (event);

    mouseListeners.callChecked (BailOutChecker (this), &MouseListener::mouseDrag, event);
}

//==============================================================================

void Component::internalMouseUp (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    mouseUp (event);

    mouseListeners.callChecked (BailOutChecker (this), &MouseListener::mouseUp, event);
}

//==============================================================================

void Component::internalMouseDoubleClick (const MouseEvent& event)
{
    if (! isVisible())
        return;

    mouseDoubleClick (event);

    mouseListeners.callChecked (BailOutChecker (this), &MouseListener::mouseDoubleClick, event);
}

//==============================================================================

void Component::internalMouseWheel (const MouseEvent& event, const MouseWheelData& wheelData)
{
    if (! isVisible())
        return;

    mouseWheel (event, wheelData);

    mouseListeners.callChecked (BailOutChecker (this), &MouseListener::mouseWheel, event, wheelData);
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
    boundsInParent = boundsInParent.withSize (Size<float> (width, height));

    resized();
}

//==============================================================================

void Component::internalMoved (int xpos, int ypos)
{
    boundsInParent = boundsInParent.withPosition (Point<float> (xpos, ypos));

    moved();
}

//==============================================================================

void Component::internalDisplayChanged()
{
}

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

void Component::updateMouseCursor()
{
    Desktop::getInstance()->setMouseCursor (mouseCursor);
}

} // namespace yup
