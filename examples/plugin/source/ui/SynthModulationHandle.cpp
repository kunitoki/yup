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

#include "SynthModulationHandle.h"
#include "SynthModulationTarget.h"
#include "SynthTheme.h"

namespace
{
yup::Font getIconFont (float height)
{
    return yup::ApplicationTheme::getGlobalTheme()->getDefaultIconFont().withHeight (height);
}
} // namespace

const yup::String SynthModulationHandle::dragText ("retrace.modulation");

SynthModulationHandle::SynthModulationHandle (SynthModulationSource sourceToDrag)
    : source (sourceToDrag)
{
    setOpaque (false);
    setMouseCursor (yup::MouseCursor::Hand);
    setTitle ("Drag onto a control to modulate it");

    ghost.setOpaque (false);
    ghost.setSize (ghostSize, ghostSize);
}

SynthModulationHandle::~SynthModulationHandle()
{
    // The ghost belongs to this handle, so a drag still running must let go of it first.
    if (isCurrentlyDragging())
        yup::DragAndDropManager::getInstance()->cancelDrag();
}

void SynthModulationHandle::paint (yup::Graphics& g)
{
    const auto color = getSourceColor (*this, source);
    const auto bounds = getLocalBounds().reduced (2.0f);

    g.setFillColor (color.withAlpha (hovered || isCurrentlyDragging() ? 0.3f : 0.14f));
    g.fillRoundedRect (bounds, 5.0f);
    g.setStrokeColor (color.withAlpha (0.7f));
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (bounds, 5.0f);

    g.setFillColor (color);
    g.fillFittedText (yup::String (yup::CharPointer_UTF8 (YUP_ICON_ARROWS_UP_DOWN_LEFT_RIGHT)),
                      getIconFont (yup::jmin (bounds.getWidth(), bounds.getHeight()) * 0.8f),
                      bounds,
                      yup::Justification::center);
}

void SynthModulationHandle::mouseEnter (const yup::MouseEvent&)
{
    hovered = true;
    repaint();
}

void SynthModulationHandle::mouseExit (const yup::MouseEvent&)
{
    hovered = false;
    repaint();
}

void SynthModulationHandle::mouseDrag (const yup::MouseEvent& event)
{
    if (isCurrentlyDragging() || event.getPosition().distanceTo (event.getLastMouseDownPosition()) < dragThreshold)
        return;

    // The ghost floats outside the interface, where the theme's colors cannot be looked up.
    ghost.color = getSourceColor (*this, source);
    ghost.iconColor = SynthTheme::onAccent (*this);

    startDragging (DragOptions {}
                       .withData (yup::DragAndDropData().withText (dragText))
                       .withDragImageComponent (&ghost, { ghostSize * 0.5f, ghostSize * 0.5f })
                       .withImageOpacity (0.9f)
                       .withImageInTopLevelComponent (true)
                       .withAllowedActions (yup::dragAndDropActionLink));
    repaint();
}

void SynthModulationHandle::dragOperationEnded (const yup::DragAndDropData&, yup::DragAndDropAction)
{
    repaint();
}

void SynthModulationHandle::Ghost::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds().reduced (1.0f);

    g.setFillColor (color);
    g.fillEllipse (bounds);
    g.setFillColor (iconColor);
    g.fillFittedText (yup::String (yup::CharPointer_UTF8 (YUP_ICON_ARROWS_UP_DOWN_LEFT_RIGHT)),
                      getIconFont (bounds.getHeight() * 0.7f),
                      bounds,
                      yup::Justification::center);
}
