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

YUP_IMPLEMENT_SINGLETON (DragAndDropManager)

//==============================================================================

DragAndDropManager::DragAndDropManager() = default;

DragAndDropManager::~DragAndDropManager()
{
    cancelDrag();

    clearSingletonInstance();
}

//==============================================================================

bool DragAndDropManager::startDragging (DragAndDropSource& source, Component& component, DragAndDropSource::DragOptions options)
{
    if (isDragging())
        return false;

    if (options.data.isEmpty())
        return false;

    return beginSession (source, component, std::move (options));
}

bool DragAndDropManager::beginSession (DragAndDropSource& source, Component& component, DragAndDropSource::DragOptions&& options)
{
    currentOptions = std::move (options);
    currentData = currentOptions.data;
    sourceComponent = &component;
    hoveredComponent = nullptr;

    createGhost();
    moveGhostTo (Desktop::getInstance()->getCurrentMouseLocation());

    // The gesture has to keep flowing when the pointer leaves the source window, even for a window
    // that was not created with ComponentNative::captureMouse.
    if (auto* native = component.getNativeComponent())
        native->setGlobalMouseCaptureActive (true);

    Desktop::getInstance()->addGlobalMouseListener (this);

    source.dragOperationStarted (currentData);

    if (source.onDragStarted)
        source.onDragStarted (currentData);

    return true;
}

void DragAndDropManager::endSession (DragAndDropAction performed)
{
    const auto data = currentData;

    Desktop::getInstance()->removeGlobalMouseListener (this);

    auto* sourceComponentPtr = sourceComponent.get();

    if (sourceComponentPtr != nullptr)
        if (auto* native = sourceComponentPtr->getNativeComponent())
            native->setGlobalMouseCaptureActive (false);

    clearHoveredTarget (data);

    if (ghost != nullptr)
        ghost->clearDragImage();

    if (auto* source = dynamic_cast<DragAndDropSource*> (sourceComponentPtr))
    {
        source->dragOperationEnded (data, performed);

        if (source->onDragEnded)
            source->onDragEnded (data, performed);
    }

    currentData = {};
    currentOptions = {};
    sourceComponent = nullptr;
    hoveredComponent = nullptr;
    externalDragData = {};
    cancelled = false;
}

void DragAndDropManager::cancelDrag()
{
    if (! isDragging() || cancelled)
        return;

    cancelled = true;

    if (ghost != nullptr)
        ghost->clearDragImage();

    clearHoveredTarget (currentData);
}

//==============================================================================

bool DragAndDropManager::isDragging() const noexcept
{
    return sourceComponent != nullptr;
}

const DragAndDropData& DragAndDropManager::getCurrentDragData() const noexcept
{
    return currentData;
}

Component* DragAndDropManager::getCurrentDragSourceComponent() const
{
    return sourceComponent.get();
}

DragAndDropTarget* DragAndDropManager::getCurrentDragTarget() const
{
    return dynamic_cast<DragAndDropTarget*> (hoveredComponent.get());
}

//==============================================================================

void DragAndDropManager::mouseDrag (const MouseEvent& event)
{
    if (! isDragging() || cancelled)
        return;

    const auto screenPosition = event.getScreenPosition();
    auto* component = resolveComponentAt (screenPosition);

    if (component == nullptr && currentOptions.allowExternalDrag)
    {
        currentOptions.allowExternalDrag = false;

        if (auto* source = getCurrentDragSourceComponent())
        {
            if (auto performed = performNativeDrag (*source, currentData))
            {
                endSession (*performed);
                return;
            }
        }
    }

    moveGhostTo (screenPosition);
    dispatchHover (component, screenPosition, currentData);
}

void DragAndDropManager::mouseUp (const MouseEvent& event)
{
    if (cancelled)
    {
        endSession (DragAndDropAction::none);
        return;
    }

    if (! isDragging())
        return;

    const auto screenPosition = event.getScreenPosition();
    auto* component = resolveComponentAt (screenPosition);

    auto performed = DragAndDropAction::none;

    if (component != nullptr
        && DragAndDropTarget::dispatchItemDrop (*component, currentData, toWindowPosition (*component, screenPosition)))
    {
        // Shift asks for a move; anything else copies. Only the move/copy pair is negotiated for now.
        performed = (event.getModifiers().isShiftDown() && currentOptions.allowedActions.test (dragAndDropActionMove))
                        ? DragAndDropAction::move
                        : DragAndDropAction::copy;
    }

    endSession (performed);
}

//==============================================================================

Component* DragAndDropManager::resolveComponentAt (const Point<float>& screenPosition) const
{
    return Desktop::getInstance()->findComponentAt (screenPosition, getGhostComponent());
}

Component* DragAndDropManager::getGhostComponent() const
{
    return ghost.get();
}

Point<float> DragAndDropManager::toWindowPosition (Component& component, const Point<float>& screenPosition) const
{
    if (auto* topLevel = component.getTopLevelComponent())
        return topLevel->screenToLocal (screenPosition);

    return screenPosition;
}

void DragAndDropManager::moveGhostTo (const Point<float>& screenPosition)
{
    if (ghost != nullptr)
        ghost->moveToScreenPosition (screenPosition);
}

void DragAndDropManager::createGhost()
{
    if (ghost == nullptr)
        ghost = std::make_unique<DragImageComponent>();

    if (currentOptions.dragImageComponent != nullptr)
        ghost->setDragImageComponent (currentOptions.dragImageComponent, currentOptions.imageOffset, currentOptions.imageOpacity);
    else
        ghost->setDragImage (currentOptions.dragImage, currentOptions.imageOffset, currentOptions.imageOpacity);
}

//==============================================================================

void DragAndDropManager::dispatchHover (Component* component, const Point<float>& screenPosition, const DragAndDropData& data)
{
    if (component == hoveredComponent.get())
    {
        if (component != nullptr)
            DragAndDropTarget::dispatchItemDragMove (*component, data, toWindowPosition (*component, screenPosition));

        return;
    }

    clearHoveredTarget (data);

    hoveredComponent = component;

    if (component != nullptr)
        DragAndDropTarget::dispatchItemDragEnter (*component, data, toWindowPosition (*component, screenPosition));
}

void DragAndDropManager::clearHoveredTarget (const DragAndDropData& data)
{
    if (auto* previous = hoveredComponent.get())
        DragAndDropTarget::dispatchItemDragExit (*previous, data);

    hoveredComponent = nullptr;
}

//==============================================================================

void DragAndDropManager::handleExternalDragPosition (Component& rootComponent, const Point<float>& position, const DragAndDropData& data)
{
    externalDragData = data;

    if (isDragging())
        return;

    auto* component = rootComponent.findComponentAt (position);
    dispatchHover (component,
                   rootComponent.getScreenBounds().getPosition() + position,
                   externalDragData);
}

bool DragAndDropManager::handleExternalDrop (Component& rootComponent, const Point<float>& position, const DragAndDropData& data)
{
    externalDragData = data;

    if (isDragging())
        return false;

    const auto screenPosition = rootComponent.getScreenBounds().getPosition() + position;
    auto* component = rootComponent.findComponentAt (position);

    const auto handled = component != nullptr
                             && DragAndDropTarget::dispatchItemDrop (*component, externalDragData, toWindowPosition (*component, screenPosition));

    clearHoveredTarget (externalDragData);
    externalDragData = {};

    return handled;
}

void DragAndDropManager::handleExternalDragExit()
{
    if (isDragging())
        return;

    clearHoveredTarget (externalDragData);
    externalDragData = {};
}

} // namespace yup
