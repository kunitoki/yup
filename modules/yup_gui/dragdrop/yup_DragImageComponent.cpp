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

//==============================================================================

DragImageComponent::DragImageComponent()
{
    addToDesktop (ComponentNative::Options{}
                      .withDecoration (false)
                      .withResizableWindow (false)
                      .withAlwaysOnTop (true)
                      .withTransparent (true)
                      .withFocusable (false)
                      .withTemporaryWindow (true)
                      .withClearColor (Colors::transparentBlack));

    setVisible (false);
    setOpaque (false);
}

DragImageComponent::~DragImageComponent()
{
    clearDragImage();
    removeFromDesktop();
}

//==============================================================================

void DragImageComponent::setDragImage (const Image& newImage, const Point<float>& newHotspot, float newOpacity)
{
    clearDragImage();

    dragImage = newImage;
    hotspot = newHotspot;
    opacity = newOpacity;

    updateWindowSize();
}

void DragImageComponent::setDragImageComponent (Component* newComponent, const Point<float>& newHotspot, float newOpacity)
{
    clearDragImage();

    hostedComponent = newComponent;
    hotspot = newHotspot;
    opacity = newOpacity;

    updateWindowSize();
}

void DragImageComponent::clearDragImage()
{
    if (hostedComponent != nullptr)
    {
        removeChildComponent (hostedComponent);
        hostedComponent = nullptr;
    }

    dragImage = {};

    setVisible (false);
}

void DragImageComponent::moveToScreenPosition (const Point<float>& screenPosition)
{
    setPosition (screenPosition - hotspot);
}

//==============================================================================

void DragImageComponent::updateWindowSize()
{
    const auto width = hostedComponent != nullptr ? hostedComponent->getWidth() : dragImage.getWidth();
    const auto height = hostedComponent != nullptr ? hostedComponent->getHeight() : dragImage.getHeight();

    if (width <= 0 || height <= 0)
    {
        setVisible (false);
        return;
    }

    setSize (static_cast<float> (width), static_cast<float> (height));

    if (hostedComponent != nullptr)
    {
        if (hostedComponent->getParentComponent() != this)
            addAndMakeVisible (hostedComponent);

        hostedComponent->setBounds (getLocalBounds());
    }

    if (auto* native = getNativeComponent())
        native->setOpacity (opacity);

    // A transparent window shows nothing until it is repainted, so a fresh ghost always needs one.
    setVisible (true);
    repaint();
}

void DragImageComponent::paint (Graphics& g)
{
    if (hostedComponent == nullptr && dragImage.isValid())
        g.drawImageAt (dragImage, Point<float>());
}

} // namespace yup
