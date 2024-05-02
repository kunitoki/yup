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

namespace juce
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

void Component::setSize (const Size<int>& newSize)
{
    boundsInParent = boundsInParent.withSize (newSize.to<float>());

    if (options.onDesktop)
        native->setSize (newSize);

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

void Component::resized()
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
    component.parentComponent = this;

    children.addIfNotAlreadyThere (&component);
}

void Component::addChildComponent (Component* component)
{
    component->parentComponent = this;

    children.addIfNotAlreadyThere (component);
}

void Component::removeChildComponent (Component& component)
{
    component.parentComponent = nullptr;

    children.removeAllInstancesOf (&component);
}

void Component::removeChildComponent (Component* component)
{
    component->parentComponent = nullptr;

    children.removeAllInstancesOf (component);
}

//==============================================================================

void Component::paint (Graphics& g, float frameRate) {}

//==============================================================================

void Component::mouseDown (const MouseEvent& event) {}
void Component::mouseMove (const MouseEvent& event) {}
void Component::mouseDrag (const MouseEvent& event) {}
void Component::mouseUp (const MouseEvent& event) {}
void Component::keyDown (const KeyPress& keys, double x, double y) {}
void Component::keyUp (const KeyPress& keys, double x, double y) {}

//==============================================================================

void Component::userTriedToCloseWindow() {}

//==============================================================================

void Component::internalPaint (Graphics& g, float frameRate)
{
    paint (g, frameRate);
}

void Component::internalMouseDown (const MouseEvent& event)
{
    mouseDown (event);
}

void Component::internalMouseMove (const MouseEvent& event)
{
    mouseMove (event);
}

void Component::internalMouseDrag (const MouseEvent& event)
{
    mouseDrag (event);
}

void Component::internalMouseUp (const MouseEvent& event)
{
    mouseUp (event);
}

void Component::internalKeyDown (const KeyPress& keys, double x, double y)
{
    keyDown (keys, x, y);
}

void Component::internalKeyUp (const KeyPress& keys, double x, double y)
{
    keyUp (keys, x, y);
}

void Component::internalUserTriedToCloseWindow()
{
    userTriedToCloseWindow();
}

} // namespace juce
