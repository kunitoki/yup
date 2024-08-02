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
    boundsInParent = boundsInParent.withSize (newSize);

    if (options.onDesktop)
        native->setSize (newSize.to<int>());

    resized();
}

Size<float> Component::getSize() const
{
    if (options.onDesktop)
        return native->getSize().to<float>();

    return boundsInParent.getSize();
}

Size<float> Component::getContentSize() const
{
    if (options.onDesktop)
        return native->getContentSize().to<float>();

    return getSize();
}

float Component::getWidth() const
{
    return boundsInParent.getWidth();
}

float Component::getHeight() const
{
    return boundsInParent.getHeight();
}

void Component::setBounds (const Rectangle<float>& newBounds)
{
    boundsInParent = newBounds;

    if (options.onDesktop)
        native->setBounds (newBounds.to<int>());

    resized();
}

Rectangle<float> Component::getBounds() const
{
    return boundsInParent;
}

Rectangle<float> Component::getLocalBounds() const
{
    return boundsInParent.withZeroPosition();
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

        if (options.onDesktop)
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
    if (auto nativeComponent = getNativeComponent())
        nativeComponent->repaint (getBounds());
}

void Component::repaint (const Rectangle<float>& rect)
{
    if (auto nativeComponent = getNativeComponent())
        nativeComponent->repaint (rect.translated (getBounds().getTopLeft()));
}

//==============================================================================

void* Component::getNativeHandle() const
{
    if (options.onDesktop)
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

void Component::addToDesktop (ComponentNative::Flags flags, void* parent, std::optional<float> framerateRedraw)
{
    if (options.onDesktop)
        removeFromDesktop();

    if (parentComponent != nullptr)
    {
        parentComponent->removeChildComponent (this);
        parentComponent = nullptr;
    }

    options.onDesktop = true;

    native = ComponentNative::createFor (*this, flags, parent, framerateRedraw);

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

void Component::setWantsKeyboardFocus (KeyboardFocusMode focusMode)
{
    options.wantsKeyboardFocus = focusMode != KeyboardFocusMode::wantsNoFocus;
    options.wantsTextInput = focusMode == KeyboardFocusMode::wantsTextInputCallback;
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
        nativeComponent->setFocusedComponent (nullptr);
}

bool Component::hasFocus() const
{
    if (auto nativeComponent = getNativeComponent())
        return nativeComponent->getFocusedComponent() == this;

    return false;
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

//==============================================================================

void Component::mouseEnter (const MouseEvent& event) {}

void Component::mouseExit (const MouseEvent& event) {}

void Component::mouseDown (const MouseEvent& event) {}

void Component::mouseMove (const MouseEvent& event) {}

void Component::mouseDrag (const MouseEvent& event) {}

void Component::mouseUp (const MouseEvent& event) {}

void Component::mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) {}

void Component::keyDown (const KeyPress& keys, const Point<float>& position) {}

void Component::keyUp (const KeyPress& keys, const Point<float>& position) {}

void Component::textInput (const String& text) {}

//==============================================================================

void Component::userTriedToCloseWindow() {}

//==============================================================================

void Component::internalPaint (Graphics& g, bool renderContinuous)
{
    if (! isVisible() || (getWidth() == 0 || getHeight() == 0))
        return;

    auto bounds = (options.onDesktop ? getLocalBounds() : getBounds());

    auto dirtyBounds = getNativeComponent()->getRepaintArea();
    auto boundsToRedraw = bounds.intersection (dirtyBounds);
    if (! renderContinuous && boundsToRedraw.isEmpty())
        return;

    const auto opacity = g.getOpacity() * getOpacity();
    if (opacity == 0.0f)
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
        child->internalPaint (g, renderContinuous);

    g.setDrawingArea (bounds);
    if (! options.unclippedRendering)
        g.setClipPath (boundsToRedraw);

    paintOverChildren (g);
}

void Component::internalMouseEnter (const MouseEvent& event)
{
    if (! isVisible())
        return;

    mouseEnter (event);
}

void Component::internalMouseExit (const MouseEvent& event)
{
    if (! isVisible())
        return;

    mouseExit (event);
}

void Component::internalMouseDown (const MouseEvent& event)
{
    if (! isVisible())
        return;

    mouseDown (event);
}

void Component::internalMouseMove (const MouseEvent& event)
{
    if (! isVisible())
        return;

    mouseMove (event);
}

void Component::internalMouseDrag (const MouseEvent& event)
{
    if (! isVisible())
        return;

    mouseDrag (event);
}

void Component::internalMouseUp (const MouseEvent& event)
{
    if (! isVisible())
        return;

    mouseUp (event);
}

void Component::internalMouseWheel (const MouseEvent& event, const MouseWheelData& wheelData)
{
    if (! isVisible())
        return;

    mouseWheel (event, wheelData);
}

void Component::internalKeyDown (const KeyPress& keys, const Point<float>& position)
{
    if (! isVisible())
        return;

    keyDown (keys, position);
}

void Component::internalKeyUp (const KeyPress& keys, const Point<float>& position)
{
    if (! isVisible())
        return;

    keyUp (keys, position);
}

void Component::internalTextInput (const String& text)
{
    if (! isVisible() || ! options.wantsTextInput)
        return;

    textInput (text);
}

void Component::internalResized (int width, int height, float scaleDpi)
{
    boundsInParent = boundsInParent.withSize (Size<float> (width, height) * scaleDpi);

    resized();
}

void Component::internalMoved (int xpos, int ypos, float scaleDpi)
{
    boundsInParent = boundsInParent.withPosition (Point<float> (xpos, ypos) * scaleDpi);

    moved();
}

void Component::internalUserTriedToCloseWindow()
{
    userTriedToCloseWindow();
}

} // namespace yup
