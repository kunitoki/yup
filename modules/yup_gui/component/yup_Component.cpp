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

extern void yup_setMouseCursor (const MouseCursor& mouseCursor);

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
    return ! options.isDisabled;
}

void Component::setEnabled (bool shouldBeEnabled)
{
    if (options.isDisabled != ! shouldBeEnabled)
    {
        options.isDisabled = ! shouldBeEnabled;

        //if (native != nullptr)
        //    native->setEnabled (shouldBeEnabled);

        enablementChanged();
    }
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
    if (options.isVisible != shouldBeVisible)
    {
        options.isVisible = shouldBeVisible;

        if (native != nullptr)
            native->setVisible (shouldBeVisible);

        visibilityChanged();

        repaint();
    }
}

void Component::visibilityChanged()
{
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

void Component::setPosition (const Point<float>& newPosition)
{
    if (! boundsInParent.getPosition().approximatelyEqualTo (newPosition))
    {
        boundsInParent = boundsInParent.withPosition (newPosition);

        if (options.onDesktop && native != nullptr)
            native->setPosition (newPosition.to<int>());

        moved();
    }
}

float Component::getX() const
{
    return boundsInParent.getX();
}

float Component::getY() const
{
    return boundsInParent.getY();
}

Point<float> Component::getPosition() const
{
    return boundsInParent.getPosition();
}

void Component::moved()
{
}

//==============================================================================

void Component::setSize (const Size<float>& newSize)
{
    if (! boundsInParent.getSize().approximatelyEqualTo (newSize))
    {
        boundsInParent = boundsInParent.withSize (newSize);

        if (options.onDesktop && native != nullptr)
            native->setSize (newSize.to<int>());

        resized();
    }
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
    if (! boundsInParent.approximatelyEqualTo (newBounds))
    {
        boundsInParent = newBounds;

        if (options.onDesktop && native != nullptr)
            native->setBounds (newBounds.to<int>());

        resized();
        moved();
    }
}

Rectangle<float> Component::getBounds() const
{
    return boundsInParent;
}

Rectangle<float> Component::getLocalBounds() const
{
    return boundsInParent.withZeroPosition();
}

Rectangle<float> Component::getBoundsRelativeToAncestor() const
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
    if (options.isFullScreen != shouldBeFullScreen)
    {
        options.isFullScreen = shouldBeFullScreen;

        if (options.onDesktop && native != nullptr)
            native->setFullScreen (shouldBeFullScreen);
    }
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
        nativeComponent->repaint (getBoundsRelativeToAncestor());
}

