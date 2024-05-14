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
    for (auto component : children)
        component->parentComponent = nullptr;

    children.clear();

    masterReference.clear();
}

//==============================================================================

void Component::setVisible (bool shouldBeVisible)
{
    if (options.isVisible != shouldBeVisible)
    {
        options.isVisible = shouldBeVisible;

        if (native != nullptr)
            native->setVisible (shouldBeVisible);

        visibilityChanged();
    }
}

bool Component::isVisible() const
{
    return options.isVisible;
}

void Component::visibilityChanged()
{
}

//==============================================================================

void Component::setTitle (const String& title)
{
    componentTitle = title;

    if (options.onDesktop)
        native->setTitle (title);
}

String Component::getTitle() const
{
    return componentTitle;
}

//==============================================================================

int Component::getX() const
{
    return static_cast<int> (boundsInParent.getX());
}

int Component::getY() const
{
    return static_cast<int> (boundsInParent.getY());
}

Point<int> Component::getPosition() const
{
    return boundsInParent.getPosition().to<int>();
}

void Component::moved()
{
}

//==============================================================================

void Component::setSize (const Size<int>& newSize)
{
    if (options.onDesktop)
    {
        boundsInParent = boundsInParent.withSize (newSize * getScaleDpi());

        native->setSize (newSize);
    }
    else
    {
        boundsInParent = boundsInParent.withSize (newSize);
    }

    resized();
}

Size<int> Component::getSize() const
{
    if (options.onDesktop)
        return native->getSize ();

    return boundsInParent.getSize().to<int>();
}

Size<int> Component::getContentSize() const
{
    if (options.onDesktop)
        return native->getContentSize ();

    return getSize();
}

int Component::getWidth() const
{
    return static_cast<int> (boundsInParent.getWidth());
}

int Component::getHeight() const
{
    return static_cast<int> (boundsInParent.getHeight());
}

void Component::setBounds (const Rectangle<int>& newBounds)
{
    if (options.onDesktop)
    {
        boundsInParent = newBounds.withScaledSize (getScaleDpi()).to<float>();

        native->setBounds (newBounds);
    }
    else
    {
        boundsInParent = newBounds.to<float>();
    }

    resized();
}

Rectangle<int> Component::getBounds() const
{
    return boundsInParent.to<int>();
}

Rectangle<int> Component::getLocalBounds() const
{
    return boundsInParent.withZeroPosition().to<int>();
}

int Component::proportionOfWidth (float proportion) const
{
    return static_cast<int> (getWidth() * proportion);
}

int Component::proportionOfHeight (float proportion) const
{
    return static_cast<int> (getHeight() * proportion);
}

void Component::resized()
{
}

//==============================================================================

void Component::setFullScreen (bool shouldBeFullScreen)
{
    if (options.isFullScreen != shouldBeFullScreen)
    {
        options.isFullScreen = shouldBeFullScreen;

        if (options.onDesktop)
            native->setFullScreen (shouldBeFullScreen);
    }
}

bool Component::isFullScreen() const
{
    return options.isFullScreen;
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

//==============================================================================

void Component::addToDesktop (std::optional<float> framerateRedraw)
{
    if (options.onDesktop)
        return;

    if (parentComponent != nullptr)
    {
        parentComponent->removeChildComponent (this);
        parentComponent = nullptr;
    }

    options.onDesktop = true;

    native = ComponentNative::createFor (*this, framerateRedraw);
}

void Component::removeFromDesktop()
{
    if (! options.onDesktop)
        return;

    options.onDesktop = false;

    native.reset();
}

bool Component::isOnDesktop() const noexcept
{
    return options.onDesktop;
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

void Component::takeFocus()
{
    getNativeComponent()->setFocusedComponent (this);
}

void Component::leaveFocus()
{
    getNativeComponent()->setFocusedComponent (nullptr);
}

//==============================================================================

void Component::paint (Graphics& g, float frameRate) {}
void Component::paintOverChildren (Graphics& g, float frameRate) {}

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

//==============================================================================

void Component::userTriedToCloseWindow() {}

//==============================================================================

void Component::internalPaint (Graphics& g, float frameRate)
{
    if (! isVisible())
        return;

    const auto state = g.saveState();

    g.setOpacity (static_cast<uint8> (getOpacity() * 255));
    g.setDrawingArea (getBounds().to<float>());

    paint (g, frameRate);

    for (auto child : children)
    {
        if (! child->isVisible())
            continue;

        const auto state = g.saveState();

        g.setDrawingArea (child->getBounds().to<float>());

        child->internalPaint (g, frameRate);
    }

    paintOverChildren (g, frameRate);
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

void Component::internalResized (int width, int height)
{
    if (options.onDesktop)
        boundsInParent = boundsInParent.withSize (Size<float> (width, height) * getScaleDpi());
    else
        boundsInParent = boundsInParent.withSize (width, height);

    resized();
}

void Component::internalMoved (int xpos, int ypos)
{
    if (options.onDesktop)
        boundsInParent = boundsInParent.withPosition (Point<float> (xpos, ypos) * getScaleDpi());
    else
        boundsInParent = boundsInParent.withPosition (xpos, ypos);

    moved();
}

void Component::internalUserTriedToCloseWindow()
{
    userTriedToCloseWindow();
}

} // namespace yup
