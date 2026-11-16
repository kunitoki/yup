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

struct ResizableCornerIds
{
    static inline const Identifier cornerSize { "cornerSize" };
};

} // namespace

//==============================================================================
const Identifier ResizableCorner::Style::lineColorId { "resizableCornerLine" };

//==============================================================================
const ComponentTypeInfo& ResizableCorner::getClassTypeInfo()
{
    static const auto info = ComponentTypeBuilder<ResizableCorner> (Identifier ("ResizableCorner"))
                                 .color ("lineColor", Style::lineColorId)
                                 .property (ResizableCornerIds::cornerSize, &ResizableCorner::getCornerSize, &ResizableCorner::setCornerSize, 16.0, "The side length of the square resize handle, in points.")
                                 .build();

    return info;
}

//==============================================================================
ResizableCorner::ResizableCorner()
    : ResizableCorner (StringRef {})
{
}

ResizableCorner::ResizableCorner (StringRef componentID)
    : Component (componentID)
{
    setOpaque (false);
    setMouseCursor (MouseCursor::ResizeTopLeftRightBottom);
}

ResizableCorner::~ResizableCorner() = default;

//==============================================================================
void ResizableCorner::setCornerSize (float newCornerSize)
{
    const auto clamped = jmax (1.0f, newCornerSize);
    if (approximatelyEqual (cornerSize, clamped))
        return;

    cornerSize = clamped;
    notifyPropertyChanged (ResizableCornerIds::cornerSize);
    updatePosition();
}

float ResizableCorner::getCornerSize() const noexcept
{
    return cornerSize;
}

//==============================================================================
void ResizableCorner::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

void ResizableCorner::mouseDown (const MouseEvent& event)
{
    if (auto* parent = getParentComponent())
    {
        sizeAtMouseDown = parent->getSize();
        screenPositionAtMouseDown = event.getScreenPosition();
    }
}

void ResizableCorner::mouseDrag (const MouseEvent& event)
{
    auto* parent = getParentComponent();
    if (parent == nullptr)
        return;

    const auto delta = event.getScreenPosition() - screenPositionAtMouseDown;

    auto newSize = Size<float> { jmax (cornerSize, sizeAtMouseDown.getWidth() + delta.getX()),
                                 jmax (cornerSize, sizeAtMouseDown.getHeight() + delta.getY()) };

    if (constrainSize)
        newSize = constrainSize (newSize);

    if (newSize != parent->getSize())
        parent->setSize (newSize);
}

void ResizableCorner::parentSizeChanged()
{
    updatePosition();
}

void ResizableCorner::parentHierarchyChanged()
{
    updatePosition();
}

//==============================================================================
void ResizableCorner::updatePosition()
{
    auto* parent = getParentComponent();
    if (parent == nullptr)
        return;

    setBounds (parent->getWidth() - cornerSize, parent->getHeight() - cornerSize, cornerSize, cornerSize);

    if (parent->getIndexOfChildComponent (this) != parent->getNumChildComponents() - 1)
        toFront (false);
}

} // namespace yup