void Component::repaint (const Rectangle<float>& rect)
{
    if (rect.isEmpty())
        return;

    if (auto nativeComponent = getNativeComponent())
        nativeComponent->repaint (rect.translated (getBoundsRelativeToAncestor().getTopLeft()));
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

//==============================================================================

bool Component::isOnDesktop() const
{
    return options.onDesktop;
}

void Component::addToDesktop (const ComponentNative::Options& nativeOptions, void* parent)
{
    if (options.onDesktop)
        removeFromDesktop();

    if (parentComponent != nullptr)
    {
        parentComponent->removeChildComponent (this);
        parentComponent = nullptr;
    }

    options.onDesktop = true;

    native = ComponentNative::createFor (*this, nativeOptions, parent);

    setBounds (getBounds()); // This is needed to update based on scaleDpi
}

void Component::removeFromDesktop()
{
    if (! options.onDesktop)
        return;

    options.onDesktop = false;

    native.reset();
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

void Component::addChildComponent (Component& component)
{
    addChildComponent (&component);
}

void Component::addChildComponent (Component* component)
{
    component->parentComponent = this;

    children.addIfNotAlreadyThere (component);
}

void Component::addAndMakeVisible (Component& component)
{
    addAndMakeVisible (&component);
}

void Component::addAndMakeVisible (Component* component)
{
    addChildComponent (component);

    component->setVisible (true);
}

void Component::insertChildComponent (Component& component, int index)
{
    insertChildComponent (&component, index);
}

void Component::insertChildComponent (Component* component, int index)
{
    const int currentIndex = children.indexOf (component);

    if (isPositiveAndBelow (currentIndex, children.size()))
        children.move (currentIndex, index);
}

void Component::removeChildComponent (Component& component)
{
    removeChildComponent (&component);
}

void Component::removeChildComponent (Component* component)
{
    component->parentComponent = nullptr;

    children.removeAllInstancesOf (component);
}

//==============================================================================

int Component::getNumChildComponents() const
{
    return children.size();
}

Component* Component::getComponentAt (int index) const
{
    return children.getUnchecked (index);
}

Component* Component::findComponentAt (const Point<float>& p)
{
    if (options.isVisible && boundsInParent.withZeroPosition().contains (p))
    {
        for (int index = children.size(); --index >= 0;)
        {
            auto child = children.getUnchecked (index);
            if (child == nullptr || ! child->isVisible() || ! child->boundsInParent.contains (p))
                continue;

            child = child->findComponentAt (p - child->boundsInParent.getPosition());
            if (child != nullptr)
                return child;
        }

        return this;
    }

    return nullptr;
}

//==============================================================================

void Component::toFront()
{
    if (parentComponent == nullptr)
        return;

    parentComponent->insertChildComponent (this, parentComponent->getNumChildComponents());
}

void Component::toBack()
{
    if (parentComponent == nullptr)
        return;

    parentComponent->insertChildComponent (this, 0);
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

void Component::takeFocus()
{
    if (options.wantsKeyboardFocus)
    {
        if (auto nativeComponent = getNativeComponent())
            nativeComponent->setFocusedComponent (this);
    }
}

void Component::leaveFocus()
{
    if (auto nativeComponent = getNativeComponent())
    {
        if (nativeComponent->getFocusedComponent() == this)
            nativeComponent->setFocusedComponent (nullptr);
    }
}

bool Component::hasFocus() const
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

    auto bounds = getBoundsRelativeToAncestor();

    auto dirtyBounds = repaintArea;
    auto boundsToRedraw = bounds.intersection (dirtyBounds);
    if (! renderContinuous && boundsToRedraw.isEmpty())
        return;

    const auto opacity = g.getOpacity() * getOpacity();
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

    mouseListeners.call (&MouseListener::mouseEnter, event);
}

//==============================================================================

void Component::internalMouseExit (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    mouseExit (event);

    mouseListeners.call (&MouseListener::mouseExit, event);
}

//==============================================================================

void Component::internalMouseDown (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    mouseDown (event);

    mouseListeners.call (&MouseListener::mouseDown, event);
}

//==============================================================================

void Component::internalMouseMove (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    mouseMove (event);

    mouseListeners.call (&MouseListener::mouseMove, event);
}

//==============================================================================

void Component::internalMouseDrag (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    mouseDrag (event);

    mouseListeners.call (&MouseListener::mouseDrag, event);
}

//==============================================================================

void Component::internalMouseUp (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    mouseUp (event);

    mouseListeners.call (&MouseListener::mouseUp, event);
}

//==============================================================================

void Component::internalMouseDoubleClick (const MouseEvent& event)
{
    if (! isVisible())
        return;

    mouseDoubleClick (event);

    mouseListeners.call (&MouseListener::mouseDoubleClick, event);
}

//==============================================================================

void Component::internalMouseWheel (const MouseEvent& event, const MouseWheelData& wheelData)
{
    if (! isVisible())
        return;

    mouseWheel (event, wheelData);

    mouseListeners.call (&MouseListener::mouseWheel, event, wheelData);
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
    if (! isVisible() || ! options.wantsTextInput)
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
    yup_setMouseCursor (mouseCursor);
}

} // namespace yup
