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

DragAndDropSource::DragOptions& DragAndDropSource::DragOptions::withData (DragAndDropData newData)
{
    data = std::move (newData);
    return *this;
}

DragAndDropSource::DragOptions& DragAndDropSource::DragOptions::withDragImage (Image newImage, Point<float> offset)
{
    dragImage = std::move (newImage);
    dragImageComponent = nullptr;
    imageOffset = offset;
    return *this;
}

DragAndDropSource::DragOptions& DragAndDropSource::DragOptions::withDragImageComponent (Component* component, Point<float> offset)
{
    dragImageComponent = component;
    imageOffset = offset;
    return *this;
}

DragAndDropSource::DragOptions& DragAndDropSource::DragOptions::withImageOpacity (float newOpacity)
{
    imageOpacity = newOpacity;
    return *this;
}

DragAndDropSource::DragOptions& DragAndDropSource::DragOptions::withAllowedActions (DragAndDropActions newActions)
{
    allowedActions = newActions;
    return *this;
}

DragAndDropSource::DragOptions& DragAndDropSource::DragOptions::withExternalDragAllowed (bool shouldAllowExternalDrag)
{
    allowExternalDrag = shouldAllowExternalDrag;
    return *this;
}

//==============================================================================

bool DragAndDropSource::startDragging (DragOptions options)
{
    if (auto* component = getDragSourceComponent())
        return DragAndDropManager::getInstance()->startDragging (*this, *component, std::move (options));

    return false;
}

bool DragAndDropSource::isCurrentlyDragging() const
{
    auto* manager = DragAndDropManager::getInstanceWithoutCreating();

    if (manager == nullptr || ! manager->isDragging())
        return false;

    return dynamic_cast<const DragAndDropSource*> (manager->getCurrentDragSourceComponent()) == this;
}

//==============================================================================

void DragAndDropSource::dragOperationStarted (const DragAndDropData&) {}

void DragAndDropSource::dragOperationEnded (const DragAndDropData&, DragAndDropAction) {}

//==============================================================================

Component* DragAndDropSource::getDragSourceComponent()
{
    auto* source = dynamic_cast<Component*> (this);

    // A DragAndDropSource is only meaningful when it is also a Component.
    jassert (source != nullptr);

    return source;
}

} // namespace yup
